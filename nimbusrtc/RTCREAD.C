/**
 * Set the DOS date/time from the RTC
 * For my RTC board on the RM Nimbus PC-186
 * @author David Knoll <david@davidknoll.me.uk>
 * @license MIT
 */
#include <conio.h>
#include <dos.h>
#include <stdio.h>

#define RTCSLOT 4

#define ror(b) (((b << 7) & 0x80) | (b >> 1))
#define bcdtobin(b) (((b >> 4) * 10) + (b & 0x0F))
#define rtcaddr(a) ((a * 2) + (RTCSLOT * 0x80) + 0x400)

int main(int argc, char *argv[])
{
	union REGS regs;
	if ((ror(inp(rtcaddr(0x0D))) & 0x80) != 0x80) {
		fprintf(stderr, "Warning: RTC battery is dead\n");
	}
	if (
		((ror(inp(rtcaddr(0x0B))) & 0x86) != 0x02) ||
		((ror(inp(rtcaddr(0x0A))) & 0x70) != 0x20)
	) {
		fprintf(stderr, "Error: RTC halted or not set to BCD 24h\n");
		return 1;
	}
	while (ror(inp(rtcaddr(0x0A))) & 0x80); // Wait for not UIP

	regs.h.ah = 0x2B; // Set system date
	regs.x.cx = (bcdtobin(ror(inp(rtcaddr(0x32)))) * 100) +
		bcdtobin(ror(inp(rtcaddr(0x09))));     // Year
	regs.h.dh = bcdtobin(ror(inp(rtcaddr(0x08)))); // Month
	regs.h.dl = bcdtobin(ror(inp(rtcaddr(0x07)))); // Date
	printf("DOS time has been set to: %02d/%02d/%04d",
		regs.h.dl, regs.h.dh, regs.x.cx);
	intdos(&regs, &regs);

	regs.h.ah = 0x2D; // Set system time
	regs.h.ch = bcdtobin(ror(inp(rtcaddr(0x04)))); // Hour
	regs.h.cl = bcdtobin(ror(inp(rtcaddr(0x02)))); // Minute
	regs.h.dh = bcdtobin(ror(inp(rtcaddr(0x00)))); // Second
	regs.h.dl = 0;
	printf(" %02d:%02d:%02d\n", regs.h.ch, regs.h.cl, regs.h.dh);
	intdos(&regs, &regs);

	return 0;
}
