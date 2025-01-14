#include "DeviceParallel.h"

DeviceParallel::DeviceParallel(void)
  : _servera(PARALLEL_TCP_PORTA), _serverb(PARALLEL_TCP_PORTB) {}

/**
 * To be called during sketch setup()
 */
void DeviceParallel::setup(void) {
  DeviceBase::setup();

  // 6522 VIA registers
  _orb  = 0x00;
  _irb  = 0xFF;
  _ora  = 0x00;
  _ira  = 0xFF;
  _ddrb = 0x00;
  _ddra = 0x00;
  _acr  = 0x00;
  _pcr  = 0x00;
  _ifr  = 0x00;
  _ier  = 0x80;

  _servera.begin();
  _serverb.begin();
  // _last_ch_ts = time_us_64();
}

/**
 * To be called during sketch loop()
 */
void DeviceParallel::loop(void) {
  // Stop any clients which disconnect
  if (_clienta && !_clienta.connected()) {
    _clienta.stop();
  }
  if (_clientb && !_clientb.connected()) {
    _clientb.stop();
  }

  // Check for any new clients connecting
  if (!_clienta) {
    WiFiClient client = _servera.accept();
    if (client) { _clienta = client; }
  }
  if (!_clientb) {
    WiFiClient client = _serverb.accept();
    if (client) { _clientb = client; }
  }

  // Check for input data
  // This handshaking uses the CA2 / CB2 interrupts for received data ready,
  // which isn't what's in the datasheet, but it coexists with emulating
  // the parallel printer port over a bidirectional network port.
  if (_clienta && _clienta.available()) {
    // If "latching" is enabled and there's already a CA2 interrupt pending,
    // don't overwrite the existing byte.
    // Only set the CA2 interrupt if it is in handshake / pulse mode.
    if (!((_acr & 0x01) && (_ifr & 0x01))) {
      _ira = _clienta.read();
      if ((_pcr & 0x0C) == 0x08) { _ifr |= 0x01; }
    }
  }
  if (_clientb && _clientb.available()) {
    // If "latching" is enabled and there's already a CB2 interrupt pending,
    // don't overwrite the existing byte.
    // Only set the CB2 interrupt if it is in handshake / pulse mode.
    if (!((_acr & 0x02) && (_ifr & 0x08))) {
      _irb = _clientb.read();
      if ((_pcr & 0xC0) == 0x80) { _ifr |= 0x08; }
    }
  }

  // TESTING- Send the occasional spurious interrupt in case we got stuck
  // if (time_us_64() - _last_ch_ts > 100000) { // 100ms since last char
  //   _last_ch_ts = time_us_64();
  //   if ((_pcr & 0x0C) == 0x08) { _ifr |= 0x02; }
  // }
}

/**
 * Update IFR bit 7 and the IRQ pin, based on the rest of the IFR and IER
 */
bool DeviceParallel::irq(void) {
  if (_ifr & _ier & 0x7F) {
#if PARALLEL_DEBUG
    if (!(_ifr & 0x80)) {
      Serial.print("LPT IRQSET ");
      Serial.println(_ifr, HEX);
    }
#endif
    _ifr |= 0x80;
  } else {
#if PARALLEL_DEBUG
    if (_ifr & 0x80) {
      Serial.print("LPT IRQCLR ");
      Serial.println(_ifr, HEX);
    }
#endif
    _ifr &= ~0x80;
  }

  return _ifr & 0x80;
}

/**
 * If handshake / pulse mode, issue CA1 interrupt for the ack
 */
int64_t DeviceParallel::_irqhsa(alarm_id_t id, void *user_data) {
  DeviceParallel *me = (DeviceParallel *) user_data;
  if ((me->_pcr & 0x0C) == 0x08) { me->_ifr |= 0x02; }
  return 0;
}

/**
 * If handshake / pulse mode, issue CB1 interrupt for the ack
 */
int64_t DeviceParallel::_irqhsb(alarm_id_t id, void *user_data) {
  DeviceParallel *me = (DeviceParallel *) user_data;
  if ((me->_pcr & 0xC0) == 0x80) { me->_ifr |= 0x10; }
  return 0;
}

/**
 * Read from an emulated parallel port card accessible over WiFi
 * CA2 is /STROBE output, CA1 is /ACK input
 */
