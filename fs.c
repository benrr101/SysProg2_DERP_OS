////////////////////////////////////////////////////////////////////////////
// DERP_OS - DERP_FS
//
// @file	fs.c
// @descrip	This file defines all the functions needed for writing to and from
//			a DERP_FS. This also includes all necessary initialization tools.
// @author	Benjamin Russell
////////////////////////////////////////////////////////////////////////////

// REQUIRED FILES //////////////////////////////////////////////////////////

#include "headers.h"
#include "pci.h"
#include "startup.h"
#include "ata.h"
#include "fs.h"

// FUNCTIONS ///////////////////////////////////////////////////////////////

/**
 * Probes the given ata device for any DERP_FS partitions. If a partition is
 * found, the partition is mounted to the mount_points global variable and the
 * value of mount_point_count is incremented.
 * @param	ATADevice*	dev		The device to scan for DERP_FS partitions
 */
void _fs_probe(ATADevice *dev) {
	// Create a blank sector for reading in the mbr
	ATASector mbr;
	_ata_blank_sector(&mbr);

	// Read in the MBR
	_ata_read_sector(*dev, SECT_MBR, &mbr);
	
	// Go to the partition table and iterate over each entry
	Uint16 i;
	Uint16 endOfTable = FS_PART_TABLE_OFF + (FS_PART_TABLE_SIZE * FS_PART_TABLE_ENTRIES);
	for(i = FS_PART_TABLE_OFF; i < endOfTable; i += FS_PART_TABLE_SIZE) {
		// What type is the partiton?
		if(mbr[i + FS_PART_ENTRY_PT] == FS_PARTITION_TYPE) {
			// We found a DERP_FS partition!!
			// Read the first sector of the partiton to see if it's derp
			ATASector br;
			Uint32 brAddr = _sector_get_long(&mbr, i + FS_PART_ENTRY_LBA);
			_ata_read_sector(*dev, brAddr, &br);

			// Is it DERP?
			Uint32 header = br[0]<<24|br[1]<<16|br[2]<<8|br[3];
			Uint32 size   = _sector_get_long(&br, FS_BR_SIZE);
			if(header == FS_PARTITION_ID) {
				// Increment the count of mount points and set up the mp
				Uint8 mp_id = mount_point_count;
				mount_point_count++;
				MountPoint mp;
				
				// Build the MountPoint
				mp.device = dev;
				mp.bootRecord.header = header;
				mp.bootRecord.size = size;
				mp.offset = brAddr;
				mp.letter = mp_id + 0x41;

				// Store the MountPoint
				mount_points[mp_id] = mp;
			}
		}
	}
}

FS_STATUS _fs_create_partition(ATADevice *dev, Uint32 start, Uint32 size, Uint8 index) {
	// Detect stupidity errors
	if(index >= FS_PART_TABLE_ENTRIES) {
 		// Can't have > 4 partitions
		return FS_ERR_BADINDEX; 
	}
	if(size >= dev->size || size + start >= dev->size) { 
		// Can't partition off more sectors that the drive can hold
		return FS_ERR_TOOBIG; 
	}
	if(size <= 4) {
		// Can't have less sectors than required for metadata
		return FS_ERR_TOOSMALL;
	}
	if(start == SECT_MBR) {
		// Can't place a partition over the MBR
		return FS_ERR_BADSECT;
	}

	// Make the size divisible by 112
	if(size % FS_SECT_PER_IB != 0) {		
		// Size of partition is not divisible by 112. Make it so.
		size += FS_SECT_PER_IB - (size % FS_SECT_PER_IB);
		size++; // Making room for the BootRecord
	}

	// Read in the current MBR
	ATASector mbr;
	_ata_read_sector(*dev, SECT_MBR, &mbr);

	// Verify that it's actually an MBR
	if(!(mbr[510] == 0x55 && mbr[511] == 0xAA)) {
		// It's not a valid MBR. We need to rewrite it.
		_ata_blank_sector(&mbr);
		mbr[510] = 0x55;	// The MBR signature
		mbr[511] = 0xAA;
	}

	// Now jump to the index of the partition
	Uint32 indexAddr = FS_PART_TABLE_OFF + index * FS_PART_TABLE_SIZE;

	// Write the type of the partition
	mbr[indexAddr + FS_PART_ENTRY_PT] = FS_PARTITION_TYPE;

	// Write the geometry of the partition
	_sector_put_long(&mbr, indexAddr + FS_PART_ENTRY_LBA, start);
	_sector_put_long(&mbr, indexAddr + FS_PART_ENTRY_SIZE, size);

	// Write the mbr back to the disk
	_ata_write_sector(*dev, SECT_MBR, &mbr);
	return FS_SUCCESS;
}

