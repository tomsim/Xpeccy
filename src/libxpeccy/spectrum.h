#ifndef _XPECTR
#define _XPECTR

#ifdef __cplusplus
extern "C" {
#endif

#include "cpu/cpu.h"
#include "video/video.h"
#include "memory.h"

#include "input.h"
#include "tape.h"
#include "fdc.h"
#include "hdd.h"
#include "sdcard.h"
#include "cartridge.h"

#include "sound/ayym.h"
#include "sound/gs.h"
#include "sound/saa1099.h"
#include "sound/soundrive.h"
#include "sound/gbsound.h"
#include "sound/nesapu.h"

#ifdef HAVEZLIB
	#include <zlib.h>
#endif

// hw reset rompage
enum {
	RES_DEFAULT = 0,
	RES_48,
	RES_128,
	RES_DOS,
	RES_SHADOW
};

// io breaks
#define IO_BRK_RD	1
#define IO_BRK_WR	(1<<1)

enum {
	DBG_VIEW_CODE = 0x00,
	DBG_VIEW_BYTE = 0x10,
	DBG_VIEW_WORD = 0x20,
	DBG_VIEW_ADDR = 0x30,
	DBG_VIEW_TEXT = 0x40,
	DBG_VIEW_EXEC = 0x50
};

typedef struct {
	unsigned char l;
	unsigned char h;
	unsigned char x;
} DMAaddr;

typedef struct {
	unsigned char flag;
	unsigned char page;
} memEntry;

typedef struct {
	unsigned brk:1;			// breakpoint
	unsigned debug:1;		// dont' do breakpoints
	unsigned maping:1;		// map memory during execution
	unsigned frmStrobe:1;		// new frame started
	unsigned intStrobe:1;		// int front
	unsigned nmiRequest:1;		// Magic button pressed
	unsigned firstRun:1;

	unsigned rom:1;			// b4,7ffd
	unsigned dos:1;			// BDI dos
	unsigned cpm:1;

	unsigned evenM1:1;		// scorpion wait mode
	unsigned contMem:1;		// contended mem
	unsigned contIO:1;		// contended IO

	unsigned irq:1;
	unsigned brkirq:1;		// break on irq

	double fps;
	double cpuFrq;
	int frqMul;
	unsigned char intVector;

	char* msg;			// message ptr for displaying outside

	struct HardWare *hw;
	CPU* cpu;
	Memory* mem;
	Video* vid;

	Keyboard* keyb;
	Joystick* joy;
	Mouse* mouse;

	Tape* tape;
	DiskIF* dif;
	IDE* ide;

	bitChan* beep;
	SDCard* sdc;
	TSound* ts;
	GSound* gs;
	SDrive* sdrv;
	saaChip* saa;
	gbSound* gbsnd;
	nesAPU* nesapu;

	xCartridge* slot;		// cartrige slot (MSX, GB, NES)

#ifdef HAVEZLIB

	struct {
		unsigned start:1;
		unsigned play:1;
		unsigned overio:1;
		int fTotal;
		int fCurrent;
		int fCount;
		FILE* file;		// converted tmp-file
		struct {
			int fetches;
			int size;
			unsigned char data[0x10000];
			int pos;
		} frm;
	} rzx;

#endif

	int tickCount;
	int frmtCount;
	int nsPerTick;

	unsigned char p7FFD;		// stored port out
	unsigned char p1FFD;
	unsigned char pEFF7;
	unsigned char pDFFD;
	unsigned char p77hi;
	unsigned char prt2;		// scorpion ProfROM layer (0..3)

	memEntry memMap[16];			// memory map for ATM2, PentEvo
	unsigned char brkRamMap[0x400000];	// ram brk/type : b0..3:brk flags, b4..7:type
	unsigned char brkRomMap[0x80000];	// rom brk/type : b0..3:brk flags, b4..7:type
	unsigned char brkAdrMap[0x10000];	// adr brk
	unsigned char brkIOMap[0x10000];	// io brk

	unsigned short padr;
	unsigned char pval;

	struct {
		unsigned char evoBF;		// PentEvo rw ports
		unsigned char evo2F;
		unsigned char evo4F;
		unsigned char evo6F;
		unsigned char evo8F;
		unsigned char blVer[16];	// bootloader info
		unsigned char bcVer[16];	// baseconf info
	} evo;
	struct {
		DMAaddr src;
		DMAaddr dst;
		unsigned char len;
		unsigned char num;
	} dma;
	struct {
		int flag;
		unsigned short tsMapAdr;	// adr for altera mapping
		unsigned char Page0;
//		unsigned char p00af;		// ports to be updated from next line
		unsigned char p01af;
		unsigned char p02af;
		unsigned char p03af;
		unsigned char p04af;
		unsigned char p05af;
//		unsigned char p07af;
		unsigned char p21af;
		unsigned char pwr_up;		// 1 on 1st run, 0 after reading 00AF
		unsigned char vdos;
	} tsconf;
	struct {
		unsigned char p7E;		// color num (if trig7E=0)
	} profi;
	struct {
		unsigned char keyLine;		// selected keyboard line
		unsigned char pA8;		// port A8
		unsigned char pAA;		// port AA
		unsigned char mFFFF;		// mem FFFF : mapper secondary slot
		unsigned char pF5;
		unsigned char pslot[4];
		unsigned char sslot[4];
		unsigned char memMap[4];	// RAM pages (ports FC..FF)
		struct {
			unsigned char regA;
			unsigned char regB;
			unsigned char regC;
		} ppi;
	} msx;
	struct {
		int type;			// DENDY | NTSC | PAL
		int priPadState;		// b0..7 = A,B,sel,start,up,down,left,right,0,0,0,0,....
		int secPadState;
		int priJoy;			// shift registers (reading 4016/17 returns bit 0 & shift right)
		int secJoy;			//		write b0=1 to 4016 restore status
	} nes;
	struct {
		unsigned boot:1;	// boot rom on
		unsigned inpint:1;	// button pressed: request interrupt
		int buttons;
		struct {
			struct {
				long per;
				long cnt;
			} div;		// divider (16KHz, inc FF04)
			struct {
				unsigned on:1;
				unsigned intrq:1;
				long per;
				long cnt;
			} t;		// manual timer
		} timer;

		int vbank;		// video bank (0,1)
		int wbank;		// ram bank (d000..dfff)
		int rbank;		// slot ram bank (a000..bfff)
		unsigned bgpal[0x3f];
		unsigned sppal[0x3f];
		unsigned char iram[256];	// internal ram (FF80..FFFE)
		unsigned short iomap[128];
	} gb;
	int romsize;
	CMOS cmos;
	int resbank;			// rompart active after reset
	int tapCount;
} Computer;

#include "hardware.h"

Computer* compCreate();
void compDestroy(Computer*);
void compReset(Computer*,int);
int compExec(Computer*);

void compKeyPress(Computer*, int, keyEntry*);
void compKeyRelease(Computer*, int, keyEntry*);

void compSetBaseFrq(Computer*,double);
void compSetTurbo(Computer*,int);
void compSetHardware(Computer*,const char*);
void compUpdateTimings(Computer*);

// read-write cmos
unsigned char cmsRd(Computer*);
void cmsWr(Computer*,unsigned char);

void rzxStop(Computer*);

unsigned char* getBrkPtr(Computer*, unsigned short);
unsigned char getBrk(Computer*, unsigned short);
void setBrk(Computer*, unsigned short, unsigned char);

#ifdef __cplusplus
}
#endif

#endif
