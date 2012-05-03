/*
** SCCS ID: @(#)users.c 1.1 4/5/12
**
** File:    sata.c
** Author:  Benjamin Russell
** Description: User routines.
*/

// REQUIRED FILES //////////////////////////////////////////////////////////

#include "headers.h"
#include "pci.h"
#include "startup.h"
#include "sata.h"

// DEFINES /////////////////////////////////////////////////////////////////

// FUNCTIONS ///////////////////////////////////////////////////////////////

Uint16 _sata_get_bar(Uint16 bus, Uint16 device, Uint16 func, Uint16 offset) {
	// Read the register at the offset
	Uint32 reg = _pci_config_readl(bus, device, func, offset);
	// Return the address after bitmasking
	// The format of the register looks like:
	//	31:16	- Reserved
	//	15:3	- Base Address		
	//	 2:1	- Reserved
	//	   0	- Hardwired to 1	<- Not needed, so AND it off
	return (reg & 0xFFFFFFFE);
}


/**
 * Reads a byte from the specified ide channel register. THAR BE MAGIC IN HERE.
 * @param	IDEChannel channel	The channel to read from
 * @param	IDEReg reg			The register on the channel to read from
 * @source	http://wiki.osdev.org/IDE
 */
Uint8 _sata_read_reg(ATAChannel channel, IDERegs reg) {
	Uint8 result;
	if(reg > 0x07 && reg < 0x0C) {
		_sata_write_reg(channel, IDE_REG_CONTROL, 0x80);
	}

	if(reg < 0x08) {
		result = __inb(channel.command + reg - 0x00);
	} else if(reg < 0x0C) {
		result = __inb(channel.command  + reg - 0x06);
	} else if(reg < 0x0E) {
		result = __inb(channel.control  + reg - 0x0A);
	} else if (reg < 0x16) {
		result = __inb(channel.busmast + reg - 0x0E);
	}
   
	if(reg > 0x07 && reg < 0x0C) {
		_sata_write_reg(channel, IDE_REG_CONTROL, 0x00);
	}
   
	return result;
}

Uint16 _sata_read_data(ATAChannel channel) {
	return __inw(channel.command);
}

/**
 * Writes a byte to the specified ide channel reg. THAR BE MAGIC HERE.
 * @param	IDEChannel channel	The channel to write to
 * @param	IDERegs reg			The register on the channel to write to
 * @param	Uint8 payload		The byte to write the the register
 * @source	http://wiki.osdev.org/IDE
 */
void _sata_write_reg(ATAChannel channel, IDERegs reg, Uint8 payload) {
	if(reg > 0x07 && reg < 0x0C) {
		_sata_write_reg(channel, IDE_REG_CONTROL, 0x80);
	} 

	if(reg < 0x08) {
		__outb(channel.command  + reg - 0x00, payload);
	} else if(reg < 0x0C) {
		__outb(channel.command  + reg - 0x06, payload);
	} else if(reg < 0x0E) {
		__outb(channel.control  + reg - 0x0A, payload);
    } else if(reg < 0x16) {
		__outb(channel.busmast + reg - 0x0E, payload);
	}

	if(reg > 0x07 && reg < 0x0C) {
		_sata_write_reg(channel, IDE_REG_CONTROL, 0x0);
	}
}

void _sata_initialize(ATAController *cont, Uint16 bus, Uint16 device, Uint16 func) {
	// Initialize the controller into IDE mode
	_pci_config_write(bus, device, func, SATA_PCI_REG_MAP, 0x00);

	// Store the info about the channels
	(*cont)[ATA_PORT_CHANPRI].command = _sata_get_bar(bus, device, func, SATA_PCI_REG_PCMD);
	(*cont)[ATA_PORT_CHANPRI].control = _sata_get_bar(bus, device, func, SATA_PCI_REG_PCTRL);
	(*cont)[ATA_PORT_CHANSEC].command = _sata_get_bar(bus, device, func, SATA_PCI_REG_SCMD);
	(*cont)[ATA_PORT_CHANSEC].control = _sata_get_bar(bus, device, func, SATA_PCI_REG_SCTRL);
	(*cont)[ATA_PORT_CHANPRI].busmast = _sata_get_bar(bus, device, func, SATA_PCI_REG_BMAST);
	(*cont)[ATA_PORT_CHANSEC].busmast = (*cont)[ATA_PORT_CHANPRI].busmast + 0x8;
}