uint8_t DeviceParallel::read(uint8_t address) {
  uint data = 0x00;
  switch ((address >> 1) & 0xF) {
    case 0x0: // IRB
      // data = (_orb & _ddrb) | (_irb & ~_ddrb);
      data = _irb;

      // Clear CB1 interrupt, clear CB2 interrupt if not independent
      _ifr &= ~0x10;
      if ((_pcr & 0xA0) != 0x20) {
        _ifr &= ~0x08;
      }
      break;

    case 0x1: // IRA
    case 0xF: // IRA*
      // data = (_ora & _ddra) | (_ira & ~_ddra);
      data = _ira;

      // Clear CA1 interrupt, clear CA2 interrupt if not independent
      _ifr &= ~0x02;
      if ((_pcr & 0x0A) != 0x02) {
        _ifr &= ~0x01;
      }
      break;

    case 0x2: // DDRB
      data = _ddrb;
      break;
    case 0x3: // DDRA
      data = _ddra;
      break;

    // Timers and shift register not implemented

    case 0xB: // ACR
      data = _acr;
      break;
    case 0xC: // PCR
      data = _pcr;
      break;

    case 0xD: // IFR
      data = _ifr;
      break;
    case 0xE: // IER
      data = _ier;
      break;
  }

#if PARALLEL_DEBUG
  Serial.print("LPT READ  ");
  Serial.print(address >> 1, HEX);
  Serial.print(" ");
  Serial.println(data, HEX);
#endif
  return data & 0xFF;
}

/**
 * Write to an emulated parallel port card accessible over WiFi
 * CA2 is /STROBE output, CA1 is /ACK input
 */
void DeviceParallel::write(uint8_t address, uint8_t data) {
  switch ((address >> 1) & 0xF) {
    case 0x0: // ORB
      _orb = data;
      if (_clientb && _clientb.connected()) {
        // _clientb.write((_orb & _ddrb) | (_irb & ~_ddrb));
        _clientb.write(_orb);
        _clientb.flush();
      }

      // Clear CB1 interrupt, clear CB2 interrupt if not independent
      _ifr &= ~0x10;
      if ((_pcr & 0xA0) != 0x20) {
        _ifr &= ~0x08;
      }

      // Schedule the handshake interrupt, for pacing
      while (add_alarm_in_us(PARALLEL_TXACK_US, _irqhsb, this, true) < 0);
      break;

    case 0x1: // ORA
      _ora = data;
      if (_clienta && _clienta.connected()) {
        // _clienta.write((_ora & _ddra) | (_ira & ~_ddra));
        _clienta.write(_ora);
        _clienta.flush();
      }

      // Clear CA1 interrupt, clear CA2 interrupt if not independent
      _ifr &= ~0x02;
      if ((_pcr & 0x0A) != 0x02) {
        _ifr &= ~0x01;
      }

      // Schedule the handshake interrupt, for pacing
      while (add_alarm_in_us(PARALLEL_TXACK_US, _irqhsa, this, true) < 0);
      // _last_ch_ts = time_us_64();
      break;

    case 0x2: // DDRB
      _ddrb = data;
      break;
    case 0x3: // DDRA
      _ddra = data;
      break;

    // Timers and shift register not implemented

    case 0xB: // ACR
      _acr = data;
      break;
    case 0xC: // PCR
      _pcr = data;
      break;

    case 0xD: // IFR
      _ifr &= ~(data & 0x7F);   // Clear some interrupts
      break;

    case 0xE: // IER
      if (data & 0x80) {
        _ier |= data & 0x7F;    // Enable some interrupts
      } else {
        _ier &= ~(data & 0x7F); // Disable some interrupts
      }
      break;

    case 0xF: // ORA*
      _ora = data;
      if (_clienta && _clienta.connected()) {
        // _clienta.write((_ora & _ddra) | (_ira & ~_ddra));
        _clienta.write(_ora);
        _clienta.flush();
      }

      // Clear CA1 interrupt, clear CA2 interrupt if not independent
      _ifr &= ~0x02;
      if ((_pcr & 0x0A) != 0x02) {
        _ifr &= ~0x01;
      }
      break;
  }

#if PARALLEL_DEBUG
  Serial.print("LPT WRITE ");
  Serial.print(address >> 1, HEX);
  Serial.print(" ");
  Serial.println(data, HEX);
#endif
}
