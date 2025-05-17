#ifndef _DeviceMulti_h_
#define _DeviceMulti_h_
#include "DeviceBase.h"

class DeviceMulti : public DeviceBase {
  public:
    void setup(void);
    void loop(void);
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);

  private:
    File _charfile[4];
    WiFiServer _server;
    WiFiClient _client[4];

    File _diskfile[4];
    uint8_t _secbuf[65536];
    absolute_time_t _noptimeout;
    volatile uint8_t  _status;
    volatile uint16_t _secsz;
    volatile uint8_t  _unit;
    volatile uint8_t  _error;
    volatile uint8_t  _command;
    volatile uint32_t _ptr;
    volatile uint32_t _lba;
    volatile uint16_t _secct;

    uint8_t _dostime[6];
    uint8_t _dostimeptr;
    void _update_dostime(void);
    template <class Int> static constexpr
      Int _days_from_civil(Int y, unsigned m, unsigned d) noexcept;
};

#endif
