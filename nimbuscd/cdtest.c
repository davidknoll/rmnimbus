#include <stdio.h>
#include <string.h>
#include "driver.h"

unsigned char cmdbuf[16], secbuf[4096];

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
	bytes = sasi_docommand((unsigned char) target, cmdbuf, secbuf);
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
	bytes = sasi_docommand((unsigned char) target, cmdbuf, secbuf);
	printf("Read ID %d sector %d, sasi_docommand returned %d:\n", target, sector, bytes);
	hexdump(cmdbuf, 16);
	if (bytes > 8) { hexdump(secbuf, bytes - 8); }
}

int main(int argc, char *argv[])
{
	int i;
	rh0_t rh_init;
	rh3_t rh_ioctl;
	rh128_t rh_read;

	printf("cdtest\n======\n\n");

	for (i = 0; i < 8; i++) { test_inquiry(i); }
	test_sector(0, 0);
	test_sector(0, 1);
	test_sector(1, 0);
	test_sector(6, 16);

	rh_init.rh.cmd = 0;
	rhptr = (rh_t far *) &rh_init;
	init();

	rh_ioctl.rh.cmd = 3;
	rhptr = (rh_t far *) &rh_ioctl;
	ioctlinput();
	printf("Capacity: %d sectors\n", ((ii8_t far *) rh_ioctl.buf)->volsz);

	rh_read.rh.cmd = 128;
	rh_read.adr_mode = 0;
	rh_read.buf = secbuf;
	rh_read.count = 2;
	rh_read.start = 16;
	rh_read.rmode = 0;
	rh_read.isize = 0;
	rh_read.iskip = 0;
	rhptr = (rh_t far *) &rh_read;
	readlong();
	hexdump(secbuf, 4096);

	return 0;
}
