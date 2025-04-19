#include <string>
#include <cstdio>
#include <unistd.h>
#include <string.h>

#include "drives.h"
#include "x86.h"
#include "config.h"


//////////////////
// Disk Handler
///////////////////

static FILE *driveA;
static FILE *driveC;

static bool Get_Disk_Parameters_A() {

  char Floppy[0x200]; // Char array to hold the data only reading first sector

  fseek(driveA, 0, SEEK_SET);
  fread(Floppy, 1, sizeof(Floppy), driveA);
  
  char Media_Descriptor[0x10] = {0x04, 0,    0, 0, 0, 0,    0, 0,
                                 0xF8, 0x02, 0, 0, 0, 0x01, 0, 0};
  // Start at port 11
#if NEW_BIOS
  pi86IoWrite8(0xF00B, Media_Descriptor[Floppy[0x15] & 0x0F]); // Media Descriptor - BL
  pi86IoWrite8(0xF00C, Floppy[0x1A]); // Low byte heads per cylinder - DH -1
  pi86IoWrite8(0xF00D, Floppy[0x1B]); // High byte heads per cylinder
  pi86IoWrite8(0xF00E, Floppy[0x0B]); // Low bytes per sector - uS
  pi86IoWrite8(0xF00F, Floppy[0x0C]); // High bytes per sector
#else
  pi86MemWrite8(0xF800B, Media_Descriptor[Floppy[0x15] & 0x0F]); // Media Descriptor - BL
  pi86MemWrite8(0xF800C, Floppy[0x1A]); // Low byte heads per cylinder - DH -1
  pi86MemWrite8(0xF800D, Floppy[0x1B]); // High byte heads per cylinder
  pi86MemWrite8(0xF800E, Floppy[0x0B]); // Low bytes per sector - uS
  pi86MemWrite8(0xF800F, Floppy[0x0C]); // High bytes per sector
#endif

  int Head_Per_Cylinder = (Floppy[0x1B] << 8) + Floppy[0x1A];
  int Sector_Per_Track = (Floppy[0x19] << 8) + Floppy[0x18];
  int Number_Of_Sectors = (Floppy[0x14] << 8) + Floppy[0x13];

  int Number_Of_Cylinders =
      Number_Of_Sectors / Sector_Per_Track / Head_Per_Cylinder;
#if NEW_BIOS
  pi86IoWrite8(0xF011, Number_Of_Cylinders); // CH
#else
  pi86MemWrite8(0xF8011, Number_Of_Cylinders); // CH
#endif

  Number_Of_Cylinders = (Number_Of_Cylinders >> 2) & 0xC0;
  Sector_Per_Track = Sector_Per_Track & 0X3F; // CL
#if NEW_BIOS
  pi86IoWrite8(0xF010, Number_Of_Cylinders + Sector_Per_Track);
  pi86IoWrite8(0xF012, 0x00); // Always zero for floppy
  pi86IoWrite8(0xF013, 0x00); // Always zero for floppy
  pi86IoWrite8(0xF014, 0x00); // Always zero for floppy
  pi86IoWrite8(0xF015, 0x00); // Always zero for floppy
  pi86IoWrite8(0xF016, 0X01); // Drive type
#else
  pi86MemWrite8(0xF8010, Number_Of_Cylinders + Sector_Per_Track);
  pi86MemWrite8(0xF8012, 0x00); // Always zero for floppy
  pi86MemWrite8(0xF8013, 0x00); // Always zero for floppy
  pi86MemWrite8(0xF8014, 0x00); // Always zero for floppy
  pi86MemWrite8(0xF8015, 0x00); // Always zero for floppy
  pi86MemWrite8(0xF8016, 0X01); // Drive type
#endif
  
  return true;
}

