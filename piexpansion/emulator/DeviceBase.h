#ifndef _DeviceBase_h_
#define _DeviceBase_h_
#include <cstdint>

class DeviceBase {
  public:
    virtual void setup(void) {}
    virtual void loop(void) {}
    virtual uint8_t read(uint8_t address) = 0;
    virtual void write(uint8_t address, uint8_t data) = 0;
    virtual bool irq(void) { return false; }
    virtual bool dma(void) { return false; }
};

#endif
