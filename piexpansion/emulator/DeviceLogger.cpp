#include <Arduino.h>
#include <cstdio>
#include "DeviceLogger.h"

DeviceLogger::DeviceLogger(DeviceBase *inner) {
  _inner = inner;
  _lastirq = _lastdma = false;
}

void DeviceLogger::setup(void) {
  _inner->setup();
  Serial.print("[DL init]");
  Serial.flush();
}

void DeviceLogger::loop(void) { _inner->loop(); }

uint8_t DeviceLogger::read(uint8_t address) {
  char msg[9];
  uint8_t data = _inner->read(address);
  snprintf(msg, 9, "[r%02X:%02X]", address, data);
  Serial.print(msg);
  Serial.flush();
  return data;
}

void DeviceLogger::write(uint8_t address, uint8_t data) {
  char msg[9];
  _inner->write(address, data);
  snprintf(msg, 9, "[w%02X:%02X]", address, data);
  Serial.print(msg);
  Serial.flush();
}

bool DeviceLogger::irq(void) {
  bool result = _inner->irq();
  if (result != _lastirq) {
    _lastirq = result;
    Serial.print(result ? "[I]" : "[i]");
    Serial.flush();
  }
  return result;
}

bool DeviceLogger::dma(void) {
  bool result = _inner->dma();
  if (result != _lastdma) {
    _lastdma = result;
    Serial.print(result ? "[D]" : "[d]");
    Serial.flush();
  }
  return result;
}
