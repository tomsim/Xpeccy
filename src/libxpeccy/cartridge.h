#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#if __cplusplus
extern "C" {
#endif

#include <stdio.h>

// cartridge mapers

#define MAP_UNKNOWN -1

enum {
	MAPER_MSX = 0,
	MAPER_GB,
	MAPER_NES,
};

// MSX
enum {
	MAP_MSX_NOMAPPER = 0,
	MAP_MSX_KONAMI4,
	MAP_MSX_KONAMI5,
	MAP_MSX_ASCII8,
	MAP_MSX_ASCII16
};
// GameBoy
enum {
	MAP_GB_NOMAP,
	MAP_GB_MBC1,
	MAP_GB_MBC2,
	MAP_GB_MBC3,
	MAP_GB_MBC5
};
// NES (1.0)
enum {
	MAP_NES_NROM = 0,
	MAP_NES_MMC1 = 1,
	MAP_NES_UNROM = 2,
	MAP_NES_CNROM = 3,
	MAP_NES_MMC3 = 4,
	MAP_NES_AOROM = 7,
	MAP_NES_CAMERICA = 71,
	MAP_NES_VRC3 = 73,
	MAP_NES_VRC1 = 75,
	MAP_NES_VRC7 = 85
};

// slot memory type
enum {
	SLT_PRG = 1,
	SLT_CHR,		// NES PPU 0000..1fff
	SLT_RAM,		// NES 6000..7fff
	SLT_EXT			// NES 5000..5fff
};

// NES nametable mirroring
enum {
	NES_NT_SINGLE = 0,
	NES_NT_HORIZ,
	NES_NT_VERT,
	NES_NT_QUATRO
};

// memory flags
#define	MEM_BRK_FETCH	1
#define	MEM_BRK_RD	(1<<1)
#define	MEM_BRK_WR	(1<<2)
#define	MEM_BRK_ANY	(MEM_BRK_FETCH | MEM_BRK_RD | MEM_BRK_WR)
#define MEM_BRK_TFETCH	(1<<3)

typedef struct xCartridge xCartridge;

typedef struct {
	int id;
	unsigned char (*rd)(xCartridge*, int, unsigned short, int);
	void (*wr)(xCartridge*, int, unsigned short, int, unsigned char);
	int (*adr)(xCartridge*, int, unsigned short);
	void (*chk)(xCartridge*, unsigned short);
} xCardCallback;

struct xCartridge {
	unsigned ramen:1;		// ram enabled (gb, nes)
	unsigned ramwe:1;		// ram writing enable (nes)
	unsigned ramod:1;		// ram banking mode (gb)
	unsigned brk:1;
	unsigned irqen:1;		// irq enable (nes)
	unsigned irqrl:1;		// irq reload request (nes)
	unsigned irq:1;			// irq signal (nes)
	unsigned irqs:1;		// irq control signal (A12 for nes mmc3)

	char name[FILENAME_MAX];
	int memMap[4];			// max 4x8K PRG pages
	int prglast;			// last 16K PRG page number
	int mapType;			// user defined mapper type, if auto-detect didn't worked (msx)

	unsigned char shift;		// shift register (mmc1)
	int bitcount;			// counter of bits writing to shift register (mmc1)
	unsigned char regCT;		// control registers
	unsigned char reg00;
	unsigned char reg01;
	unsigned char reg02;
	unsigned char reg03;
	unsigned char reg04;
	unsigned char reg05;
	unsigned char reg06;
	unsigned char reg07;

	unsigned char ival;		// irq init value (nes mmc3)
	unsigned char icnt;		// irq counter (nes mmc3)

	unsigned blk1:1;		// BLK0/1 selection signal
	int mirror;			// mirroring type

	int memMask;
	int chrMask;
	int ramMask;
	unsigned char ram[0x8000];	// onboard ram (32K max)
	xCardCallback* core;
	unsigned char* data;		// onboard rom (malloc) = nes prg-rom
	unsigned char* brkMap;
	unsigned char* chrrom;		// nes chr rom (malloc)
};

xCartridge* sltCreate();
void sltDestroy(xCartridge*);
void sltEject(xCartridge*);

xCardCallback* sltFindMaper(int,int);
int sltSetMaper(xCartridge*,int, int);

unsigned char sltRead(xCartridge*, int, unsigned short);
void sltWrite(xCartridge*, int, unsigned short, unsigned char);
void sltChecker(xCartridge*, unsigned short);

// translate ppu nt vadr according mirroring type
unsigned short nes_nt_vadr(xCartridge*, unsigned short);

#if __cplusplus
}
#endif

#endif
