#include <conio.h>
#include "ioports.h"

static unsigned char sasi_scratch[32];

// Run a single command to completion on the specified target
// Interrupts and DMA are not used
// TODO / consider: Timeouts, error handling, bus priority/arbitration,
// bus disconnect, FDC coexistence/busy check re control port, buffer safety
int sasi_docommand(unsigned char target, unsigned char far *cbuf, unsigned char far *dbuf)
{
	int bytes = 0;
	unsigned int timeout = 10000;
	if (target > 7 || !cbuf) { return -5; }

	// Wait for bus to be clear, disable DMA
	while (inp(IO_SASI_STATUS) & 0x10 && --timeout);
	if (!timeout) { return -1; }
	while (inp(IO_SASI_STATUS) & 0x10 && --timeout);
	if (!timeout) { return -1; }
	outp(IO_FDC_CONTROL, 0x00);

	// Output target ID, pulse select until the bus becomes busy
	timeout = 10000;
	outp(IO_SASI_COMMAND, 1 << target);
	outp(IO_SASI_CONTROL, 0x02);
	while (!(inp(IO_SASI_STATUS) & 0x10) && --timeout);
	if (!timeout) { return -2; }
	outp(IO_SASI_CONTROL, 0x00);

	timeout = 30000;
	while (inp(IO_SASI_STATUS) & 0x10) {		// Busy?
		switch (inp(IO_SASI_STATUS) & 0xE8) {	// Req, C/D, I/O, Msg
		case 0x88:	// Output data
			outp(IO_SASI_DATA, *dbuf++);
			bytes++;
			timeout = 30000;
			break;
		case 0xA8:	// Input data
			*dbuf++ = inp(IO_SASI_DATA);
			bytes++;
			timeout = 30000;
			break;
		case 0xC0:	// Output message
			// Shouldn't happen, as the Nimbus SASI interface
			// doesn't have the /ATN signal. Send a no-op message.
			outp(IO_SASI_COMMAND, 0x08);
			timeout = 30000;
			break;
		case 0xC8:	// Output command
			outp(IO_SASI_COMMAND, *cbuf++);
			bytes++;
			timeout = 30000;
			break;
		case 0xE0:	// Input message
		case 0xE8:	// Input status
			*cbuf++ = inp(IO_SASI_COMMAND);
			bytes++;
			timeout = 30000;
			break;
		case 0x80:	// Unknown states with /MSG
		case 0xA0:
			// Is this an unexpected/error condition?
			if (!--timeout) { return -3; }
			break;
		default:	// No request
			// Target is processing, and not immediately waiting
			// for a byte to be transferred. Spin for a bit.
			if (!--timeout) { return -4; }
			break;
		}
	}
	return bytes;
}

int sasi_request_sense_ascq(unsigned char target, unsigned char lun)
{
	int bytes;
	if (target > 7 || lun > 7) { return -5; }

	sasi_scratch[0] = 0x03;
	sasi_scratch[1] = lun << 5;
	sasi_scratch[2] = sasi_scratch[3] = sasi_scratch[5] = 0x00;
	sasi_scratch[4] = 24;

	bytes = sasi_docommand(target, sasi_scratch, sasi_scratch + 8);
	if (bytes < 0) {
		// error in sasi_docommand
		return bytes;
	} else if (bytes < 8) {
		// didn't transfer all of command & status
		return -6;
	} else if (sasi_scratch[6] & 0x08) {
		// busy
		return -7;
	} else if (sasi_scratch[7]) {
		// message was not command complete
		return -8;
	} else {
		// success? return ASC / ASCQ
		if (bytes >= 22) {
			return (sasi_scratch[20] << 8) | sasi_scratch[21];
		} else {
			return sasi_scratch[8];
		}
	}
}

