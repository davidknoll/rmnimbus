/**
 * Test that the board is able to use its key hardware and respond to the Nimbus accessing it.
 */

#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <SDFS.h>
#include <WiFi.h>
#include <hardware/pio.h>
#include <sys/time.h>

#include "arduino_secrets.h"
#include "leds.pio.h"

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#include "pico/cyw43_arch.h"
#include "boards/pico_w.h"
#define ledon() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1)
#define ledoff() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0)

void setup() {
  Serial.begin(115200);
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  // Connect to WiFi
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
  WiFi.waitForConnectResult();
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());

  // Set the clock to local time
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

  // And list the root directory
  Serial.println("Directory:");
  Dir dir = SDFS.openDir("/");
  while (dir.next()) {
    Serial.print("\t");
    if (dir.isFile()) {
      Serial.print(" ");
    } else if (dir.isDirectory()) {
      Serial.print("[");
    } else {
      Serial.print("{");
    }
    Serial.print(dir.fileName());
    if (dir.isFile()) {
      Serial.print(" ");
    } else if (dir.isDirectory()) {
      Serial.print("]");
    } else {
      Serial.print("}");
    }
    Serial.print("\t");
    Serial.print(dir.fileSize());
    Serial.print("\t");
    time_t ft = dir.fileTime();
    Serial.print(ctime(&ft));
  }

  // Start the PIO LED test program
  uint offset = pio_add_program(pio0, &leds_program);
  leds_program_init(pio0, 0, offset);

  Serial.println("Ready");
}

void loop() {
  ArduinoOTA.handle();

  // Pico W onboard LED
  ledon();
  delay(250);
  ledoff();
  delay(250);

  // Some LEDs attached to AD0-7, using the PIO
  leds_program_outb(pio0, 0, 0x00);
  delay(250);
  leds_program_outb(pio0, 0, 0x01);
  delay(250);
  leds_program_outb(pio0, 0, 0x02);
  delay(250);
  leds_program_outb(pio0, 0, 0x04);
  delay(250);
  leds_program_outb(pio0, 0, 0x08);
  delay(250);
  leds_program_outb(pio0, 0, 0x10);
  delay(250);
  leds_program_outb(pio0, 0, 0x20);
  delay(250);
  leds_program_outb(pio0, 0, 0x40);
  delay(250);
  leds_program_outb(pio0, 0, 0x80);
  delay(250);
  leds_program_outb(pio0, 0, 0xFF);
  delay(250);
}
