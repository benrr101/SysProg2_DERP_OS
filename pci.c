/**
 * File:	pci.c
 * Author:	Grant Kurtz
 *
 * Description:
 *		The PCI module is for interfacing with PCI devices, such as finding,
 * identifying, and communicating configuration data with PCI devices.
 */

#include "headers.h"
#include "startup.h"
#include "pci.h"
#include "ata.h"
#include "fs.h"
#include "ufs.h"

/**
 * Scans for all PCI devices, and printing information about discovered devices
 * to the terminal.
 */
void _pci_probe_devices(){
	
	// report that the PCI probe is starting
	c_puts("Probing for PCI Devices...\n");

	// Initialize the number of ATA devices to 0
	ata_device_count = 0;

	// Initialize the mount points
	mount_point_count = 0;

	// Initialize the file pointers
	Uint8 i;
	for(i = 0; i < FS_MAX_FILEPOINTERS; i++) {
		file_pointers[i].code = FS_AVAILABLE;
	}

	// Iterate over the entire PCI bus
	Uint16 bus;
	Uint16 device;
	Uint8 func;
	Uint16 vend;
	Uint8 class;
	Uint8 subclass; 
	for(bus = 0; bus < 255; bus++) {
		for(device = 0; device < 255; device++) {
			for(func = 0; func < 7; func++) {
				// If the vendor ID is not FFFF, then there is a device there
				vend = _pci_config_read(bus, device, func, 0x0);
				if(vend != 0xFFFF) {
					// Get some info about it
					class = _pci_config_readb(bus, device, func, 0xB);
					subclass = _pci_config_read(bus, device, func, 0xA);
	
					// Print the class information
					switch(class) {
						case 0x01:
							// Odds are this is our SATA/IDE controller. Let's
							// probe it and find out.
							_ata_probe(bus, device, func);
							break;
						default:
							// All other devices are unsupported in this kernel
							//c_puts("Unsupported Device");
							break;
					}
				}
			}
		}
	}

	// Print out all the ATA devices found
	c_printf("%d ATA devices found:\n", ata_device_count);
	//Uint8 i;
	for(i = 0; i < ata_device_count; i++) {
		c_printf("   %s Model: %s\n       Serial: %s\n       Sectors: %x\n", 
			(ata_devices[i].type == ATA_TYPE_ATAPI) ? "ATAPI" : "ATA",
			ata_devices[i].model,
			ata_devices[i].serial,
			ata_devices[i].size
			);

		// Probe it for partitions
		_fs_probe(&ata_devices[i]);
	}

	// If there were no partitions found notify the user
	if(mount_point_count == 0 || 1) {
		c_puts("No DERP_FS Partitions Found!\n");
		c_puts("... Creating test partition\n");

		Uint8 code = _fs_create_partition(&ata_devices[1], 0x1, 2048, 0);
		if(code != FS_SUCCESS) {
			c_printf("*** Filesystem creation failed with code 0x%x\n", code);
		}
		code = _fs_format(&mount_points[mount_point_count], &ata_devices[1], 0);
		if(code != FS_SUCCESS) {
			c_printf("*** Filesystem format failed with code 0x%x\n", code);
		}
	}

	c_puts("Beginning tests...\n");
	c_puts("Opening new file\n");
	char buffer[21];
	buffer[20] = 0x0;
	FILE *f = fopen("A:BOOMSHIT");
	if(f == NULL) { __panic("FAILED"); }

	c_puts("Reading 19b from file\n");
	if(fread(f, buffer, 19)) { c_printf("*** read: %s\n", buffer); __panic("FAILED"); }

	c_puts("Writing 19b to the file\n");
	Uint32 result = fwrite(f, "abcdefghijklmnopqrs", 19);
	if(result != 19) { c_printf("*** got back: %d\n",  result); __panic("FAILED"); }
	
	c_puts("Reading 19b from file\n");
	if(fread(f, buffer, 19)) { c_printf("*** read: %s\n", buffer); __panic("FAILED"); }
	
	c_puts("Seeking back 19b\n");
	result = fseek(f, 19, FS_SEEK_REL_REV);
	if(result != FS_SUCCESS) { c_printf("*** got back: 0x%x\n", result); __panic("FAILED"); }

	c_puts("Reading 19b from file\n");
	result = fread(f, buffer, 19);
	if(result != result) { c_printf("*** got back: %d | %s\n", result, buffer); __panic("FAILED"); }
	else { c_printf("... got back: %s\n", buffer); }
	
	c_puts("Seeking 10b ahead\n");
	result = fseek(f, 10, FS_SEEK_REL);
	if(result == FS_SUCCESS) { c_puts("*** successfule\n"); __panic("FAILED"); }

	c_puts("writing to fill 2x sectors\n");
	Uint16 j;
	for(j = 0; j < 580; j++) {
		fwrite(f, "1", 1);
	}
	c_printf("... file buffer is sector %d\n", f->bufsect);
	
	c_puts("Seeking to 0\n");
	if(fseek(f, 0, FS_SEEK_ABS) != FS_SUCCESS) { __panic("FAILED"); }

	c_puts("Seeking to 0x200 on sector 2\n");
	if(fseek(f, 0, FS_SEEK_ABS) != FS_SUCCESS) { __panic("FAILED"); }

	c_puts("Seeking to 0x3F1 on sector 3\n");
	if(fseek(f, 0, FS_SEEK_ABS) != FS_SUCCESS) { __panic("FAILED"); }

	c_puts("Closing the file\n");
	if(fclose(f) != FS_SUCCESS) { __panic("FAILED"); }
	c_printf("... file code is now 0x%x\n", f->code);
	
	c_puts("Reopening the file\n");
	f = fopen("A:BOOMSHIT");
	if(f->code != FS_SUCCESS) { c_printf("*** code is: 0x%x\n", f->code); __panic("FAILED"); }
	if(f->offset != 0) { c_printf("*** offset is: 0x%x\n", f->offset); __panic("FAILED"); }
	if(f->bufindex != 0) { c_printf("*** bufindex is: 0x%x\n", f->bufindex); __panic("FAILED"); }

	// Print out the first sector of drive 0
	__panic("HOLY FUCK.");
}

