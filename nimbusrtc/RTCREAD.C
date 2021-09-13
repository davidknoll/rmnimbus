/**
 * Set the DOS date/time from the RTC
 * For my RTC board on the RM Nimbus PC-186
 * @author David Knoll <david@davidknoll.me.uk>
 * @license MIT
 */
#include <conio.h>
#include <dos.h>
#include <stdio.h>

#define ror(b) ((((b) << 7) & 0x80) | ((b) >> 1))
#define bcdtobin(b) ((((b) >> 4) * 10) + ((b) & 0x0F))

unsigned int iobase = 0x600; // Default to slot 4
unsigned char rtcmode;

unsigned char rdrtcbin(unsigned char addr)
{
	unsigned char data = ror(inp(iobase + (addr << 1))), result;
	// If reading an RTC register in BCD mode, convert that
	if ((addr <= 0x09 || addr == 0x32) && !(rtcmode & 0x4)) {
		// Hours register in 12h clock?
		if (addr >= 0x04 && addr <= 0x05 && !(rtcmode & 0x2)) {
			result = bcdtobin(data & 0x7F) % 12;
			if (data & 0x80) { result += 12; }
			return result;
		} else {
			return bcdtobin(data);
		}
	// RTC registers are in binary, and/or we're looking at RAM
	} else {
		// Hours register in 12h clock?
		if (addr >= 0x04 && addr <= 0x05 && !(rtcmode & 0x2)) {
			result = (data & 0x7F) % 12;
			if (data & 0x80) { result += 12; }
			return result;
		} else {
			return data;
		}
	}
}

int main(int argc, char *argv[])
{
	union REGS regs;
	// Slot number can be specified with switch /0 to /4
	if (
		argc >= 2 &&
		argv[1][0] == '/' && argv[1][2] == 0x00 &&
		argv[1][1] >= '0' && argv[1][1] <= '4'
	) {
		iobase = 0x400 + ((argv[1][1] - '0') * 0x80);
	}
	printf("Reading DS12C887-compatible RTC at %03Xh\n", iobase);
	printf("(c) 2021 David Knoll\n");
	if (!(rdrtcbin(0x0D) & 0x80)) {
		fprintf(stderr, "Warning: RTC battery is dead\n");
	}

	rtcmode = rdrtcbin(0x0B);
	if (rdrtcbin(0x0A) & 0x70 != 0x20 || rtcmode & 0x80) {
		fprintf(stderr, "Error: RTC is halted\n\n");
		return 1;
	}
	while (rdrtcbin(0x0A) & 0x80); // Wait for not UIP

	regs.h.ah = 0x2B;           // Set system date
	regs.x.cx = (rdrtcbin(0x32) * 100) + rdrtcbin(0x09); // Year
	regs.h.dh = rdrtcbin(0x08); // Month
	regs.h.dl = rdrtcbin(0x07); // Date
	printf("DOS time has been set to: %02d/%02d/%04d",
		regs.h.dl, regs.h.dh, regs.x.cx);
	intdos(&regs, &regs);

	regs.h.ah = 0x2D;           // Set system time
	regs.h.ch = rdrtcbin(0x04); // Hour
	regs.h.cl = rdrtcbin(0x02); // Minute
	regs.h.dh = rdrtcbin(0x00); // Second
	regs.h.dl = 0;              // 1/100 second
	printf(" %02d:%02d:%02d\n\n", regs.h.ch, regs.h.cl, regs.h.dh);
	intdos(&regs, &regs);

	return 0;
}
