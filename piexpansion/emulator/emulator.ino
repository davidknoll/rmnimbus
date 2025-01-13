/**
 * I/O Device Emulator for RM Nimbus PC-186
 */

#include <ArduinoOTA.h>
#include <CircularBuffer.hpp>
#include <NTPClient.h>
#include <SDFS.h>
#include <SerialBT.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <hardware/pio.h>
#include <hardware/rtc.h>
#include <hardware/watchdog.h>
#include <sys/time.h>
#include <time.h>

#include "DeviceBase.h"
#include "DeviceDsrtc.h"
#include "arduino_secrets.h"
#include "device.pio.h"

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#define rol(b) ((((b) >> 7) & 0x01) | (((b) << 1) & 0xFE))
#define ror(b) ((((b) << 7) & 0x80) | (((b) >> 1) & 0x7F))
#define bintobcd(b) ((((b) / 10) << 4) | ((b) % 10))
#define bcdtobin(b) ((((b) >> 4) * 10) + ((b) & 0x0F))

#define TZ_OFFSET 0L
#define SD_CS_PIN 17
#define DEVICE_PIO pio0

#define DCB_ENABLE_FLOPPY 0
#define DCB_ENABLE_SASI   1

#define PARALLEL_DEBUG 0
#define PARALLEL_TCP_PORTA 1100
#define PARALLEL_TCP_PORTB 1101
#define PARALLEL_TXACK_US 100

#define DCC_DEBUG 0

DeviceBase *slot[4]; // Board supports slots 0-3
volatile bool parallel_irq_flag = false;
volatile bool dcc_irq_flag      = false;

void setup() {
  Serial.begin();
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  init_pio();
  alarm_pool_init_default();
  init_wifi();
  init_time();
  init_rtc();
  init_ota();
  init_sd();

  dcb_setup();
  parallel_setup();
  slot[2] = new DeviceDsrtc();
  slot[2]->setup();
  dcc_setup();

  Serial.println("Ready");
}

void loop() {
  struct busreq req;
  ArduinoOTA.handle();
  dcb_loop();
  parallel_loop();
  dcc_loop();

  // If /RESET is asserted by the Nimbus, reboot this card using the watchdog
  if (!digitalRead(nRESET)) {
    watchdog_enable(1, 1);
    while (1);
  }

  digitalWrite(nINT1, slot[2]->irq() ? LOW : HIGH);
  digitalWrite(nINT2, (parallel_irq_flag || dcc_irq_flag) ? LOW : HIGH);

  // Check and respond to a pending request from the Nimbus
  if (
    (device_program_get(DEVICE_PIO, 0, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 1, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 2, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 3, &req) != nullptr)
  ) {
    if (req.isread) {
      switch (req.cs) {
        case 0: req.data = dcb_read(     req.address); break;
        case 1: req.data = parallel_read(req.address); break;
        case 2: req.data = slot[2]->read(req.address); break;
        case 3: req.data = dcc_read(     req.address); break;
      }
      device_program_respond(DEVICE_PIO, req.cs, &req);
    } else {
      switch (req.cs) {
        case 0: dcb_write(     req.address, req.data); break;
        case 1: parallel_write(req.address, req.data); break;
        case 2: slot[2]->write(req.address, req.data); break;
        case 3: dcc_write(     req.address, req.data); break;
      }
    }
  }
}
