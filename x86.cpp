#include "x86.h"
#include "gpio.h"

static uint8_t RAM[0x100000];
static uint8_t IO[0x10000];
static bool    IRQ0_Flag;
static bool    IRQ1_Flag;
static int32_t total_cycles;

void IRQ0() {
  IRQ0_Flag = true;
  gpioLevelSet(PIN_INTR, GPIO_LEVEL_HIGH);
}

void IRQ1() {
  IRQ1_Flag = true;
  gpioLevelSet(PIN_INTR, GPIO_LEVEL_HIGH);
}

char Read_Interrupts() {
  char intr = IRQ0_Flag + (IRQ1_Flag << 1);
  return intr;
}

//////////////////////////////
// System Bus
///////////////////////////////

int32_t Read_Address() {
  int32_t Address = 0;
  Address |= gpioLevelGet(AD0) << 0;
  Address |= gpioLevelGet(AD1) << 1;
  Address |= gpioLevelGet(AD2) << 2;
  Address |= gpioLevelGet(AD3) << 3;
  Address |= gpioLevelGet(AD4) << 4;
  Address |= gpioLevelGet(AD5) << 5;
  Address |= gpioLevelGet(AD6) << 6;
  Address |= gpioLevelGet(AD7) << 7;
  Address |= gpioLevelGet(A8)  << 8;
  Address |= gpioLevelGet(A9)  << 9;
  Address |= gpioLevelGet(A10) << 10;
  Address |= gpioLevelGet(A11) << 11;
  Address |= gpioLevelGet(A12) << 12;
  Address |= gpioLevelGet(A13) << 13;
  Address |= gpioLevelGet(A14) << 14;
  Address |= gpioLevelGet(A15) << 15;
  Address |= gpioLevelGet(A16) << 16;
  Address |= gpioLevelGet(A17) << 17;
  Address |= gpioLevelGet(A18) << 18;
  Address |= gpioLevelGet(A19) << 19;
  return Address;
}

static char Read_Control_Bus() {

  // /SSO pin details:
  //
  // /SSO   IO/M    DT/R
  // 0      0       0                 Code Access
  // 1      0       0                 Read Memory
  // 0      0       1                 Write Memory
  // 1      0       1                 Passive
  // 0      1       0                 Int Ack
  // 1      1       0                 Read I/O Port
  // 0      1       1                 Write I/O port
  // 1      1       1                 Halt

  // function returns:
  //
  // INTA   IO/M    DT/R
  // 0      1       0      = 2    Interrupt
  // 1      0       0      = 4    Read Mem
  // 1      0       1      = 5    Write Mem
  // 1      1       0      = 6    Read IO
  // 1      1       1      = 7    Write IO

  char Control_Bus = 0;
  Control_Bus |= gpioLevelGet(PIN_DTR)  << 0;
  Control_Bus |= gpioLevelGet(PIN_IO_M) << 1;
  Control_Bus |= gpioLevelGet(PIN_INTA) << 2;
  return Control_Bus;
}

// Reads the selected Memory Bank, High/Low 8086 only
char Read_Memory_Bank() {
  char Memory_Bank = 0;
  Memory_Bank |= gpioLevelGet(AD0)     << 0;
  Memory_Bank |= gpioLevelGet(PIN_BHE) << 1;
  return Memory_Bank;
}

// Sets the Data Port direction for read and writes
void Data_Bus_Direction_8088_IN() {
  gpioDirSet(AD0, GPIO_DIR_IN);
  gpioDirSet(AD1, GPIO_DIR_IN);
  gpioDirSet(AD2, GPIO_DIR_IN);
  gpioDirSet(AD3, GPIO_DIR_IN);
  gpioDirSet(AD4, GPIO_DIR_IN);
  gpioDirSet(AD5, GPIO_DIR_IN);
  gpioDirSet(AD6, GPIO_DIR_IN);
  gpioDirSet(AD7, GPIO_DIR_IN);
}

void Data_Bus_Direction_8088_OUT() {
  gpioDirSet(AD0, GPIO_DIR_OUT);
  gpioDirSet(AD1, GPIO_DIR_OUT);
  gpioDirSet(AD2, GPIO_DIR_OUT);
  gpioDirSet(AD3, GPIO_DIR_OUT);
  gpioDirSet(AD4, GPIO_DIR_OUT);
  gpioDirSet(AD5, GPIO_DIR_OUT);
  gpioDirSet(AD6, GPIO_DIR_OUT);
  gpioDirSet(AD7, GPIO_DIR_OUT);
}

