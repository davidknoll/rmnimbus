/**
 * Connect to a WiFi network using details from the secrets file
 */
void init_wifi() {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  char hostname[20];
  WiFi.macAddress(mac);
  sprintf(hostname, "nimbus-%02x%02x%02x", mac[3], mac[4], mac[5]);
  WiFi.setHostname(hostname);

  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
  WiFi.waitForConnectResult();

  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * Set the system timer to local time over WiFi using World Time API
 */
void init_time() {
  HTTPClient http;
  if (http.begin("http://worldtimeapi.org/api/timezone/" TIMEZONE)) {
    if (http.GET() > 0) {
      JSONVar timedata = JSON.parse(http.getString());
      struct timeval tv = {
        (long) timedata["unixtime"] + (long) timedata["raw_offset"] +
        ((bool) timedata["dst"] ? (long) timedata["dst_offset"] : 0),
        0
      };
      settimeofday(&tv, nullptr);

      time_t now = time(nullptr);
      Serial.print("Public IP address: ");
      Serial.println((const char *) timedata["client_ip"]);
      Serial.print("Current time: ");
      Serial.print(ctime(&now));
    }
    http.end();
  }
}

/**
 * Set the Pico's RTC based on the system timer
 */
void init_rtc() {
  rtc_init();
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
  rtc_set_datetime(&dt);
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
