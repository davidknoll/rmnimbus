#ifndef _DeviceLogger_h_
#define _DeviceLogger_h_
#include "DeviceBase.h"

class DeviceLogger : public DeviceBase {
  public:
    DeviceLogger(DeviceBase *inner);
    void setup(void);
    void loop(void);
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);
    bool irq(void);
    bool dma(void);

  private:
    DeviceBase *_inner;
    bool _lastirq;
    bool _lastdma;
};

#endif
