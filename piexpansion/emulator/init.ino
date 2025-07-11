/**
 * Connect to a WiFi network using details from the secrets file
 * and activate the Bluetooth serial port
 */
void init_wifi() {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  char hostname[20];
  WiFi.macAddress(mac);
  snprintf(hostname, 20, "nimbus-%02x%02x%02x", mac[3], mac[4], mac[5]);

  WiFi.setHostname(hostname);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
  WiFi.waitForConnectResult();

  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());

  SerialBT.setName(hostname);
  SerialBT.begin();
}

/**
 * Set the system timer and RTC over WiFi using NTP
 */
void init_time() {
  NTP.waitSet();
  setenv("TZ", TZ_POSIX, 1);
  tzset();

  // Set the Pico's RTC to local time based on the system timer
  time_t now = time(nullptr);
  struct tm *ltime = localtime(&now);
  datetime_t dt = {
    .year  = (int16_t) (ltime->tm_year + 1900),
    .month = (int8_t)  (ltime->tm_mon  + 1),
    .day   = (int8_t)   ltime->tm_mday,
    .dotw  = (int8_t)   ltime->tm_wday,
    .hour  = (int8_t)   ltime->tm_hour,
    .min   = (int8_t)   ltime->tm_min,
    .sec   = (int8_t)   ltime->tm_sec
  };
  rtc_init();
  rtc_set_datetime(&dt);

  Serial.print("Current time: ");
  Serial.print(ctime(&now));
}

/**
 * Allow firmware updates over WiFi
 */
void init_ota() {
  ArduinoOTA.setHostname(WiFi.getHostname());
  ArduinoOTA.setPassword(SECRET_OTA_PASS);
  ArduinoOTA.begin();
}

/**
 * Initialise the microSD card
 */
void init_sd() {
  SDFSConfig sc;
  SPI.begin();
  sc.setAutoFormat(false);
  sc.setCSPin(SD_CS_PIN);
  sc.setSPISpeed(SPI_FULL_SPEED);
  sc.setSPI(SPI);
  sc.setPart(0);
  SDFS.setConfig(sc);
  SDFS.begin();
}

/**
 * Start the PIO with the Nimbus expansion bus interface
 */
void init_pio() {
  uint offset = pio_add_program(DEVICE_PIO, &device_program);
  // device_program_init(DEVICE_PIO, 0, offset, 0);
  device_program_init(DEVICE_PIO, 1, offset, 1);
  device_program_init(DEVICE_PIO, 2, offset, 2);
  device_program_init(DEVICE_PIO, 3, offset, 3);
}

static void _rebootme(void) { rp2040.reboot(); }

/**
 * Enable reset by the Nimbus, output reset reason
 */
void init_reset() {
  rp2040.enableDoubleResetBootloader();

  pinMode(nRESET, INPUT);
  attachInterrupt(digitalPinToInterrupt(nRESET), _rebootme, LOW);

  Serial.print("Reset reason: ");
  switch (rp2040.getResetReason()) {
    case RP2040::UNKNOWN_RESET:  Serial.println("unknown");        break;
    case RP2040::PWRON_RESET:    Serial.println("power on");       break;
    case RP2040::RUN_PIN_RESET:  Serial.println("RUN pin");        break;
    case RP2040::SOFT_RESET:     Serial.println("software");       break;
    case RP2040::WDT_RESET:      Serial.println("watchdog");       break;
    case RP2040::DEBUG_RESET:    Serial.println("debugger");       break;
    case RP2040::GLITCH_RESET:   Serial.println("power glitch");   break;
    case RP2040::BROWNOUT_RESET: Serial.println("power brownout"); break;
    default:                     Serial.println("(new)");
  }
}
