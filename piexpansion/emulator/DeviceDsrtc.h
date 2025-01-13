#ifndef _DeviceDsrtc_h_
#define _DeviceDsrtc_h_
#include "DeviceBase.h"

class DeviceDsrtc : public DeviceBase {
  public:
    void setup(void);
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);
    bool irq(void);

  private:
    static const uint32_t _rates[16];
    static bool _rtcb(repeating_timer_t *rt);

    volatile uint8_t _ctla, _ctlb, _ctlc;
    datetime_t _t;
    repeating_timer_t _rt;
};

#endif
