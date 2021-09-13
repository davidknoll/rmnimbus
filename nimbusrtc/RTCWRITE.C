/**
 * Write the current DOS date/time to the RTC
 * For my RTC board on the RM Nimbus PC-186
 * @author David Knoll <david@davidknoll.me.uk>
 * @license MIT
 */
#include <conio.h>
#include <dos.h>
#include <stdio.h>

#define rol(b) ((((b) >> 7) & 1) | ((b) << 1))
#define bintobcd(b) ((((b) / 10) << 4) | ((b) % 10))

unsigned int iobase = 0x600;  // Default to slot 4
unsigned char rtcmode = 0x02; // Default to BCD 24h when reinit RTC

void wrrtcbin(unsigned char addr, unsigned char data)
{
	// We always write the RTC registers in BCD mode
	if (addr <= 0x09 || addr == 0x32) {
		data = bintobcd(data);
	}
	outp(iobase + (addr << 1), rol(data));
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
	printf("Writing DS12C887-compatible RTC at %03Xh\n", iobase);
	printf("(c) 2021 David Knoll\n");
	if (!(inp(iobase + (0x0D << 1)) & rol(0x80))) {
		fprintf(stderr, "Warning: RTC battery is dead\n");
	}

	wrrtcbin(0x0A, 0x20);           // Enable oscillator
	wrrtcbin(0x0B, rtcmode | 0x80); // Set mode, halt updates
	inp(iobase + (0x0C << 1));      // Clear interrupt flags

	regs.h.ah = 0x2A;                // Get system date
	intdos(&regs, &regs);
	wrrtcbin(0x32, regs.x.cx / 100); // Century
	wrrtcbin(0x09, regs.x.cx % 100); // Year
	wrrtcbin(0x08, regs.h.dh);       // Month
	wrrtcbin(0x07, regs.h.dl);       // Date
	wrrtcbin(0x06, regs.h.al + 1);   // Day (Sun = 0 in DOS, 1 in RTC)
	printf("RTC has been set to: %02d/%02d/%04d",
		regs.h.dl, regs.h.dh, regs.x.cx);

	regs.h.ah = 0x2C;          // Get system time
	intdos(&regs, &regs);
	wrrtcbin(0x05, 0);         // Hours Alarm
	wrrtcbin(0x04, regs.h.ch); // Hour
	wrrtcbin(0x03, 0);         // Minutes Alarm
	wrrtcbin(0x02, regs.h.cl); // Minute
	wrrtcbin(0x01, 0);         // Seconds Alarm
	wrrtcbin(0x00, regs.h.dh); // Second
	printf(" %02d:%02d:%02d\n\n", regs.h.ch, regs.h.cl, regs.h.dh);

	wrrtcbin(0x0B, rtcmode); // Resume updates
	return 0;
}