void Data_Bus_Direction_8086_IN() {
  gpioDirSet(AD0, GPIO_DIR_IN);
  gpioDirSet(AD1, GPIO_DIR_IN);
  gpioDirSet(AD2, GPIO_DIR_IN);
  gpioDirSet(AD3, GPIO_DIR_IN);
  gpioDirSet(AD4, GPIO_DIR_IN);
  gpioDirSet(AD5, GPIO_DIR_IN);
  gpioDirSet(AD6, GPIO_DIR_IN);
  gpioDirSet(AD7, GPIO_DIR_IN);

  gpioDirSet(A8,  GPIO_DIR_IN);
  gpioDirSet(A9,  GPIO_DIR_IN);
  gpioDirSet(A10, GPIO_DIR_IN);
  gpioDirSet(A11, GPIO_DIR_IN);
  gpioDirSet(A12, GPIO_DIR_IN);
  gpioDirSet(A13, GPIO_DIR_IN);
  gpioDirSet(A14, GPIO_DIR_IN);
  gpioDirSet(A15, GPIO_DIR_IN);
}

void Data_Bus_Direction_8086_OUT() {
  gpioDirSet(AD0, GPIO_DIR_OUT);
  gpioDirSet(AD1, GPIO_DIR_OUT);
  gpioDirSet(AD2, GPIO_DIR_OUT);
  gpioDirSet(AD3, GPIO_DIR_OUT);
  gpioDirSet(AD4, GPIO_DIR_OUT);
  gpioDirSet(AD5, GPIO_DIR_OUT);
  gpioDirSet(AD6, GPIO_DIR_OUT);
  gpioDirSet(AD7, GPIO_DIR_OUT);

  gpioDirSet(A8,  GPIO_DIR_OUT);
  gpioDirSet(A9,  GPIO_DIR_OUT);
  gpioDirSet(A10, GPIO_DIR_OUT);
  gpioDirSet(A11, GPIO_DIR_OUT);
  gpioDirSet(A12, GPIO_DIR_OUT);
  gpioDirSet(A13, GPIO_DIR_OUT);
  gpioDirSet(A14, GPIO_DIR_OUT);
  gpioDirSet(A15, GPIO_DIR_OUT);
}

// Writes Data to Data Port 0-7
void Write_To_Data_Port_0_7(char Byte) {
  gpioLevelSet(AD0, Byte & 0x01);
  gpioLevelSet(AD1, Byte & 0x02);
  gpioLevelSet(AD2, Byte & 0x04);
  gpioLevelSet(AD3, Byte & 0x08);
  gpioLevelSet(AD4, Byte & 0x10);
  gpioLevelSet(AD5, Byte & 0x20);
  gpioLevelSet(AD6, Byte & 0x40);
  gpioLevelSet(AD7, Byte & 0x80);
}

// Writes Data to Data Port 8-15 8086 only
void Write_To_Data_Port_8_15(char Byte) {
  gpioLevelSet(A8,  Byte & 0x01);
  gpioLevelSet(A9,  Byte & 0x02);
  gpioLevelSet(A10, Byte & 0x04);
  gpioLevelSet(A11, Byte & 0x08);
  gpioLevelSet(A12, Byte & 0x10);
  gpioLevelSet(A13, Byte & 0x20);
  gpioLevelSet(A14, Byte & 0x40);
  gpioLevelSet(A15, Byte & 0x80);
}

// Reads Data to Data Port 0-7
char Read_From_Data_Port_0_7() {
  char ret = 0;
  ret |= gpioLevelGet(AD0) << 0;
  ret |= gpioLevelGet(AD1) << 1;
  ret |= gpioLevelGet(AD2) << 2;
  ret |= gpioLevelGet(AD3) << 3;
  ret |= gpioLevelGet(AD4) << 4;
  ret |= gpioLevelGet(AD5) << 5;
  ret |= gpioLevelGet(AD6) << 6;
  ret |= gpioLevelGet(AD7) << 7;
  return ret;
}

