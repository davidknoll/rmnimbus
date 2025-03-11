/**
 * YM audio player for Nimbus
 * wcl -1 -mh ymplay.c
 */

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../nimbuscd/ioports.h"

unsigned long framecount; // Number of frames in the song
unsigned long songattr;   // Song attributes
unsigned char framesize;  // Bytes per frame
unsigned long framenum;   // Current frame number
unsigned char *framedata; // Pointer to buffer holding audio data

unsigned int getuint16be(const unsigned char *ptr)
{
  return ((unsigned int) ptr[0] << 8) | ptr[1];
}

unsigned long getuint32be(const unsigned char *ptr)
{
  return ((unsigned long) ptr[0] << 24) |
         ((unsigned long) ptr[1] << 16) |
         ((unsigned long) ptr[2] <<  8) |
                          ptr[3];
}

void wait_for_vblank(void)
{
  // Poll the timer already used for the 50Hz interrupt
  outpw(IO_186_T2CTL, inpw(IO_186_T2CTL) & ~0x0020);
  while (!(inpw(IO_186_T2CTL) & 0x0020));
}

void write_ay(unsigned char reg, unsigned char data)
{
  outp(IO_GA3_AYADDR, reg);
  outp(IO_GA3_AYDATW, data);
}

void write_frame(void)
{
  int i;
  unsigned char *ptr;

  // Point to data for first register in current frame
  if (songattr & 1) {
    // Interleaved
    ptr = framedata + framenum;
  } else {
    // Non-interleaved
    ptr = framedata + (framenum * framesize);
  }

  for (i = 0; i < 14; i++) {
    write_ay(i, *ptr);

    // Advance to next register in current frame
    if (songattr & 1) {
      ptr += framecount;
    } else {
      ptr++;
    }
  }
}

void play_song(void)
{
  while (framenum < framecount) {
    wait_for_vblank();
    write_frame();
    framenum++;
  }
}

int main(int argc, char *argv[])
{
  unsigned long frameloop, offset, loadcount, loadresult;
  unsigned int loopcount;
  unsigned char *loadptr;
  FILE *fp;

  printf("YM audio player for Nimbus\n");
  if (argc == 2) {
    loopcount = 1;
    fp = fopen(argv[1], "rb");
  } else if (argc == 4 && !strcmp(argv[1], "-l")) {
    loopcount = atoi(argv[2]);
    fp = fopen(argv[3], "rb");
  } else {
    fprintf(stderr, "Usage: %s [-l loops] <filename>\n", argv[0]);
    return 1;
  }

  // Read YM file header
  if (!fp) {
    fprintf(stderr, "Could not open file\n");
    return 1;
  }
  framedata = malloc(256);
  if (!framedata) {
    fprintf(stderr, "Out of memory reading file header\n");
    fclose(fp);
    return 1;
  }
  if (fread(framedata, 256, 1, fp) != 1) {
    fprintf(stderr, "Could not read file header\n");
    fclose(fp);
    free(framedata);
    return 1;
  }

  // Check the file format is one that we implement
  if (
    strncmp(framedata, "YM5!LeOnArD!", 12) || // Uncompressed YM5
    getuint16be(framedata + 20)            || // No digidrums
    getuint16be(framedata + 32)               // No additional data
  ) {
    fprintf(stderr, "Invalid or unimplemented file format\n");
    fclose(fp);
    free(framedata);
    return 1;
  }

  // Extract certain details from the header, note numbers are big-endian
  framecount = getuint32be(framedata + 12);
  songattr   = getuint32be(framedata + 16);
  frameloop  = getuint32be(framedata + 28);
  framesize  = 16;

  offset = 34;
  printf("Title:   %s\n", framedata[0] ? (framedata + offset) : "(none)");
  offset += strlen(framedata + offset) + 1;
  printf("Author:  %s\n", framedata[0] ? (framedata + offset) : "(none)");
  offset += strlen(framedata + offset) + 1;
  printf("Comment: %s\n\n", framedata[0] ? (framedata + offset) : "(none)");
  offset += strlen(framedata + offset) + 1;

  printf("Frame count:     %lu (%um%us at 50Hz)\n",
    framecount, framecount / 3000, (framecount % 3000) / 50);
  printf("Song attributes: 0x%08X\n", songattr);
  printf("Digidrums:       %u (not implemented)\n",
    getuint16be(framedata + 20));
  printf("YM input clock:  %#01.2fMHz (Nimbus is 2MHz)\n",
    (float) getuint32be(framedata + 22) / 1000000);
  printf("Frame rate:      %uHz (Nimbus is 50Hz)\n",
    getuint16be(framedata + 26));
  printf("Loop from frame: %lu\n",    frameloop);
  printf("Data offset:     0x%X\n",   offset);

  // Read entire audio data
  framedata = realloc(framedata, framecount * framesize);
  if (!framedata) {
    fprintf(stderr, "Out of memory reading audio data\n");
    fclose(fp);
    return 1;
  }
  if (fseek(fp, offset, SEEK_SET)) {
    fprintf(stderr, "Could not seek to audio data\n");
    fclose(fp);
    free(framedata);
    return 1;
  }

  printf("Loading");
  fflush(stdout);
  loadptr = framedata;
  loadcount = framecount;
  while (loadcount) {
    loadresult = fread(loadptr, framesize, min(loadcount, 4000), fp);
    if (!loadresult) {
      fprintf(stderr, "\nCould not read audio data\n");
      fclose(fp);
      free(framedata);
      return 1;
    }
    loadptr += framesize * loadresult;
    loadcount -= loadresult;
    putchar('.');
    fflush(stdout);
  }

  printf("\nPlaying");
  fflush(stdout);
  framenum = 0;
  while (loopcount--) {
    play_song();
    framenum = frameloop;
    putchar('.');
    fflush(stdout);
  }
  putchar('\n');

  write_ay(7, 0x3F); // Silence
  fclose(fp);
  free(framedata);
  return 0;
}
