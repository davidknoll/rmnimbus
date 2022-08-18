/**
 * PS/2 mouse to RM Nimbus PC-186 adaptor
 * Quadrature generation
 * @author David Knoll <david@davidknoll.me.uk>
 */

void t1setup(void)
{
  TCCR1A = 0x00;                   // CTC mode
  TCCR1B = _BV(WGM12) | _BV(CS11); // Use OCR1A, prescaler /8 (1MHz)
  TCCR1C = 0x00;
  OCR1A  = 2000;                   // Interrupt at ~1/2KHz
  TIMSK |= _BV(OCIE1A);
}

/*
ISR(TIMER1_COMPA_vect)
{
  static uint8_t ph = 0;
  switch (ph) {
  case 0:
    if (qx < 0) { pinMode(NIMLT, OUTPUT); } // Left
    if (qx > 0) { pinMode(NIMDN, OUTPUT); } // Right
    if (qy < 0) { pinMode(NIMRT, OUTPUT); } // Up?
    if (qy > 0) { pinMode(NIMUP, OUTPUT); } // Down?
    break;
  case 1:
    if (qx < 0) { pinMode(NIMDN, OUTPUT); }
    if (qx > 0) { pinMode(NIMLT, OUTPUT); }
    if (qy < 0) { pinMode(NIMUP, OUTPUT); }
    if (qy > 0) { pinMode(NIMRT, OUTPUT); }
    break;
  case 2:
    if (qx < 0) { pinMode(NIMLT, INPUT); }
    if (qx > 0) { pinMode(NIMDN, INPUT); }
    if (qy < 0) { pinMode(NIMRT, INPUT); }
    if (qy > 0) { pinMode(NIMUP, INPUT); }
    break;
  case 3:
    if (qx < 0) { pinMode(NIMDN, INPUT); qx++; }
    if (qx > 0) { pinMode(NIMLT, INPUT); qx--; }
    if (qy < 0) { pinMode(NIMUP, INPUT); qy++; }
    if (qy > 0) { pinMode(NIMRT, INPUT); qy--; }
    break;
  }
  ph = (ph + 1) % 4;
}
*/

ISR(TIMER1_COMPA_vect)
{
  // qx = -255 ie &100 : xwl = 1
  // qx = -1 ie &1FF : xwl = 255
  // qx = 0 ie &000 : xwl = anything nonzero
  // qx = 1 ie &001 : xwl = 255
  // qx = 255 ie &0FF : xwl = 1
  // Then vary the interrupt freq to adjust speed globally?
  static uint8_t xwc = 0, xph = 0, ywc = 0, yph = 0;
  int xwl = 256 - abs(qx), ywl = 256 - abs(qy);
  if (!xwl) { xwl = 255; }
  if (!ywl) { ywl = 255; }

  if (!xwc) {
    switch (xph) {
    case 0:
      if (qx < 0) { pinMode(NIMLT, OUTPUT); }
      if (qx > 0) { pinMode(NIMDN, OUTPUT); }
      break;
    case 1:
      if (qx < 0) { pinMode(NIMDN, OUTPUT); }
      if (qx > 0) { pinMode(NIMLT, OUTPUT); }
      break;
    case 2:
      if (qx < 0) { pinMode(NIMLT, INPUT); }
      if (qx > 0) { pinMode(NIMDN, INPUT); }
      break;
    case 3:
      if (qx < 0) { pinMode(NIMDN, INPUT); }
      if (qx > 0) { pinMode(NIMLT, INPUT); }
      break;
    }
    xph = (xph + 1) % 4;
  }
  xwc = (xwc + 1) % xwl;

  if (qy && !ywc) {
    switch (yph) {
    case 0:
      pinMode((qy < 0) ? NIMRT : NIMUP, OUTPUT);
      break;
    case 1:
      pinMode((qy < 0) ? NIMUP : NIMRT, OUTPUT);
      break;
    case 2:
      pinMode((qy < 0) ? NIMRT : NIMUP, INPUT);
      break;
    case 3:
      pinMode((qy < 0) ? NIMUP : NIMRT, INPUT);
      break;
    }
    yph = (yph + 1) % 4;
  }
  ywc = (ywc + 1) % ywl;
}
