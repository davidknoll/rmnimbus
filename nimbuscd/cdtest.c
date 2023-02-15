#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "ioports.h"

unsigned char cmdbuf[16], secbuf[4096];

// Run a single command to completion on the specified target
// Interrupts and DMA are not used
// TODO / consider: Timeouts, error handling, bus priority/arbitration,
// bus disconnect, FDC coexistence/busy check re control port, buffer safety
int sasi_docommand(unsigned char target, unsigned char *cbuf, unsigned char *dbuf)
{
	int bytes = 0;
	unsigned int timeout = 10000;
	if (target > 7 || cbuf == NULL) { return -5; }

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
		case 0xC8:	// Output command
			outp(IO_SASI_COMMAND, *cbuf++);
			bytes++;
			timeout = 30000;
			break;
		case 0xE0:	// Input 2nd status byte (command completed)
		case 0xE8:	// Input 1st status byte
			*cbuf++ = inp(IO_SASI_COMMAND);
			bytes++;
			timeout = 30000;
			break;
		case 0x80:	// Unknown states with /MSG
		case 0xA0:
		case 0xC0:
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

void hexdump(unsigned char *buf, int len)
{
	int i, j;
	for (i = 0; i < len; i += 16) {
		printf("%04X | ", i);
		for (j = 0; j < 16; j++) {
			if (i + j >= len) {
				printf("   ");
			} else {
				printf("%02X ", buf[i + j]);
			}
			if (j == 7) { printf(" "); }
		}
		printf("| ");
		for (j = 0; j < 16; j++) {
			if (i + j >= len) {
				printf(" ");
			} else if (buf[i + j] < 0x20 || buf[i + j] > 0x7E) {
				printf(".");
			} else {
				printf("%c", buf[i + j]);
			}
			if (j == 7) { printf(" "); }
		}
		if (!((i + 16) % 256)) {
			printf("*");
			getch();
		}
		printf("\n");
	}
}

void test_inquiry(int target)
{
	int bytes;
	cmdbuf[0] = 0x12;
	cmdbuf[1] = cmdbuf[2] = cmdbuf[3] = cmdbuf[5] = 0x00;
	cmdbuf[4] = 0x80;
	memset(secbuf, 0x00, 0x80);
	bytes = sasi_docommand(target, cmdbuf, secbuf);
	printf("Target %d inquiry, sasi_docommand returned %d\n", target, bytes);
	if (bytes > 8) { hexdump(secbuf, 0x80); }
	printf("*\n");
	getch();
}

void test_sector(int target, int sector)
{
	int bytes;
	cmdbuf[0] = 0x08;
	cmdbuf[1] = cmdbuf[5] = 0x00;
	cmdbuf[2] = sector >> 8;
	cmdbuf[3] = sector;
	cmdbuf[4] = 0x01;
	bytes = sasi_docommand(target, cmdbuf, secbuf);
	printf("Read ID %d sector %d, sasi_docommand returned %d:\n", target, sector, bytes);
	hexdump(cmdbuf, 16);
	if (bytes > 8) { hexdump(secbuf, bytes - 8); }
}

int main(int argc, char *argv[])
{
	int i;
	printf("cdtest\n======\n\n");

	for (i = 0; i < 8; i++) { test_inquiry(i); }
	test_sector(0, 0);
	test_sector(0, 1);
	test_sector(1, 0);
	test_sector(6, 16);

	return 0;
}
