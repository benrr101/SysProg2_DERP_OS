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
			Uint32 size   = _sector_get_long(&br, 0x8);
			if(header == FS_PARTITION_ID) {
				// Increment the count of mount points and set up the mp
				Uint8 mp_id = mount_point_count;
				mount_point_count++;
				MountPoint mp = mount_points[mp_id];
				
				// Build the MountPoint
				mp.device = dev;
				mp.bootRecord.header = header;
				mp.bootRecord.size = size;
				mp.offset = brAddr;
				mp.letter = mp_id + 0x41;
			}
		}
	}

	// DEBUG: Output the found partitions
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
	if(start == SECT_MBR) {
		// Can't place a partition over the MBR
		return FS_ERR_BADSECT;
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
	mbr[indexAddr + FS_PART_ENTRY_LBA] = start;
	mbr[indexAddr + FS_PART_ENTRY_SIZE] = size;

	// Write the mbr back to the disk
	_ata_write_sector(*dev, SECT_MBR, &mbr);
	return FS_SUCCESS;
}

void _fs_format(MountPoint *mp, ATADevice *dev, Uint32 size) {
	// Start building structures for the boot record
	
}
