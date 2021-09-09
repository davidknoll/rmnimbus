/**
 * Write the current DOS date/time to the RTC
 * For my RTC board on the RM Nimbus PC-186
 * @author David Knoll <david@davidknoll.me.uk>
 * @license MIT
 */
#include <conio.h>
#include <dos.h>
#include <stdio.h>

#define RTCSLOT 4

#define rol(b) (((b >> 7) & 1) | (b << 1))
#define bintobcd(b) (((b / 10) << 4) | (b % 10))
#define rtcaddr(a) ((a * 2) + (RTCSLOT * 0x80) + 0x400)

int main(int argc, char *argv[])
{
	union REGS regs;
	outp(rtcaddr(0x0B), rol(0x82)); // Halt updates, BCD 24h mode
	outp(rtcaddr(0x0A), rol(0x20)); // Enable oscillator
	inp(rtcaddr(0x0C));             // Clear interrupt flags
	if (!(inp(rtcaddr(0x0D)) & rol(0x80))) {
		fprintf(stderr, "Warning: RTC battery is dead\n");
	}

	regs.h.ah = 0x2A; // Get system date
	intdos(&regs, &regs);
	outp(rtcaddr(0x32), rol(bintobcd(regs.x.cx / 100))); // Century
	outp(rtcaddr(0x09), rol(bintobcd(regs.x.cx % 100))); // Year
	outp(rtcaddr(0x08), rol(bintobcd(regs.h.dh)));       // Month
	outp(rtcaddr(0x07), rol(bintobcd(regs.h.dl)));       // Date
	outp(rtcaddr(0x06), rol(bintobcd(regs.h.al + 1)));   // Day
	printf("RTC has been set to: %02d/%02d/%04d",
		regs.h.dl, regs.h.dh, regs.x.cx);

	regs.h.ah = 0x2C; // Get system time
	intdos(&regs, &regs);
	outp(rtcaddr(0x05), rol(bintobcd(0)));         // Hours Alarm
	outp(rtcaddr(0x04), rol(bintobcd(regs.h.ch))); // Hour
	outp(rtcaddr(0x03), rol(bintobcd(0)));         // Minutes Alarm
	outp(rtcaddr(0x02), rol(bintobcd(regs.h.cl))); // Minute
	outp(rtcaddr(0x01), rol(bintobcd(0)));         // Seconds Alarm
	outp(rtcaddr(0x00), rol(bintobcd(regs.h.dh))); // Second
	printf(" %02d:%02d:%02d\n", regs.h.ch, regs.h.cl, regs.h.dh);

	outp(rtcaddr(0x0B), rol(0x02)); // Resume updates
	return 0;
}