/**
 * Reads a single word (2-bytes) of data from the specified device's config.
 * 
 * @param	bus		The bus the device is on.
 * @param	slot	Which slot on the bus the device is on.
 * @param	func	Which function of the device to use.  Note, not all devices
 *					are multifunction.
 * @param	offset	The amount to shift the final address by.
 * @return			The word that was read back from the device.  Note, that a
 *					return value of 0xFFFF means an invalid device selection.
 *
 * @source			http://wiki.osdev.org/PCI#Configuration_Mechanism_.231
 */
Uint16 _pci_config_read( Uint16 bus, Uint16 slot, 
		Uint16 func, Uint16 offset){

	// we need to compute the address of the device
	Uint32 addr = _pci_config_get_address(bus, slot, func, offset);

	// select the device
	__outl(CONF_ACC, addr);

	// read the data, we want the first word of the 32-bit register
	return (Uint16) ((__inl(CONF_DAT) >> ((offset & 2) * 8)) & 0xffff);
}

/**
 * Read a byte from the PCI bus. IE read a word from the bus but AND off the
 * upper byte such that a single byte is returned.
 */
Uint8 _pci_config_readb( Uint16 bus, Uint16 device, Uint16 func, 
		Uint16 offset ) {
	Uint16 word = _pci_config_read(bus, device, func, offset);

	// The controller appears to only give back aligned by 2 words. So IF we
	// wanted the upper bytes (%2 = 1) then we'll outsmart the controller and
	// give the user back upper bytes only.
	Uint8 byte;
	if(offset % 2 == 0) {
		// Lower bytes
		byte = word & 0xFFFF;
	} else {
		// Upper bytes
		byte = word >> 8;
	}
	return byte;
}


