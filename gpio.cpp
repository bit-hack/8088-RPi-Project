#include <wiringPi.h>

#include "gpio.h"


bool gpio_init() {
    wiringPiSetup();
    return true;
}

void gpioLevelSet(uint32_t pin, int level) {
    digitalWrite(pin, (level != 0) ? HIGH : LOW);
}

gpioLevel gpioLevelGet(uint32_t pin) {
    return digitalRead(pin) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

void gpioDirSet(uint32_t pin, gpioDir dir) {
    pinMode(pin, dir == GPIO_DIR_IN ? INPUT : OUTPUT);
}

void gpioPullSet(uint32_t pin, gpioPull pull) {
    // todo
}