static bool Get_Disk_Parameters_C() {

  fseek(driveC, 0, SEEK_SET);

  uint8_t drive[0x200] = { 0 };
  fread(drive, 1, sizeof(drive), driveC);

  int Hidden_Sectors = 0;

  // Locates boot partition
  if      (drive[0x1BE] == 0x80) { Hidden_Sectors = drive[0x1C6] | (drive[0x1C7] << 8) | (drive[0x1C8] << 16) | (drive[0x1C9] << 24); }
  else if (drive[0x1CE] == 0x80) { Hidden_Sectors = drive[0x1D6] | (drive[0x1D7] << 8) | (drive[0x1D8] << 16) | (drive[0x1D9] << 24); }
  else if (drive[0x1DE] == 0x80) { Hidden_Sectors = drive[0x1E6] | (drive[0x1E7] << 8) | (drive[0x1E8] << 16) | (drive[0x1E9] << 24); }
  else if (drive[0x1EE] == 0x80) { Hidden_Sectors = drive[0x1F6] | (drive[0x1F7] << 8) | (drive[0x1F8] << 16) | (drive[0x1F9] << 24); }
  else {
    // No boot partion MBR
    pi86Stop();
    printf("No boot partion in MBR C: \n");
    return false;
  }

  // Read volume boot record
  fseek(driveC, 0x200 * Hidden_Sectors, SEEK_SET);
  fread(drive, 1, sizeof(drive), driveC);

#if NEW_BIOS
  pi86IoWrite8(0xF00C, drive[0x1A]); // Low byte heads per cylinder - DH-1
  pi86IoWrite8(0xF00D, drive[0x1B]); // High byte heads per cylinder
  pi86IoWrite8(0xF00E, drive[0x0B]); // Low bytes per sector - uS
  pi86IoWrite8(0xF00F, drive[0x0C]); // High bytes per sector
#else
  pi86MemWrite8(0xF800C, drive[0x1A]); // Low byte heads per cylinder - DH-1
  pi86MemWrite8(0xF800D, drive[0x1B]); // High byte heads per cylinder
  pi86MemWrite8(0xF800E, drive[0x0B]); // Low bytes per sector - uS
  pi86MemWrite8(0xF800F, drive[0x0C]); // High bytes per sector
#endif

  int Head_Per_Cylinder   = (drive[0x1B] << 8) | drive[0x1A];
  int Sector_Per_Track    = (drive[0x19] << 8) | drive[0x18];
  int Number_Of_Sectors   = (drive[0x14] << 8) | drive[0x13];
  int Number_Of_Cylinders = Number_Of_Sectors / Sector_Per_Track / Head_Per_Cylinder;

#if NEW_BIOS
  pi86IoWrite8(0xF011, Number_Of_Cylinders); // CH
#else
  pi86MemWrite8(0xF8011, Number_Of_Cylinders); // CH
#endif

  Number_Of_Cylinders = (Number_Of_Cylinders >> 2) & 0xC0;
  Sector_Per_Track    = Sector_Per_Track & 0X3F; // CL
#if NEW_BIOS
  pi86IoWrite8(0xF010, Number_Of_Cylinders + Sector_Per_Track);
#else
  pi86MemWrite8(0xF8010, Number_Of_Cylinders + Sector_Per_Track);
#endif

  int Small_Sectors = (drive[0x14] << 8) | drive[0x13];
  if (Small_Sectors == 0x0000) {
#if NEW_BIOS
    pi86IoWrite8(0xF012, drive[0x20]); // Big sector
    pi86IoWrite8(0xF013, drive[0x21]);
    pi86IoWrite8(0xF014, drive[0x22]);
    pi86IoWrite8(0xF015, drive[0x23]);
#else
    pi86MemWrite8(0xF8012, drive[0x20]); // Big sector
    pi86MemWrite8(0xF8013, drive[0x21]);
    pi86MemWrite8(0xF8014, drive[0x22]);
    pi86MemWrite8(0xF8015, drive[0x23]);
#endif
  } else {
#if NEW_BIOS
    pi86IoWrite8(0xF012, drive[0x13]); // Small sector
    pi86IoWrite8(0xF013, drive[0x14]);
    pi86IoWrite8(0xF014, 0x00);
    pi86IoWrite8(0xF015, 0x00);
#else
    pi86MemWrite8(0xF8012, drive[0x13]); // Small sector
    pi86MemWrite8(0xF8013, drive[0x14]);
    pi86MemWrite8(0xF8014, 0x00);
    pi86MemWrite8(0xF8015, 0x00);
#endif
  }
#if NEW_BIOS
  pi86IoWrite8(0xF016, 0X03);
#else
  pi86MemWrite8(0xF8016, 0X03);
#endif

  return true;
}