// Reads Data to Data Port 8-15 8086 only
char Read_From_Data_Port_8_15() {
  char ret = 0;
  ret |= gpioLevelGet(A8)  << 0;
  ret |= gpioLevelGet(A9)  << 1;
  ret |= gpioLevelGet(A10) << 2;
  ret |= gpioLevelGet(A11) << 3;
  ret |= gpioLevelGet(A12) << 4;
  ret |= gpioLevelGet(A13) << 5;
  ret |= gpioLevelGet(A14) << 6;
  ret |= gpioLevelGet(A15) << 7;
  return ret;
}

// Clicks the CLK pin
static void CLK(uint32_t cycles=1) {
  total_cycles -= cycles;

  static const uint32_t delay = 11;

  while (cycles--) {
    for (uint32_t i=0; i<delay; ++i) {
      gpioLevelSet(PIN_CLK, GPIO_LEVEL_HIGH);
    }
    for (uint32_t i=0; i<delay; ++i) {
      gpioLevelSet(PIN_CLK, GPIO_LEVEL_LOW);
    }
  }
}

// Sets up Raspberry PI pins in the begining
static bool Setup() {
  Stop_Flag = false;
  if (!gpioInit()) {
    return false;
  }

  gpioDirSet(PIN_CLK,   GPIO_DIR_OUT);
  gpioDirSet(PIN_RESET, GPIO_DIR_OUT);
  gpioDirSet(PIN_ALE,   GPIO_DIR_IN);
  gpioDirSet(PIN_IO_M,  GPIO_DIR_IN);
  gpioDirSet(PIN_DTR,   GPIO_DIR_IN);
  gpioDirSet(PIN_BHE,   GPIO_DIR_IN);
  gpioDirSet(PIN_INTR,  GPIO_DIR_OUT);
  gpioDirSet(PIN_INTA,  GPIO_DIR_IN);

  gpioLevelSet(PIN_INTR, GPIO_LEVEL_LOW);

  gpioDirSet(AD0, GPIO_DIR_IN);
  gpioDirSet(AD1, GPIO_DIR_IN);
  gpioDirSet(AD2, GPIO_DIR_IN);
  gpioDirSet(AD3, GPIO_DIR_IN);
  gpioDirSet(AD4, GPIO_DIR_IN);
  gpioDirSet(AD5, GPIO_DIR_IN);
  gpioDirSet(AD6, GPIO_DIR_IN);
  gpioDirSet(AD7, GPIO_DIR_IN);

  gpioDirSet(A8,  GPIO_DIR_IN);
  gpioDirSet(A9,  GPIO_DIR_IN);
  gpioDirSet(A10, GPIO_DIR_IN);
  gpioDirSet(A11, GPIO_DIR_IN);
  gpioDirSet(A12, GPIO_DIR_IN);
  gpioDirSet(A13, GPIO_DIR_IN);
  gpioDirSet(A14, GPIO_DIR_IN);
  gpioDirSet(A15, GPIO_DIR_IN);

  gpioDirSet(A16, GPIO_DIR_IN);
  gpioDirSet(A17, GPIO_DIR_IN);
  gpioDirSet(A18, GPIO_DIR_IN);
  gpioDirSet(A19, GPIO_DIR_IN);

  return true;
}

static void CPU_Clock_V20() {
  while (Stop_Flag != true && total_cycles > 0) {
    CLK();  // -> T1
    // wait for ALE to be asserted
    if (gpioLevelGet(PIN_ALE) != 1) {
      continue;
    }
    const int32_t Address = Read_Address();
    CLK();  // -> T2
    switch (Read_Control_Bus()) {
    // Read Mem
    case 0x04:
      Data_Bus_Direction_8088_OUT();
      Write_To_Data_Port_0_7(RAM[Address]);
      CLK();  // -> T3
      CLK();  // -> T4
      Data_Bus_Direction_8088_IN();
      break;

    // Write Mem
    case 0x05:
      RAM[Address] = Read_From_Data_Port_0_7();
      CLK();  // -> T3
      CLK();  // -> T4
      break;

    // Read IO
    case 0x06:
      Data_Bus_Direction_8088_OUT();
      Write_To_Data_Port_0_7(IO[Address]);
      CLK();  // -> T3
      CLK();  // -> T4
      Data_Bus_Direction_8088_IN();
      break;

    // Write IO
    case 0x07:
      IO[Address] = Read_From_Data_Port_0_7();
      //printf("Write IO %#X, ", Address);
      //printf("Data %#X \n", Read_From_Data_Port_0_7());
      CLK();  // -> T3
      CLK();  // -> T4
      break;

    // Interrupt
    case 0x02:
      // Waits for second INTA bus cycle 4 CLKS 8088
      CLK(4);
      switch (Read_Interrupts()) {
      case 0x01:
        Data_Bus_Direction_8088_OUT();
        Write_To_Data_Port_0_7(0x08);
        CLK(2);
        Data_Bus_Direction_8088_IN();
        IRQ0_Flag = false;
        gpioLevelSet(PIN_INTR, GPIO_LEVEL_LOW);
        break;
      case 0x02:
        Data_Bus_Direction_8088_OUT();
        Write_To_Data_Port_0_7(0x09);
        CLK(2);
        Data_Bus_Direction_8088_IN();
        IRQ1_Flag = false;
        gpioLevelSet(PIN_INTR, GPIO_LEVEL_LOW);
        break;
      case 0x03:
        Data_Bus_Direction_8088_OUT();
        Write_To_Data_Port_0_7(0x08);
        CLK(2);
        Data_Bus_Direction_8088_IN();
        IRQ0_Flag = false;
        break;
      default:
        printf("Default Interrupt %#X \n", Read_Interrupts());
        break;
      }
      break;
    default:
      printf("Default \n");
      break;
    }
  }
}