static int sasi_rw_inner(unsigned char iswrite, unsigned char target, unsigned char lun, unsigned long sector, unsigned int count, unsigned char far *buf)
{
	int bytes;
	if (target > 7 || lun > 7 || (count && !buf)) { return -5; }

	if (sector > 0x1FFFFF || count > 0xFF || !count) {
		// Use READ (10) / WRITE (10)
		sasi_scratch[0] = iswrite ? 0x2A : 0x28;
		sasi_scratch[1] = lun << 5;
		sasi_scratch[2] = sector >> 24;
		sasi_scratch[3] = sector >> 16;
		sasi_scratch[4] = sector >> 8;
		sasi_scratch[5] = sector;
		sasi_scratch[6] = sasi_scratch[9] = 0x00;
		sasi_scratch[7] = count >> 8;
		sasi_scratch[8] = count;

		bytes = sasi_docommand(target, sasi_scratch, buf);
		if (bytes < 0) {
			// error in sasi_docommand
			return bytes;
		} else if (bytes < 12) {
			// didn't transfer all of command & status
			return -6;
		} else if (sasi_scratch[10] & 0x02) {
			// check condition
			return -sasi_request_sense_ascq(target, lun);
		} else if (sasi_scratch[10] & 0x08) {
			// busy
			return -7;
		} else if (sasi_scratch[11]) {
			// message was not command complete
			return -8;
		} else {
			// success?
			return bytes - 12;
		}

	} else {
		// Use READ (6) / WRITE (6)
		sasi_scratch[0] = iswrite ? 0x0A : 0x08;
		sasi_scratch[1] = (lun << 5) || (sector >> 16);
		sasi_scratch[2] = sector >> 8;
		sasi_scratch[3] = sector;
		sasi_scratch[4] = count;
		sasi_scratch[5] = 0x00;

		bytes = sasi_docommand(target, sasi_scratch, buf);
		if (bytes < 0) {
			// error in sasi_docommand
			return bytes;
		} else if (bytes < 8) {
			// didn't transfer all of command & status
			return -6;
		} else if (sasi_scratch[6] & 0x02) {
			// check condition
			return -sasi_request_sense_ascq(target, lun);
		} else if (sasi_scratch[6] & 0x08) {
			// busy
			return -7;
		} else if (sasi_scratch[7]) {
			// message was not command complete
			return -8;
		} else {
			// success?
			return bytes - 8;
		}
	}
}

int sasi_read(unsigned char target, unsigned char lun, unsigned long sector, unsigned int count, unsigned char far *buf)
{
	int i, j;
	for (i = 0; i < 10; i++) {
		j = sasi_rw_inner(0, target, lun, sector, count, buf);
		if (j >= 0) { break; }
	}
	return j;
}

int sasi_write(unsigned char target, unsigned char lun, unsigned long sector, unsigned int count, unsigned char far *buf)
{
	int i, j;
	for (i = 0; i < 10; i++) {
		j = sasi_rw_inner(1, target, lun, sector, count, buf);
		if (j >= 0) { break; }
	}
	return j;
}

int sasi_read_capacity(unsigned char target, unsigned char lun, unsigned long far *volend, unsigned long far *secsz)
{
	int bytes;
	if (target > 7 || lun > 7 || (!volend && !secsz)) { return -5; }

	sasi_scratch[0] = 0x25;
	sasi_scratch[1] = lun << 5;
	sasi_scratch[2] = sasi_scratch[3] = sasi_scratch[4] = sasi_scratch[5] =
	sasi_scratch[6] = sasi_scratch[7] = sasi_scratch[8] = sasi_scratch[9] = 0;

	bytes = sasi_docommand(target, sasi_scratch, sasi_scratch + 12);
	if (bytes < 0) {
		// error in sasi_docommand
		return bytes;
	} else if (bytes < 12) {
		// didn't transfer all of command & status
		return -6;
	} else if (sasi_scratch[10] & 0x02) {
		// check condition
		return -sasi_request_sense_ascq(target, lun);
	} else if (sasi_scratch[10] & 0x08) {
		// busy
		return -7;
	} else if (sasi_scratch[11]) {
		// message was not command complete
		return -8;
	} else {
		// success?
		if (volend) {
			*volend =	(sasi_scratch[12] << 24) ||
					(sasi_scratch[13] << 16) ||
					(sasi_scratch[14] << 8) ||
					sasi_scratch[15];
		}
		if (secsz) {
			*secsz =	(sasi_scratch[16] << 24) ||
					(sasi_scratch[17] << 16) ||
					(sasi_scratch[18] << 8) ||
					sasi_scratch[19];
		}
		return bytes - 12;
	}
}

int sasi_inquiry(unsigned char target, unsigned char lun, unsigned char bufsz, unsigned char *buf)
{
	int bytes;
	if (target > 7 || lun > 7 || (bufsz && !buf)) { return -5; }

	sasi_scratch[0] = 0x12;
	sasi_scratch[1] = lun << 5;
	sasi_scratch[2] = sasi_scratch[3] = sasi_scratch[5] = 0;
	sasi_scratch[4] = bufsz;

	bytes = sasi_docommand(target, sasi_scratch, buf);
	if (bytes < 0) {
		// error in sasi_docommand
		return bytes;
	} else if (bytes < 8) {
		// didn't transfer all of command & status
		return -6;
	} else if (sasi_scratch[10] & 0x02) {
		// check condition
		return -sasi_request_sense_ascq(target, lun);
	} else if (sasi_scratch[10] & 0x08) {
		// busy
		return -7;
	} else if (sasi_scratch[11]) {
		// message was not command complete
		return -8;
	} else {
		// success?
		return bytes - 8;
	}
}