void _sata_wait() {
	volatile int i;
	for(i = 0; i < 1000000; i++);
}

void _sata_probe(Uint16 bus, Uint16 device, Uint16 func) {
	// Create a controller struct to assist in constructing the drives
	ATAController cont;

	// Initialize the sata and the channel info
	_sata_initialize(&cont, bus, device, func);
	
	// IF the command port is 00, then we need to ignore this invalid device
	if(cont[ATA_PORT_CHANPRI].command == 0) {
		return;
	}

	// Turn off IRQ's for the channels
	_sata_write_reg(cont[ATA_PORT_CHANPRI], IDE_REG_CONTROL, 2);
	_sata_write_reg(cont[ATA_PORT_CHANSEC], IDE_REG_CONTROL, 2);
	
	// Initialize vars for iterating and storing device info
	Uint8 dev;
	Uint8 chan;
	Uint16 identSpace[256];

	// Iterate over the channels to probe
	for(chan = ATA_PORT_CHANPRI; chan <= ATA_PORT_CHANSEC; chan++) {
		// Iterate over each device on the channel
		for(dev = ATA_PORT_CHANMAST; dev <= ATA_PORT_CHANSLAV; dev++) {
			// Tell the controller which drive we want
			_sata_write_reg(cont[chan], IDE_REG_DRIVESEL, 0xA0 | (dev << 4));
			_sata_wait();

			_sata_write_reg(cont[chan], IDE_REG_COMMAND, IDE_CMD_IDENTIFY);
			_sata_wait();
			
			// If the device exists, then identify it and store it
			Uint8 deviceId = ata_device_count;

			Uint8 status = _sata_read_reg(cont[chan], IDE_REG_STATUS);
			if(status & 0x40) {
				ata_devices[deviceId].command = cont[chan].command;
				ata_devices[deviceId].control = cont[chan].control;
				ata_devices[deviceId].busmast = cont[chan].busmast;
				ata_devices[deviceId].channel = chan;
				ata_devices[deviceId].device  = device;
				
				// Increment the number of devices
				ata_device_count++;
			} else {
				// There probably isn't a device (although I'm not too sure about that)
				continue;
			}

			// Determine if the device is ATAPI
			Uint8 lba1 = _sata_read_reg(cont[chan], IDE_REG_LBALOW);
			Uint8 lba2 = _sata_read_reg(cont[chan], IDE_REG_LBAMID);
			if(
				(lba1 == ATAPI_LBA1 && lba2 == ATAPI_LBA2) || 
				(lba1 == ATAPI_ALT_LBA1 && lba2 == ATAPI_ALT_LBA2)
			) {
				// The device is ATAPI
				ata_devices[deviceId].type = ATA_TYPE_ATAPI;
			} else {
				// The device is ATA
				ata_devices[deviceId].type = ATA_TYPE_ATA;
			}

			// Read the identification data into the identSpace var
			Uint16 words;
			Uint16 word;
			for(words = 0; words < 256; words++) {
				// Read from the register
				word = _sata_read_data(cont[chan]);
				
				// Copy into the identspace
				identSpace[words] = word;
			}

			// Grab relevant data from the identify command
			// Serial String - copy byte by byte
			Uint8 m;
			for(m = 0; m < 10; m++) {
				// Copy high byte then low byte
				ata_devices[deviceId].serial[m * 2]     = (char)(identSpace[10 + m] >> 8);
				ata_devices[deviceId].serial[m * 2 + 1] = (char)identSpace[10 + m];
			}
			ata_devices[deviceId].serial[20] = 0x0;		// Null terminate

			// Model String - copy byte by byte
			for(m = 0; m < 20; m++) {
				// Copy high byten then low byte
				ata_devices[deviceId].model[m*2]   = (char)(identSpace[27+m]>>8);
				ata_devices[deviceId].model[m*2+1] = (char)identSpace[27+m];
			}
			ata_devices[deviceId].model[40] = 0x0;		// Null terminate

			// Size in Sectors - OR together the sector count
			Uint32 size = identSpace[101] << 16 | identSpace[100];
			ata_devices[deviceId].size = size;
		}	
	}
}
