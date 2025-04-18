#include <atomic>
#include <unistd.h>
#include <wiringPi.h>

#include "x86.h"
#include "drives.h"
#include "vcd.h"


#define PIN_CLK   29
#define PIN_RESET 27
#define PIN_ALE   26
#define PIN_IO_M  10
#define PIN_DTR   11
#define PIN_INTR  28
#define PIN_INTA  31
#define PIN_AD0   25
#define PIN_AD1   24
#define PIN_AD2   23
#define PIN_AD3   22
#define PIN_AD4   21
#define PIN_AD5   30
#define PIN_AD6   14
#define PIN_AD7   13
#define PIN_A8    12
#define PIN_A9    3
#define PIN_A10   2
#define PIN_A11   0
#define PIN_A12   7
#define PIN_A13   9
#define PIN_A14   8
#define PIN_A15   15
#define PIN_A16   16
#define PIN_A17   1
#define PIN_A18   4
#define PIN_A19   5

using namespace std;

static vcd::vcd_t vcdFile;
static vcd::var_t vcdClk;
static vcd::var_t vcdReset;
static vcd::var_t vcdAle;
static vcd::var_t vcdIom;
static vcd::var_t vcdDtr;
static vcd::var_t vcdIntr;
static vcd::var_t vcdInta;
static vcd::var_t vcdA;
static vcd::var_t vcdAD;

static uint32_t traceCount;

static uint8_t RAM[0x100000];
static uint8_t IO [ 0x10000];

static bool Running = false;

static uint8_t IRQ = 0;

void pi86Irq(uint8_t line) {
  IRQ |= 1 << line;
  digitalWrite(PIN_INTR, HIGH);
}

static void Dump_Init() {
  vcdFile.begin("trace.vcd", 1, vcd::timescale_ns);
  vcdFile.defs_begin();
  vcdFile.module_begin("v20");
  vcdFile.def(vcdClk,   "clk",   vcd::var_type_wire,  1);
  vcdFile.def(vcdReset, "reset", vcd::var_type_wire,  1);
  vcdFile.def(vcdAle,   "ale",   vcd::var_type_wire,  1);
  vcdFile.def(vcdIom,   "iom",   vcd::var_type_wire,  1);
  vcdFile.def(vcdDtr,   "dtr",   vcd::var_type_wire,  1);
  vcdFile.def(vcdIntr,  "intr",  vcd::var_type_wire,  1);
  vcdFile.def(vcdInta,  "inta",  vcd::var_type_wire,  1);
  vcdFile.def(vcdA,     "A",     vcd::var_type_wire, 20);
  vcdFile.def(vcdAD,    "AD",    vcd::var_type_wire,  8);
  vcdFile.module_end();
  vcdFile.defs_end();
}

static uint32_t Read_Address(void) {
  uint32_t Address = 0;
  Address |= digitalRead(PIN_AD0) << 0;
  Address |= digitalRead(PIN_AD1) << 1;
  Address |= digitalRead(PIN_AD2) << 2;
  Address |= digitalRead(PIN_AD3) << 3;
  Address |= digitalRead(PIN_AD4) << 4;
  Address |= digitalRead(PIN_AD5) << 5;
  Address |= digitalRead(PIN_AD6) << 6;
  Address |= digitalRead(PIN_AD7) << 7;
  Address |= digitalRead(PIN_A8)  << 8;
  Address |= digitalRead(PIN_A9)  << 9;
  Address |= digitalRead(PIN_A10) << 10;
  Address |= digitalRead(PIN_A11) << 11;
  Address |= digitalRead(PIN_A12) << 12;
  Address |= digitalRead(PIN_A13) << 13;
  Address |= digitalRead(PIN_A14) << 14;
  Address |= digitalRead(PIN_A15) << 15;
  Address |= digitalRead(PIN_A16) << 16;
  Address |= digitalRead(PIN_A17) << 17;
  Address |= digitalRead(PIN_A18) << 18;
  Address |= digitalRead(PIN_A19) << 19;
  return Address;
}

