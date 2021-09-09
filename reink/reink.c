/**
 * Outputs block 0 (SBD parameters) for Winchester drives of arbitrary
 * geometry on the RM Nimbus PC-186. May be suitable for SASI/SCSI drives that
 * don't need low-level formatting by the user, such as more modern drives or
 * the SCSI2SD. You still need to run STAMP and/or HARDDISK after this.
 * Note that those utilities won't create a partition of more than 64MB
 * and won't create more than one DOS partition.
 * This was inspired by extracting the block 0 images for the supported drives
 * from HDFORM4.EXE, and comparing them with the example in section 8.11.A
 * of the Service Manual as available at https://www.thenimbus.co.uk/
 *
 * @author David Knoll <david@davidknoll.me.uk>
 * @license MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void writeuintbe(unsigned char *buf, unsigned int val)
{
  buf[0] = val >> 8;
  buf[1] = val;
}

static void writeuintle(unsigned char *buf, unsigned int val)
{
  buf[0] = val;
  buf[1] = val >> 8;
}

static void writeulongle(unsigned char *buf, unsigned long val)
{
  writeuintle(buf, val);
  writeuintle(buf + 2, val >> 16);
}

int main(int argc, char *argv[])
{
  unsigned int cyl, hd, sec;
  unsigned char buf[512] = {
    // Non-zero bytes here can be the same for any drive
    0xE9, 0xFD, 0x01, 0x52, 0x4D, 0x4C, 0x24, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  if (argc != 5 || isatty(1)) {
    fprintf(stderr, "Usage: %s cyl hd sec \"name\" > out.bin\n", argv[0]);
    return 1;
  }
  cyl = atoi(argv[1]);
  hd = atoi(argv[2]);
  sec = atoi(argv[3]);
  if (cyl * hd * sec > 131072) {
    fprintf(stderr, "Warning: STAMP / HARDDISK will only partition 131072 sectors (64 MB)\n");
  }

  // Insert the supplied parameters into the block 0 image
  // BIOS parameter block
  writeulongle(buf + 0x13, cyl * hd * sec); // Total sectors
  writeuintle(buf + 0x18, sec);
  writeuintle(buf + 0x1A, hd);
  // Controller parameters
  writeuintbe(buf + 0x21, cyl);
  buf[0x23] = hd;
  // Backup parameters
  writeuintle(buf + 0x3E, cyl * hd); // Total tracks
  writeuintle(buf + 0x40, sec);
  writeuintle(buf + 0x48, cyl);
  // Disk name, string terminated by 0xFF
  memset(buf + 0x50, 0, (sizeof buf) - 0x50);
  buf[0x50 + sprintf((char *) (buf + 0x50), "%s", argv[4])] = 0xFF;

  fwrite(buf, sizeof buf, 1, stdout);
  return 0;
}
