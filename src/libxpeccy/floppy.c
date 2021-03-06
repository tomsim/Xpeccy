#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "floppy.h"

Floppy* flpCreate(int id) {
	Floppy* flp = (Floppy*)malloc(sizeof(Floppy));
	memset(flp,0x00,sizeof(Floppy));
	flp->id = id;
	flp->trk80 = 1;
	flp->doubleSide = 1;
	flp->trk = 0;
	flp->rtrk = 0;
	flp->pos = 0;
	flp->path = NULL;
	return flp;
}

void flpDestroy(Floppy* flp) {
	free(flp);
}

void flpWr(Floppy* flp,unsigned char val) {
	flp->wr = 1;
	flp->changed = 1;
	flp->data[flp->rtrk].byte[flp->pos] = val;
}

unsigned char flpRd(Floppy* flp) {
	flp->rd = 1;
	return flp->data[flp->rtrk].byte[flp->pos];
}

unsigned char flpGetField(Floppy* flp) {
	return flp->data[flp->rtrk].field[flp->pos];
}

void flpStep(Floppy* flp,int dir) {
	switch (dir) {
		case FLP_FORWARD:
			if (flp->trk < (flp->trk80 ? 86 : 43)) flp->trk++;
			break;
		case FLP_BACK:
			if (flp->trk > 0) flp->trk--;
			break;
	}
}

int flpNext(Floppy* flp, int fdcSide) {
	int res = 0;
	flp->rtrk = (flp->trk << 1);
	if (flp->doubleSide && !fdcSide) flp->rtrk++;		// /SIDE1 = 0 when upper head (1) selected
	if (flp->insert) {
		flp->pos++;
		if (flp->pos >= TRACKLEN) {
			flp->pos = 0;
			res = 1;
		}
		flp->index = (flp->pos < 4) ? 1 : 0;		// ~90ms index pulse
		flp->field = flp->data[flp->rtrk].field[flp->pos];
	} else {
		flp->field = 0;
	}
	return res;
}

void flpPrev(Floppy* flp, int fdcSide) {
	flp->rtrk = (flp->trk << 1);
	if (flp->doubleSide && !fdcSide) flp->rtrk++;
	if (flp->insert) {
		if (flp->pos > 0) {
			flp->pos--;
		} else {
			flp->pos = TRACKLEN - 1;
		}
		flp->field = flp->data[flp->rtrk].field[flp->pos];
	} else {
		flp->field = 0;
	}

}

void flpClearTrack(Floppy* flp,int tr) {
	memset(flp->data[tr].byte, 0, TRACKLEN);
	memset(flp->data[tr].field, 0, TRACKLEN);
}

void flpClearDisk(Floppy* flp) {
	int i;
	for (i = 0; i < 160; i++) flpClearTrack(flp,i);
}

unsigned short getCrc(unsigned char* ptr, int len) {
	unsigned int crc = 0xcdb4;
	int i;
	while (len--) {
		crc ^= *ptr << 8;
		for (i = 0; i<8 ; i++) {
			if ((crc *= 2) & 0x10000) crc ^= 0x1021;
		}
		ptr++;
	}
	return  (crc & 0xffff);
}

void flpFillFields(Floppy* flp,int tr, int fcrc) {
	int i, bcnt = 0, sct = 1;
	unsigned char fld = 0;
	unsigned char* cpos = flp->data[tr].byte;
	unsigned char* bpos = cpos;
	unsigned short crc;
	if (tr > 255) return;
	for (i = 0; i < TRACKLEN; i++) {
		flp->data[tr].field[i] = fld;
		if (fcrc) {
			switch (fld) {
				case 0:
					if ((*bpos) == 0xf5) *bpos = 0xa1;
					if ((*bpos) == 0xf6) *bpos = 0xc2;
					break;
				case 4:
					if (*bpos == 0xf7) {
						crc = getCrc(cpos, bpos - cpos);
						*bpos = ((crc & 0xff00) >> 8);
						*(bpos + 1) = (crc & 0xff);
					}
					break;
			}
		}
		if (bcnt > 0) {
			bcnt--;
			if (bcnt==0) {
				if (fld < 4) {
					fld = 4; bcnt = 2;
				} else {
					fld = 0;
				}
			}
		} else {
			if (flp->data[tr].byte[i] == 0xfe) {
				cpos = bpos;
				fld = 1;
				bcnt = 4;
				sct = flp->data[tr].byte[i+4];
			}
			if (flp->data[tr].byte[i] == 0xfb) {
				cpos = bpos;
				fld = 2;
				bcnt = (128 << (sct & 3));
			}
			if (flp->data[tr].byte[i] == 0xf8) {
				cpos = bpos;
				fld = 3;
				bcnt = (128 << (sct & 3));
			}
		}
		bpos++;
	}
}

int flpEject(Floppy* flp) {
	free(flp->path);
	flp->path = NULL;
	flp->insert = 0;
	flp->changed = 0;
	return 1;
}


void flpGetTrack(Floppy* flp,int tr,unsigned char* dst) {
	memcpy(dst,flp->data[tr].byte,TRACKLEN);
}

void flpGetTrackFields(Floppy* flp,int tr,unsigned char* dst) {
	memcpy(dst,flp->data[tr].field,TRACKLEN);
}

void flpPutTrack(Floppy* flp,int tr,unsigned char* src,int len) {
	flpClearTrack(flp,tr);
	memcpy(flp->data[tr].byte,src,len);
	flpFillFields(flp,tr,0);
}
