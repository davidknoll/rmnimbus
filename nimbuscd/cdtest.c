#include <conio.h>
#include <stdio.h>
#include "ioports.h"

unsigned char cmdbuf[16], secbuf[4096];

// Run a single command to completion on the specified target
// Interrupts and DMA are not used
// TODO / consider: Timeouts, error handling, bus priority/arbitration,
// bus disconnect, FDC coexistence/busy check re control port, buffer safety
int sasi_docommand(int target, unsigned char *cbuf, unsigned char *dbuf)
{
	int bytes = 0;

	// Wait for bus to be clear, disable DMA
	while (inp(IO_SASI_STATUS) & 0x10);
	while (inp(IO_SASI_STATUS) & 0x10);
	outp(IO_FDC_CONTROL, 0x00);

	// Output target ID, pulse select until the bus becomes busy
	outp(IO_SASI_COMMAND, 1 << target);
	outp(IO_SASI_CONTROL, 0x02);
	while (!(inp(IO_SASI_STATUS) & 0x10));
	outp(IO_SASI_CONTROL, 0x00);

	while (inp(IO_SASI_STATUS) & 0x10) {		// Busy?
		switch (inp(IO_SASI_STATUS) & 0xE8) {	// Req, C/D, I/O, Msg
		case 0x88:	// Output data
			outp(IO_SASI_DATA, *dbuf++);
			bytes++;
			break;
		case 0xA8:	// Input data
			*dbuf++ = inp(IO_SASI_DATA);
			bytes++;
			break;
		case 0xC8:	// Output command
			outp(IO_SASI_COMMAND, *cbuf++);
			bytes++;
			break;
		case 0xE0:	// Input 2nd status byte (command completed)
		case 0xE8:	// Input 1st status byte
			*cbuf++ = inp(IO_SASI_COMMAND);
			bytes++;
			break;
		case 0x80:	// Unknown states with /MSG
		case 0xA0:
		case 0xC0:
			// Is this an unexpected/error condition?
			break;
		default:	// No request
			// Target is processing, and not immediately waiting
			// for a byte to be transferred. Spin for a bit.
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

int main(int argc, char *argv[])
{
	int bytes;
	printf("cdtest\n======\n\n");

	cmdbuf[0] = 0x08;
	cmdbuf[1] = cmdbuf[2] = cmdbuf[3] = cmdbuf[5] = 0x00;
	cmdbuf[4] = 0x01;
	bytes = sasi_docommand(0, cmdbuf, secbuf);
	printf("Read ID 0 sector 0, sasi_docommand returned %d:\n", bytes);
	hexdump(cmdbuf, 16);
	hexdump(secbuf, 512);

	cmdbuf[0] = 0x08;
	cmdbuf[1] = cmdbuf[2] = cmdbuf[5] = 0x00;
	cmdbuf[3] = cmdbuf[4] = 0x01;
	bytes = sasi_docommand(0, cmdbuf, secbuf);
	printf("Read ID 0 sector 1, sasi_docommand returned %d:\n", bytes);
	hexdump(cmdbuf, 16);
	hexdump(secbuf, 512);

	cmdbuf[0] = 0x08;
	cmdbuf[1] = cmdbuf[2] = cmdbuf[3] = cmdbuf[5] = 0x00;
	cmdbuf[4] = 0x01;
	bytes = sasi_docommand(1, cmdbuf, secbuf);
	printf("Read ID 1 sector 0, sasi_docommand returned %d:\n", bytes);
	hexdump(cmdbuf, 16);
	hexdump(secbuf, 512);

	cmdbuf[0] = 0x08;
	cmdbuf[1] = cmdbuf[2] = cmdbuf[5] = 0x00;
	cmdbuf[3] = 0x10;
	cmdbuf[4] = 0x01;
	bytes = sasi_docommand(6, cmdbuf, secbuf);
	printf("Read ID 6 sector 16, sasi_docommand returned %d:\n", bytes);
	hexdump(cmdbuf, 16);
	hexdump(secbuf, 2048);

	return 0;
}
