#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <assert.h>

#include "SDL.h"

#include "drives.h"
#include "vga.h"
#include "x86.h"
#include "keyboard.h"
#include "config.h"

// TODO: convert to using SDL surface not renderer

using namespace std;

static bool keyboard(void);
static void display(SDL_Surface *surf);


static bool clk_interval(uint32_t &clk, const uint32_t elapsed, const uint32_t reload) {
  if (elapsed >= clk) {
    assert(reload >= elapsed);
    clk += reload - elapsed;
    return true;
  }
  else {
    clk -= elapsed;
    return false;
  }
}

int main(int argc, char *argv[]) {

  SDL_Init(SDL_INIT_VIDEO);
  
  SDL_Surface *screen = SDL_SetVideoMode(720, 400, 32, 0);
  if (!screen) {
    return 1;
  }

  if (!pi86LoadBios("bios/pcxtbios.bin")) {
    return 1;
  }
  if (!pi86LoadRom("bios/disc/disc.bin", 0xf8000)) {
    return 1;
  }

  pi86Start();
  //pi86Trace(1024 * 32);

  // Drive images a: and C:
  if (!drivesStart("img/floppy.img", "img/hdd.img")) {
    return 1;
  }

  const uint32_t clk_freq   = 300000;  // 4.77Mhz
  const uint32_t cycles_vga = clk_freq / 25;
  const uint32_t cycles_pit = clk_freq / 18;

  uint32_t clk_display = 0;
  uint32_t clk_timer   = 0;

  while (pi86Running()) {

    const uint32_t cycles = min( clk_display, clk_timer );

    pi86BusCycle(cycles);

    while (keyboard());

    // 25hz
    if (clk_interval(clk_display, cycles, cycles_vga)) {
      display(screen);
    }

    // 18hz (pit channel 0) (~18.206313949678hz)
    if (clk_interval(clk_timer, cycles, cycles_pit)) {
      pi86Irq(0);
    }

    // Check for stop command
    if (pi86IoRead8(0xf0ff) == 0x00) {
      pi86Stop();
      break; // If stop = 0x00 then stop threads
    }
  }

  printf("%s shutdown\n", __func__);
  
  // Clean up
  SDL_Quit();

  return 0;
}

static void display(SDL_Surface *screen) {

  const uint8_t *mem = pi86MemPtr(0xB8000);
  const uint8_t *cur = pi86MemPtr(0x00450);

  const uint32_t mode = pi86MemRead8(0x00449);

  switch (mode) {
  case 0:
  case 1:
    Mode_0_40x25(screen, mem, cur);
    break;
  case 2:
  case 3:
    Mode_2_80x25(screen, mem, cur);
    break;
  }
}

static bool keyboard(void) {

  SDL_Event e;
  if (!SDL_PollEvent(&e)) {
    return false;
  }

  if (e.type == SDL_QUIT) {
    pi86Stop();
    return false;
  }

  if (e.type == SDL_KEYDOWN) {
    // place keycode in port 60h
    pi86IoWrite8(0x60, 0x00 | keyScanCode(e.key.keysym.sym));
    // let CPU know keycode is waiting
    pi86Irq(1);
  }

  if (e.type == SDL_KEYUP) {
    // place keycode in port 60h
    pi86IoWrite8(0x60, 0x80 | keyScanCode(e.key.keysym.sym));
    // let CPU know keycode is waiting
    pi86Irq(1);
  }
  
  return true;
}
