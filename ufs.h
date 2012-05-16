////////////////////////////////////////////////////////////////////////////
// DERP_OS - Userspace Filesystem Header
//
// @descrip	Declares all the functions for manipulating the filesystem from
//			userspace. These functions allow the user to create, delete,
//			read and write to files.
// @file	ufs.h
// @author	Benjamin Russell
////////////////////////////////////////////////////////////////////////////

#ifndef		DERP_UFS
#define		DERP_UFS

// REQUIRED HEADERS ////////////////////////////////////////////////////////
#include "fs.h"

// CONSTANTS ///////////////////////////////////////////////////////////////
// Seek start points
typedef enum {
	FS_SEEK_ABS,
	FS_SEEK_REL,
	FS_SEEK_REL_REV
} FS_FILE_SEEK;

// FUNCTIONS ///////////////////////////////////////////////////////////////
FILE fopen(char filepath[10]);
FS_STATUS fseek(FILE *file, Uint64 offset, FS_FILE_SEEK dir);

#endif
