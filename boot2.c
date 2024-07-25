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
#define ATA_CMD_READ            0x20
#define ATA_STATUS_DRQ_MASK     (1<<3)

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
} fat16BootSect;

uint8_t* const vmem = (uint8_t*)0xA0000;

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

// maybe bad ata pio read func, from my oldest os project
void ataPioRead28(uint32_t lba, uint8_t sectCnt, void* data) {
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));

    outb(ATA_PRIMARY_SECT_CNT, sectCnt);

    outb(ATA_PRIMARY_LBA_L, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_M, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_H, (uint8_t)(lba >> 16));

    outb(ATA_PRIMARY_CMD, ATA_CMD_READ);

    uint8_t status = inb(ATA_PRIMARY_STATUS);
    while(!(status & ATA_STATUS_DRQ_MASK)) {
        status = inb(ATA_PRIMARY_STATUS);
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
}

__attribute__((section(".first"))) void boot2() {
    // set the 2nd pixel to a greenish colour to show we made it
    vmem[1] = 0x32;

    fat16BootSect* firstSectData = (fat16BootSect*)0x3000;
    ataPioRead28(0, 1, firstSectData);

    // jmp to this address for ever, not rlly needed but wtv
    __asm__("jmp .");
}
