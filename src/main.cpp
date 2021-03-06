#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QFontDatabase>

#include <locale.h>

#include "xcore/xcore.h"
#include "xcore/sound.h"
#include "xgui/xgui.h"
#include "libxpeccy/spectrum.h"
#include "emulwin.h"
#include "xgui/debuga/debuger.h"
#include "setupwin.h"
#include "filer.h"

#include <SDL.h>
#undef main

//#ifdef _WIN32
//	#include <mmsystem.h>
//#endif

void help() {
	printf("Xpeccy command line arguments:\n");
	printf("-h | --help\t\tshow this help\n");
	printf("-d | --debug\t\tstart debugging immediately\n");
	printf("-p | --profile NAME\tset current profile\n");
	printf("-b | --bank NUM\t\tset rampage NUM to #c000 memory window\n");
	printf("-a | --adr ADR\t\tset loading address (see -f below)\n");
	printf("-f | --file NAME\tload binary file to address defined by -a (see above)\n");
	printf("-l | --labels NAME\tload labels list generated by LABELSLIST in SJASM+\n");
	printf("--pc ADR\t\tset PC\n");
	printf("--sp ADR\t\tset SP\n");
	printf("--bp ADR\t\tset fetch brakepoint to address ADR\n");
}

int main(int ac,char** av) {

// NOTE:SDL_INIT_VIDEO must be here for SDL_Joystick event processing. Joystick doesn't works without video init
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
	atexit(SDL_Quit);
	SDL_version sdlver;
	SDL_VERSION(&sdlver);
	printf("Using SDL ver %u.%u.%u\n", sdlver.major, sdlver.minor, sdlver.patch);

//#ifdef HAVEALSA
//	printf("Using ALSA ver %s\n",SND_LIB_VERSION_STR);
//#endif
#ifdef HAVEZLIB
	printf("Using ZLIB ver %s\n",ZLIB_VERSION);
#endif
	printf("Using Qt ver %s\n",qVersion());

	QApplication app(ac,av,true);

	conf.running = 0;
	conf.joy.dead = 8192;
	vidInitAdrs();
	sndInit();
	initPaths(av[0]);
	addProfile("default","xpeccy.conf");

	QFontDatabase::addApplicationFont("://DejaVuSansMono.ttf");

	int i;
	MainWin mwin;
	char* parg;
//	unsigned char* ptr = NULL;
	int adr = 0x4000;
	i = 1;
	mwin.setProfile("");
	int dbg = 0;
	// int hlp = 0;
	while (i < ac) {
		parg = av[i++];
		if ((strcmp(parg,"-d") == 0) || (strcmp(parg,"--debug") == 0)) {
			dbg = 1;
		} else if (!strcmp(parg,"-h") || !strcmp(parg,"--help")) {
			help();
			exit(0);
		} else if (i < ac) {
			if (!strcmp(parg,"-p") || !strcmp(parg,"--profile")) {
				mwin.setProfile(std::string(av[i]));
				i++;
			} else if (!strcmp(parg,"--pc")) {
				mwin.comp->cpu->pc = strtol(av[i],NULL,0);
				i++;
			} else if (!strcmp(parg,"--sp")) {
				mwin.comp->cpu->sp = strtol(av[i],NULL,0);
				i++;
			} else if (!strcmp(parg,"-b") || !strcmp(parg,"--bank")) {
				memSetBank(mwin.comp->mem, MEM_BANK3, MEM_RAM, strtol(av[i],NULL,0), NULL, NULL, NULL);
				i++;
			} else if (!strcmp(parg,"-a") || !strcmp(parg,"--adr")) {
				adr = strtol(av[i],NULL,0) & 0xffff;
				i++;
			} else if (!strcmp(parg,"-f") || !strcmp(parg,"--file")) {
				loadDUMP(mwin.comp, av[i], adr);
				i++;
			} else if (!strcmp(parg,"--bp")) {
				brkSet(BRK_MEMCELL, MEM_BRK_FETCH, strtol(av[i],NULL,0) & 0xffff, -1);
				i++;
			} else if (!strcmp(parg,"-l") || !strcmp(parg,"--labels")) {
				mwin.loadLabels(av[i]);
				i++;
			} else if (strlen(parg) > 0) {
				loadFile(mwin.comp, parg, FT_ALL, 0);
			}
		} else if (strlen(parg) > 0) {
			loadFile(mwin.comp, parg, FT_ALL, 0);
		}
	}
	prfFillBreakpoints(conf.prof.cur);
	if (dbg) mwin.doDebug();
	mwin.show();
	mwin.raise();
	mwin.activateWindow();
	mwin.updateWindow();
	mwin.checkState();
	conf.running = 1;
	app.exec();
	conf.running = 0;
	sndClose();
	return 0;
}
