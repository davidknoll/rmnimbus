/**
 * Test IRQ generation from I/O Device Emulator for RM Nimbus PC-186
 * Configure the Pico with IRQ testing firmware in slot 3,
 * compile this on the Nimbus with QuickC 2.0, and enter:
 *  IRQTEST X
 * where X is the IRQ number to assert on the I/O bus.
 * You should get [IRQ X] output to the screen.
 */

#include <conio.h>
#include <dos.h>
#include <stdio.h>

const unsigned int iobase = 0x400 + (3 * 0x80); // Slot 3

void interrupt cdecl far irq0(
  unsigned _es, unsigned _ds, unsigned _di, unsigned _si, unsigned _bp,
  unsigned _sp, unsigned _bx, unsigned _dx, unsigned _cx, unsigned _ax,
  unsigned _ip, unsigned _cs, unsigned _flags
)
{
  outp(iobase + 0x00, 0); // Deassert IRQ0 (normally also used by DCB)
  outpw(0xFF22, 0x000C);  // Specific EOI for INT0 (GA3 cascade)
  _enable();
  puts("[IRQ 0]");
}

void interrupt cdecl far irq1(
  unsigned _es, unsigned _ds, unsigned _di, unsigned _si, unsigned _bp,
  unsigned _sp, unsigned _bx, unsigned _dx, unsigned _cx, unsigned _ax,
  unsigned _ip, unsigned _cs, unsigned _flags
)
{
  outp(iobase + 0x02, 0); // Deassert IRQ1
  outpw(0xFF22, 0x000C);  // Specific EOI for INT0 (GA3 cascade)
  _enable();
  puts("[IRQ 1]");
}

void interrupt cdecl far irq2(
  unsigned _es, unsigned _ds, unsigned _di, unsigned _si, unsigned _bp,
  unsigned _sp, unsigned _bx, unsigned _dx, unsigned _cx, unsigned _ax,
  unsigned _ip, unsigned _cs, unsigned _flags
)
{
  outp(iobase + 0x04, 0); // Deassert IRQ2 (possibly also used by LPT)
  outpw(0xFF22, 0x000F);  // Specific EOI for INT3 (bus IRQ2)
  _enable();
  puts("[IRQ 2]");
}

int main(int argc, char *argv[])
{
  const void far * far * ivt = 0; // Or rather, 0000:0000h
  unsigned char irq;
  if (argc != 2) { return 1; }
  irq = argv[1][0] - '0'; // Argument is the IRQ line number

  switch (irq) {
    case 0:
      ivt[0x80] = irq0;
      outp(0x92, inp(0x92) | 0x1); // Unmask IRQ0 in GA3
      break;
    case 1:
      ivt[0x82] = irq1;
      outp(0x92, inp(0x92) | 0x2); // Unmask IRQ1 in GA3
      break;
    case 2:
      ivt[0x0F] = irq2;
      outpw(0xFF3E, inpw(0xFF3E) & ~0x8); // Unmask IRQ2 in 80186
      break;
    default:
      return 1;
  }

  puts("Press enter to assert...");
  getchar();
  outp(iobase + (2 * irq), 1); // Tell the Pico to assert that IRQ
  puts("Press enter to exit...");
  getchar();

  return 0;
}
