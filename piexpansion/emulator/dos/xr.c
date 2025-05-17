#include <stdio.h>
#include <time.h>
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define SUB 0x1A

struct xmodem_packet {
  unsigned char type;
  unsigned char number;
  unsigned char not_number;
  unsigned char data[128];
  unsigned char checksum;
};

FILE *fser, *fout;

int xmgetb(void)
{
  int c;
  clock_t start = clock();
  do {
    c = fgetc(fser);
  } while (
    (c == EOF) &&
    ((clock() - start) < (10 * CLOCKS_PER_SEC))
  );
  return c;
}

void xmputb(unsigned char c) { fputc(c, fser); }

int xmodem_receive_packet(struct xmodem_packet *packet)
{
  int i, j;
  unsigned char checksum = 0;

  // Wait for the start of the packet
  while (1) {
    j = xmgetb();
    if (j < 0) { return j; }
    packet->type = j;
    if (j == SOH) { break;    }
    if (j == EOT) { return 0; }
    if (j == CAN) { return 0; }
  }

  // Read the packet number
  j = xmgetb();
  if (j < 0) { return j; }
  packet->number = j;

  // Read the complement of the packet number
  j = xmgetb();
  if (j < 0) { return j; }
  packet->not_number = j;

  // Read the packet data
  for (i = 0; i < 128; i++) {
    j = xmgetb();
    if (j < 0) { return j; }
    packet->data[i] = j;
    checksum += j;
  }

  // Read the packet checksum
  j = xmgetb();
  if (j < 0) { return j; }
  packet->checksum = j;

  // Check for errors
  if (packet->number != ~packet->not_number) {
    return -2;
  }
  if (packet->checksum != checksum) {
    return -3;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  struct xmodem_packet packet;
  int result, curpnum = 1;
  if (argc != 3) {
    printf("Usage: %s <port> <filename>\n", argv[0]);
    return 1;
  }
  fser = fopen(argv[1], "ab+");
  if (!fser) {
    printf("Error opening port %s\n", argv[1]);
    return 2;
  }
  fout = fopen(argv[2], "wb");
  if (!fout) {
    printf("Error opening file %s\n", argv[2]);
    return 2;
  }

  while (1) {
    result = xmodem_receive_packet(&packet);
    if (result < 0) {
      while (xmgetb() >= 0);
      fputc('!', stderr);
      xmputb(NAK);

    } else if (packet.type == SOH && packet.number == 0) {
      // fputc('\n', stderr);
      // fwrite(packet.data, 1, 128, stderr);
      // fputc('\n', stderr);
      fputc('/', stderr);
      xmputb(ACK);
    } else if (packet.type == SOH && packet.number == curpnum - 1) {
      fputc('-', stderr);
      xmputb(ACK);
    } else if (packet.type == SOH && packet.number == curpnum) {
      fwrite(packet.data, 1, 128, fout);
      fputc('.', stderr);
      curpnum++;
      xmputb(ACK);
    } else if (packet.type == SOH) {
      while (xmgetb() >= 0);
      fputc('@', stderr);
      xmputb(CAN);
      xmputb(CAN);
      xmputb(CAN);
      break;

    } else if (packet.type == EOT) {
      while (xmgetb() >= 0);
      fputc('*', stderr);
      xmputb(ACK);
      break;
    } else if (packet.type == CAN) {
      while (xmgetb() >= 0);
      fputc('X', stderr);
      xmputb(CAN);
      xmputb(CAN);
      xmputb(CAN);
      break;
    } else {
      while (xmgetb() >= 0);
      fputc('?', stderr);
      xmputb(NAK);
    }
  }

  fclose(fser);
  fclose(fout);
  return 0;
}
