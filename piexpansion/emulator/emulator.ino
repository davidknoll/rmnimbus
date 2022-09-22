/**
 * I/O Device Emulator for RM Nimbus PC-186
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

#define SD_CS_PIN 17
#define DEVICE_PIO pio0
#define DEVICE_SM 0

#define DEVICE_CS 3
#define DEVICE_READ dsrtc_read
#define DEVICE_WRITE dsrtc_write

void setup() {
  Serial.begin(115200);
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  init_wifi();
  init_time();
  init_rtc();
  init_ota();
  init_sd();
  init_pio();
  alarm_pool_init_default();

  Serial.println("Ready");
}

void loop() {
  struct busreq req;
  ArduinoOTA.handle();

  // Check and respond to a pending request from the Nimbus
  if (device_program_get(DEVICE_PIO, DEVICE_SM, &req) != nullptr) {
    if (req.isread) {
      req.data = DEVICE_READ(req.address);
      device_program_respond(DEVICE_PIO, DEVICE_SM, &req);
    } else {
      DEVICE_WRITE(req.address, req.data);
    }
  }
}
