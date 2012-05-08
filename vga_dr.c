/*
** VGA Driver initialization code
*/

#define	__KERNEL__20113__

#include "headers.h"
#include "vga_dr.h"
#include "vga_define.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/
VESA_INFO *vga_vesa_info;
MODE_INFO *vga_mode_info;

/*
** PRIVATE FUNCTIONS
 */

/*
 ** PUBLIC FUNCTIONS
 */

/*
 ** _vga_init()
 **
 ** initialize the vga module
 */
void _vga_init( void ) {
	//Check the contents of the information extracted from REAL MODE 
	vga_vesa_info = (VESA_INFO *)(VESA_INFO_ADDR << 4);
	vga_mode_info = (MODE_INFO *)(VGA__INFO_ADDR << 4);
	
	#ifdef VGA_DEBUG
	_vga_print_info();
	#endif

	draw_display();
	
}

Uint32 _vga_get_end_mem( void ) {
	//setup my buffer space @ PhysBasePtr + TotalMemory
	return (vga_mode_info->PhysBasePtr + (vga_vesa_info->TotalMemory * 64 * 1024));
}
	
void _vga_print_info( void ) {
	#ifdef VGA_DEBUG
	c_printf("\nVESA_INFO(%x):\n", vga_vesa_info);
	c_printf("-version: %d\n", vga_vesa_info->VESAVersion);
	c_printf("-VideoModePtr: %x\n", vga_vesa_info->VideoModePtr);
	c_printf("-TtlMem: %d, %d\n", vga_vesa_info->TotalMemory, (vga_vesa_info->TotalMemory * 64)/1024);
	c_printf("\nMODE_INFO(%x):\n", vga_mode_info);
	c_printf("-ModeAttr: %x\n", vga_mode_info->ModeAttributes);
	c_printf("-MemModel: %x\n", vga_mode_info->MemoryModel);
	c_printf("-WinSize: %d\n", vga_mode_info->WinSize);
	c_printf("-WinSize (X, Y): (%d, %d)\n", vga_mode_info->XResolution, vga_mode_info->YResolution);
	c_printf("-CharSize (X, Y): (%d, %d)\n", vga_mode_info->XCharSize, vga_mode_info->YCharSize);
	c_printf("-WinFuncPrt: %x\n", vga_mode_info->WinFuncPtr);
	c_printf("-PhysBasePrt: %x\n", vga_mode_info->PhysBasePtr);
	c_printf("-LinbytesPerScanLine: %d\n", vga_mode_info->LinbytesPerScanLine);
	#endif	
}

//Main draw program
void draw_display( void ) {
	Uint32 *ptr = (Uint32*)(vga_mode_info->PhysBasePtr);
	static int o = 0;
	
    int i = 0;
    int j = 0;

    for(i = o; i < vga_mode_info->XResolution; i++){
        for( j = 0; j < vga_mode_info->YResolution; j++){
			ptr[(j*vga_mode_info->LinbytesPerScanLine) + i] = i+j;
        }
    }
    if ( o > vga_mode_info->XResolution)
    	o = 0;
    o++;
}
