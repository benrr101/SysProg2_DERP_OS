/*
** SCCS ID:	@(#)system.c	1.1	4/5/12
**
** File:	system.c
**
** Author:	4003-506 class of 20113
**
** Contributor:
**
** Description:	Miscellaneous OS support functions
*/

#define	__KERNEL__20113__

#include "headers.h"

#include "startup.h"
#include "system.h"
#include "clock.h"
#include "pcbs.h"
#include "bootstrap.h"
#include "syscalls.h"
#include "sio.h"
#include "scheduler.h"
#include "vga_dr.h"
#include "gl.h"
#include "win_man.h"
#include "vmem.h"
#include "vmemL2.h"
#include "vmem_isr.h"
#include "vmem_ref.h"
#include "pci.h"
#include "fs.h"

// need init() address
#include "users.h"

// need the exit() prototype
#include "ulib.h"

Uint32 stack_copy_reserve[ STACK_SIZE / 1024 ];
Uint32 stack_copy_reserve_size = STACK_SIZE / 1024;

/*
** PUBLIC FUNCTIONS
*/

/*
** _put_char_or_code( ch )
**
** prints the character on the console, unless it is a non-printing
** character, in which case its hex code is printed
*/

void _put_char_or_code( int ch ) {

	if( ch >= ' ' && ch < 0x7f ) {
		c_putchar( ch );
	} else {
		c_printf( "\\x%02x", ch );
	}

}

/*
** _cleanup(pcb)
**
** deallocate a pcb and associated data structures
*/

void _cleanup( Pcb *pcb ) {
	Status status;

	if( pcb == NULL ) {
		return;
	}

	//change to defualt page 
	_vmeml2_change_page( (Uint32)_vmem_page_dir );
	//release the old page
	_vmeml2_release_page_dir( pcb->pdt );

	//deallocate the pcb
	pcb->state = FREE;
	status = _pcb_dealloc( pcb );
	if( status != SUCCESS ) {
		_kpanic( "_cleanup", "pcb dealloc status %s\n", status );
	}
}


/*
** _create_process(pcb,entry)
**
** initialize a new process' data structures (PCB, stack)
**
** returns:
**	success of the operation
*/

Status _create_process( Pcb *pcb, Uint32 entry ) {
	Context *context;
	Stack *stack;
	Uint32 *ptr;

	// don't need to do this if called from _sys_exec(), but
	// we are called from other places, so...

	if( pcb == NULL ) {
		return( BAD_PARAM );
	}

	// if the PCB doesn't already have a stack, we
	// need to allocate one

	stack = pcb->stack;
	if( stack == NULL ) {
		__panic( "No stack?");
		/*stack = _stack_alloc();
		if( stack == NULL ) {
			return( ALLOC_FAILED );
		}
		pcb->stack = stack;*/
	}

	// clear the stack

	_kmemclr( (void *) stack, sizeof(Stack) );

	/*
	** Set up the initial stack contents for a (new) user process.
	**
	** We reserve one longword at the bottom of the stack as
	** scratch space.  Above that, we simulate a call to exit() by
	** pushing the address of exit() as a "return address".  Finally,
	** above that we place an context_t area that is initialized with
	** the standard initial register contents.
	**
	** The low end of the stack will contain these values:
	**
	**      esp ->  ?       <- context save area
	**              ...     <- context save area
	**              ?       <- context save area
	**              exit    <- return address for main()
	**              filler  <- last word in stack
	**
	** When this process is dispatched, the context restore
	** code will pop all the saved context information off
	** the stack, leaving the "return address" on the stack
	** as if the main() for the process had been "called" from
	** the exit() stub.
	*/

	// first, compute a pointer to the second-to-last longword

	ptr = ((Uint32 *) (stack + 1)) - 2;

	// assign the "return" address

	*ptr = (Uint32) exit;

	// next, set up the process context

	context = ((Context *) ptr) - 1;
	pcb->context = context;

	// initialize all the fields that should be non-zero, starting
	// with the segment registers

	context->cs = GDT_CODE;
	context->ss = GDT_STACK;
	context->ds = GDT_DATA;
	context->es = GDT_DATA;
	context->fs = GDT_DATA;
	context->gs = GDT_DATA;

	// EFLAGS must be set up to re-enable IF when we switch
	// "back" to this context

	context->eflags = DEFAULT_EFLAGS;

	// EIP must contain the entry point of the process; in
	// essence, we're pretending that this is where we were
	// executing when the interrupt arrived

	context->eip = entry;

	return( SUCCESS );

}