static void Dump_State() {
  const uint32_t address = Read_Address();
  vcdFile.set(vcdClk,   digitalRead(PIN_CLK));
  vcdFile.set(vcdAle,   digitalRead(PIN_ALE));
  vcdFile.set(vcdIom,   digitalRead(PIN_IO_M));
  vcdFile.set(vcdDtr,   digitalRead(PIN_DTR));
  vcdFile.set(vcdReset, digitalRead(PIN_RESET));
  vcdFile.set(vcdIntr,  digitalRead(PIN_INTR));
  vcdFile.set(vcdInta,  digitalRead(PIN_INTA));
  vcdFile.set(vcdA,     address);
  vcdFile.set(vcdAD,    address & 0xff);
  vcdFile.timeinc();
}

static uint8_t Read_Control_Bus(void) {

  //  inta iom  dtr
  //  1    0    0    Read Mem
  //  1    0    1    Write Mem
  //  1    1    0    Read IO
  //  1    1    1    Write IO
  //  0    1    0    Interrupt

  uint8_t Control_Bus = 0;
  Control_Bus |= digitalRead(PIN_DTR)  << 0;
  Control_Bus |= digitalRead(PIN_IO_M) << 1;
  Control_Bus |= digitalRead(PIN_INTA) << 2;
  return Control_Bus;
}

static void Data_Bus_Direction_8088_IN(void) {
  pinMode(PIN_AD0, INPUT);
  pinMode(PIN_AD1, INPUT);
  pinMode(PIN_AD2, INPUT);
  pinMode(PIN_AD3, INPUT);
  pinMode(PIN_AD4, INPUT);
  pinMode(PIN_AD5, INPUT);
  pinMode(PIN_AD6, INPUT);
  pinMode(PIN_AD7, INPUT);
}

static void Data_Bus_Direction_8088_OUT(void) {
  pinMode(PIN_AD0, OUTPUT);
  pinMode(PIN_AD1, OUTPUT);
  pinMode(PIN_AD2, OUTPUT);
  pinMode(PIN_AD3, OUTPUT);
  pinMode(PIN_AD4, OUTPUT);
  pinMode(PIN_AD5, OUTPUT);
  pinMode(PIN_AD6, OUTPUT);
  pinMode(PIN_AD7, OUTPUT);
}

// Writes Data to Data Port 0-7
void Write_To_Data_Port_0_7(uint8_t Byte) {
  digitalWrite(PIN_AD0, (Byte >> 0) & 1);
  digitalWrite(PIN_AD1, (Byte >> 1) & 1);
  digitalWrite(PIN_AD2, (Byte >> 2) & 1);
  digitalWrite(PIN_AD3, (Byte >> 3) & 1);
  digitalWrite(PIN_AD4, (Byte >> 4) & 1);
  digitalWrite(PIN_AD5, (Byte >> 5) & 1);
  digitalWrite(PIN_AD6, (Byte >> 6) & 1);
  digitalWrite(PIN_AD7, (Byte >> 7) & 1);
}

// Writes Data to Data Port 8-15 8086 only
static void Write_To_Data_Port_8_15(uint8_t Byte) {
  digitalWrite(PIN_A8,  (Byte >> 0) & 1);
  digitalWrite(PIN_A9,  (Byte >> 1) & 1);
  digitalWrite(PIN_A10, (Byte >> 2) & 1);
  digitalWrite(PIN_A11, (Byte >> 3) & 1);
  digitalWrite(PIN_A12, (Byte >> 4) & 1);
  digitalWrite(PIN_A13, (Byte >> 5) & 1);
  digitalWrite(PIN_A14, (Byte >> 6) & 1);
  digitalWrite(PIN_A15, (Byte >> 7) & 1);
}

// Reads Data to Data Port 0-7
static uint8_t Read_From_Data_Port_0_7(void) {
  uint8_t ret = 0;
  ret |= digitalRead(PIN_AD0) << 0;
  ret |= digitalRead(PIN_AD1) << 1;
  ret |= digitalRead(PIN_AD2) << 2;
  ret |= digitalRead(PIN_AD3) << 3;
  ret |= digitalRead(PIN_AD4) << 4;
  ret |= digitalRead(PIN_AD5) << 5;
  ret |= digitalRead(PIN_AD6) << 6;
  ret |= digitalRead(PIN_AD7) << 7;
  return ret;
}

