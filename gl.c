#include "gl.h"
#include "win_man.h"
#include "vga_dr.h"
#include "font.h"

static screen_info* scrn_info_arr;
static Uint32* video_mem_ptr;
//static Uint32 buff_bytes_perline;
static Uint32 bytes_perline;

void _gl_init( void ) {
	scrn_info_arr = get_screen_infos( );
	video_mem_ptr = (Uint32 *)_vga_get_start_mem( );
	#ifndef GL_DEBUG
	//clear_display();
	#endif
	bytes_perline = vga_mode_info->LinbytesPerScanLine/4;
	//buff_bytes_perline = bytes_perline/2;

}

void draw_active_screens() {

	//set_priority( PRIO_HIGH );
	while ( 1 ) {
		Uint32* 		displayed;
		Uint32*  		buffer_ptr;
		screen_info* 	curr_si;
		int i = 0;
		int x = 0;
		int y = 0;
		int x_off = 0;
		int y_off = 0;

		displayed = get_current_bufs( );

		for(i = 0; i < 4; i++) {
			curr_si = &(scrn_info_arr[displayed[i]]);

			buffer_ptr = (Uint32 *) (curr_si->bPtr);
			//#ifdef GL_DEBUG
			//write( 'M' );
			//c_printf("SI: %x - (%x=%x) ++ ", curr_si, curr_si->bPtr, buffer_ptr);
			//#endif

			//x_off = curr_si.w * (i % 2);
			if( i == 1 || i == 3 )
				x_off = curr_si->w+1;
			else
				x_off = 0;

			if( i == 2 || i == 3 )
				y_off = curr_si->h+1;
			else
				y_off = 0;

			//copy the buffer
			for(x = 0; x < curr_si->w -1; x++) {
				for(y = 0; y < curr_si->h-1; y++) {
					int pos1 = ((y + y_off) * bytes_perline) + (x + x_off);
					int pos2 = (y * curr_si->w) + x;

					#ifndef GL_DEBUG
					video_mem_ptr[pos1] = buffer_ptr[pos2];
					#endif
				}
			}
		}
		//draw system stuff mouse, screen seperators etc
		/*
		pixel p;
		p.r = 0;
		p.b = 0;
		p.g = 0;
		p.a = 0;
		draw_line(640, 0, 640, 512, p);
		draw_line(0, 512, 640, 512, p);
		*/
		for(x = 0; x <= 1280; x++) {
			video_mem_ptr[(512 * bytes_perline) + x] = 0xffffffff;
		}
		for(y = 0; y <= 1024; y++) {
			video_mem_ptr[(y * bytes_perline) + 640] = 0xffffffff;
		}
		msleep( 1 );
	}
}

void draw_scr_0() {

	int x = 0;
	int y = 0;
	int t = 0;
	pixel p;
	char* str = "HELLO WORLD";

	while ( 1 ) {

		if( t > 45 )
			t = 0;

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0x0c;
				p.b = 0x0c;
				p.g = 0x0c;
				p.a = 0x0c;
				draw_pixel(x, y, p);
			}
		}
		p.r = 0xff;
		p.g = 0xff;
		p.b = 0xff;
		p.a = 0xff;
		draw_string(str, 10, 10, p);
		msleep(1500);
	}
}

void draw_scr_1() {

	int x = 0;
	int y = 0;
	char* str = "HELLO WORLD";
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0x66;
				p.b = 0x23;
				p.g = 0x32;
				p.a = 0x80;
				draw_pixel(x, y, p);
			}
		}
		p.r = 0x00;
		p.g = 0x00;
		p.b = 0x00;
		p.a = 0x00;
		draw_string(str, 10, 10, p);
		msleep(2000);
	}
}

void draw_scr_2() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0xc0;
				p.b = 0xc0;
				p.g = 0xc0;
				p.a = 0xc0;
				draw_pixel(x, y, p);
			}
		}
		p.r = 0x00;
		p.g = 0x00;
		p.b = 0x00;
		p.a = 0x00;
		draw_character('A', 10, 20, p);
		msleep(2500);
	}
}

void draw_scr_3() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0xc0;
				p.b = 0xc0;
				p.g = 0xc0;
				p.a = 0xc0;
				draw_pixel(x, y, p);
			}
		}
		msleep(3000);
	}
}

void draw_scr_4() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0xff;
				p.b = 0x00;
				p.g = 0x00;
				p.a = 0x00;
				draw_pixel(x, y, p);
			}
		}
		msleep(3000);
	}
}

void draw_scr_5() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0x00;
				p.b = 0xff;
				p.g = 0x00;
				p.a = 0x00;
				draw_pixel(x, y, p);
			}
		}
		msleep(3000);
	}
}

void draw_scr_6() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0x00;
				p.b = 0x00;
				p.g = 0xff;
				p.a = 0x00;
				draw_pixel(x, y, p);
			}
		}
		msleep(3000);
	}
}

void draw_scr_7() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0xff;
				p.b = 0xff;
				p.g = 0x00;
				p.a = 0x00;
				draw_pixel(x, y, p);
			}
		}
		msleep(3000);
	}
}

