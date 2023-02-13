Projects and tools I've made while tinkering with the RM Nimbus PC-186 & PC-386
===============================================================================

Unless otherwise stated, anything here that was made by me can be used/distributed/etc under the MIT license, and none of it is approved or endorsed by [RM plc](https://www.rm.com/).

* ***8087repro*** - Reproduction of the 8087 option board for the PC-186, using a DS1100 delay line IC instead of EP8274. Not yet tested- I am awaiting prototype PCBs.
* ***idecf*** - Ideas for how to attach a CF card to the PC-386, which uses the MCA bus so the right hardware can be harder to find.
* ***nimbuscd*** - This contains some experiments with using a SCSI CD-ROM drive (initially, emulated with BlueSCSI) on the PC-186 with original DCB. I'm hoping to make it into `nimbuscd.sys`, a CD-ROM device driver to use with MSCDEX.
* ***nimbusmouse*** - I saw [this PS/2 mouse adaptor](https://www.thenimbus.co.uk/upgrades-and-maintenance/ps2mouse) and thought it could be done with a smaller microcontroller (specifically, an ATtiny2313 using [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore)).
  * My version isn't working at the moment, but I've had aspects of it working. Work in progress.
* ***nimbusrtc*** - Attach a DS12C887A RTC pretty much directly to the bus as an expansion card. EAGLE files included, but only a stripboard prototype has been made IRL, which works. Awaiting prototype PCBs.
  * This and similar RTCs are made for a multiplexed bus, so no extra latches and buffers are necessary. But because the Nimbus I/O bus is just half of a 16-bit bus, it only sees accesses where A0 is 0. The bus is shifted by one bit to make sure A0 can be 0 and the RTC registers can all still be accessed. The shift is reversed in software.
  * Not all of the RTC's RAM is accessible due to the available address space in one slot. Only chip selects 0, 2 and 4 can be used, as we also need to make sure A7 is 0 to address the right half of the RTC. Chip select 0 and interrupt 0 are generally reserved for the disk controller. An interrupt is unnecessary for the RTC if only using simple get/set commands.
  * The get/set commands provided can be compiled on the Nimbus itself using Microsoft QuickC under SETPC. They default to expecting the RTC in slot 4, but this can be set with a switch ```/0``` - ```/4```.
* ***PC-386 RAM*** - Incomplete attempts at reproducing a decent size memory module for the PC-386.
* ***Piconet transceiver*** - Facilitates connecting a microcontroller to Piconet. Awaiting prototype PCBs.
* ***piexpansion*** - Put a Raspberry Pi Pico W with an SD card on the PC-186 expansion bus, to hopefully emulate any physical expansion card, even the disk controller. Awaiting prototype PCBs.
* ***reink*** - Generate block 0 for SCSI disks of any geometry, so you're not restricted to the few models in HDFORM.
  * This doesn't write the sector to the disk itself, as I don't have access to the system programmer's manual that would probably tell me how to do that. The sector is written to stdout instead, and you'll need to write it to the disk separately. (Which is made a whole lot simpler if using a SCSI2SD.)
  * You then need to run STAMP and/or HARDDISK on the Nimbus to partition the disk. I found a 64MB limit on partition size, and only one partition is supported per disk.
  * This is not a replacement for an actual low-level format if you need one.
