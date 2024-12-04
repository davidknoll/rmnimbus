static volatile uint8_t dcc_wr0a  = 0x00;
static volatile uint8_t dcc_wr0b  = 0x00;
static volatile uint8_t dcc_wr1a  = 0x00;
static volatile uint8_t dcc_wr1b  = 0x00;
static volatile uint8_t dcc_wr2   = 0x00;
static volatile uint8_t dcc_wr3a  = 0x00;
static volatile uint8_t dcc_wr5a  = 0x00;
static volatile uint8_t dcc_wr9   = 0x00;
static volatile uint8_t dcc_wr12a = 0x00;
static volatile uint8_t dcc_wr13a = 0x00;
static volatile uint8_t dcc_wr14a = 0x00;
static volatile uint8_t dcc_wr15a = 0xF8;
static volatile uint8_t dcc_rr3   = 0x00;
static volatile int16_t dcc_rr8a  = -1;
static volatile uint8_t dcc_auxwr = 0x00;
static volatile uint8_t dcc_auxrd = 0x00;
static volatile bool    dcc_auxip = false;

WiFiServer dcc_server(2345);
WiFiClient dcc_client;
CircularBuffer<uint8_t,4096> dcc_txbuf;
CircularBuffer<uint8_t,4096> dcc_rxbuf;

/*
 * Aux read register bits:
 * 7-4 Undefined
 * 3   Channel B /RI
 * 2   Channel B /DSR
 * 1   Channel A /RI
 * 0   Channel A /DSR
 * DSR change interrupt (one flag for both channels) is cleared on read
 *
 * Aux write register bits:
 * 7   Enable interrupt on channel B DSR change
 * 6   Enable interrupt on channel A DSR change
 * 5   0 = /RxCB is /RxC1B (external), 1 = /RxCB is /ITxCA (internal)
 * 4   0 = /RxCA is /RxC1A (external), 1 = /RxCA is /ITxCB (internal)
 * 3   0 = /TxCB enabled, 1 = /TxCB held high
 * 2   0 = /TxCB is /TxC1B (external), 1 = /TxCB is /ITxCA (internal)
 * 1   0 = /TxCA is /TxC1A (external), 1 = /TxCA is /ITxCB (internal)
 * 0   0 = /TxCA enabled, 1 = /TxCA held high
 *
 * SCC interrupt source priority:
 *  Receive Channel A (Highest)
 *  Transmit Channel A
 *  External/Status Channel A
 *  Receive Channel B
 *  Transmit Channel B
 *  External/Status Channel B (Lowest)
 *  (and then the aux / DSR change interrupt external to the SCC)
 */

/**
 * To be called during sketch setup()
 */
void dcc_setup() {
  dcc_server.begin();
}

/**
 * To be called during sketch loop()
 */
void dcc_loop() {
  if (Serial.available() && dcc_rr8a == -1 && (dcc_wr3a & 0x01)) {
    dcc_rr8a = Serial.read();

    switch (dcc_wr3a & 0xC0) {            // Rx bits/char
      case 0x00: dcc_rr8a |= 0xE0; break; // 5 bits
      case 0x40: dcc_rr8a |= 0x80; break; // 7 bits
      case 0x80: dcc_rr8a |= 0xC0; break; // 6 bits
    }

    if (dcc_wr14a & 0x08) { dcc_tx(dcc_rr8a); } // Auto echo?

    switch (dcc_wr1a & 0x18) { // Rx interrupt mode
      case 0x08:               // Rx int on 1st char or special
        dcc_wr1a &= ~0x18;     // Got that 1st char, now clear the mode
        // Missing break; intentional
      case 0x10:               // Rx int on all char or special
        dcc_setip(0x20);
        break;
    }
    if ((dcc_wr1a & 0xE0) == 0xE0) { digitalWrite(nDMA, LOW); } // Rx DMA?
  }

  if (dcc_client) {
    if (dcc_client.connected()) {
      while (dcc_client.available() && !dcc_rxbuf.isFull()) {
        dcc_rxbuf.push(dcc_client.read());
        switch (dcc_wr1b & 0x18) { // Rx interrupt mode
          case 0x08:               // Rx int on 1st char or special
            dcc_wr1b &= ~0x18;     // Got that 1st char, now clear the mode
            // Missing break; intentional
          case 0x10:               // Rx int on all char or special
            dcc_setip(0x04);
            break;
        }
      }
      while (!dcc_txbuf.isEmpty()) {
        dcc_client.write(dcc_txbuf.shift());
      }
    } else {
      dcc_client.stop();
      dcc_client = dcc_server.accept();
    }
  } else {
    dcc_client = dcc_server.accept();
  }
}

/**
 * Read from an emulated Data Communications Controller card
 */
