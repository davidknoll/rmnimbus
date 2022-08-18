/**
 * PS/2 mouse to RM Nimbus PC-186 adaptor
 * Inspired by https://www.thenimbus.co.uk/upgrades-and-maintenance/ps2mouse
 * but recreated to use a smaller AVR and release the code openly.
 * Written for an ATtiny2313, suggested fuses: E:FF H:9F L:E4
 * @author David Knoll <david@davidknoll.me.uk>
 */
#include <avr/wdt.h>

// Pin definitions
#define PS2BCLK 2
#define PS2BDAT 3
#define PS2CLK  4
#define PS2DAT  5
#define LED     6
#define NIMRT   7
#define NIMSL   8
#define NIMLB   9
#define NIMRB  10
#define NIMLT  11
#define NIMUP  12
#define NIMDN  13

// Global variables
volatile int qx = 0, qy = 0;

void setup(void)
{
  wdt_disable();

  // PS/2 mouse
  pinMode(PS2CLK, INPUT);
  pinMode(PS2DAT, INPUT);
  digitalWrite(PS2CLK, HIGH);
  digitalWrite(PS2DAT, HIGH);

  // Nimbus mouse (or joystick), these have pullups
  pinMode(NIMRT, INPUT);
  pinMode(NIMLB, INPUT);
  pinMode(NIMRB, INPUT);
  pinMode(NIMLT, INPUT);
  pinMode(NIMUP, INPUT);
  pinMode(NIMDN, INPUT);
  digitalWrite(NIMRT, LOW);
  digitalWrite(NIMLB, LOW);
  digitalWrite(NIMRB, LOW);
  digitalWrite(NIMLT, LOW);
  digitalWrite(NIMUP, LOW);
  digitalWrite(NIMDN, LOW);

  // Not normally used, connected anyway
  pinMode(PS2BCLK, INPUT);
  pinMode(PS2BDAT, INPUT);
  digitalWrite(PS2BCLK, HIGH);
  digitalWrite(PS2BDAT, HIGH);
  pinMode(NIMSL, INPUT);
  digitalWrite(NIMSL, LOW);

  t1setup();   // Timer 1 used for quadrature generation
  delay(3000); // Allow mouse to init
  wdt_enable(WDTO_500MS);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

void loop(void)
{
  uint8_t status, px, py;
  wdt_reset();
  delay(10);     // Approximate sample rate 100Hz
  ps2send(0xEB); // Read data
  ps2receive();  // Should be 0xFA (acknowledge)

  // Receive movement packet
  status = ps2receive();
  px = ps2receive();
  py = ps2receive();
  digitalWrite(LED, (status & (1 << 2)) ? HIGH : LOW);

  // Act on that packet
  pinMode(NIMLB, (status & (1 << 0)) ? OUTPUT : INPUT);
  pinMode(NIMRB, (status & (1 << 1)) ? OUTPUT : INPUT);
  qx = (status & (1 << 4)) ? ((-1 << 8) | px) : px;
  qy = (status & (1 << 5)) ? ((-1 << 8) | py) : py;
}
