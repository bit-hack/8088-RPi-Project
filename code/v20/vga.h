#pragma once

#include "SDL.h"
#include "x86.h"

using namespace std;

void Mode_0_40x25(SDL_Surface *surf, const uint8_t *mem, const uint8_t *cur);
void Mode_2_80x25(SDL_Surface *surf, const uint8_t *mem, const uint8_t *cur);
