#include "vga.h"
#include "font.h"


void Print_Char_18x16(SDL_Surface *surf, int x, int y, uint8_t val) {

  uint32_t *dsta  = (uint32_t*)surf->pixels;
  uint32_t  pitch = surf->pitch / 4;

  dsta += y * pitch;
  dsta += x;

  const uint16_t *glyph = font_9x16[val];

  for (int j = 0; j < 16; j++) {

    uint32_t *dstb = dsta;
    const uint16_t  grow = glyph[j];

    for (int i = 0; i < 9; i++) {

      const uint16_t mask = 0x8000 >> i;
      const uint32_t rgb = (grow & mask) ? 0xf0f0f0 : 0x101010;

      dstb[i] = rgb;
    }

    dsta += pitch;
  }
}

void Print_Char_9x16(SDL_Surface *surf, int x, int y, uint8_t val) {

  uint32_t *dsta  = (uint32_t*)surf->pixels;
  uint32_t  pitch = surf->pitch / 4;

  dsta += y * pitch;
  dsta += x;

  const uint16_t *glyph = font_9x16[val];

  for (int j = 0; j < 16; j++) {

    uint32_t *dstb = dsta;
    const uint16_t  grow = glyph[j];

    for (int i = 0; i < 9; i++) {

      const uint16_t mask = 0x8000 >> i;
      const uint32_t rgb = (grow & mask) ? 0xf0f0f0 : 0x101010;

      dstb[i] = rgb;
    }

    dsta += pitch;
  }
}

void Mode_0_40x25(SDL_Surface *surf, const uint8_t *mem, const uint8_t *cur) {
  for (int j = 0; j < 25; j++) {
    for (int i = 0; i < 40; i++) {
      Print_Char_18x16(surf, (i * 18), (j * 16), *mem);
      mem += 2;
    }
  }
  Print_Char_18x16(surf, (cur[0] * 18), (cur[1] * 16), 0xDB);
  SDL_Flip(surf);
}

void Mode_2_80x25(SDL_Surface *surf, const uint8_t *mem, const uint8_t *cur) {
  for (int j = 0; j < 25; j++) {
    for (int i = 0; i < 80; i++) {
      Print_Char_9x16(surf, (i * 9), (j * 16), *mem);
      mem += 2;
    }
  }
  Print_Char_9x16(surf, (cur[0] * 9), (cur[1] * 16), 0xDB);
  SDL_Flip(surf);
}