/**
 * Read a long from the PCI bus. IE read a short from the bus, read another
 * short, shift the second's bits and OR it together.
 */
Uint32 _pci_config_readl( Uint16 bus, Uint16 device, Uint16 func, 
		Uint16 offset ) {
	// Read the two shorts
	Uint16 word1 = _pci_config_read(bus, device, func, offset);
	Uint16 word2 = _pci_config_read(bus, device, func, offset + 2);

	// OR them together
	Uint32 word3 = word2 << 16;
	Uint32 dword = word1 | word3;

	return dword;
}

/**
 * Writes a short to the PCI bus.
 * @param	Uint16	bus		The bus of the PCI device to write to
 * @param	Uint16	device	The device # of the device to write to
 * @param	Uint16	func	The function of the device to write to
 * @param	Uint16	offset	Offset of the register to write to. Must be aligned
 *							to 16-bits
 * @param	Uint16	payload	The short to write to the device
 */
void _pci_config_write( Uint16 bus, Uint16 device, Uint16 func, Uint16 offset,
		Uint16 payload ){
	
	// If we're writing to a non-2byte aligned address, I don't think it will
	// work right
	if(offset % 2 != 0) {
		__panic("Attempting to write to a non-2byte aligned PCI config reg");
	}

	// Get the address we're writing to and tell the controller we're writing
	// to it.
	Uint32 addr = _pci_config_get_address(bus, device, func, offset);
	__outl(CONFIG_ACCESS, addr);
	
	// Write to the data port
	__outw(CONFIG_DATA, payload);
}

/**
 * Writes a long to the PCI bus.
 * @param	Uint16	bus		The bus of the PCI device to write to
 * @param	Uint16	device	The device # of the device to write to
 * @param	Uint16	func	The function of the device to write to
 * @param	Uint16	offset	Offset of the register to write to. Must be aligned
 *							to 32-bits
 * @param	Uint32	payload	The long to write to the device
 */
void _pci_config_writel( Uint16 bus, Uint16 device, Uint16 func, Uint16 offset,
		Uint32 payload ) {
	
	// If we're writing to a non-4byte aligned address, I don't think it'll
	// work right
	if(offset % 4 != 0) {
		__panic("Attempting to write to a non-4byte aligned PCI config reg");
	}

	Uint32 addr = _pci_config_get_address(bus, device, func, offset);
	__outl(CONFIG_ACCESS, addr);
	__outl(CONFIG_DATA, payload);
}


/**
 * Calculates the address for the PCI config space register.
 * How the address is computed:
 * 	Bit  31:		Enable Bit (always on)
 *	Bits 30 - 24:	Reserved (always off)
 *	Bits 23 - 16:	Bus Number
 *	Bits 15 - 11:	Device Number
 *	Bits 10 - 08:	Function Number
 *	Bits 07 - 02:	Register Number
 *	Bits 01 - 00:	Empty (always off)
 * 
 * @param	bus		The bus the device is on.
 * @param	slot	Which slot on the bus the device is on.
 * @param	func	Which function of the device to use.  Note, not all devices
 *					are multifunction.
 * @param	offset	The amount to shift the final address by.
 */
Uint32 _pci_config_get_address( Uint16 bus, Uint16 device, Uint16 func, 
		Uint16 offset ){

		// Turn the 16-bit values into 32-bit values
		Uint32 lbus    = (Uint32) bus;
		Uint32 ldevice = (Uint32) device;
		Uint32 lfunc   = (Uint32) func;

		// Calculate and return the address
		return (Uint32) ((lbus << 16) | (ldevice << 11) | (lfunc << 8) |
			(offset & 0xFC) | ((Uint32) 0x80000000));
}