// Reads Data to Data Port 8-15 8086 only
static uint8_t Read_From_Data_Port_8_15(void) {
  uint8_t ret = 0;
  ret |= digitalRead(PIN_A8)  << 0;
  ret |= digitalRead(PIN_A9)  << 1;
  ret |= digitalRead(PIN_A10) << 2;
  ret |= digitalRead(PIN_A11) << 3;
  ret |= digitalRead(PIN_A12) << 4;
  ret |= digitalRead(PIN_A13) << 5;
  ret |= digitalRead(PIN_A14) << 6;
  ret |= digitalRead(PIN_A15) << 7;
  return ret;
}

static void CLK() {

  static const uint32_t clocks = 12;
  for (uint32_t i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, HIGH);
  }

  if (traceCount) {
    Dump_State();
  }

  for (int i=0; i<clocks; ++i) {
    digitalWrite(PIN_CLK, LOW);
  }

  if (traceCount) {
    Dump_State();
    if (0 == --traceCount) {
      Running = false;
    }
  }
}

// Sets up Raspberry PI pins in the begining
static void Setup(void) {  
  Running = true;
  
  wiringPiSetup();

  pinMode(PIN_CLK,   OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_ALE,   INPUT);
  pinMode(PIN_IO_M,  INPUT);
  pinMode(PIN_DTR,   INPUT);

  pinMode(PIN_INTR,  OUTPUT);
  pinMode(PIN_INTA,  INPUT);
  digitalWrite(PIN_INTR, LOW);

  pinMode(PIN_AD0, INPUT);
  pinMode(PIN_AD1, INPUT);
  pinMode(PIN_AD2, INPUT);
  pinMode(PIN_AD3, INPUT);
  pinMode(PIN_AD4, INPUT);
  pinMode(PIN_AD5, INPUT);
  pinMode(PIN_AD6, INPUT);
  pinMode(PIN_AD7, INPUT);
  pinMode(PIN_A8,  INPUT);
  pinMode(PIN_A9,  INPUT);
  pinMode(PIN_A10, INPUT);
  pinMode(PIN_A11, INPUT);
  pinMode(PIN_A12, INPUT);
  pinMode(PIN_A13, INPUT);
  pinMode(PIN_A14, INPUT);
  pinMode(PIN_A15, INPUT);
  pinMode(PIN_A16, INPUT);
  pinMode(PIN_A17, INPUT);
  pinMode(PIN_A18, INPUT);
  pinMode(PIN_A19, INPUT);
  
  Dump_Init();
}

static void Bus_Cycle_Mem_Read_88(uint32_t Address) {
  Data_Bus_Direction_8088_OUT();
  const uint8_t data = pi86MemRead8(Address);
  Write_To_Data_Port_0_7(data);
  CLK();
  CLK();
  Data_Bus_Direction_8088_IN();
}

static void Bus_Cycle_Mem_Write_88(uint32_t Address) {
  const uint8_t data = Read_From_Data_Port_0_7();
  pi86MemWrite8(Address, data);
  CLK();
  CLK();
}

static void Bus_Cycle_Io_Read_88(uint32_t Address) {
  Data_Bus_Direction_8088_OUT();
  const uint8_t data = pi86IoRead8(Address);  
  Write_To_Data_Port_0_7(data);
  CLK();
  CLK();
  Data_Bus_Direction_8088_IN();
}

static void Bus_Cycle_Io_Write_88(uint32_t Address) {
  const uint8_t data = Read_From_Data_Port_0_7();
  pi86IoWrite8(Address, data);
  CLK();
  CLK();
}