FS_STATUS _fs_format(MountPoint *mp, ATADevice *dev, Uint8 index) {
	// Load up the mbr of the device
	ATASector mbr;
	_ata_read_sector(*dev, SECT_MBR, &mbr);

	// Verify that the partition is DERP
	Uint32 indexAddr = FS_PART_TABLE_OFF + index * FS_PART_TABLE_SIZE;
	if(mbr[indexAddr + FS_PART_ENTRY_PT] != FS_PARTITION_TYPE) {
		return FS_ERR_NOTDERP;
	}

	// BOOT RECORD /////////////////////////////////////////////////////////
	// Grab the LBA for the partition
	Uint32 lba  = _sector_get_long(&mbr, indexAddr + FS_PART_ENTRY_LBA);
	Uint32 size = _sector_get_long(&mbr, indexAddr + FS_PART_ENTRY_SIZE);

	// Build a boot record for the partition
	ATASector br;
	_ata_blank_sector(&br);

	// Write the header of the br
	br[0]=0x44; br[1]=0x45; br[2]=0x52; br[3]=0x50;

	// Write the size of the partition to the br
	_sector_put_long(&br, FS_BR_SIZE, size);

	// Write out the bootrecord
	_ata_write_sector(*dev, lba, &br);
	
	// INDEX BLOCKS ////////////////////////////////////////////////////////
	// Build an index block
	ATASector fsTable;
	ATASector nameTable;
	_ata_blank_sector(&fsTable);
	_ata_blank_sector(&nameTable);

	// Write the header of the index block
	fsTable[0]=0x49; fsTable[1]=0x42;

	// Write the header of the name table
	nameTable[0]=0x4E; nameTable[1]=0x41; nameTable[2]=0x4D; nameTable[3]=0x45;

	// These are written every FS_MOD_INDEXBLK sectors, offset by 1 for the mbr
	Uint32 i, count;
	for(i = 1; i < size; i += FS_SECT_PER_IB) {
		// Mark the first 3 sectors are in use
		Uint32 bits = 0x7;
		fsTable[FS_BT_END - 1] = bits;

		// Write the FSTable
		_ata_write_sector(*dev, lba + i, &fsTable);

		// Write the blank name table
		_ata_write_sector(*dev, lba + i + 1, &nameTable);
		_ata_write_sector(*dev, lba + i + 2, &nameTable);
		count++;
	} 

	return FS_SUCCESS;
}

Uint32 _fs_find_empty_sector(MountPoint *mp) {
	// Start checking index blocks
	Uint32 i, j, byte;
	Uint32 end = mp->offset + mp->bootRecord.size;
	ATASector s;

	// Loop from the first index block to the last index block
	for(i = mp->offset + 1; i < end; i += FS_SECT_PER_IB) {
		// Load the sector
		_ata_read_sector(*(mp->device), i, &s);
		
		// Start iterating over the bits in the bitfield
		for(j = 0; j < FS_BT_SIZE * sizeof(char); j++) {
			// Which byte do we need 
			byte = FS_BT_OFFSET + (FS_BT_END - (FS_BT_OFFSET + (j / 8)));
			byte -= 1;

			// Grab that byte and check if the bit we need is allocated
			if(((s[byte] >> (j % 8)) & FS_BT_ALLOCATED) == 0) {
				// Sector isn't allocated. This is our golden nugget
				// Calculate the sector into the partition this is
				return (i - mp->offset) + j;
			}
		}
	}

	// If we make it here, there's no free sectors
	return 0x0;
}

/**
 * Finds the first empty file pointer in the filesystem. If one is
 * not found, the MountPoint (mp) of the returned FSPointer will be
 * NULL. There are no guarantees on the buffered sector in the FSPointer.
 * @param	MountPoint*	mp	The MountPoint to search for free FSPointers.
 * @return	FSPointer	A FSPointer representing the first free pointer
 *						index of the MountPoint. mp will be NULL if there
 *						are no FSPointers available.
 */
