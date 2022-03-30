#pragma once

#include <cstdint>


enum gpioDir {
    GPIO_DIR_IN  = 0,
    GPIO_DIR_OUT = 1,
};

enum gpioLevel {
    GPIO_LEVEL_LOW   = 0,
    GPIO_LEVEL_HIGH  = 1,
};

enum gpioPull {
    GPIO_PULL_DOWN   = 0,
    GPIO_PULL_UP     = 1,
};

bool      gpioInit    ();
void      gpioLevelSet(uint32_t pin, int level);
gpioLevel gpioLevelGet(uint32_t pin);
void      gpioDirSet  (uint32_t pin, gpioDir   dir);
void      gpioPullSet (uint32_t pin, gpioPull  pull);
