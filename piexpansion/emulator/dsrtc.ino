static volatile uint8_t dsrtc_ctlb = 0x06;

/**
 * Read from an emulated Dallas RTC module
 */
uint dsrtc_read(uint address) {
  uint8_t hour;
  datetime_t t;
  rtc_get_datetime(&t);

  // Note bit shifting hack to fit the DS12C887A directly on the bus-
  // the case numbers below match the datasheet
  switch (ror(address)) {
    // Time and date
    case 0x00: return rol((dsrtc_ctlb & 0x04) ? (t.sec)        : bintobcd(t.sec));
    case 0x02: return rol((dsrtc_ctlb & 0x04) ? (t.min)        : bintobcd(t.min));
    case 0x06: return rol((dsrtc_ctlb & 0x04) ? (t.dotw + 1)   : bintobcd(t.dotw + 1));
    case 0x07: return rol((dsrtc_ctlb & 0x04) ? (t.day)        : bintobcd(t.day));
    case 0x08: return rol((dsrtc_ctlb & 0x04) ? (t.month)      : bintobcd(t.month));
    case 0x09: return rol((dsrtc_ctlb & 0x04) ? (t.year % 100) : bintobcd(t.year % 100));
    case 0x32: return rol((dsrtc_ctlb & 0x04) ? (t.year / 100) : bintobcd(t.year / 100));

    // Hours with 12/24h mode
    case 0x04:
      hour = t.hour;
      if (!(dsrtc_ctlb & 0x02)) {
        hour %= 12;
        if (!hour) { hour = 12; }
      }
      if (!(dsrtc_ctlb & 0x04)) { hour = bintobcd(hour); }
      if (!(dsrtc_ctlb & 0x02) && t.hour >= 12) { hour |= 0x80; }
      return rol(hour);

    // Control registers
    case 0x0A: return rol(rtc_running() ? 0x20 : 0x70);
    case 0x0B: return rol(dsrtc_ctlb);
    case 0x0C: return rol(0x00);
    case 0x0D: return rol(0x80);

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
void dsrtc_write(uint address, uint data) {
  switch (ror(address)) {
    case 0x0B: dsrtc_ctlb = ror(data) & 0x06; break;
    case 0x3E: SerialBT.write(data); break;
  }
}