void draw_scr_8() {

	int x = 0;
	int y = 0;
	pixel p;

	while ( 1 ) {

		for(x = 0; x < 180; x++) {
			for(y = 0; y < 180; y++) {
				p.r = 0x00;
				p.b = 0xff;
				p.g = 0xff;
				p.a = 0x00;
				draw_pixel(x, y, p);
			}
		}
		msleep(3000);
	}
}

void draw_pixel(Uint32 x, Uint32 y, pixel p) {
	Uint32			pix = 0;
	char*  			buffer_ptr;
	screen_info* 	curr_si;
	Pid pid = 0;
	int i = 0;
	Status s;

	s = get_pid( &pid );
	#ifdef GL_DEBUG
	//c_printf(" S:%d P:%d ", s, pid);
	#endif

	curr_si = ( get_screen_info( pid ) );
	if(curr_si != NULL) {
		#ifdef GL_DEBUG
		if(x == 0)
			c_printf(" (P:%d)0x%x(%d,%d) ", pid, curr_si, curr_si->w,curr_si->h);
		#endif

		buffer_ptr = 	(char*) (curr_si->bPtr);

		pix = p.r;
		pix = (pix << 8) | p.r;
		pix = (pix << 8) | p.g;
		pix = (pix << 8) | p.b;
		for(i = 0; i < 4; i++) {
			buffer_ptr[ ( y * curr_si->w * 4  ) + (x*4)  + i] = ((char*)(&pix))[i];
		}
	}
}

void set_pixel(Uint32 x, Uint32 y, pixel p, screen_info* curr_si) {
	Uint32			pix = 0;
	Uint32*			buffer_ptr;

	buffer_ptr = 	(Uint32*) (curr_si->bPtr);

	buffer_ptr[ ( y * curr_si->w  ) + (x)] = 0x0000000;
}

void draw_line(Uint32 x, Uint32 y, Uint32 x2, Uint32 y2, pixel p) {
	Uint32			pix = 0;
	char*  			buffer_ptr;
	screen_info* 	curr_si;
	Pid pid = 0;
	int i = 0;
	Status s;
	int dx = 0;
	int dy = 0;


	s = get_pid( &pid );

	curr_si = ( get_screen_info( pid ) );
	if(curr_si != NULL) {
		#ifdef GL_DEBUG
		if(x == 0)
			c_printf(" (P:%d)0x%x(%d,%d) ", pid, curr_si, curr_si->w,curr_si->h);
		#endif

		pix = p.r;
		pix = (pix << 8) | p.r;
		pix = (pix << 8) | p.g;
		pix = (pix << 8) | p.b;

		if( x == x2 ){
			for(dy = 0; dy <= y2; dy++) {
				set_pixel(x, dy, p, curr_si);
			}
		} else if( y == y2 ) {
			for(dx = 0; dx <= x2; dx++) {
				set_pixel(dx, y, p, curr_si);
			}
		}
	}
}

void draw_character(char c, Uint32 x, Uint32 y, pixel p) {
    int i = 0;
    char a = 'A';
    char aa = 'a';
    screen_info* 	curr_si;
    Pid pid = 0;
    unsigned char shift = 0x01;
    unsigned char* curr = 0;
    Status s;
    int dx = 0;
    int dy = 0;

    s = get_pid( &pid );

    curr_si = ( get_screen_info( pid ) );
    if(c >= aa && c <= 122)
        c -= 32; // uppercase no lower case yet

    curr = FONT[a-c];
    for(dy = 0; dy < 10; dy++) {
        shift = 0x01;
        for(dx = 7; dx >= 0; dx--) {
            if((curr[dy] & shift) == shift) {
                set_pixel(x+dx, y+dy, p, curr_si);
            }
            shift = shift << 1;
        }
    }
}

void draw_string(char* str, Uint32 x, Uint32 y, pixel p) {
    screen_info* curr_si;
    Status s;
    Pid pid = 0;
    char a = 'A';
    char aa = 'a';
    char c = 0;
    unsigned char shift = 0x01;
    unsigned char* curr = 0;
    int i = 0;
    int dx = 0;
    int dy = 0;
    int len = 0;


    //find the length
    while(str[len] != '\0') {
        if(str[len] >= aa && str[len] <= 122)
            str[len] -= 32; // uppercase no lower case yet
        len++;
    }
    len++; // obo
    //setup the pid and curr_si for drawing
    s = get_pid( &pid );
    curr_si = ( get_screen_info( pid ) );

    //height
    for(dy = 0; dy < 10; dy++) {
        //length or changing between the characters
        for(i = 0; i < len; i++) {
            if(str[i] == ' ')
            	continue;
            c = str[i];
            curr = FONT[a-c];
            
            //actual drawing of the current line
            shift = 0x01;
            for(dx = 7; dx >= 0; dx--) {
                if((curr[dy] & shift) == shift) {
                    //x == the start position plus the offset for the backwards character
                    //PLUS the offset for the current character
                    set_pixel(x+dx+(i*10), y+dy, p, curr_si);
                }
                shift = shift << 1;
            }
        }
    }
}






