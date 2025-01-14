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
#include "DeviceDcc.h"
#include "DeviceDsrtc.h"
#include "DeviceParallel.h"
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

void setup() {
  int i;
  Serial.begin();
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  init_reset();
  init_pio();
  alarm_pool_init_default();
  init_wifi();
  init_time();
  init_rtc();
  init_ota();
  init_sd();

  // Card select assignments
  // slot[0] = new DeviceDcb();
  slot[1] = new DeviceParallel();
  slot[2] = new DeviceDsrtc();
  slot[3] = new DeviceDcc();

  dcb_setup();
  for (i = 0; i < 4; i++) {
    if (slot[i]) {
      slot[i]->setup();
    }
  }

  Serial.println("Ready");
}

void loop() {
  int i;
  struct busreq req;
  ArduinoOTA.handle();
  dcb_loop();
  for (i = 0; i < 4; i++) {
    if (slot[i]) {
      slot[i]->loop();
    }
  }

  // IRQ / DMA assignments
  // digitalWrite(nDMA, (slot[0]->dma() || slot[3]->dma()) ? LOW : HIGH);
  // digitalWrite(nINT0, slot[0]->irq() ? LOW : HIGH);
  digitalWrite(nINT1, slot[2]->irq() ? LOW : HIGH);
  digitalWrite(nINT2, (slot[1]->irq() || slot[3]->irq()) ? LOW : HIGH);

  // Check and respond to a pending request from the Nimbus
  if (
    (device_program_get(DEVICE_PIO, 0, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 1, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 2, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 3, &req) != nullptr)
  ) {
    if (req.isread) {
      switch (req.cs) {
        case 0: req.data = dcb_read(req.address); break;
        default: if (slot[req.cs]) {
          req.data = slot[req.cs]->read(req.address);
        }
      }
      device_program_respond(DEVICE_PIO, req.cs, &req);
    } else {
      switch (req.cs) {
        case 0: dcb_write(req.address, req.data); break;
        default: if (slot[req.cs]) {
          slot[req.cs]->write(req.address, req.data);
        }
      }
    }
  }
}
