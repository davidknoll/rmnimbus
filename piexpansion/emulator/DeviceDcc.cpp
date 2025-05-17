#include <CircularBuffer.hpp>
#include <WiFi.h>
#include "DeviceDcc.h"
#define DCC_DEBUG 0

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

DeviceDcc::DeviceDcc(void) : _server(2345) {}

/**
 * To be called during sketch setup()
 */
void DeviceDcc::setup(void) {
  DeviceBase::setup();

  _wr0a  = 0x00;
  _wr0b  = 0x00;
  _wr1a  = 0x00;
  _wr1b  = 0x00;
  _wr2   = 0x00;
  _wr3a  = 0x00;
  _wr5a  = 0x00;
  _wr9   = 0x00;
  _wr12a = 0x00;
  _wr13a = 0x00;
  _wr14a = 0x00;
  _wr15a = 0xF8;
  _rr3   = 0x00;
  _rr8a  = -1;
  _auxwr = 0x00;
  _auxrd = 0x00;
  _auxip = false;

  _server.begin();
}

/**
 * To be called during sketch loop()
 */
void DeviceDcc::loop(void) {
  if (Serial.available() && _rr8a == -1 && (_wr3a & 0x01)) {
    _rr8a = Serial.read();

    switch (_wr3a & 0xC0) {            // Rx bits/char
      case 0x00: _rr8a |= 0xE0; break; // 5 bits
      case 0x40: _rr8a |= 0x80; break; // 7 bits
      case 0x80: _rr8a |= 0xC0; break; // 6 bits
    }

    if (_wr14a & 0x08) { _tx(_rr8a); } // Auto echo?

    switch (_wr1a & 0x18) {    // Rx interrupt mode
      case 0x08:               // Rx int on 1st char or special
        _wr1a &= ~0x18;        // Got that 1st char, now clear the mode
        // Missing break; intentional
      case 0x10:               // Rx int on all char or special
        _setip(0x20);
        break;
    }
    if ((_wr1a & 0xE0) == 0xE0) { _dma_flag = true; } // Rx DMA?
  }

  if (_client) {
    if (_client.connected()) {
      while (_client.available() && !_rxbuf.isFull()) {
        _rxbuf.push(_client.read());
        switch (_wr1b & 0x18) {    // Rx interrupt mode
          case 0x08:               // Rx int on 1st char or special
            _wr1b &= ~0x18;        // Got that 1st char, now clear the mode
            // Missing break; intentional
          case 0x10:               // Rx int on all char or special
            _setip(0x04);
            break;
        }
      }
      while (!_txbuf.isEmpty()) {
        _client.write(_txbuf.shift());
      }
    } else {
      _client.stop();
      _client = _server.accept();
    }
  } else {
    _client = _server.accept();
  }
}

/**
 * Read from an emulated Data Communications Controller card
 */
