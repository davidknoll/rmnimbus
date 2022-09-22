/**
 * Read from an emulated Dallas RTC module
 */
uint dsrtc_read(uint address) {
  datetime_t t;
  rtc_get_datetime(&t);

  // Note bit shifting hack to fit the DS12C887A directly on the bus-
  // the case numbers below match the datasheet
  switch (ror(address)) {
    case 0x00: return rol(bintobcd(t.sec));
    case 0x02: return rol(bintobcd(t.min));
    case 0x04: return rol(bintobcd(t.hour));
    case 0x06: return rol(bintobcd(t.dotw + 1));
    case 0x07: return rol(bintobcd(t.day));
    case 0x08: return rol(bintobcd(t.month));
    case 0x09: return rol(bintobcd(t.year % 100));
    case 0x0A: return rol(0x20);
    case 0x0B: return rol(0x02);
    case 0x0C: return rol(0x00);
    case 0x0D: return rol(0x80);
    case 0x32: return rol(bintobcd(t.year / 100));
    default: return 0x00;
  }
}

/**
 * Write to an emulated Dallas RTC module
 */
void dsrtc_write(uint address, uint data) {
  // Nothing, as the time comes from WiFi via the Pico's RTC,
  // and the emulated RTC's NVRAM isn't implemented
}
