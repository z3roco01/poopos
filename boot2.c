#include <stdint.h>

// Definiton pulled from my old os project for the ata read
#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_DRIVE_SEL   0x1F6
#define ATA_PRIMARY_SECT_CNT    0x1F2
#define ATA_PRIMARY_LBA_L       0x1F3
#define ATA_PRIMARY_LBA_M       0x1F4
#define ATA_PRIMARY_LBA_H       0x1F5
#define ATA_PRIMARY_CMD         0x1F7
#define ATA_PRIMARY_STATUS      0x1F7
#define ATA_PRIMARY_CTRL        0x3F6
#define ATA_CMD_READ            0x20
#define ATA_STATUS_DRQ_MASK     (1<<3)
#define ATA_MASTER_VAL          0xA0
#define ATA_CMD_IDENT           0xEC
#define ATA_STATUS_BSY_MASK     (1<<7)

typedef struct fat16BootSect {
    uint8_t  jmpBootcode[3];
    uint8_t  oemIdent[8];
    uint16_t bps;
    uint8_t  spc;
    uint16_t reservedSects;
    uint8_t  numFats;
    uint16_t rootDirEnts;
    uint16_t sectCnt;
    uint8_t  mediaDesc;
    uint16_t spf;
    uint16_t spt;
    uint16_t heads;
    uint32_t hiddenSects;
    uint32_t largeSectCnt;

    // FAT12/16 only
    uint8_t  driveNum;
    uint8_t  reserved;
    uint8_t  sig;
    uint32_t volId;
    uint8_t  volLbl[11];
    uint8_t  sysIdent[8];
    uint8_t  bootCode[448];
    uint16_t bootSig;
}__attribute__((packed)) fat16BootSect;

typedef struct fatDir {
    uint8_t  name[11];
    uint8_t  attrs;
    uint8_t  reserved;
    uint8_t  cTimeTens;
    uint16_t cTime;
    uint16_t cDate;
    uint16_t aDate;
    uint16_t highClustNum;
    uint16_t mTime;
    uint16_t mDate;
    uint16_t lowClustNum;
    uint32_t size;
}__attribute__((packed)) fatDir_t;

uint8_t* const vmem = (uint8_t*)0xB8000;

// port functions for ata read
void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void softwareAtaReset() {
   uint8_t reg = inb(ATA_PRIMARY_CTRL);
   uint8_t regReset = reg | 0b00000100;
   outb(ATA_PRIMARY_CTRL, regReset);
   for(int i = 0; i<50; ++i) {
   }
   outb(ATA_PRIMARY_CTRL, reg);
}

void ataIdentify() {
    outb(ATA_PRIMARY_DRIVE_SEL, ATA_MASTER_VAL);

    outb(ATA_PRIMARY_SECT_CNT, 0x00);
    outb(ATA_PRIMARY_LBA_L, 0x00);
    outb(ATA_PRIMARY_LBA_M, 0x00);
    outb(ATA_PRIMARY_LBA_H, 0x00);

    outb(ATA_PRIMARY_CMD, ATA_CMD_IDENT);

    uint8_t status = inb(ATA_PRIMARY_STATUS);

    while(status & ATA_STATUS_BSY_MASK) {
        status = inb(ATA_PRIMARY_STATUS);
    }

    uint8_t lbaM = inb(ATA_PRIMARY_LBA_M);
    uint8_t lbaH = inb(ATA_PRIMARY_LBA_H);

    status = inb(ATA_PRIMARY_STATUS);
    while(!(status & ATA_STATUS_DRQ_MASK)) {
        status = inb(ATA_PRIMARY_STATUS);
    }
    vmem[0] = 'B';
    vmem[1] = 0x0F;
}

// maybe bad ata pio read func, from my oldest os project
void ataPioRead28(uint32_t lba, uint8_t sectCnt, void* data) {
    vmem[0] = 'T';
    vmem[1] = 0x0F;
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));

    outb(ATA_PRIMARY_SECT_CNT, sectCnt);

    outb(ATA_PRIMARY_LBA_L, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_M, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_H, (uint8_t)(lba >> 16));

    outb(ATA_PRIMARY_CMD, ATA_CMD_READ);

    vmem[0] = 'C';
    vmem[1] = 0x0F;
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    int i = 0;
    while(!(status & ATA_STATUS_BSY_MASK)) {
        status = inb(ATA_PRIMARY_STATUS);
        if(i++ >= 100) {
            i = 0;
        }
    }

    uint16_t curWord = 0;
    for(uint32_t i = 0; i < sectCnt; ++i){
        status = inb(ATA_PRIMARY_STATUS);
        while(!(status & ATA_STATUS_DRQ_MASK)) {
            status = inb(ATA_PRIMARY_STATUS);
        }
        for(uint16_t j = 0; j < 256; ++j) {
            ((uint16_t*)data)[j+(i*256)] = inw(ATA_PRIMARY_DATA);
        }
    }

    vmem[0] = 'X';
    vmem[1] = 0x0F;
    __asm__("jmp .");
}

__attribute__((section(".first"))) void boot2() {
    // set the 2nd pixel to a greenish colour to show we made it
    vmem[0] = 'A';
    vmem[1] = 0x0F;

    // Make the bootsect pointer point to the already loaded boot sect at 0x7C00
    fat16BootSect* bootSect = (fat16BootSect*)0x7C00;

    // find and read the root directory
    // get the lba ( logical block address ) of the root dir
    uint32_t lba = bootSect->reservedSects + bootSect->spf * bootSect->numFats;
    // get number of sectors it takes up
    uint32_t sects = (sizeof(fatDir_t) * bootSect->rootDirEnts) / bootSect->bps;
    if((sizeof(fatDir_t) * bootSect->rootDirEnts) % bootSect->bps > 0)
        sects++;

    // find the start and end of the root dir
    uint32_t rootDirStart = lba * bootSect->bps;
    uint32_t rootDirEnd   = rootDirStart + (sects * bootSect->bps);

    // read the whole of the root dir
    fatDir_t* rootDir = (fatDir_t*)0x3000;
    ataIdentify();

    ataPioRead28(rootDirStart, sects * bootSect->bps, rootDir);

    vmem[2] = bootSect->oemIdent[0];
    vmem[3] = 0x0F;

    // jmp to this address for ever, not rlly needed but wtv
    __asm__("jmp .");
}