static void CPU_Clock_V30() {
  int32_t Address;
  char Memory_IO_Bank;
  while (Stop_Flag != true && total_cycles > 0) {
    CLK();
    if (gpioLevelGet(PIN_ALE) == 1) {
      Address = Read_Address();
      Memory_IO_Bank = Read_Memory_Bank();
      CLK();
      switch (Read_Control_Bus() + (Memory_IO_Bank << 4)) {
        // Write Mem
      case 0x07:
        RAM[Address] = Read_From_Data_Port_0_7();
        RAM[Address + 1] = Read_From_Data_Port_8_15();
        CLK(2);
        break;
      case 0x17:
        RAM[Address] = Read_From_Data_Port_8_15();
        CLK(2);
        break;
      case 0x27:
        RAM[Address] = Read_From_Data_Port_0_7();
        CLK(2);
        break;
        // Read Mem
      case 0x06:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7(RAM[Address]);
        Write_To_Data_Port_8_15(RAM[Address + 1]);
        CLK(2);
        Data_Bus_Direction_8086_IN();
        break;
      case 0x16:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_8_15(RAM[Address]);
        CLK(2);
        Data_Bus_Direction_8086_IN();
        break;
      case 0x26:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7(RAM[Address]);
        CLK(2);
        Data_Bus_Direction_8086_IN();
        break;
        // Write IO
      case 0x05:
        IO[Address] = Read_From_Data_Port_0_7();
        IO[Address + 1] = Read_From_Data_Port_8_15();
        CLK(2);
        break;
      case 0x15:
        IO[Address] = Read_From_Data_Port_8_15();
        CLK(2);
        break;
      case 0x25:
        IO[Address] = Read_From_Data_Port_0_7();
        CLK(2);
        break;
        // Read IO
      case 0x04:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7(IO[Address]);
        Write_To_Data_Port_8_15(IO[Address + 1]);
        CLK(2);
        Data_Bus_Direction_8086_IN();
        break;
      case 0x14:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_8_15(IO[Address]);
        CLK(2);
        Data_Bus_Direction_8086_IN();
        break;
      case 0x24:
        Data_Bus_Direction_8086_OUT();
        Write_To_Data_Port_0_7(IO[Address]);
        CLK(2);
        Data_Bus_Direction_8086_IN();
        break;
        // Interrupt
      case 0x00:
        // Waits for second INTA bus cycle 7 CLKS 8086
        CLK(7);
        switch (Read_Interrupts()) {
          // System Timer
        case 0x01:
          Data_Bus_Direction_8086_OUT();
          Write_To_Data_Port_0_7(0x08);
          CLK(2);
          Data_Bus_Direction_8086_IN();
          IRQ0_Flag = false;
          gpioLevelSet(PIN_INTR, GPIO_LEVEL_LOW);
          break;
          // Keyboard
        case 0x02:
          Data_Bus_Direction_8086_OUT();
          Write_To_Data_Port_0_7(0x09);
          CLK(2);
          Data_Bus_Direction_8086_IN();
          IRQ1_Flag = false;
          gpioLevelSet(PIN_INTR, GPIO_LEVEL_LOW);
          break;
          // System Timer and Keyboard, System Timer is handled
        case 0x03:
          Data_Bus_Direction_8086_OUT();
          Write_To_Data_Port_0_7(0x08);
          CLK(2);
          Data_Bus_Direction_8086_IN();
          IRQ0_Flag = false;
          break;
        default:
          printf("Default Interrupt %#X \n", Read_Interrupts());
          break;
        }
        break;
      default:
        printf("Default \n");
        break;
      }
    }
  }
}

