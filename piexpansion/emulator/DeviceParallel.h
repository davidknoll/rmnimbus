#ifndef _DeviceParallel_h_
#define _DeviceParallel_h_
#include "DeviceBase.h"

class DeviceParallel : public DeviceBase {
  public:
    DeviceParallel(void);
    void setup(void);
    void loop(void);
    bool irq(void);
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);

  private:
    static int64_t _irqhsa(alarm_id_t id, void *user_data);
    static int64_t _irqhsb(alarm_id_t id, void *user_data);

    // 6522 VIA registers
    volatile uint8_t _orb;
    volatile uint8_t _irb;
    volatile uint8_t _ora;
    volatile uint8_t _ira;
    volatile uint8_t _ddrb;
    volatile uint8_t _ddra;
    volatile uint8_t _acr;
    volatile uint8_t _pcr;
    volatile uint8_t _ifr;
    volatile uint8_t _ier;

    WiFiServer _servera;
    WiFiServer _serverb;
    WiFiClient _clienta;
    WiFiClient _clientb;
    // uint64_t _last_ch_ts;
};

#endif