FSPointer _fs_find_empty_fspointer(MountPoint *mp) {
	// Start checking index blocks
	Uint32 i, j;
	Uint32 end = mp->offset + mp->bootRecord.size;
	ATASector s;
	
	// Loop from the first index block to the last index block
	for(i = mp->offset + 1; i < end; i += FS_SECT_PER_IB) {
		// Load the sector
		_ata_read_sector(*(mp->device), i, &s);

		// Start iterating over the pointers in the table
		for(j = FS_FP_OFFSET; j < FS_FP_END; j += FS_FP_LENGTH) {
			if(_sector_get_long(&s, j) == FS_FP_FREE) {
				// This file pointer entry is free
				// Create a fs ptr to represent it and return it
				FSPointer f;
				f.mp      = mp;
				f.ib      = i - mp->offset;
				f.ibindex = (j - FS_FP_OFFSET) / FS_FP_LENGTH;
				return f;	
			}
		}
	}

	// If we make it here, there are no free pointers
	FSPointer f;
	f.mp = NULL;
	return f;
}

/**
 * Searches for and deletes the requested filename from the provided MountPoint.
 * If the file cannot be found FS_ERR_FILENOTFOUND is returned.
 * @param	MountPoint*	mp			The mountpoint to delete the file from
 * @param	char[8]		filename	The name of the file to delete. Filename
 *									only please, no mountpoint letter.
 * @return	FS_STATUS	FS_SUCCESS on successful deletion. Anything else on
 *						failure.
 */
FS_STATUS _fs_delete_file(MountPoint *mp, char filename[8]) {
	// Find the file
	FSPointer fp = _fs_find_file(mp, filename);

	// Verify we found it
	if(fp.mp == NULL) {
		return FS_ERR_FILENOTFOUND;
	}

	// Load the ib of the file
	ATASector ib;
	_ata_read_sector(*(mp->device), mp->offset + fp.ib, &ib);

	// @TODO: Verify it's an IB
	// Get the sector value from the table
	Uint32 offset = (fp.ibindex * FS_FP_LENGTH) + FS_FP_OFFSET;
	Uint32 sector = _sector_get_long(&ib, offset);

	// Mark the filepointer as available
	_sector_put_long(&ib, offset, FS_FP_FREE);

	// Write it back to the disk
	_ata_write_sector(*(mp->device), mp->offset + fp.ib, &ib);
	
	// Traverse the file chain's sector's and unallocate as we go
	ATASector fileSector;
	while(sector != FS_FILE_EOC) {
		// Grab the sector
		_ata_read_sector(*(mp->device), mp->offset + sector, &fileSector);

		// Unallocate the sector
		_fs_unallocate_sector(mp, sector);

		// Go to next sector
		sector = _sector_get_long(&fileSector, FS_FILE_SECT_OFF);
	}

	return FS_SUCCESS;
}

FILE _fs_create_file(MountPoint *mp, char filename[8]) {
	// Get a free file pointer
	FSPointer fp = _fs_find_empty_fspointer(mp);

	// Get a free sector 
	Uint32 sector = _fs_find_empty_sector(mp);

	// Make sure they're valid
	if(fp.mp == NULL || sector == 0) { 
		FILE f;
		f.code = FS_ERR_FULL;
		return f;
	}

	// Allocate the sector that's free
	_fs_allocate_sector(mp, sector);

	// Allocate the FSPointer ----------------------------------------------
	// Grab the ib sector for the fp
	ATASector ibSector;
	_ata_read_sector(*(mp->device), mp->offset + fp.ib, &ibSector);

	// Put a pointer to the file's first sector
	Uint32 ibindex = (fp.ibindex * FS_FP_LENGTH) + FS_FP_OFFSET;
	_sector_put_long(&ibSector, ibindex, sector);

	// Write it back to the disk
	_ata_write_sector(*(mp->device), mp->offset + fp.ib, &ibSector);

	// Allocate the filename -----------------------------------------------
	ATASector nameSector;
	ibindex = fp.ibindex;
	if(fp.ibindex >= FS_NAME_S1ENTRIES) {
		// Read from second name sector
		_ata_read_sector(*(mp->device), mp->offset + fp.ib + 2, &nameSector);
		ibindex -= FS_NAME_S1ENTRIES;
	} else {
		// Read from first name sector
		_ata_read_sector(*(mp->device), mp->offset + fp.ib + 1, &nameSector);
	}

	// Write the name to the sector
	Uint8 i;
	for(i = 0; i < 8; i++) {
		nameSector[FS_NAME_OFFSET + (ibindex * FS_NAME_SIZE) + i] = filename[i];
	}

	// Reset the sector's flags
	nameSector[FS_NAME_OFFSET + (ibindex * FS_NAME_SIZE) + 8] = 0x0;

	// Write the sector back to th disk
	if(fp.ibindex >= FS_SECTOR_SIZE) {
		// Write to second name sector
		_ata_write_sector(*(mp->device), mp->offset + fp.ib + 2, &nameSector);
	} else {
		// Write to first name sector
		_ata_write_sector(*(mp->device), mp->offset + fp.ib + 1, &nameSector);
	}

	// Allocate 0th Sector of File -----------------------------------------
	// Grab the sector that the file will start on
	ATASector fileSector;
	_ata_blank_sector(&fileSector);

	// Just set the header and write it back to the disk
	fileSector[0]=0x46;fileSector[1]=0x49;fileSector[2]=0x4C;fileSector[3]=0x45;
	_ata_write_sector(*(mp->device), mp->offset + sector, &fileSector);

	// Store the filesector as the bufferred sector
	_fs_copy_sector(&fileSector, &fp.buffer);
	fp.bufsect  = sector;
	fp.bufindex = 0;

	// Return a FILE pointer -----------------------------------------------
	FILE f;
	f.fp = fp;
	f.offset = 0;
	
	// Mark the file in use
	_fs_toggle_file(&fp);

	// @TODO: Find errors
	return f;
}

