/**
 * YM audio player for Nimbus
 * wcl -1 -mh ymplay.c
 */

#include <conio.h>
#include <dos.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../nimbuscd/ioports.h"

unsigned long framecount; // Number of frames in the song
unsigned long songattr;   // Song attributes
unsigned char framesize;  // Bytes per frame
unsigned long framenum;   // Current frame number
unsigned char *filedata;  // Pointer to buffer holding entire file
unsigned char *framedata; // Pointer to audio data within file

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
  // The DOS call allows Ctrl-C to take effect during playback
  outpw(IO_186_T2CTL, inpw(IO_186_T2CTL) & ~0x0020);
  while (!(inpw(IO_186_T2CTL) & 0x0020)) { bdos(0x0B, 0, 0); }
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
    if (i == 13 && *ptr == 0xFF) {
      // Skip this register
    } else {
      write_ay(i, *ptr);
    }

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

void ctrlc_handler(int sig)
{
  write_ay(7, 0x3F); // Silence
  exit(1);
}

unsigned long common_56(void)
{
  // Skip digidrums
  unsigned long offset = 34;
  int drumcount = getuint16be(filedata + 20);
  while (drumcount--) {
    offset += getuint32be(filedata + offset) + 4;
  }

  // Skip additional data
  offset += getuint16be(filedata + 32);

  printf("YM input clock:  %#01.2fMHz (Nimbus is 2MHz)\n",
    (float) getuint32be(filedata + 22) / 1000000);
  printf("Frame rate:      %uHz (Nimbus is 50Hz)\n",
    getuint16be(filedata + 26));

  return offset;
}

unsigned long common_456(unsigned long offset)
{
  framecount = getuint32be(filedata + 12);
  songattr   = getuint32be(filedata + 16);
  framesize  = 16;

  printf("Title:           %s\n",
    filedata[offset] ? (filedata + offset) : "(none)");
  offset += strlen(filedata + offset) + 1;
  printf("Author:          %s\n",
    filedata[offset] ? (filedata + offset) : "(none)");
  offset += strlen(filedata + offset) + 1;
  printf("Comment:         %s\n",
    filedata[offset] ? (filedata + offset) : "(none)");
  offset += strlen(filedata + offset) + 1;
  return offset;
}

int main(int argc, char *argv[])
{
  unsigned long offset, loadcount, loadresult, digidrums, frameloop;
  unsigned int loopcount;
  unsigned char *loadptr;
  long filesize;
  FILE *fp;

  printf("YM audio player for Nimbus\n");
  signal(SIGINT, ctrlc_handler);
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

  // Read the entire file into memory
  if (!fp) {
    fprintf(stderr, "Could not open file\n");
    return 1;
  }
  if (fseek(fp, 0, SEEK_END)) {
    fprintf(stderr, "Could not seek to end of file\n");
    fclose(fp);
    return 1;
  }
  filesize = ftell(fp);
  if (filesize < 0) {
    fprintf(stderr, "Could not determine file size\n");
    fclose(fp);
    return 1;
  }
  filedata = malloc(filesize);
  if (!filedata) {
    fprintf(stderr, "Out of memory reading file\n");
    fclose(fp);
    return 1;
  }
  rewind(fp);

  printf("Loading");
  fflush(stdout);
  loadptr = filedata;
  loadcount = filesize;
  while (loadcount) {
    loadresult = fread(loadptr, 1, min(8192, loadcount), fp);
    if (!loadresult) {
      fprintf(stderr, "\nCould not read audio data\n");
      fclose(fp);
      free(filedata);
      return 1;
    }
    loadptr += loadresult;
    loadcount -= loadresult;
    putchar('.');
    fflush(stdout);
  }
  fclose(fp);
  putchar('\n');

  // Check the file format is one that we implement
  // Extract certain details from the header, note numbers are big-endian
  if (!strncmp(filedata, "YM2!", 4)) {
    offset = 4;
    digidrums = 0;
    framesize = 14;
    songattr = 0x00000001;
    framecount = (filesize - 4) / 14;
    frameloop = 0;
  } else if (!strncmp(filedata, "YM3!", 4)) {
    offset = 4;
    digidrums = 0;
    framesize = 14;
    songattr = 0x00000001;
    framecount = (filesize - 4) / 14;
    frameloop = 0;
  } else if (!strncmp(filedata, "YM3b", 4)) {
    offset = 4;
    digidrums = 0;
    framesize = 14;
    songattr = 0x00000001;
    framecount = (filesize - 8) / 14;
    frameloop = getuint32be(filedata + filesize - 4);

  } else if (!strncmp(filedata, "YM4!LeOnArD!", 12)) {
    digidrums = getuint32be(filedata + 20);
    frameloop = getuint32be(filedata + 24);
    offset = 28;
    for (loadcount = 0; loadcount < digidrums; loadcount++) {
      offset += getuint32be(filedata + offset) + 4;
    }
    offset = common_456(offset);
  } else if (!strncmp(filedata, "YM5!LeOnArD!", 12)) {
    digidrums = getuint16be(filedata + 20);
    frameloop = getuint32be(filedata + 28);
    offset = common_56();
    offset = common_456(offset);
  } else if (!strncmp(filedata, "YM6!LeOnArD!", 12)) {
    digidrums = getuint16be(filedata + 20);
    frameloop = getuint32be(filedata + 28);
    offset = common_56();
    offset = common_456(offset);

  } else if (!strncmp(filedata + 2, "-lh", 3)) {
    fprintf(stderr, "This file is compressed, please decompress with LHA\n");
    free(filedata);
    return 1;
  } else {
    fprintf(stderr, "Invalid or unimplemented file format\n");
    free(filedata);
    return 1;
  }

  printf("File format:     %c%c%c%c\n",
    filedata[0], filedata[1], filedata[2], filedata[3]);
  printf("Song attributes: 0x%08lX\n", songattr);
  printf("Digidrums:       %lu (not implemented)\n", digidrums);
  printf("Loop from frame: %lu\n",     frameloop);
  printf("Data offset:     0x%lX\n",   offset);
  printf("Frame count:     %lu (%lum%lus at 50Hz)\n",
    framecount, framecount / 3000, (framecount % 3000) / 50);

  printf("Playing");
  fflush(stdout);
  framenum = 0;
  framedata = filedata + offset;
  while (loopcount--) {
    play_song();
    framenum = frameloop;
    putchar('.');
    fflush(stdout);
  }
  putchar('\n');

  write_ay(7, 0x3F); // Silence
  free(filedata);
  return 0;
}