// System Bus decoder
void CPU_Clock(int32_t type, int32_t cycles) {

#define PROFILE 1

#if PROFILE
  const auto start = clock();
#endif // PROFILE

  total_cycles += cycles;
  if (type == CPU_TYPE_V20) {
    CPU_Clock_V20();
  }
  if (type == CPU_TYPE_V30) {
    CPU_Clock_V30();
  }

#if PROFILE
  const auto end = clock();
  const double secs = double(end - start) / double(CLOCKS_PER_SEC);
  printf("%f SEC\n", secs);

  const double ips = double(cycles) / double(secs);
  printf("%f IPS\n", ips);

#endif // PROFILE
}

void Write_Memory_Array(uint64_t Address, char code_for_8088[],
  int32_t Length) {
  for (int32_t i = 0; i < Length; i++) {
    RAM[Address] = code_for_8088[i];
    Address++;
  }
}

void Read_Memory_Array(uint64_t Address, char *char_Array,
  int32_t Length) {
  for (int32_t i = 0; i < Length; ++i) {
    char_Array[i] = RAM[Address];
    Address++;
  }
}

void Write_Memory_Byte(uint64_t Address, char byte_for_8088) {
  RAM[Address] = byte_for_8088;
}

char Read_Memory_Byte(uint64_t Address) {
  return RAM[Address];
}

void Write_Memory_Word(uint64_t Address,
  uint16_t word_for_8088) {
  RAM[Address] = word_for_8088;
  RAM[Address + 1] = word_for_8088 >> 8;
}

void Write_IO_Byte(uint64_t Address, char byte_for_8088) {
  IO[Address] = byte_for_8088;
}

char Read_IO_Byte(uint64_t Address) {
  return IO[Address];
}

void Write_IO_Word(uint64_t Address,
  uint16_t word_for_8088) {
  IO[Address] = word_for_8088;
  IO[Address + 1] = word_for_8088 >> 8;
}

// Resest the x86
void CPU_Reset() {
  gpioLevelSet(PIN_RESET, GPIO_LEVEL_HIGH);
  CLK(8);
  gpioLevelSet(PIN_RESET, GPIO_LEVEL_LOW);
}

bool CPU_Start() {
  // Sets up Ports
  if (!Setup()) {
    return false;
  }
  // Resets the x86
  CPU_Reset();
  return true;
}

void CPU_Load_Bios(std::string Bios_file) {
  std::ifstream MemoryFile;               // New ifstream
  MemoryFile.open(Bios_file);             // Open Rom.bin
  MemoryFile.seekg(0, std::ios::end);     // Find the end of the file
  int32_t FileSize = MemoryFile.tellg();  // Get the size of the file
  MemoryFile.seekg(0, MemoryFile.beg);    // Start reading at the begining
  char Rom[FileSize];                     // New char array the size of the rom file
  MemoryFile.read(Rom, FileSize);         // Read the file
  MemoryFile.close();                     // Close the file

  // Jump code to be written to 0xFFFFF, =JMP FAR 0xF000:0X0100
  char FFFF0[] = { 0XEA, 0X00, 0X01, 0X00, 0XF8, 'E', 'M', ' ',
                  '0',  '4',  '/',  '1',  '0',  '/', '2', '0' };
  Write_Memory_Array(0xFFFF0, FFFF0, sizeof(FFFF0)); // Jump Code
  Write_Memory_Array(0xF8000, Rom, sizeof(Rom));     // The Rom file
  Write_Memory_Byte(0xF80FF, 0xFF); // Make sure STOP byte is not zero 0x00 = Stop
  Write_Memory_Byte(0xF8000, 0xFF); // Make sure int13 command port is 0xFF
  Write_Memory_Byte(0xF80F0, 0x03); // Video mode

  // Video port something...?? makes it work
  Write_IO_Byte(0X3DA, 0xFF);
}
