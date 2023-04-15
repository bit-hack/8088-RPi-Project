#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string>

#include "SDL.h"
#include "drives.h"
#include "font.h"
#include "keycodes.h"
#include "vga.h"
#include "x86.h"


// TODO: convert to using SDL surface not renderer

using namespace std;

static bool keyboard(void);
static void display(SDL_Window *Window, SDL_Renderer *Renderer);

static bool clk_interval(uint32_t &clk, const uint32_t hz) {
  
  const uint32_t threshold = 1000 / hz;
  
  const uint32_t diff = SDL_GetTicks() - clk;
  
  if (clk == 0) {
    clk = diff;
    return false;
  }
  
  if (diff >= threshold) {
    clk += threshold;
    return true;
  }
  
  return false;
}

int main(int argc, char *argv[]) {

  SDL_Window   *window   = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Init(SDL_INIT_VIDEO);

  window = SDL_CreateWindow("x86",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            720,
                            400,
                            SDL_WINDOW_OPENGL
  );

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  pi86LoadBios("bios.bin");

  pi86Start(V20);

  // Drive images a: and C:
  Start_Drives("floppy.img", "hdd.img");

  uint32_t clk_keyboard = 0;
  uint32_t clk_display  = 0;
  uint32_t clk_timer    = 0;

  while (pi86Running()) {

    pi86BusCycle(20000);

    while (keyboard());
    
    // 25hz
    if (clk_interval(clk_display, /*hz=*/25)) {
      display(window, renderer);
    }

    // 18hz (pit channel 0) (~18.206313949678hz)
    if (clk_interval(clk_timer, /*hz=*/18)) {
      pi86Irq0();
    }

    // Check for stop command
    if (pi86MemRead8(0xF80FF) == 0X00) {
      pi86Stop();
      break; // If stop = 0x00 then stop threads
    }
  }

  printf("%s shutdown\n", __func__);

  // Close and destroy the window
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  
  // Clean up
  SDL_Quit();

  return 0;
}

static void display(SDL_Window *Window, SDL_Renderer *Renderer) {
  uint8_t Vblank[0x4000];
  uint8_t Video_Memory_40x25[2000];
  uint8_t Video_Memory_80x25[4000];
  uint8_t Video_Memory_320x200[0x4000];
  uint8_t Cursor_Position[2];

  if (pi86MemRead8(0x00449) == 0x00 |
      pi86MemRead8(0x00449) == 0x01) {
    pi86MemReadPtr(0xB8000, Video_Memory_40x25, 2000);
    pi86MemReadPtr(0x00450, Cursor_Position, 2);
    Mode_0_40x25(Renderer, Video_Memory_40x25, Cursor_Position);
  }

  if (pi86MemRead8(0x00449) == 0x02 |
      pi86MemRead8(0x00449) == 0x03) {
    pi86MemReadPtr(0xB8000, Video_Memory_80x25, 4000);
    pi86MemReadPtr(0x00450, Cursor_Position, 2);
    Mode_2_80x25(Renderer, Video_Memory_80x25, Cursor_Position);
  }

  if (pi86MemRead8(0x00449) == 0x04 &
      pi86MemRead8(0x00466) == 0x00) {
    pi86MemReadPtr(0xB8000, Video_Memory_320x200, 0x4000);
    Graphics_Mode_320_200_Palette_0(Renderer, Video_Memory_320x200);
  }

  if (pi86MemRead8(0x00449) == 0x04 &
      pi86MemRead8(0x00466) == 0x01) {
    pi86MemReadPtr(0xB8000, Video_Memory_320x200, 0x4000);
    Graphics_Mode_320_200_Palette_1(Renderer, Video_Memory_320x200);
  }
}

