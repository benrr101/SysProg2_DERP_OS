#ifndef _WIN_MAN_H_
#define _WIN_MAN_H_

#include "headers.h"

// KB
#define WIN_MAN_MEM 		1024
#define DEFAULT_SCREENS		12

// Callback function definition for users
/*typedef void (*mouse_handler)(Uint8 x_pos, Uint8 y_pos, char left, char right,
		char middle);
*/

typedef struct screen_info
{
	Uint32	buf_num;
	Uint32 	w;
	Uint32	h;
	Pid		pid;
	Uint32*	bPtr;
	Uint8	active;
	Uint8	blocking;
	void (*mouse_handler)(Uint8, Uint8, char, char, char );
}	screen_info;

//to be 1024b
typedef struct win_man_vars
{
	Uint32 	active_quad;
	Uint32	screens[4];
	//char reserve[1024-sizeofwin_man_vars]
	//don't need to have the array here I just did t oshow the rest of the space
} 	win_man_vars; 


/*
Screen positions
+------------+------------+
|            |            |
|     0      |      1     |
|            |            |
+------------+------------+
|            |            |
|     2      |      3     |
|            |            |
+------------+------------+

and there are 12 screens in the background
*/
void _win_man_init( void );

Uint8 get_blocking( Uint32 buf_num );
Uint8 set_blocking( Uint32 buf_num, Uint8 quadrant);

//returns the active quadrant
Uint8 get_active( void );

//Set quadrant to a buffer
Uint8 set_active( Uint32 buf_num, Uint8 quadrant);

//returns the active quadrants pid
Pid get_active_pid( void );

//returns the screen info for the pid provided
screen_info* get_screen_info( Pid pid );

//return the screen_info structs
screen_info* get_screen_infos( void );

//grabs the screens array
Uint32* get_current_bufs( void ); 

//reserves a screen
Status get_screen_buffer( Pid pid );
void free_screen( Pid pid );


Uint8 switch_active( Uint8 quadrant );
Uint8 replace_active( Uint32 buffer_num );

// Mouse functions
void register_mouse_listener( void (*mouse_handler)(Uint8, Uint8, char, char,
		char ) );
void update_cursor_pos( Uint8 x_pos, Uint8 y_pos );
void update_mouse_button( char left, char right, char middle );


#endif //_WIN_MAN_H_
