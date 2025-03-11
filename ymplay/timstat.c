/**
 * This is just for testing and experimentation, not for end users
 */

#include <conio.h>
#include <stdio.h>
#include <time.h>
#include "../nimbuscd/ioports.h"

int main(int argc, char *argv[])
{
  unsigned long timstat_counts[16];
  unsigned long i, j, k;
  for (i = 0; i < 16; i++) { timstat_counts[i] = 0; }
  printf("Display timing status port test\n");

  for (i = 0; i < 1000000UL; i++) {
    j = inp(IO_GA2_TIMSTAT) & 0xF;
    timstat_counts[j]++;
  }

  for (i = 0; i < 16; i++) {
    printf("%lX: %lu\n", i, timstat_counts[i]);
  }
  printf("Bit 0 set: %lu, clear: %lu\n",
    timstat_counts[ 1] + timstat_counts[ 3] + timstat_counts[ 5] + timstat_counts[ 7] +
    timstat_counts[ 9] + timstat_counts[11] + timstat_counts[13] + timstat_counts[15],
    timstat_counts[ 0] + timstat_counts[ 2] + timstat_counts[ 4] + timstat_counts[ 6] +
    timstat_counts[ 8] + timstat_counts[10] + timstat_counts[12] + timstat_counts[14]
  );
  printf("Bit 1 set: %lu, clear: %lu\n",
    timstat_counts[ 2] + timstat_counts[ 3] + timstat_counts[ 6] + timstat_counts[ 7] +
    timstat_counts[10] + timstat_counts[11] + timstat_counts[14] + timstat_counts[15],
    timstat_counts[ 0] + timstat_counts[ 1] + timstat_counts[ 4] + timstat_counts[ 5] +
    timstat_counts[ 8] + timstat_counts[ 9] + timstat_counts[12] + timstat_counts[13]
  );
  printf("Bit 2 set: %lu, clear: %lu\n",
    timstat_counts[ 4] + timstat_counts[ 5] + timstat_counts[ 6] + timstat_counts[ 7] +
    timstat_counts[12] + timstat_counts[13] + timstat_counts[14] + timstat_counts[15],
    timstat_counts[ 0] + timstat_counts[ 1] + timstat_counts[ 2] + timstat_counts[ 3] +
    timstat_counts[ 8] + timstat_counts[ 9] + timstat_counts[10] + timstat_counts[11]
  );
  printf("Bit 3 set: %lu, clear: %lu\n",
    timstat_counts[ 8] + timstat_counts[ 9] + timstat_counts[10] + timstat_counts[11] +
    timstat_counts[12] + timstat_counts[13] + timstat_counts[14] + timstat_counts[15],
    timstat_counts[ 0] + timstat_counts[ 1] + timstat_counts[ 2] + timstat_counts[ 3] +
    timstat_counts[ 4] + timstat_counts[ 5] + timstat_counts[ 6] + timstat_counts[ 7]
  );

  // printf("time %lu\n", time(NULL));
  // j = inp(IO_GA2_TIMSTAT) & 0x4;
  // for (k = 0; k < 200000UL; k++) {
  //   do {
  //     i = j;
  //     j = inp(IO_GA2_TIMSTAT) & 0x4;
  //   } while (i == j);
  // }
  printf("time %lu\n", time(NULL));

  printf("t2 mca %04X ctl %04X\n", inpw(IO_186_T2MCA), inpw(IO_186_T2CTL));
  for (k = 0; k < 42 * 50; k++) {
    outpw(IO_186_T2CTL, inpw(IO_186_T2CTL) & ~0x0020);
    while (!(inpw(IO_186_T2CTL) & 0x0020));
  }
  printf("time %lu\n", time(NULL));
  return 0;
}
