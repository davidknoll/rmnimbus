#ifndef _DevicePrinter_h_
#define _DevicePrinter_h_
#include "DeviceBase.h"

class DevicePrinter : public DeviceBase {
  public:
    void setup(void);
    void loop(void);
    bool irq(void);
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);

  private:
    static int64_t _ack(alarm_id_t id, void *user_data);
    static bool _unstick(repeating_timer_t *rt);
    void _outch(uint8_t ch);
    repeating_timer_t _unstick_timer;
    uint8_t _ora, _ddra, _ier;
    volatile uint8_t _ifr;
};

#endif