static void Int13(void) {

  const uint8_t Int13_Command = NEW_BIOS ? pi86IoRead8(0xF000) :
                                           pi86MemRead8(0xF8000);
  const uint8_t Drive = NEW_BIOS ? pi86IoRead8(0xF006) :
                                   pi86MemRead8(0xF8006);

  if (Int13_Command != 0xFF) {

    if (Drive == 0x00) { Get_Disk_Parameters_A(); }
    if (Drive == 0x80) { Get_Disk_Parameters_C(); }

    uint8_t int13_data[0X20] = { 0 };
#if NEW_BIOS
    memcpy(int13_data, pi86IoPtr(0xF000), 0x20);
#else
    pi86MemReadPtr(0xF8000, int13_data, 0X20);
#endif

    if (Int13_Command == 0x00) {
      // BIOS DOES ALL THE WORK
      // RESET DISK SYSTEM
      // NOTHING TO RESET
    }
    if (Int13_Command == 0x01) {
      // BIOS DOES ALL THE WORK
      // GET STATUS OF LAST OPERATION
    }

    // read
    if (Int13_Command == 0x02) {
      int Cylinder          = (int13_data[3] << 8) | int13_data[2];
      int Sector            =  int13_data[4];
      int Head              =  int13_data[5];
      int Bytes_Per_Sector  = (int13_data[0x0F] << 8) | int13_data[0x0E];
      int Sector_Per_Track  = (int13_data[0x10] & 0X3F);
      int Head_Per_Cylinder = (int13_data[0x0D] << 8) | int13_data[0x0C];
      int Number_Of_Sectors =  int13_data[1];
      int LBA               = (Cylinder * Head_Per_Cylinder + Head) * Sector_Per_Track + (Sector - 1);
      int Buffer_Address    = (int13_data[10] << 12) +
                              (int13_data[9]  <<  4) +
                              (int13_data[8]  <<  8) +
                              (int13_data[7]  <<  0);

      FILE *fd = (Drive == 0x00) ? driveA :
                 (Drive == 0x80) ? driveC :
                 nullptr;

      if (fd) {
        const uint32_t offset = LBA * Bytes_Per_Sector;
        fseek(fd, offset, SEEK_SET);
        const uint32_t size = Number_Of_Sectors * Bytes_Per_Sector;
        uint8_t drive[size] = {0};
        fread(drive, 1, size, fd);
        pi86MemWritePtr(Buffer_Address, drive, size);
      }
    }

    // write
    if (Int13_Command == 0x03) {
      int Cylinder          = (int13_data[3] << 8) | int13_data[2];
      int Sector            =  int13_data[4];
      int Head              =  int13_data[5];
      int Bytes_Per_Sector  = (int13_data[0x0F] << 8) | int13_data[0x0E];
      int Sector_Per_Track  = (int13_data[0x10] & 0X3F);
      int Head_Per_Cylinder = (int13_data[0x0D] << 8) | int13_data[0x0C];
      int Number_Of_Sectors =  int13_data[1];
      int LBA               = (Cylinder * Head_Per_Cylinder + Head) * Sector_Per_Track + (Sector - 1);
      int Buffer_Address    = (int13_data[10] << 12) +
                              (int13_data[9]  << 4)  +
                              (int13_data[8]  << 8)  +
                              (int13_data[7]  << 0);

      const uint32_t size = Number_Of_Sectors * Bytes_Per_Sector;
      uint8_t drive[size] = { 0 };
      pi86MemReadPtr(Buffer_Address, drive, sizeof(drive));

      FILE *fd = (Drive == 0x00) ? driveA :
                 (Drive == 0x80) ? driveC :
                 nullptr;

      if (fd) {
        const uint32_t offset = LBA * Bytes_Per_Sector;
        fseek(fd, offset, SEEK_SET);
        fwrite(drive, 1, size, fd);
      }
    }

    // parameters
    if (Int13_Command == 0x08) {
      if (int13_data[6] == 0x00) { Get_Disk_Parameters_A(); }
      if (int13_data[6] == 0x80) { Get_Disk_Parameters_C(); }
    }

    // get disk type
    if (Int13_Command == 0x15) {
      if (int13_data[6] == 0x00) { Get_Disk_Parameters_A(); }
      if (int13_data[6] == 0x80) { Get_Disk_Parameters_C(); }
    }

#if NEW_BIOS
    pi86IoWrite8(0xF000, 0xff);
#else
    pi86MemWrite8(0xF8000, 0xFF);
#endif
  }
}

void drivesPollInt13(void) {
  const uint8_t Int13_Command = NEW_BIOS ? pi86IoRead8(0xF000) :
                                           pi86MemRead8(0xF8000);
  if (Int13_Command != 0XFF) {
    Int13();
  }
}

bool drivesStart(const std::string &fdd, const std::string &hdd) {

  driveA = fopen(fdd.c_str(), "ab+");
  if (!driveA) {
    return false;
  }

  driveC = fopen(hdd.c_str(), "ab+");
  if (!driveC) {
    return false;
  }

  return true;
}