static void Bus_Cycle_Interrupt_88(void) {

  //pi86Trace(512);

  // Waits for second INTA bus cycle 4 CLKS 8088
  CLK();
  CLK();
  CLK();
  CLK();

  const uint8_t irq = (IRQ & 1) ? 1 :
                      (IRQ & 2) ? 2 :
                                  0;

  const uint8_t vec = (IRQ & 1) ? 8 :
                      (IRQ & 2) ? 9 :
                                  0;

  // if there was a pending interrupt
  if (irq) {
    Data_Bus_Direction_8088_OUT();
    Write_To_Data_Port_0_7(vec);
    CLK();
    CLK();
    Data_Bus_Direction_8088_IN();

    // acknowlege pending interrupt
    IRQ &= ~irq;
  }

  // lower IRQ if there are no more pending interrupts
  if (!IRQ) {
    digitalWrite(PIN_INTR, 0);
  }
}

static void Bus_Cycle_88(void) {

    CLK();

    if (digitalRead(PIN_ALE) != 1) {
      return;
    }

    // latch address
    const uint32_t Address = Read_Address();
    CLK();

    // handle bus cycle type
    switch (Read_Control_Bus()) {
    case 0b100: // Read Mem
      Bus_Cycle_Mem_Read_88(Address);
      break;
    case 0b101: // Write Mem
      Bus_Cycle_Mem_Write_88(Address);
      break;
    case 0b110: // Read IO
      Bus_Cycle_Io_Read_88(Address);
      break;
    case 0b111: // Write IO
      Bus_Cycle_Io_Write_88(Address);
      break;
    case 0b010: // Interrupt
      Bus_Cycle_Interrupt_88();
      break;
    }
}

void pi86BusCycle(uint32_t cycles) {
  while (cycles--) {
    Bus_Cycle_88();
  }
}

void pi86MemWritePtr(uint32_t addr, const uint8_t *src, uint32_t size) {
  for (int i = 0; i < size; i++) {
    RAM[addr] = src[i];
    addr++;
  }
}

void pi86MemReadPtr(uint32_t addr, uint8_t *dst, uint32_t size) {
  for (int i = 0; i < size; ++i) {
    dst[i] = RAM[addr];
    addr++;
  }
}

void pi86MemWrite8(uint32_t addr, uint8_t data) {

  RAM[addr] = data;

  if (addr == 0xF8000) {
    // poll in case this is an int13 handler request
    drivesPollInt13();
  }
}

uint8_t pi86MemRead8(uint32_t addr) {
  return RAM[addr];
}

void pi86IoWrite8(uint32_t addr, uint8_t data) {
  IO[addr] = data;
}

uint8_t pi86IoRead8(uint32_t addr) {
  return IO[addr];
}

// Resest the x86
void pi86Reset(void) {
  digitalWrite(PIN_RESET, HIGH);
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  CLK();
  digitalWrite(PIN_RESET, LOW);
}

void pi86Start() {
  Setup();
  pi86Reset();
}

bool pi86LoadBios(const string &path) {

  FILE *fd = fopen(path.c_str(), "rb");
  if (!fd) {
    printf("unable to load '%s'\n", path.c_str());
    return false;
  }

  fseek(fd, 0, SEEK_END);
  const size_t biosSize = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  uint8_t *ptr = pi86MemPtr(0xF8000);
  fread(ptr, 1, biosSize, fd);

  fclose(fd);

  // Jump code to be written to 0xFFFFF, =JMP FAR 0xF000:0X0100
  static const uint8_t FFFF0[] = {
    0XEA, 0X00, 0X01, 0X00, 0XF8, 'E', 'M', ' ', '0',  '4',  '/',  '1',  '0',  '/', '2', '0'
  };

  pi86MemWritePtr(0xFFFF0, FFFF0, sizeof(FFFF0)); // Jump Code
  pi86MemWrite8(0xF80FF, 0xFF); // Make sure STOP byte is not zero 0x00 = Stop
  pi86MemWrite8(0xF8000, 0xFF); // Make sure int13 command port is 0xFF
  pi86MemWrite8(0xF80F0, 0x03); // Video mode
  pi86IoWrite8 (0X3DA,   0xFF);

  return true;
}

uint8_t *pi86MemPtr(uint32_t addr) {
  return &RAM[addr];
}

void pi86Stop(void) {
  Running = false;
}

bool pi86Running(void) {
  return Running;
}

void pi86Trace(uint32_t cycles) {
  traceCount = cycles;

  Dump_State();
}
