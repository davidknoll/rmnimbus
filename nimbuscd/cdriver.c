#include <dos.h>
#include "driver.h"

static int cd_target = -1;

void ioctlinput(void)
{
	rh3_t far *rh3 = (rh3_t far *) rhptr;
	switch (rh3->buf[0]) {
	case 0:		// Return Address of Device Header
		((ii0_t far *) rh3->buf)->devhdr = &deviceheader;
		break;
	case 6:		// Device Status
		((ii6_t far *) rh3->buf)->params = 0x000;	// Temporary
		break;
	case 7:		// Return Sector Size
		((ii7_t far *) rh3->buf)->secsz = 2048;		// Temporary
		break;
	case 8:		// Return Volume Size
		sasi_read_capacity((unsigned char) cd_target, 0, &((ii8_t far *) rh3->buf)->volsz, 0);
		((ii8_t far *) rh3->buf)->volsz++;
		break;
	case 9:		// Media Changed
		((ii9_t far *) rh3->buf)->media = 1;		// Temporary
		break;
	default:
		rhptr->status = DONE | ERROR | UNKNOWN;
		return;
	}
	rhptr->status = DONE;
}

void inputflush(void) { rhptr->status = DONE; }
void ioctloutput(void) { rhptr->status = DONE; }
void deviceopen(void) { rhptr->status = DONE; }
void deviceclose(void) { rhptr->status = DONE; }
void badcommand(void) { rhptr->status = DONE | ERROR | UNKNOWN; }

void readlong(void)
{
	// TODO: Addressing and read mode currently ignored/assumed
	rh128_t far *rh128 = (rh128_t far *) rhptr;
	int bytes = sasi_read((unsigned char) cd_target, 0, rh128->start, rh128->count, rh128->buf);
	if (bytes < 0) {
		rhptr->status = DONE | ERROR | GENERALFAILURE;
	} else {
		rhptr->status = DONE;
	}
}

void readlongprefetch(void) { rhptr->status = DONE; }
void seek(void) { rhptr->status = DONE; }
void playaudio(void) { badcommand(); }
void stopaudio(void) { badcommand(); }
void resumeaudio(void) { badcommand(); }

/* Initialisation code below (can be overwritten) */

void init(void)
{
	int i;
	unsigned char target, dbuf[36];
	rh0_t far *rh0 = (rh0_t far *) rhptr;
	rh0->nunits = 0;
	bdos(9, (unsigned int) "\r\n\
SCSI CD-ROM device driver for RM Nimbus PC-186\r\n\
(c) 2023 David Knoll\r\n$", 0);

	/* Scan SCSI targets for a CD-ROM device */
	for (target = 0; target < 8; target++) {
		i = sasi_inquiry(target, 0, 36, dbuf);
		if (i < 0) { i = sasi_inquiry(target, 0, 36, dbuf); }
		if (i == 36 && dbuf[0] == 0x05) {
			/* Found one, print the vendor / product / version */
			bdos(9, (unsigned int) "\tTarget $", 0);
			bdos(6, target + '0', 0);
			bdos(9, (unsigned int) ": $", 0);
			for (i = 8; i < 36; i++) { bdos(6, dbuf[i], 0); }
			bdos(9, (unsigned int) "\r\n$", 0);
			break;
		}
	}

	/* No CD-ROM device found */
	if (target == 8) {
		bdos(9, (unsigned int) "\tNo CD-ROM device found.\r\n\r\n$", 0);
		deviceheader.units = 0;
		rh0->brkadr = (char far *) &deviceheader;
		rh0->rh.status = DONE;
		return;
	}

	cd_target = target;
	bdos(9, (unsigned int) "\r\n$", 0);
	deviceheader.units = 1;
	rh0->brkadr = (char far *) init;
	rh0->rh.status = DONE;
}
