#include <stdio.h>
#include "driver.h"

void probedevice(unsigned char target, unsigned char lun)
{
	int bytes;
	unsigned char buf[128];
	unsigned long volend, secsz;

	// Is there a device there that knows REQUEST SENSE?
	sasi_request_sense_ascq(target, lun);
	bytes = sasi_request_sense_ascq(target, lun);
	if (bytes < 0) { return; }

	// Does it also know INQUIRY?
	bytes = sasi_inquiry(target, lun, 128, buf);
	if (bytes < 0) {
		printf("  %d |   %d | Unknown  | SASI Device      |      |            |     |\n", target, lun);
		return;
	}

	// Unsupported (not just unoccupied) LUN
	if ((buf[0] & 0xE0) == 0x60) { return; }

	// Vendor / Product / Revision
	printf("  %d |   %d | ", target, lun);
	for (bytes =  8; bytes < 16; bytes++) { putchar(buf[bytes]); }
	printf(" | ");
	for (bytes = 16; bytes < 32; bytes++) { putchar(buf[bytes]); }
	printf(" | ");
	for (bytes = 32; bytes < 36; bytes++) { putchar(buf[bytes]); }

	// Device type
	switch (buf[0] & 0x1F) {
	case 0x00:
		printf(" | Direct     | ");
		break;
	case 0x01:
		printf(" | Sequential | ");
		break;
	case 0x02:
		printf(" | Printer    | ");
		break;
	case 0x03:
		printf(" | Processor  | ");
		break;
	case 0x04:
		printf(" | WORM       | ");
		break;
	case 0x05:
		printf(" | CD-ROM     | ");
		break;
	case 0x06:
		printf(" | Scanner    | ");
		break;
	case 0x07:
		printf(" | Optical    | ");
		break;
	case 0x08:
		printf(" | Changer    | ");
		break;
	case 0x09:
		printf(" | Comms      | ");
		break;
	case 0x0A:
	case 0x0B:
		printf(" | Pre-press  | ");
		break;
	case 0x0C:
		printf(" | Array      | ");
		break;
	case 0x0D:
		printf(" | Enclosure  | ");
		break;
	case 0x0E:
		printf(" | Red. Block | ");
		break;
	case 0x0F:
		printf(" | Opt. Card  | ");
		break;
	case 0x10:
		printf(" | Expander   | ");
		break;
	case 0x11:
		printf(" | Object     | ");
		break;
	case 0x12:
		printf(" | Automation | ");
		break;
	case 0x13:
		printf(" | Security   | ");
		break;
	case 0x14:
		printf(" | Zoned Blk. | ");
		break;
	case 0x15:
		printf(" | Red. Media | ");
		break;
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
		printf(" | Reserved   | ");
		break;
	case 0x1E:
		printf(" | Well-known | ");
		break;
	case 0x1F:
		printf(" | Unknown    | ");
		break;
	}

	// Removable?
	if (buf[1] & 0x80) {
		printf("Y   | ");
	} else {
		printf("N   | ");
	}

	// Capacity in MiB
	bytes = sasi_read_capacity(target, lun, &volend, &secsz);
	if (bytes >= 0) {
		printf("%lu", ((volend + 1) * secsz) / 1048576);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	unsigned char target, lun;
	printf("Tgt | LUN | Vendor   | Product          | Rev  | Type       | Rm? | Cap. MiB\n");
	printf("----+-----+----------+------------------+------+------------+-----+---------\n");

	for (target = 0; target < 8; target++) {
		for (lun = 0; lun < 8; lun++) {
			probedevice(target, lun);
		}
	}
	return 0;
}
