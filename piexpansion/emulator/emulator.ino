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
#include <hardware/watchdog.h>
#include <sys/time.h>
#include <time.h>

#include "arduino_secrets.h"
#include "device.pio.h"

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#define rol(b) ((((b) >> 7) & 1) | ((b) << 1))
#define ror(b) ((((b) << 7) & 0x80) | ((b) >> 1))
#define bintobcd(b) ((((b) / 10) << 4) | ((b) % 10))
#define bcdtobin(b) ((((b) >> 4) * 10) + ((b) & 0x0F))

#define TIMEZONE "Europe/London"
#define SD_CS_PIN 17
#define DEVICE_PIO pio0
#define DEVICE_SM 0

#define DEVICE_CS 2
#define DEVICE_READ dcc_read
#define DEVICE_WRITE dcc_write

#define PARALLEL_DEBUG 1
#define PARALLEL_IRQ_PIN nINT2
#define PARALLEL_TCP_PORTA 1100
#define PARALLEL_TCP_PORTB 1101
#define PARALLEL_TXACK_US 100

#define DCC_DEBUG 0
#define DCC_IRQ_PIN nINT2

void setup() {
  Serial.begin(115200);
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  alarm_pool_init_default();
  init_wifi();
  init_time();
  init_rtc();
  init_ota();
  init_sd();
  init_pio();
  // parallel_setup();
  dcc_setup();

  Serial.println("Ready");
}

void loop() {
  struct busreq req;
  ArduinoOTA.handle();
  // parallel_loop();
  dcc_loop();

  if (!digitalRead(nRESET)) {
    watchdog_enable(1, 1);
    while (1);
  }

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
