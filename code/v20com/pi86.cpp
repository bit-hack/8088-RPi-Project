#include <stdio.h>
#include <string.h>

#include "x86.h"

static uint8_t ram[0x100000];
static uint8_t io [ 0x10000];

static uint8_t memRead8(uint32_t addr) {
  const uint8_t data = ram[addr];
  printf("%6llu mem %05x r %02x\n",
    pi86CycleCount(), addr, data);
  return data;
}

static void memWrite8(uint32_t addr, uint8_t data) {
  printf("%6llu mem %05x w %02x\n",
    pi86CycleCount(), addr, data);
  ram[addr] = data;
}

static uint8_t ioRead8(uint32_t addr) {
  const uint8_t data = io[addr];
  printf("%6llu  io  %04x r %02x\n",
    pi86CycleCount(), addr, data);
  return data;
}

static void ioWrite8(uint32_t addr, uint8_t data) {
  io[addr] = data;
  printf("%6llu  io  %04x w %02x\n",
    pi86CycleCount(), addr, data);
}

static bool loadComFile(const char *path) {
  FILE *fd = fopen(path, "rb");
  if (!fd) {
    return false;
  }
  fread(ram + 0x100, 1024, 1, fd);
  fclose(fd);
  return true;
}

int main(int argc, char *args[]) {

  const uint8_t bootstrap[] = {
    0xbc, 0xfe, 0xff,             // mov sp, 0xfffe
    0xea, 0x00, 0x01, 0x00, 0x00, // jmp far 0x0:0x100
  };

  // populate memory
  memset(ram,           0x90,      sizeof(ram));
  memcpy(ram + 0xffff0, bootstrap, sizeof(bootstrap));

  // load a program
  if (argc >= 2) {
    if (!loadComFile(args[1])) {
      return 1;
    }
  }

  // setup memory handlers
  pi86ExtMemRead8   = memRead8;
  pi86ExtMemWrite8  = memWrite8;
  pi86ExtIoRead8    = ioRead8;
  pi86ExtIoWrite8   = ioWrite8;

  // run some bus cycles
  pi86Start();
  pi86BusCycle(128);
  pi86Stop();

  return 0;
}
