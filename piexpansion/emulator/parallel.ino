// 6522 VIA registers
static volatile uint8_t parallel_orb  = 0x00;
static volatile uint8_t parallel_irb  = 0xFF;
static volatile uint8_t parallel_ora  = 0x00;
static volatile uint8_t parallel_ira  = 0xFF;
static volatile uint8_t parallel_ddrb = 0x00;
static volatile uint8_t parallel_ddra = 0x00;
static volatile uint8_t parallel_acr  = 0x00;
static volatile uint8_t parallel_pcr  = 0x00;
static volatile uint8_t parallel_ifr  = 0x00;
static volatile uint8_t parallel_ier  = 0x80;

static WiFiServer parallel_servera(PARALLEL_TCP_PORTA);
static WiFiServer parallel_serverb(PARALLEL_TCP_PORTB);
static WiFiClient parallel_clienta;
static WiFiClient parallel_clientb;

// static uint64_t parallel_last_ch_ts;

/**
 * To be called during sketch setup()
 */
void parallel_setup() {
  parallel_servera.begin();
  parallel_serverb.begin();
  // parallel_last_ch_ts = time_us_64();
}

/**
 * To be called during sketch loop()
 */
void parallel_loop() {
  // Stop any clients which disconnect
  if (parallel_clienta && !parallel_clienta.connected()) {
    parallel_clienta.stop();
  }
  if (parallel_clientb && !parallel_clientb.connected()) {
    parallel_clientb.stop();
  }

  // Check for any new clients connecting
  if (!parallel_clienta) {
    WiFiClient client = parallel_servera.accept();
    if (client) { parallel_clienta = client; }
  }
  if (!parallel_clientb) {
    WiFiClient client = parallel_serverb.accept();
    if (client) { parallel_clientb = client; }
  }

  // Check for input data
  // This handshaking uses the CA2 / CB2 interrupts for received data ready,
  // which isn't what's in the datasheet, but it coexists with emulating
  // the parallel printer port over a bidirectional network port.
  if (parallel_clienta && parallel_clienta.available()) {
    // If "latching" is enabled and there's already a CA2 interrupt pending,
    // don't overwrite the existing byte.
    // Only set the CA2 interrupt if it is in handshake / pulse mode.
    if (!((parallel_acr & 0x01) && (parallel_ifr & 0x01))) {
      parallel_ira = parallel_clienta.read();
      if ((parallel_pcr & 0x0C) == 0x08) { parallel_ifr |= 0x01; }
    }
  }
  if (parallel_clientb && parallel_clientb.available()) {
    // If "latching" is enabled and there's already a CB2 interrupt pending,
    // don't overwrite the existing byte.
    // Only set the CB2 interrupt if it is in handshake / pulse mode.
    if (!((parallel_acr & 0x02) && (parallel_ifr & 0x08))) {
      parallel_irb = parallel_clientb.read();
      if ((parallel_pcr & 0xC0) == 0x80) { parallel_ifr |= 0x08; }
    }
  }

  // TESTING- Send the occasional spurious interrupt in case we got stuck
  // if (time_us_64() - parallel_last_ch_ts > 100000) { // 100ms since last char
  //   parallel_last_ch_ts = time_us_64();
  //   if ((parallel_pcr & 0x0C) == 0x08) { parallel_ifr |= 0x02; }
  // }
  parallel_update_irq();
}

/**
 * Update IFR bit 7 and the IRQ pin, based on the rest of the IFR and IER
 */
static void parallel_update_irq() {
  if (parallel_ifr & parallel_ier & 0x7F) {
#if PARALLEL_DEBUG
    if (!(parallel_ifr & 0x80)) {
      Serial.print("LPT IRQSET ");
      Serial.println(parallel_ifr, HEX);
    }
#endif
    parallel_ifr |= 0x80;
    parallel_irq_flag = true;  // Assert /IRQx
  } else {
#if PARALLEL_DEBUG
    if (parallel_ifr & 0x80) {
      Serial.print("LPT IRQCLR ");
      Serial.println(parallel_ifr, HEX);
    }
#endif
    parallel_ifr &= ~0x80;
    parallel_irq_flag = false; // Deassert /IRQx
  }
}

/**
 * If handshake / pulse mode, issue CA1 interrupt for the ack
 */
static int64_t parallel_irqhsa(alarm_id_t id, void *user_data) {
  if ((parallel_pcr & 0x0C) == 0x08) { parallel_ifr |= 0x02; }
  parallel_update_irq();
  return 0;
}

/**
 * If handshake / pulse mode, issue CB1 interrupt for the ack
 */
static int64_t parallel_irqhsb(alarm_id_t id, void *user_data) {
  if ((parallel_pcr & 0xC0) == 0x80) { parallel_ifr |= 0x10; }
  parallel_update_irq();
  return 0;
}

/**
 * Read from an emulated parallel port card accessible over WiFi
 * CA2 is /STROBE output, CA1 is /ACK input
 */
