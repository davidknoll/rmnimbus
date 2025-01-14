#ifndef _DeviceDcc_h_
#define _DeviceDcc_h_
#include "DeviceBase.h"

class DeviceDcc : public DeviceBase {
  public:
    DeviceDcc(void);
    void setup(void);
    void loop(void);
    uint8_t read(uint8_t address);
    void write(uint8_t address, uint8_t data);
    bool irq(void);
    bool dma(void);

  private:
    volatile uint8_t _wr0a;
    volatile uint8_t _wr0b;
    volatile uint8_t _wr1a;
    volatile uint8_t _wr1b;
    volatile uint8_t _wr2;
    volatile uint8_t _wr3a;
    volatile uint8_t _wr5a;
    volatile uint8_t _wr9;
    volatile uint8_t _wr12a;
    volatile uint8_t _wr13a;
    volatile uint8_t _wr14a;
    volatile uint8_t _wr15a;
    volatile uint8_t _rr3;
    volatile int16_t _rr8a;
    volatile uint8_t _auxwr;
    volatile uint8_t _auxrd;
    volatile bool    _auxip;
    volatile bool    _dma_flag;

    WiFiServer _server;
    WiFiClient _client;
    CircularBuffer<uint8_t,4096> _txbuf;
    CircularBuffer<uint8_t,4096> _rxbuf;

    void _tx(uint8_t data);
    uint8_t _rx(void);
    void _setip(uint8_t mask);
    void _clrip(uint8_t mask);
};

#endif