/*
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/

void _init( void ) {
	Pcb *pcb;
	Status status;

	/*
	** BOILERPLATE CODE - taken from basic framework
	**
	** Initialize interrupt stuff.
	*/

	__init_interrupts();	// IDT and PIC initialization

	/*
	** Console I/O system.
	*/
	
	c_io_init();	
	c_setscroll( 0, 7, 99, 99 );
	c_puts_at( 0, 6, "================================================================================" );

	/*
	** 20113-SPECIFIC CODE STARTS HERE
	*/

	/*
	** Initialize various OS modules
	*/

	c_puts( "Module init: " );

	_q_init();		// must be first
	_vmem_init();
	_vmeml2_init();
	_vmem_ref_init();
	_sio_init();
	_pcb_init();
	_win_man_init();	
		//vga_init
		//gl_init

	_syscall_init();
	_sched_init();
	_pci_init();
	_fs_init();
	_ps2_keyboard_init();
	_clock_init();

	c_puts( "\n" );

	/*
	** Create the initial system ESP
	**
	** This will be the address of the next-to-last
	** longword in the system stack.
	*/

	_system_esp = ((Uint32 *) ( (&_system_stack) + 1)) - 2;

	/*
	** Install the ISRs
	*/

	__install_isr( INT_VEC_TIMER, _isr_clock );
	__install_isr( INT_VEC_SYSCALL, _isr_syscall );
	__install_isr( INT_VEC_SERIAL_PORT_1, _isr_sio );
	__install_isr( INT_VEC_GENERAL_PROTECTION, _isr_vmem_general_protect );
	__install_isr( INT_VEC_PAGE_FAULT, _isr_vmem_page_fault);
	__install_isr( 0x2A, _isr_usb_pull);

	/*
	** Create the initial process
	**
	** Code mostly stolen from _sys_fork(); if that routine
	** changes, SO MUST THIS!!!
	**
	** First, get a PCB and a stack
	*/

	pcb = _pcb_alloc();
	if( pcb == NULL  ) {
		_kpanic( "_init", "first pcb alloc failed\n", FAILURE );
	}

	
	//create reserve address for copying
	int r;
	for ( r = 0; r < stack_copy_reserve_size; r++ )
	{	
		Uint32 page =_vmem_get_next_reserve_address();
		stack_copy_reserve[r] = page;
		_vmem_set_address( stack_copy_reserve[r] );
	}

	//setup paging ans stack for idle process
	pcb->pdt = _vmeml2_create_page_dir();
	Uint32* ptable=_vmeml2_create_page_table( pcb->pdt, ( STACK_ADDRESS / PAGE_TABLE_SIZE)  );
	 _vmeml2_create_page( ptable, 0 );
	 _vmeml2_create_page( ptable, 1 );
	 _vmeml2_create_page( ptable, 2 );
	 _vmeml2_create_page( ptable, 3 );
	pcb->stack = (Stack*) ( STACK_ADDRESS);

	_vmeml2_change_page( (Uint32) pcb->pdt );

	/*
	** Next, set up various PCB fields
	*/

	pcb->pid  = _next_pid++;
	pcb->ppid = pcb->pid;
	pcb->priority = PRIO_HIGH;	// init() should finish first

	/*
	** Set up the initial process context.
	*/

	status = _create_process( pcb, (Uint32) init );
	if( status != SUCCESS ) {
		_kpanic( "_init", "create init process status %s\n", status );
	}

	/*
	** Make it the first process
	*/

	_current = pcb;

	/*
	** Turn on the SIO receiver (the transmitter will be turned
	** on/off as characters are being sent)
	*/

	_sio_enable( SIO_RX );

	/*
	** END OF 20113-SPECIFIC CODE
	**
	** Finally, report that we're all done.
	*/

	c_puts( "System initialization complete.\n" );

}


/*
** _isr_usb_pull - catches the usb being pull out and then ignores it
*/
void _isr_usb_pull( int vector, int code )
{
	__outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}