uint parallel_read(uint address) {
  uint data = 0x00;
  switch ((address >> 1) & 0xF) {
    case 0x0: // IRB
      // data = (parallel_orb & parallel_ddrb) | (parallel_irb & ~parallel_ddrb);
      data = parallel_irb;

      // Clear CB1 interrupt, clear CB2 interrupt if not independent
      parallel_ifr &= ~0x10;
      if ((parallel_pcr & 0xA0) != 0x20) {
        parallel_ifr &= ~0x08;
      }
      break;

    case 0x1: // IRA
    case 0xF: // IRA*
      // data = (parallel_ora & parallel_ddra) | (parallel_ira & ~parallel_ddra);
      data = parallel_ira;

      // Clear CA1 interrupt, clear CA2 interrupt if not independent
      parallel_ifr &= ~0x02;
      if ((parallel_pcr & 0x0A) != 0x02) {
        parallel_ifr &= ~0x01;
      }
      break;

    case 0x2: // DDRB
      data = parallel_ddrb;
      break;
    case 0x3: // DDRA
      data = parallel_ddra;
      break;

    // Timers and shift register not implemented

    case 0xB: // ACR
      data = parallel_acr;
      break;
    case 0xC: // PCR
      data = parallel_pcr;
      break;

    case 0xD: // IFR
      data = parallel_ifr;
      break;
    case 0xE: // IER
      data = parallel_ier;
      break;
  }

#if PARALLEL_DEBUG
  Serial.print("LPT READ  ");
  Serial.print(address >> 1, HEX);
  Serial.print(" ");
  Serial.println(data, HEX);
#endif

  parallel_update_irq();
  return data & 0xFF;
}

/**
 * Write to an emulated parallel port card accessible over WiFi
 * CA2 is /STROBE output, CA1 is /ACK input
 */
void parallel_write(uint address, uint data) {
  switch ((address >> 1) & 0xF) {
    case 0x0: // ORB
      parallel_orb = data;
      if (parallel_clientb && parallel_clientb.connected()) {
        // parallel_clientb.write((parallel_orb & parallel_ddrb) | (parallel_irb & ~parallel_ddrb));
        parallel_clientb.write(parallel_orb);
        parallel_clientb.flush();
      }

      // Clear CB1 interrupt, clear CB2 interrupt if not independent
      parallel_ifr &= ~0x10;
      if ((parallel_pcr & 0xA0) != 0x20) {
        parallel_ifr &= ~0x08;
      }

      // Schedule the handshake interrupt, for pacing
      while (add_alarm_in_us(PARALLEL_TXACK_US, parallel_irqhsb, nullptr, true) < 0);
      break;

    case 0x1: // ORA
      parallel_ora = data;
      if (parallel_clienta && parallel_clienta.connected()) {
        // parallel_clienta.write((parallel_ora & parallel_ddra) | (parallel_ira & ~parallel_ddra));
        parallel_clienta.write(parallel_ora);
        parallel_clienta.flush();
      }

      // Clear CA1 interrupt, clear CA2 interrupt if not independent
      parallel_ifr &= ~0x02;
      if ((parallel_pcr & 0x0A) != 0x02) {
        parallel_ifr &= ~0x01;
      }

      // Schedule the handshake interrupt, for pacing
      while (add_alarm_in_us(PARALLEL_TXACK_US, parallel_irqhsa, nullptr, true) < 0);
      // parallel_last_ch_ts = time_us_64();
      break;

    case 0x2: // DDRB
      parallel_ddrb = data;
      break;
    case 0x3: // DDRA
      parallel_ddra = data;
      break;

    // Timers and shift register not implemented

    case 0xB: // ACR
      parallel_acr = data;
      break;
    case 0xC: // PCR
      parallel_pcr = data;
      break;

    case 0xD: // IFR
      parallel_ifr &= ~(data & 0x7F);   // Clear some interrupts
      break;

    case 0xE: // IER
      if (data & 0x80) {
        parallel_ier |= data & 0x7F;    // Enable some interrupts
      } else {
        parallel_ier &= ~(data & 0x7F); // Disable some interrupts
      }
      break;

    case 0xF: // ORA*
      parallel_ora = data;
      if (parallel_clienta && parallel_clienta.connected()) {
        // parallel_clienta.write((parallel_ora & parallel_ddra) | (parallel_ira & ~parallel_ddra));
        parallel_clienta.write(parallel_ora);
        parallel_clienta.flush();
      }

      // Clear CA1 interrupt, clear CA2 interrupt if not independent
      parallel_ifr &= ~0x02;
      if ((parallel_pcr & 0x0A) != 0x02) {
        parallel_ifr &= ~0x01;
      }
      break;
  }

#if PARALLEL_DEBUG
  Serial.print("LPT WRITE ");
  Serial.print(address >> 1, HEX);
  Serial.print(" ");
  Serial.println(data, HEX);
#endif

  parallel_update_irq();
}
