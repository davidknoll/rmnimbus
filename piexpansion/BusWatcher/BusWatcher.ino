#include <hardware/pio.h>
#include "buswatch.pio.h"

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#include "pico/cyw43_arch.h"
#include "boards/pico_w.h"
#define ledon() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1)
#define ledoff() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0)

void setup() {
  Serial.begin(115200);
  Serial.println("Booting I/O Device Emulator for RM Nimbus PC-186 (BusWatcher)");

  uint offset = pio_add_program(pio0, &buswatch_program);
  buswatch_program_init(pio0, 0, offset);

  Serial.println("Ready");
}

void loop() {
  struct busreq req;
  char reqstr[20];
  if (buswatch_program_get(pio0, 0, &req) == nullptr) { return; }
  sprintf(reqstr, "[%1d%c:%02X=%02X]", req.cs, req.isread ? 'R' : 'W', req.address, req.data);
  Serial.println(reqstr);
}
