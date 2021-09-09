/**
 * PS/2 mouse to RM Nimbus PC-186 adaptor
 * Low-level PS/2 I/O functions
 * Informed by https://www.burtonsys.com/ps2_chapweske.htm
 * @author David Knoll <david@davidknoll.me.uk>
 */

static void clklo(void) // Driven low
{
  digitalWrite(PS2CLK, LOW);
  pinMode(PS2CLK, OUTPUT);
}

static void clkhi(void) // Released with pullup
{
  pinMode(PS2CLK, INPUT);
  digitalWrite(PS2CLK, HIGH);
}

static void datlo(void) // Driven low
{
  digitalWrite(PS2DAT, LOW);
  pinMode(PS2DAT, OUTPUT);
}

static void dathi(void) // Released with pullup
{
  pinMode(PS2DAT, INPUT);
  digitalWrite(PS2DAT, HIGH);
}

static void waitforclklo(void) { while (digitalRead(PS2CLK) != LOW); }
static void waitforclkhi(void) { while (digitalRead(PS2CLK) != HIGH); }
static void waitfordatlo(void) { while (digitalRead(PS2DAT) != LOW); }
static void waitfordathi(void) { while (digitalRead(PS2DAT) != HIGH); }

void ps2send(uint8_t data)
{
  int i, p = 0;

  // Start
  clklo();        // 1)
  delayMicroseconds(120); // >= 100
  datlo();        // 2)
  clkhi();        // 3)
  waitforclklo(); // 4)

  // Data
  for (i = 0; i < 8; i++) { // 8)
    if (data & (1 << i)) { dathi(); p++; } else { datlo(); } // 5)
    waitforclkhi();         // 6)
    waitforclklo();         // 7)
  }

  // Parity
  if (p % 2) { datlo(); } else { dathi(); } // 5)
  waitforclkhi(); // 6)
  waitforclklo(); // 7)

  // Stop / Ack
  dathi();        // 9)
  waitfordatlo(); // 10)
  waitforclklo(); // 11)
  waitforclkhi(); // 12)
  waitfordathi();
}

uint8_t ps2receive(void)
{
  uint8_t data;
  int i;

  // Start
  waitforclklo();
  waitforclkhi();

  // Data
  for (i = 0; i < 8; i++) {
    waitforclklo();
    data >>= 1;
    if (digitalRead(PS2DAT) == HIGH) { data |= 0x80; }
    waitforclkhi();
  }

  // Parity (ignored)
  waitforclklo();
  waitforclkhi();

  // Stop
  waitforclklo();
  waitforclkhi();

  return data;
}