FSPointer _fs_find_file(MountPoint *mp, char filename[8]) {
	// Start iterating over all the index blocks in the partition
	Uint32 start = mp->offset + 1;
	Uint32 end   = mp->offset + mp->bootRecord.size;
	Uint32 i, j;
	ATASector nameSector;
	
	// Prebuild the file pointer to save code
	FSPointer fp;
	fp.mp = mp;

	for(i = start; i <= end; i += FS_SECT_PER_IB) {
		// Read the first name sector (ib + 1)
		_ata_read_sector(*(mp->device), i + 1, &nameSector);

		// Does the file exist in this name file
		for(j = 0; j < FS_NAME_S1ENTRIES; j++) {
			// Compare the file name
			if(_fs_namecmp(&nameSector, j * FS_NAME_SIZE, filename) == 0) {
				// Grab the ib of the file and get the file's first sector
				ATASector ib;
				_ata_read_sector(*(mp->device), mp->offset + i - 1, &ib);
				Uint32 fsect = _sector_get_long(&ib, FS_FP_OFFSET + (j * FS_FP_LENGTH));
				
				// Grab the first sector of the file for buffering
				ATASector fileSect;
				_ata_read_sector(*(mp->device), mp->offset + fsect, &fileSect);

				// Return the found file
				fp.ib       = i / FS_SECT_PER_IB +1;
				fp.ibindex  = j;
				_fs_copy_sector(&fileSect, &fp.buffer);
				fp.bufindex = 0;
				return fp;
			}
		}

		// File wasn't in this sector of names. Maybe the next? (ib+2)
		_ata_read_sector(*(mp->device), i+2, &nameSector);
		
		// Does the file exist in this name file
		for(j = 0; j < FS_NAME_S2ENTRIES; j++) {
			// Compare the filenames
			if(_fs_namecmp(&nameSector, j * FS_NAME_SIZE, filename) == 0) {
				// Grab the index block of the file and get the file's first
				// sector index
				ATASector ib;
				_ata_read_sector(*(mp->device), mp->offset + i - 1, &ib);
				Uint32 fsect = _sector_get_long(&ib, FS_FP_OFFSET + (j * FS_FP_LENGTH) + FS_NAME_S1ENTRIES);

				// Grab the first sector of the file for buffering
				ATASector fileSect;
				_ata_read_sector(*(mp->device), mp->offset + fsect, &fileSect);

				// Return the found file
				fp.ib       = i / FS_SECT_PER_IB + 1;
				fp.ibindex  = j + 64;
				_fs_copy_sector(&fileSect, &fp.buffer);
				fp.bufsect  = fsect;
				fp.bufindex = 0;
				return fp;
			}
		}
	}

	// We didn't find it
	fp.mp = NULL;
	return fp;
}

