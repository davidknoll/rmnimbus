#ifndef _DeviceIrqtest_h_
#define _DeviceIrqtest_h_
#include "DeviceBase.h"

class DeviceIrqtest : public DeviceBase {
  public:
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);
};

#endif