uint8_t DeviceDcc::read(uint8_t address) {
  uint reg = 0, data = 0x00;

  switch (address & 0xE) {
    case 0x0: // Channel B control
      reg = (_wr0b & 0x30) ? (_wr0b & 0x7) : (_wr0b & 0xF);
      _wr0b = 0x00;

      switch (reg) {
        case 0: // RR0
        case 4: // RR4
          if (!_rxbuf.isEmpty()) { data |= 0x01; }
          if (!_txbuf.isFull())  { data |= 0x04; }
          break;

        case 2: // RR2
        case 6: // RR6
          data = _wr2;
          data &= (_wr9 & 0x10) ? ~0x70 : ~0x0E;
          if (_rr3 & 0x20) {        // Ch A Rx
            data |= (_wr9 & 0x10) ? 0x30 : 0x0C;
          } else if (_rr3 & 0x10) { // Ch A Tx
            data |= (_wr9 & 0x10) ? 0x10 : 0x08;
          } else if (_rr3 & 0x08) { // Ch A ES
            data |= (_wr9 & 0x10) ? 0x50 : 0x0A;
          } else if (_rr3 & 0x04) { // Ch B Rx
            data |= (_wr9 & 0x10) ? 0x20 : 0x04;
          } else if (_rr3 & 0x02) { // Ch B Tx
            data |= (_wr9 & 0x10) ? 0x00 : 0x00;
          } else if (_rr3 & 0x01) { // Ch B ES
            data |= (_wr9 & 0x10) ? 0x40 : 0x02;
          }
          break;
      }
      break;

    case 0x2: // Channel A control
      reg = (_wr0a & 0x30) ? (_wr0a & 0x7) : (_wr0a & 0xF);
      _wr0a = 0x00;

      switch (reg) {
        case 0: // RR0
        case 4: // RR4
          data = 0x2C;                          // CTS, DCD, Tx empty always set
          if (_rr8a != -1) { data |= 0x01; }    // Rx available?
          break;

        case 1: // RR1
        case 5: // RR5
          data = 0x07;
          break;

        case 2: // RR2
        case 6: // RR6
          data = _wr2;
          break;

        case 3: // RR3
        case 7: // RR7
          data = _rr3;
          break;

        case 8: // RR8
          data = _rx();
          break;

        case 9: // RR9
        case 13: // RR13
          data = _wr13a;
          break;

        case 10: // RR10
        case 14: // RR14
          // Leave it at 0x00
          break;

        case 11: // RR11
        case 15: // RR15
          data = _wr15a;
          break;

        case 12: // RR12
          data = _wr12a;
          break;
      }
      break;

    case 0x4: // Channel B data
      data = _rxbuf.shift();
      _clrip(0x04);
      break;

    case 0x6: // Channel A data
      data = _rx();
      break;

    default:  // Aux register
      data = _auxrd;
      _auxip = false;
      _clrip(0x00);
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
void DeviceDcc::write(uint8_t address, uint8_t data) {
  uint reg = 0;

  switch (address & 0xE) {
    case 0x0: // Channel B control
      reg = (_wr0b & 0x30) ? (_wr0b & 0x7) : (_wr0b & 0xF);
      _wr0b = 0x00;

      switch (reg) {
        case 0: // WR0
          _wr0b = data;
          switch (_wr0b & 0x38) {
            case 0x10: // Reset ES int
              _clrip(0x01);
              break;
            case 0x20: // Enable next Rx int
              _wr1b &= ~0x18;
              _wr1b |= 0x08;
              break;
            case 0x28: // Reset Tx int
              _clrip(0x02);
              break;
          }
          break;

        case 1: // WR1
          _wr1b = data;
          break;

        case 2: // WR2
          _wr2 = data;
          break;
      }
      break;

    case 0x2: // Channel A control
      reg = (_wr0a & 0x30) ? (_wr0a & 0x7) : (_wr0a & 0xF);
      _wr0a = 0x00;

      switch (reg) {
        case 0: // WR0
          _wr0a = data;
          switch (_wr0a & 0x38) {
            case 0x10: // Reset ES int
              _clrip(0x08);
              break;
            case 0x20: // Enable next Rx int
              _wr1a &= ~0x18;
              _wr1a |= 0x08;
              break;
            case 0x28: // Reset Tx int
              _clrip(0x10);
              break;
          }
          break;

        case 1: // WR1
          _wr1a = data;
          break;

        case 2: // WR2
          _wr2 = data;
          break;

        case 3: // WR3
          _wr3a = data;
          break;

        case 5: // WR5
          _wr5a = data;
          break;

        case 8: // WR8
          _tx(data);
          break;

        case 9: // WR9
          _wr9 = data;
          if (data & 0x80) {           // Channel reset A (or force hardware reset)
            _wr0a = 0x00;
            _wr1a = 0x00;
            _wr3a = 0x00;
            _wr5a = 0x00;
            _wr12a = 0x00;
            _wr13a = 0x00;
            _wr14a = 0x00;
            _wr15a = 0xF8;
            _rr8a = -1;
            _clrip(0xF8);
            _dma_flag = false;
          }
          if (data & 0x40) {           // Channel reset B (or force hardware reset)
            _wr0b = 0x00;
            _clrip(0xC7);
          }
          if ((data & 0xC0) == 0xC0) { // Force hardware reset
            _wr2 = 0x00;
          }
          break;

        case 12: // WR12
          _wr12a = data;
          break;

        case 13: // WR13
          _wr13a = data;
          break;

        case 14: // WR14
          _wr14a = data;
          break;

        case 15: // WR15
          _wr15a = data;
          break;
      }
      break;

    case 0x4: // Channel B data
      _txbuf.push(data);
      break;

    case 0x6: // Channel A data
      _tx(data);
      break;

    default:  // Aux register
      _auxwr = data;
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
void DeviceDcc::_tx(uint8_t data) {
  _clrip(0x10);
  if ((_wr1a & 0xE0) == 0xC0) { _dma_flag = false; }

  switch (_wr5a & 0x60) {          // Tx bits/char
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

  if (_wr5a & 0x08) { Serial.write(data); } // Tx enabled?
  if (_wr14a & 0x10) { _rr8a = data; }      // Local loopback?
  if (_wr1a & 0x02) { _setip(0x10); }       // Tx interrupt?
  if ((_wr1a & 0xE0) == 0xC0) { _dma_flag = true; } // Tx DMA?
}

/**
 * Read a byte, if there is one
 */
uint8_t DeviceDcc::_rx(void) {
  uint8_t data = 0x00;
  if (_rr8a != -1) {
    data = _rr8a;
    _rr8a = -1;
    _clrip(0x20);
    if ((_wr1a & 0xE0) == 0xE0) { _dma_flag = false; }
  }
  return data;
}

/**
 * Set interrupt pending flag(s) and if necessary the /IRQ line
 */
void DeviceDcc::_setip(uint8_t mask) {
  _rr3 |= mask;

  if (((_wr9 & 0x08) && _rr3) || (!(_wr9 & 0x04) && _auxip)) {
#if DCC_DEBUG
    Serial.print("DCC IRQ SET ");
    Serial.println(mask, HEX);
#endif
  }
}

/**
 * Clear interrupt pending flag(s) and if necessary the /IRQ line
 */
void DeviceDcc::_clrip(uint8_t mask) {
  _rr3 &= ~mask;

  if (!((_wr9 & 0x08) && _rr3) && !(!(_wr9 & 0x04) && _auxip)) {
#if DCC_DEBUG
    Serial.print("DCC IRQ CLR ");
    Serial.println(mask, HEX);
#endif
  }
}

bool DeviceDcc::irq(void) {
  return ( (_wr9 & 0x08) && _rr3  )  //     MIE, and SCC interrupt pending
      || (!(_wr9 & 0x04) && _auxip); // or !DLC, and aux interrupt pending
}

bool DeviceDcc::dma(void) { return _dma_flag; }
