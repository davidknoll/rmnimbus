#include <SerialBT.h>
#include <hardware/rtc.h>
#include "DeviceDsrtc.h"
#define rol(b) ((((b) >> 7) & 0x01) | (((b) << 1) & 0xFE))
#define ror(b) ((((b) << 7) & 0x80) | (((b) >> 1) & 0x7F))
#define bintobcd(b) ((((b) / 10) << 4) | ((b) % 10))
#define bcdtobin(b) ((((b) >> 4) * 10) + ((b) & 0x0F))

const uint32_t DeviceDsrtc::_rates[16] = {
  // Interrupt period to nearest whole microsecond
  0, 3906, 7813,
  122, 244, 488, 977, 1953, 3906, 7813,
  15625, 31250, 62500, 125000, 250000, 500000
};

bool DeviceDsrtc::_rtcb(repeating_timer_t *rt) {
  // Set the periodic interrupt flag
  ((DeviceDsrtc *) rt->user_data)->_ctlc |= 0x40;
  return true;
}

void DeviceDsrtc::setup(void) {
  DeviceBase::setup();
  cancel_repeating_timer(&_rt);
  _ctla = 0x00;
  _ctlb = 0x06;
  _ctlc = 0x00;
}

/**
 * Read from an emulated Dallas RTC module
 */
uint8_t DeviceDsrtc::read(uint8_t address) {
  uint8_t hour;
  if (!(_ctlb & 0x80)) { rtc_get_datetime(&_t); }

  // Note bit shifting hack to fit the DS12C887A directly on the bus-
  // the case numbers below match the datasheet
  switch (ror(address)) {
    // Time and date
    case 0x00: return rol((_ctlb & 0x04) ? (_t.sec)        : bintobcd(_t.sec));
    case 0x02: return rol((_ctlb & 0x04) ? (_t.min)        : bintobcd(_t.min));
    case 0x06: return rol((_ctlb & 0x04) ? (_t.dotw + 1)   : bintobcd(_t.dotw + 1));
    case 0x07: return rol((_ctlb & 0x04) ? (_t.day)        : bintobcd(_t.day));
    case 0x08: return rol((_ctlb & 0x04) ? (_t.month)      : bintobcd(_t.month));
    case 0x09: return rol((_ctlb & 0x04) ? (_t.year % 100) : bintobcd(_t.year % 100));
    case 0x32: return rol((_ctlb & 0x04) ? (_t.year / 100) : bintobcd(_t.year / 100));

    // Hours with 12/24h mode
    case 0x04:
      hour = _t.hour;
      if (!(_ctlb & 0x02)) {
        hour %= 12;
        if (!hour) { hour = 12; }
      }
      if (!(_ctlb & 0x04)) { hour = bintobcd(hour); }
      if (!(_ctlb & 0x02) && _t.hour >= 12) { hour |= 0x80; }
      return rol(hour);

    // Control registers
    case 0x0A: return rol((rtc_running() ? 0x20 : 0x70) | (_ctla & 0x0F));
    case 0x0B: return rol(_ctlb);
    case 0x0C:
      // Interrupt flags are cleared on read
      if (_ctlb & _ctlc & 0x70) { _ctlc |= 0x80; }
      hour = rol(_ctlc);
      _ctlc = 0x00;
      return hour;
    case 0x0D: return rol((time(nullptr) >= 10000000) ? 0x80 : 0x00);

    // Secret Bluetooth debug port approximating an 8251
    // If card select 2 is used, data will be at 57C and status at 57E
    case 0x3E: return SerialBT.read();
    case 0x3F: return ((SerialBT    ? 0x80 : 0x00) |  // DSR, is port running
      (SerialBT.overflow()          ? 0x10 : 0x00) |  // RX overflow error
      (SerialBT.availableForWrite() ? 0x05 : 0x00) |  // TX empty, TX ready
      (SerialBT.available()         ? 0x02 : 0x00));  // RX ready

    // Alarm and RAM not implemented
    default:   return rol(0x00);
  }
}

/**
 * Write to an emulated Dallas RTC module
 */
void DeviceDsrtc::write(uint8_t address, uint8_t data) {
  switch (ror(address)) {
    case 0x0A:
      cancel_repeating_timer(&_rt);
      _ctla = ror(data) & 0x0F;
      if (_ctla) { add_repeating_timer_us(_rates[_ctla], _rtcb, this, &_rt); }
      break;
    case 0x0B:
      // One more update, in case this write is to inhibit updates
      if (!(_ctlb & 0x80)) { rtc_get_datetime(&_t); }
      _ctlb = ror(data) & 0xC6;
      break;

    case 0x3E: SerialBT.write(data); break;
  }
}

bool DeviceDsrtc::irq(void) {
  // IRQF = (PF & PIE) | (AF & AIE) | (UF & UIE)
  if (_ctlb & _ctlc & 0x70) { _ctlc |= 0x80; }
  return _ctlc & 0x80;
}
