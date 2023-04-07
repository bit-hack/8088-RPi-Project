////////////////////////////
// System Timer INT 0X08
///////////////////////////
#include "timer.h"

void System_Timer() {
  while (pi86Running()) {
    usleep(54926);
    pi86Irq0();
  }
  printf("%s shutdown\n", __func__);
}

void Start_System_Timer() {
  thread system_timer(System_Timer);
  system_timer.detach();
}
