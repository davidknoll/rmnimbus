/**
 * Emulate my RTC module
 */

#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <SDFS.h>
#include <WiFi.h>
#include <hardware/pio.h>
#include <hardware/rtc.h>
#include <sys/time.h>
#include <time.h>

#include "arduino_secrets.h"
#include "device.pio.h"

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#include "pico/cyw43_arch.h"
#include "boards/pico_w.h"
#define ledon() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1)
#define ledoff() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0)
#define led(a) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (a))

#define rol(b) ((((b) >> 7) & 1) | ((b) << 1))
#define ror(b) ((((b) << 7) & 0x80) | ((b) >> 1))
#define bintobcd(b) ((((b) / 10) << 4) | ((b) % 10))
#define bcdtobin(b) ((((b) >> 4) * 10) + ((b) & 0x0F))

void setup() {
  Serial.begin(115200);
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  // Connect to WiFi
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
  WiFi.waitForConnectResult();
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());

  // Set the clock to local time
  rtc_init();
  HTTPClient http;
  if (http.begin("http://worldtimeapi.org/api/timezone/" SECRET_TIMEZONE)) {
    if (http.GET() > 0) {
      JSONVar timedata = JSON.parse(http.getString());
      struct timeval tv = {
        (long) timedata["unixtime"] + (long) timedata["raw_offset"] +
        ((bool) timedata["dst"] ? (long) timedata["dst_offset"] : 0),
        0
      };
      settimeofday(&tv, nullptr);

      time_t now = time(nullptr);
      struct tm *ltime = localtime(&now);
      datetime_t dt = {
        .year = (int16_t) (ltime->tm_year + 1900),
        .month = (int8_t) (ltime->tm_mon + 1),
        .day = (int8_t) ltime->tm_mday,
        .dotw = (int8_t) ltime->tm_wday,
        .hour = (int8_t) ltime->tm_hour,
        .min = (int8_t) ltime->tm_min,
        .sec = (int8_t) ltime->tm_sec
      };
      rtc_set_datetime(&dt);

      Serial.print("Public IP address: ");
      Serial.println((const char *) timedata["client_ip"]);
      Serial.print("Current time: ");
      Serial.print(ctime(&now));
    }
    http.end();
  }

  // Allow firmware updates over WiFi
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  char hostname[14];
  WiFi.macAddress(mac);
  sprintf(hostname, "nimbus-%02x%02x%02x", mac[3], mac[4], mac[5]);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(SECRET_OTA_PASS);
  ArduinoOTA.begin();

  // Initialise the microSD card
  SDFSConfig sc;
  SPI.begin();
  sc.setAutoFormat(false);
  sc.setCSPin(17);
  sc.setSPISpeed(SPI_FULL_SPEED);
  sc.setSPI(SPI);
  sc.setPart(0);
  SDFS.setConfig(sc);
  SDFS.begin();

  // Start the PIO bus interface
  uint offset = pio_add_program(pio0, &device_program);
  device_program_init(pio0, 0, offset, 3);

  Serial.println("Ready");
}

void loop() {
  struct busreq req;
  datetime_t t;
  ArduinoOTA.handle();

  if (device_program_get(pio0, 0, &req) != nullptr) {
    if (req.isread) {
      // Note bit shifting hack to fit the DS12C887A directly on the bus
      rtc_get_datetime(&t);
      switch (ror(req.address)) {
        case 0x00: req.data = rol(bintobcd(t.sec)); break;
        case 0x02: req.data = rol(bintobcd(t.min)); break;
        case 0x04: req.data = rol(bintobcd(t.hour)); break;
        case 0x06: req.data = rol(bintobcd(t.dotw + 1)); break;
        case 0x07: req.data = rol(bintobcd(t.day)); break;
        case 0x08: req.data = rol(bintobcd(t.month)); break;
        case 0x09: req.data = rol(bintobcd(t.year % 100)); break;
        case 0x0A: req.data = rol(0x20); break;
        case 0x0B: req.data = rol(0x02); break;
        case 0x0C: req.data = rol(0x00); break;
        case 0x0D: req.data = rol(0x80); break;
        case 0x32: req.data = rol(bintobcd(t.year / 100)); break;
        default: req.data = 0x00;
      }
      device_program_respond(pio0, 0, &req);
    }
  }
}