int _fs_namecmp(ATASector *sect, Uint16 index, char name[8]) {
	// Iterate over the chars and see if they match
	Uint8 i;
	for(i = 0; i < 8; i++) {
		if((*sect)[FS_NAME_OFFSET + index + i] != name[i]) {
			return -1;
		}
	}

	// They match!
	return 0;
}

/**
 * Given a filepointer, figure out what the size of the file is
 * @param	FSPointer*	fp	A file pointer to the file we want to size
 * @return	Uint64	The size of the file in bytes (up to like 10 exabytes...)
 */
Uint64 _fs_get_file_size(FSPointer fp) {
	// Check for sanity
	if(fp.mp == NULL) {
		return 0;
	}

	// Get the sector of the ib for this file
	ATASector ib;
	_ata_read_sector(*(fp.mp->device), fp.mp->offset + fp.ib, &ib);
	if(ib[0] != 'I' || ib[1] != 'B') {
		__panic("NOT AN IB!!");
	}

	// Read the sector number for the file
	Uint32 sector = _sector_get_long(&ib, FS_FP_OFFSET + (fp.ibindex * FS_FP_LENGTH));
	Uint64 size = 0;

	// Read the sector of the file
	ATASector s;
	while(sector != FS_FILE_EOC) {
		// Read the sector
		_ata_read_sector(*(fp.mp->device), fp.mp->offset + sector, &s);

		// Add the bytes allocated
		size += _sector_get_long(&s, FS_FILE_BYTE_OFF);

		// Get the next sector number
		sector = _sector_get_long(&s, FS_FILE_SECT_OFF);
	}

	return size;
}

/**
 * Marks a sector on a given mountpoint as allocated. Chains into
 * toggle_sector.
 * @param	MountPoint*	mp		The mountpoint of the sector
 * @param	Uint32		sector	The sector to allocate
 */
void _fs_allocate_sector(MountPoint *mp, Uint32 sector) {
	_fs_toggle_sector(mp, sector);
}

/**
 * Marks a sector on a given mountpoint as unallocated. Chains into
 * toggle_sector.
 * @param	MountPoint*	mp		The mountpoint of the sector
 * @param	Uint32		sector	The sector to allocate
 */
void _fs_unallocate_sector(MountPoint *mp, Uint32 sector) {
	_fs_toggle_sector(mp, sector);
}

/**
 * Marks a sector on a given mountpoint as allocated. Toggles the sector
 * by xor'ing the bit that marks the sector's allocation.
 * @param	MountPoint*	mp		The mountpoint of the sector
 * @param	Uint32		sector	The sector to toggle
 */
void _fs_toggle_sector(MountPoint *mp, Uint32 sector) {
	// Read the ib sector for the bitfield
	Uint32 ibAddr = mp->offset + ((sector / FS_SECT_PER_IB) * FS_SECT_PER_IB) + 1;
	ATASector s;
	_ata_read_sector(*(mp->device), ibAddr, &s);

	// Toggle the appropriate sector bit
	Uint8 mask  = FS_BT_ALLOCATED << ((sector % FS_SECT_PER_IB) % 8 - 1);
	Uint16 byte = FS_BT_END - ((sector % FS_SECT_PER_IB) / 8) - 1;
	s[byte] ^= mask;

	// Write it back to the disk
	_ata_write_sector(*(mp->device), ibAddr, &s);
}

void _fs_toggle_file(FSPointer *fp) {
	// Read the name sector for the file
	Uint32 nameSector = fp->mp->offset + fp->ib + 1;
	nameSector += (fp->ibindex > FS_NAME_S1ENTRIES) ? 1 : 0;

	ATASector sector;
	_ata_read_sector(*(fp->mp->device), nameSector, &sector);
	
	// @TODO: Verify it's a name sector
	
	// Toggle the appropriate bit
	Uint16 byte = FS_NAME_OFFSET + (fp->ibindex * FS_NAME_SIZE) + FS_NAME_FLAGS;
	byte -= (fp->ibindex > FS_NAME_S1ENTRIES) ? FS_NAME_S1ENTRIES * FS_NAME_SIZE : 0;
	sector[byte] ^= FS_NAME_FLAG_INUSE;
	
	// Write it back to the disk
	_ata_write_sector(*(fp->mp->device), nameSector, &sector);
}


void _fs_copy_sector(ATASector *source, ATASector *dest) {
	// Start copying bytes
	Uint16 b;
	for(b = 0; b < FS_SECTOR_SIZE; b++) {
		(*dest)[b] = (*source)[b];
	}
}
