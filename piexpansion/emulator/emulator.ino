/**
 * I/O Device Emulator for RM Nimbus PC-186
 */

#include <ArduinoOTA.h>
#include <SDFS.h>
#include <SerialBT.h>
#include <hardware/rtc.h>

#include "DeviceBase.h"
#include "DeviceDsrtc.h"
#include "DeviceLogger.h"
#include "DeviceMulti.h"
#include "DevicePrinter.h"
#include "arduino_secrets.h"
#include "device.pio.h"

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#define TZ_POSIX "GMT0BST,M3.5.0/1,M10.5.0"
#define SD_CS_PIN 17
#define DEVICE_PIO pio0

// Board supports slots 0-3 but not 4 due to pin count
DeviceBase *slot[4];

void setup() {
  int i;
  Serial.begin();
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186");

  init_reset();
  init_pio();
  alarm_pool_init_default();
  init_wifi();
  init_time();
  init_ota();
  init_sd();

  // Card select assignments
  // slot[0] = new DeviceDcb();
  slot[1] = new DevicePrinter();
  slot[2] = new DeviceDsrtc();
  slot[3] = new DeviceMulti();

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

  for (i = 0; i < 4; i++) {
    if (slot[i]) {
      slot[i]->loop();
    }
  }

  // IRQ / DMA assignments
  if (slot[0] && slot[0]->dma()) {
    digitalWrite(nDMA, LOW);
  } else {
    digitalWrite(nDMA, HIGH);
  }
  if (slot[0] && slot[0]->irq()) {
    digitalWrite(nINT0, LOW);
  } else {
    digitalWrite(nINT0, HIGH);
  }
  if (slot[2] && slot[2]->irq()) {
    digitalWrite(nINT1, LOW);
  } else {
    digitalWrite(nINT1, HIGH);
  }
  if (slot[1] && slot[1]->irq()) {
    digitalWrite(nINT2, LOW);
  } else {
    digitalWrite(nINT2, HIGH);
  }

  // Check and respond to a pending request from the Nimbus
  if (
    (device_program_get(DEVICE_PIO, 0, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 1, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 2, &req) != nullptr) ||
    (device_program_get(DEVICE_PIO, 3, &req) != nullptr)
  ) {
    if (req.isread) {
      if (slot[req.cs]) {
        req.data = slot[req.cs]->read(req.address);
      }
      device_program_respond(DEVICE_PIO, req.cs, &req);
    } else {
      if (slot[req.cs]) {
        slot[req.cs]->write(req.address, req.data);
      }
    }
  }
}