static void Insert_Key(char character_code, char scan_code) {

  char Key_Buffer_Tail = pi86MemRead8(
      0x041C); // Read the position of the keyboard buffer tail pointer
  pi86MemWrite8(0x400 + Key_Buffer_Tail,
                    character_code); // Write Character code at the keyboard
                                     // buffer tail pointer
  pi86MemWrite8(
      0x401 + Key_Buffer_Tail,
      scan_code); // Write scan code at the keyboard buffer tail pointer
  Key_Buffer_Tail =
      Key_Buffer_Tail + 2; // Add 2 to the keyboard buffer tail pointer
  if (Key_Buffer_Tail >=
      pi86MemRead8(0x0482)) // Check to see if the keyboard buffer tail
                                // pointer is at the end of the buffer
  {
    Key_Buffer_Tail = pi86MemRead8(0x0480);
  }
  pi86MemWrite8(
      0x041C, Key_Buffer_Tail); // Write the new keyboard buffer tail pointer
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
    switch (e.key.keysym.mod) {

    case 0x0000:    // KMOD_NONE
    case 0x1000:    //                                   KMOD_NUM
    case 0x0001:    // KMOD_LSHIFT
    case 0x0002:    //                       KMOD_RSHIFT
    case 0x1001:    // KMOD_LSHIFT                       KMOD_NUM
    case 0x1002:    //                       KMOD_RSHIFT KMOD_NUM
    case 0x0003:    //           KMOD_LSHIFT KMOD_RSHIFT
    case 0x1003:    //           KMOD_LSHIFT KMOD_RSHIFT KMOD_NUM
    case 0x2000:    // KMOD_CAPS
    case 0x2001:    // KMOD_CAPS KMOD_LSHIFT
    case 0x2002:    // KMOD_CAPS             KMOD_RSHIFT
    case 0x2003:    // KMOD_CAPS KMOD_LSHIFT KMOD_RSHIFT
    case 0x3000:    // KMOD_CAPS                         KMOD_NUM
    case 0x3001:    // KMOD_CAPS KMOD_LSHIFT             KMOD_NUM
    case 0x3002:    // KMOD_CAPS             KMOD_RSHIFT KMOD_NUM
    case 0x3003:    // KMOD_CAPS KMOD_LSHIFT KMOD_RSHIFT KMOD_NUM

      Insert_Key(character_codes_lowercase[e.key.keysym.scancode], scan_codes[e.key.keysym.scancode]);
      break;

    // KMOD_LCTRL
    case 0x0040:
      if (e.key.keysym.scancode == 0x2A) {
        pi86Reset();
      }
      Insert_Key(character_codes_ctrl[e.key.keysym.scancode],
                 scan_codes[e.key.keysym.scancode]);
      break;
    // KMOD_RCTRL
    case 0x0080:
      Insert_Key(character_codes_ctrl[e.key.keysym.scancode],
                 scan_codes[e.key.keysym.scancode]);
      break;
    // KMOD_LALT
    case 0x0100:
      if (e.key.keysym.scancode == 0x2A) {
        pi86MemWrite8(0x00449, 0x02);
      }
      Insert_Key(character_codes_ctrl[e.key.keysym.scancode],
                 scan_codes[e.key.keysym.scancode]);
      break;
    // KMOD_RALT
    case 0x0200:
      break;
    }

    pi86IoWrite8(0x0060, scan_codes[e.key.keysym.scancode]);
    pi86Irq1();
  }
  
  return true;
}

/*		X86 keyboard flag

                |7|6|5|4|3|2|1|0|  40:17  Keyboard Flags Byte 0
                 | | | | | | | `---- right shift key depressed
                 | | | | | | `----- left shift key depressed
                 | | | | | `------ CTRL key depressed
                 | | | | `------- ALT key depressed
                 | | | `-------- scroll-lock is active
                 | | `--------- num-lock is active
                 | `---------- caps-lock is active
                 `----------- insert is active

                |7|6|5|4|3|2|1|0|  40:18  Keyboard Flags Byte 1
                 | | | | | | | `---- left CTRL key depressed
                 | | | | | | `----- left ALT key depressed
                 | | | | | `------ system key depressed and held
                 | | | | `------- suspend key has been toggled
                 | | | `-------- scroll lock key is depressed
                 | | `--------- num-lock key is depressed
                 | `---------- caps-lock key is depressed
                 `----------- insert key is depressed */

/*
    KMOD_NONE     = 0x0000,
    KMOD_LSHIFT   = 0x0001,
    KMOD_RSHIFT   = 0x0002,
    KMOD_LCTRL    = 0x0040,
    KMOD_RCTRL    = 0x0080,
    KMOD_LALT     = 0x0100,
    KMOD_RALT     = 0x0200,
    KMOD_LGUI     = 0x0400,
    KMOD_RGUI     = 0x0800,
    KMOD_NUM      = 0x1000,
    KMOD_CAPS     = 0x2000,
    KMOD_MODE     = 0x4000,
    KMOD_RESERVED = 0x8000
*/