uint dcc_read(uint address) {
  uint reg = 0, data = 0x00;

  switch (address & 0xE) {
    case 0x0: // Channel B control
      reg = (dcc_wr0b & 0x30) ? (dcc_wr0b & 0x7) : (dcc_wr0b & 0xF);
      dcc_wr0b = 0x00;

      switch (reg) {
        case 0: // RR0
        case 4: // RR4
          if (!dcc_rxbuf.isEmpty()) { data |= 0x01; }
          if (!dcc_txbuf.isFull()) { data |= 0x04; }
          break;

        case 2: // RR2
        case 6: // RR6
          data = dcc_wr2;
          data &= (dcc_wr9 & 0x10) ? ~0x70 : ~0x0E;
          if (dcc_rr3 & 0x20) {        // Ch A Rx
            data |= (dcc_wr9 & 0x10) ? 0x30 : 0x0C;
          } else if (dcc_rr3 & 0x10) { // Ch A Tx
            data |= (dcc_wr9 & 0x10) ? 0x10 : 0x08;
          } else if (dcc_rr3 & 0x08) { // Ch A ES
            data |= (dcc_wr9 & 0x10) ? 0x50 : 0x0A;
          } else if (dcc_rr3 & 0x04) { // Ch B Rx
            data |= (dcc_wr9 & 0x10) ? 0x20 : 0x04;
          } else if (dcc_rr3 & 0x02) { // Ch B Tx
            data |= (dcc_wr9 & 0x10) ? 0x00 : 0x00;
          } else if (dcc_rr3 & 0x01) { // Ch B ES
            data |= (dcc_wr9 & 0x10) ? 0x40 : 0x02;
          }
          break;
      }
      break;

    case 0x2: // Channel A control
      reg = (dcc_wr0a & 0x30) ? (dcc_wr0a & 0x7) : (dcc_wr0a & 0xF);
      dcc_wr0a = 0x00;

      switch (reg) {
        case 0: // RR0
        case 4: // RR4
          data = 0x2C;                          // CTS, DCD, Tx empty always set
          if (dcc_rr8a != -1) { data |= 0x01; } // Rx available?
          break;

        case 1: // RR1
        case 5: // RR5
          data = 0x07;
          break;

        case 2: // RR2
        case 6: // RR6
          data = dcc_wr2;
          break;

        case 3: // RR3
        case 7: // RR7
          data = dcc_rr3;
          break;

        case 8: // RR8
          data = dcc_rx();
          break;

        case 9: // RR9
        case 13: // RR13
          data = dcc_wr13a;
          break;

        case 10: // RR10
        case 14: // RR14
          // Leave it at 0x00
          break;

        case 11: // RR11
        case 15: // RR15
          data = dcc_wr15a;
          break;

        case 12: // RR12
          data = dcc_wr12a;
          break;
      }
      break;

    case 0x4: // Channel B data
      data = dcc_rxbuf.shift();
      dcc_clrip(0x04);
      break;

    case 0x6: // Channel A data
      data = dcc_rx();
      break;

    default:  // Aux register
      data = dcc_auxrd;
      dcc_auxip = false;
      dcc_clrip(0x00);
  }

#if DCC_DEBUG
  Serial.print("DCC READ  ");
  Serial.print(address, HEX);
  Serial.print(" (");
  if (address < 0x8) {
    Serial.print((address & 0x4) ? "D" : "C");
    Serial.print((address & 0x2) ? "A" : "B");
    if (address < 0x4) {
      Serial.print(" RR");
      Serial.print(reg);
    }
  } else {
    Serial.print("AUX");
  }
  Serial.print(") = ");
  Serial.println(data, HEX);
#endif

  return data & 0xFF;
}

/**
 * Write to an emulated Data Communications Controller card
 */
