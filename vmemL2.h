#ifndef _VMEML2_H
#define _VMEML2_H
//#define _VMEML2_DEBUG

#include "headers.h"

#define STACK_ADDRESS	0x3000000

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32* _vmeml2_create_page_dir( void );

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32* _vmeml2_create_page_table( Uint32* dir, Uint16 index );

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_create_page( Uint32* table, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_create_page( Uint32* table, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_create_page( Uint32* table, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_create_page( Uint32* table, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_create_page( Uint32* table, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32* _vmeml2_create_page_reserved( Uint32* table, Uint16 index );

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32* _vmeml2_create_4MB_page( Uint32* dir, Uint16 index); 

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_release_page_dir( Uint32* dir );

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_static_address( Uint32 addr1, Uint32 addr2, Uint8 mark);

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_static_dir_entry( Uint32 index);

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint8 _vmeml2_is_empty_dir_entry( Uint32* dir, Uint32 index);

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint8 _vmeml2_is_empty_page_entry( Uint32* table, Uint32 index);

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32 _vmeml2_get_phyical_address( Uint32* table, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32 _vmeml2_get_phyical_address_table( Uint32* dir, Uint16 index );
/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
Uint32 _vmeml2_get_phyical_address_4MB( Uint32* dir, Uint16 index);


/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_change_page(Uint32 page);

/*
** Name:	
** Description:	
** Arguments:	
** Return:	
*/
void _vmeml2_init( void );

#endif
