#include "xcore.h"
#include "../sound.h"

#include <stdio.h>

std::vector<xLayout> layList;

bool addLayout(std::string nm,int fh,int fv,int bh,int bv,int sh,int sv,int ih,int iv,int is) {
//	printf("add Layout: %s\n",nm.c_str());
	for (unsigned int i = 0; i < layList.size(); i++) {
		if (layList[i].name == nm) return false;
	}
	if ((iv < 0) || (iv >= fv) || (ih < 0) || (ih >= fh) || (is < 1)) {
		printf("WARNING: Layout %s : INT position and/or length isn't correct\n",nm.c_str());
		iv = 0; ih = 0; is = 64;
	}
	xLayout nlay;
	nlay.name = nm;
	nlay.full.h = fh;
	nlay.full.v = fv;
	nlay.bord.h = bh;
	nlay.bord.v = bv;
	nlay.sync.h = sh;
	nlay.sync.v = sv;
	nlay.intpos.h = ih;
	nlay.intpos.v = iv;
	nlay.intsz = is;
	layList.push_back(nlay);
	return true;
}

bool addLayout(xLayout lay) {
//	printf("add Layout: %s\n",lay.name.c_str());
	for (unsigned int i = 0; i < layList.size(); i++) {
		if (layList[i].name == lay.name) return false;
	}
	layList.push_back(lay);
	return true;
}

void setLayoutList(std::vector<xLayout> lst) {
	layList = lst;
}

std::vector<xLayout> getLayoutList() {
	return layList;
}

xLayout* findLayout(std::string nm) {
	xLayout* res = NULL;
	for (uint i = 0; i < layList.size(); i++) {
		if (layList[i].name == nm) res = &layList[i];
	}
	return res;
}