void dcc_write(uint address, uint data) {
  uint reg = 0;

  switch (address & 0xE) {
    case 0x0: // Channel B control
      reg = (dcc_wr0b & 0x30) ? (dcc_wr0b & 0x7) : (dcc_wr0b & 0xF);
      dcc_wr0b = 0x00;

      switch (reg) {
        case 0: // WR0
          dcc_wr0b = data;
          switch (dcc_wr0b & 0x38) {
            case 0x10: // Reset ES int
              dcc_clrip(0x01);
              break;
            case 0x20: // Enable next Rx int
              dcc_wr1b &= ~0x18;
              dcc_wr1b |= 0x08;
              break;
            case 0x28: // Reset Tx int
              dcc_clrip(0x02);
              break;
          }
          break;

        case 1: // WR1
          dcc_wr1b = data;
          break;

        case 2: // WR2
          dcc_wr2 = data;
          break;
      }
      break;

    case 0x2: // Channel A control
      reg = (dcc_wr0a & 0x30) ? (dcc_wr0a & 0x7) : (dcc_wr0a & 0xF);
      dcc_wr0a = 0x00;

      switch (reg) {
        case 0: // WR0
          dcc_wr0a = data;
          switch (dcc_wr0a & 0x38) {
            case 0x10: // Reset ES int
              dcc_clrip(0x08);
              break;
            case 0x20: // Enable next Rx int
              dcc_wr1a &= ~0x18;
              dcc_wr1a |= 0x08;
              break;
            case 0x28: // Reset Tx int
              dcc_clrip(0x10);
              break;
          }
          break;

        case 1: // WR1
          dcc_wr1a = data;
          break;

        case 2: // WR2
          dcc_wr2 = data;
          break;

        case 3: // WR3
          dcc_wr3a = data;
          break;

        case 5: // WR5
          dcc_wr5a = data;
          break;

        case 8: // WR8
          dcc_tx(data);
          break;

        case 9: // WR9
          dcc_wr9 = data;
          if (data & 0x80) {           // Channel reset A (or force hardware reset)
            dcc_wr0a = 0x00;
            dcc_wr1a = 0x00;
            dcc_wr3a = 0x00;
            dcc_wr5a = 0x00;
            dcc_wr12a = 0x00;
            dcc_wr13a = 0x00;
            dcc_wr14a = 0x00;
            dcc_wr15a = 0xF8;
            dcc_rr8a = -1;
            dcc_clrip(0xF8);
            digitalWrite(nDMA, HIGH);
          }
          if (data & 0x40) {           // Channel reset B (or force hardware reset)
            dcc_wr0b = 0x00;
            dcc_clrip(0xC7);
          }
          if ((data & 0xC0) == 0xC0) { // Force hardware reset
            dcc_wr2 = 0x00;
          }
          break;

        case 12: // WR12
          dcc_wr12a = data;
          break;

        case 13: // WR13
          dcc_wr13a = data;
          break;

        case 14: // WR14
          dcc_wr14a = data;
          break;

        case 15: // WR15
          dcc_wr15a = data;
          break;
      }
      break;

    case 0x4: // Channel B data
      dcc_txbuf.push(data);
      break;

    case 0x6: // Channel A data
      dcc_tx(data);
      break;

    default:  // Aux register
      dcc_auxwr = data;
  }

#if DCC_DEBUG
  Serial.print("DCC WRITE ");
  Serial.print(address, HEX);
  Serial.print(" (");
  if (address < 0x8) {
    Serial.print((address & 0x4) ? "D" : "C");
    Serial.print((address & 0x2) ? "A" : "B");
    if (address < 0x4) {
      Serial.print(" WR");
      Serial.print(reg);
    }
  } else {
    Serial.print("AUX");
  }
  Serial.print(") = ");
  Serial.println(data, HEX);
#endif
}

/**
 * Send a byte, accounting for the current settings
 */
static void dcc_tx(uint8_t data) {
  dcc_clrip(0x10);
  if ((dcc_wr1a & 0xE0) == 0xC0) { digitalWrite(nDMA, HIGH); }

  switch (dcc_wr5a & 0x60) {       // Tx bits/char
    case 0x00:                     // 5 or less bits
      if (!(data & 0x80)) {        // 5 bits
        data &= 0x1F;
      } else if (!(data & 0x40)) { // 4 bits
        data &= 0x0F;
      } else if (!(data & 0x20)) { // 3 bits
        data &= 0x07;
      } else if (!(data & 0x10)) { // 2 bits
        data &= 0x03;
      } else {                     // 1 bit
        data &= 0x01;
      }
      break;

    case 0x20:                     // 7 bits
      data &= 0x7F;
      break;

    case 0x40:                     // 6 bits
      data &= 0x3F;
      break;
  }

  if (dcc_wr5a & 0x08) { Serial.write(data); } // Tx enabled?
  if (dcc_wr14a & 0x10) { dcc_rr8a = data; }   // Local loopback?
  if (dcc_wr1a & 0x02) { dcc_setip(0x10); }    // Tx interrupt?
  if ((dcc_wr1a & 0xE0) == 0xC0) { digitalWrite(nDMA, LOW); } // Tx DMA?
}

/**
 * Read a byte, if there is one
 */
static uint8_t dcc_rx() {
  uint8_t data = 0x00;
  if (dcc_rr8a != -1) {
    data = dcc_rr8a;
    dcc_rr8a = -1;
    dcc_clrip(0x20);
    if ((dcc_wr1a & 0xE0) == 0xE0) { digitalWrite(nDMA, HIGH); }
  }
  return data;
}

/**
 * Set interrupt pending flag(s) and if necessary the /IRQ line
 */
static void dcc_setip(uint8_t mask) {
  dcc_rr3 |= mask;

  if (((dcc_wr9 & 0x08) && dcc_rr3) || (!(dcc_wr9 & 0x04) && dcc_auxip)) {
    dcc_irq_flag = true;
#if DCC_DEBUG
    Serial.print("DCC IRQ SET ");
    Serial.println(mask, HEX);
#endif
  }
}

/**
 * Clear interrupt pending flag(s) and if necessary the /IRQ line
 */
static void dcc_clrip(uint8_t mask) {
  dcc_rr3 &= ~mask;

  if (!((dcc_wr9 & 0x08) && dcc_rr3) && !(!(dcc_wr9 & 0x04) && dcc_auxip)) {
    dcc_irq_flag = false;
#if DCC_DEBUG
    Serial.print("DCC IRQ CLR ");
    Serial.println(mask, HEX);
#endif
  }
}
