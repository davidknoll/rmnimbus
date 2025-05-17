#include <Arduino.h>
#include <pico/time.h>
#include "DevicePrinter.h"

void DevicePrinter::setup(void) {
  _ier = 0x80;
  // add_repeating_timer_ms(2000, _unstick, this, &_unstick_timer);
}

void DevicePrinter::loop(void) {
  if (_ifr & _ier & 0x7F) { _ifr |= 0x80; } else { _ifr &= ~0x80; }
}

bool DevicePrinter::irq(void) {
  return _ifr & 0x80;
}

uint8_t DevicePrinter::read(uint8_t address) {
  switch ((address >> 1) & 0xF) {
    case 0x1:
    case 0xF:
      return _ora;
    case 0x3:
      return _ddra;
    case 0xC:
      return 0x0A;
    case 0xD:
      return _ifr;
    case 0xE:
      return _ier;
    default:
      return 0x00;
  }
}

void DevicePrinter::write(uint8_t address, uint8_t data) {
  switch ((address >> 1) & 0xF) {
    case 0x1:
      _outch(data);
      add_alarm_in_ms(10, _ack, this, true);
    case 0xF:
      _ora = data;
      break;
    case 0x3:
      _ddra = data;
      break;
    case 0xD:
      _ifr &= ~(data & 0x7F);
      break;
    case 0xE:
      if (data & 0x80) {
        _ier |= data;
      } else {
        _ier &= ~data;
      }
      break;
  }
}

int64_t DevicePrinter::_ack(alarm_id_t id, void *user_data) {
  DevicePrinter *me = (DevicePrinter *) user_data;
  me->_ifr |= 0x02;
  return 0;
}

bool DevicePrinter::_unstick(repeating_timer_t *rt) {
  DevicePrinter *me = (DevicePrinter *) rt->user_data;
  me->_ifr |= 0x02;
  return true;
}

void DevicePrinter::_outch(uint8_t ch) {
  Serial.write(ch);
  Serial.flush();
}
