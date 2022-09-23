/**
 * Read status of IRQ lines
 */
uint irqtest_read(uint address) {
  switch (address) {
    case 0x00: return digitalRead(nINT0) ? 0x00 : 0xFF;
    case 0x02: return digitalRead(nINT1) ? 0x00 : 0xFF;
    case 0x04: return digitalRead(nINT2) ? 0x00 : 0xFF;
    default: return 0x00;
  }
}

/**
 * Write status of IRQ lines
 */
void irqtest_write(uint address, uint data) {
  switch (address) {
    case 0x00: digitalWrite(nINT0, data ? LOW : HIGH); break;
    case 0x02: digitalWrite(nINT1, data ? LOW : HIGH); break;
    case 0x04: digitalWrite(nINT2, data ? LOW : HIGH); break;
  }
}
