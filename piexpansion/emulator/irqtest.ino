#include "DeviceIrqtest.h"

/**
 * Read status of IRQ lines
 */
uint8_t DeviceIrqtest::read(uint8_t address) {
  switch (address) {
    case 0x00: return digitalRead(nINT0) ? 0x00 : 0xFF;
    case 0x02: return digitalRead(nINT1) ? 0x00 : 0xFF;
    case 0x04: return digitalRead(nINT2) ? 0x00 : 0xFF;
    case 0x06: return digitalRead(nDMA)  ? 0x00 : 0xFF;
    default: return 0x00;
  }
}

/**
 * Write status of IRQ lines
 */
void DeviceIrqtest::write(uint8_t address, uint8_t data) {
  switch (address) {
    case 0x00: digitalWrite(nINT0, data ? LOW : HIGH); break;
    case 0x02: digitalWrite(nINT1, data ? LOW : HIGH); break;
    case 0x04: digitalWrite(nINT2, data ? LOW : HIGH); break;
    case 0x06: digitalWrite(nDMA,  data ? LOW : HIGH); break;
  }
}
