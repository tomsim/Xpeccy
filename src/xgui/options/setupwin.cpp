#include <QStandardItemModel>
#include <QInputDialog>
#include <QFileDialog>
#include <QVector3D>
#include <QPainter>
#include <QDebug>
#include <stdlib.h>
#ifdef HAVESDL
	#include <SDL.h>
	#undef main
#endif

#include "filer.h"
#include "setupwin.h"
#include "xgui/xgui.h"
#include "xcore/gamepad.h"
#include "xcore/xcore.h"
#include "xcore/sound.h"
#include "libxpeccy/spectrum.h"
#include "libxpeccy/filetypes/filetypes.h"

//#include "ui_padbinder.h"
//Ui::PadBinder padui;

void fillRFBox(QComboBox* box, QStringList lst) {
	box->clear();
	box->addItem("none","");
	foreach(QString str, lst) {
		box->addItem(str, str);
	}
}

void setRFIndex(QComboBox* box, QVariant data) {
	box->setCurrentIndex(box->findData(data));
}

int getRFIData(QComboBox* box) {
	return box->itemData(box->currentIndex()).toInt();
}

QString getRFSData(QComboBox* box) {
	return box->itemData(box->currentIndex()).toString();
}

std::string getRFText(QComboBox* box) {
	QString res = "";
	if (box->currentIndex() >= 0) res = box->currentText();
	return std::string(res.toLocal8Bit().data());
}


// OBJECT

SetupWin::SetupWin(QWidget* par):QDialog(par) {
	setModal(true);
	ui.setupUi(this);

	umadial = new QDialog;
	uia.setupUi(umadial);
	umadial->setModal(true);

	rseditor = new xRomsetEditor(this);
	rsmodel = new xRomsetModel();
	ui.tvRomset->setModel(rsmodel);

	layeditor = new QDialog(this);
	layUi.setupUi(layeditor);
	layeditor->setModal(true);

	padial = new xPadBinder(this);

	block = 0;
	prfChanged = 0;

	unsigned int i;
// machine
	i = 0;
	while (hwTab[i].name != NULL) {
		if (hwTab[i].id != HW_NULL) {
			ui.machbox->addItem(trUtf8(hwTab[i].optName),QString::fromLocal8Bit(hwTab[i].name));
		} else {
			ui.machbox->insertSeparator(i);
		}
		i++;
	}
	i = 0;
	while (cpuTab[i].type != CPU_NONE) {
		ui.cbCpu->addItem(trUtf8(cpuTab[i].name), cpuTab[i].type);
		i++;
	}
	ui.resbox->addItem("BASIC 48",RES_48);
	ui.resbox->addItem("BASIC 128",RES_128);
	ui.resbox->addItem("DOS",RES_DOS);
	ui.resbox->addItem("SHADOW",RES_SHADOW);
// video
	std::map<std::string,int>::iterator it;
	for (it = shotFormat.begin(); it != shotFormat.end(); it++) {
		ui.ssfbox->addItem(QString(it->first.c_str()),it->second);

	}
// sound
	i = 0;
	while (sndTab[i].name != NULL) {
		ui.outbox->addItem(QString::fromLocal8Bit(sndTab[i].name));
		i++;
	}
	ui.ratbox->addItem("48000",48000);
	ui.ratbox->addItem("44100",44100);
	ui.ratbox->addItem("22050",22050);
	ui.ratbox->addItem("11025",11025);
	ui.schip1box->addItem(QIcon(":/images/cancel.png"),"none",SND_NONE);
	ui.schip1box->addItem(QIcon(":/images/MicrochipLogo.png"),"AY-3-8910",SND_AY);
	ui.schip1box->addItem(QIcon(":/images/YamahaLogo.png"),"Yamaha 2149",SND_YM);
	ui.schip2box->addItem(QIcon(":/images/cancel.png"),"none",SND_NONE);
	ui.schip2box->addItem(QIcon(":/images/MicrochipLogo.png"),"AY-3-8910",SND_AY);
	ui.schip2box->addItem(QIcon(":/images/YamahaLogo.png"),"Yamaha 2149",SND_YM);
#ifdef ISDEBUG
	ui.schip1box->addItem(QIcon(":/images/YamahaLogo.png"),"Yamaha 2203",SND_YM2203);
	ui.schip2box->addItem(QIcon(":/images/YamahaLogo.png"),"Yamaha 2203",SND_YM2203);
#endif
	ui.stereo1box->addItem("Mono",AY_MONO); ui.stereo2box->addItem("Mono",AY_MONO);
	ui.stereo1box->addItem("ABC",AY_ABC); ui.stereo2box->addItem("ABC",AY_ABC);
	ui.stereo1box->addItem("ACB",AY_ACB); ui.stereo2box->addItem("ACB",AY_ACB);
	ui.stereo1box->addItem("BAC",AY_BAC); ui.stereo2box->addItem("BAC",AY_BAC);
	ui.stereo1box->addItem("BCA",AY_BCA); ui.stereo2box->addItem("BCA",AY_BCA);
	ui.stereo1box->addItem("CAB",AY_CAB); ui.stereo2box->addItem("CAB",AY_CAB);
	ui.stereo1box->addItem("CBA",AY_CBA); ui.stereo2box->addItem("CBA",AY_BCA);
	ui.tsbox->addItem("None",TS_NONE);
	ui.tsbox->addItem("NedoPC",TS_NEDOPC);
//	ui.gstereobox->addItem("Mono",GS_MONO);
//	ui.gstereobox->addItem("L:1,2; R:3,4",GS_12_34);
	ui.sdrvBox->addItem("None",SDRV_NONE);
	ui.sdrvBox->addItem("Covox only",SDRV_COVOX);
	ui.sdrvBox->addItem("Soundrive 1.05 mode 1",SDRV_105_1);
	ui.sdrvBox->addItem("Soundrive 1.05 mode 2",SDRV_105_2);
// bdi
// WTF? QtDesigner doesn't save this properties
	ui.disklist->horizontalHeader()->setVisible(true);
	ui.diskTypeBox->addItem("None",DIF_NONE);
	ui.diskTypeBox->addItem("Beta disk (VG93)",DIF_BDI);
	ui.diskTypeBox->addItem("+3 DOS (uPD765)",DIF_P3DOS);
	ui.disklist->addAction(ui.actCopyToTape);
	ui.disklist->addAction(ui.actSaveHobeta);
	ui.disklist->addAction(ui.actSaveRaw);
// tape
	ui.tapelist->setColumnWidth(0,25);
	ui.tapelist->setColumnWidth(1,25);
	ui.tapelist->setColumnWidth(2,50);
	ui.tapelist->setColumnWidth(3,50);
	ui.tapelist->setColumnWidth(4,100);
	ui.tapelist->addAction(ui.actCopyToDisk);
// hdd
	ui.hiface->addItem("None",IDE_NONE);
	ui.hiface->addItem("Nemo",IDE_NEMO);
	ui.hiface->addItem("Nemo A8",IDE_NEMOA8);
	ui.hiface->addItem("Nemo Evo",IDE_NEMO_EVO);
	ui.hiface->addItem("SMUC",IDE_SMUC);
	ui.hiface->addItem("ATM",IDE_ATM);
	ui.hiface->addItem("Profi",IDE_PROFI);
	ui.hm_type->addItem(QIcon(":/images/cancel.png"),"Not connected",IDE_NONE);
	ui.hm_type->addItem(QIcon(":/images/hdd.png"),"HDD (ATA)",IDE_ATA);
//	setupUi.hm_type->addItem(QIcon(":/images/cd.png"),"CD (ATAPI) not working yet",IDE_ATAPI);
	ui.hs_type->addItem(QIcon(":/images/cancel.png"),"Not connected",IDE_NONE);
	ui.hs_type->addItem(QIcon(":/images/hdd.png"),"HDD (ATA)",IDE_ATA);
//	setupUi.hs_type->addItem(QIcon(":/images/cd.png"),"CD (ATAPI) not working yet",IDE_ATAPI);
// others
	ui.sdcapbox->addItem("32 M",SDC_32M);
	ui.sdcapbox->addItem("64 M",SDC_64M);
	ui.sdcapbox->addItem("128 M",SDC_128M);
	ui.sdcapbox->addItem("256 M",SDC_256M);
	ui.sdcapbox->addItem("512 M",SDC_512M);
	ui.sdcapbox->addItem("1024 M",SDC_1G);
	ui.cSlotType->addItem("No mapper",MAP_MSX_NOMAPPER);
	ui.cSlotType->addItem("Konami 4",MAP_MSX_KONAMI4);
	ui.cSlotType->addItem("Konami 5",MAP_MSX_KONAMI5);
	ui.cSlotType->addItem("ASCII 8K",MAP_MSX_ASCII8);
	ui.cSlotType->addItem("ASCII 16K",MAP_MSX_ASCII16);
// input
	padModel = new xPadMapModel();
	ui.tvPadTable->setModel(padModel);
	ui.tvPadTable->addAction(ui.actAddBinding);
	ui.tvPadTable->addAction(ui.actEditBinding);
	ui.tvPadTable->addAction(ui.actDelBinding);
// all
	connect(ui.okbut,SIGNAL(released()),this,SLOT(okay()));
	connect(ui.apbut,SIGNAL(released()),this,SLOT(apply()));
	connect(ui.cnbut,SIGNAL(released()),this,SLOT(reject()));
// machine
	connect(ui.rsetbox,SIGNAL(currentIndexChanged(int)),this,SLOT(buildrsetlist()));
	connect(ui.machbox,SIGNAL(currentIndexChanged(int)),this,SLOT(setmszbox(int)));
	connect(ui.tvRomset,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(editrset()));
	connect(ui.addrset,SIGNAL(released()),this,SLOT(addNewRomset()));
	connect(ui.rmrset,SIGNAL(released()),this,SLOT(rmRomset()));
	connect(ui.rsedit,SIGNAL(released()),this,SLOT(editrset()));
	connect(rseditor,SIGNAL(complete(int,QString)),this,SLOT(rscomplete(int,QString)));

// video
	connect(ui.pathtb,SIGNAL(released()),this,SLOT(selsspath()));
	connect(ui.bszsld,SIGNAL(valueChanged(int)),this,SLOT(chabsz()));
	connect(ui.sldNoflic,SIGNAL(valueChanged(int)),this,SLOT(chaflc()));

	connect(ui.layEdit,SIGNAL(released()),this,SLOT(edLayout()));
	connect(ui.layAdd,SIGNAL(released()),this,SLOT(addNewLayout()));
	connect(ui.layDel,SIGNAL(released()),this,SLOT(delLayout()));

	connect(layUi.layName,SIGNAL(textChanged(QString)),this,SLOT(layNameCheck(QString)));
	connect(layUi.lineBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.rowsBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.brdLBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.brdUBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.hsyncBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.vsyncBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.intLenBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.intPosBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.intRowBox,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.sbScrH,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.sbScrW,SIGNAL(valueChanged(int)),this,SLOT(layEditorChanged()));
	connect(layUi.okButton,SIGNAL(released()),this,SLOT(layEditorOK()));
	connect(layUi.cnButton,SIGNAL(released()),layeditor,SLOT(hide()));
// sound
/*
	connect(ui.bvsld,SIGNAL(valueChanged(int)),this,SLOT(updvolumes()));
	connect(ui.tvsld,SIGNAL(valueChanged(int)),this,SLOT(updvolumes()));
	connect(ui.avsld,SIGNAL(valueChanged(int)),this,SLOT(updvolumes()));
	connect(ui.gvsld,SIGNAL(valueChanged(int)),this,SLOT(updvolumes()));
*/
	connect(ui.sldMasterVol,SIGNAL(valueChanged(int)),ui.sbMasterVol,SLOT(setValue(int)));
	connect(ui.sldBeepVol,SIGNAL(valueChanged(int)),ui.sbBeepVol,SLOT(setValue(int)));
	connect(ui.sldTapeVol,SIGNAL(valueChanged(int)),ui.sbTapeVol,SLOT(setValue(int)));
	connect(ui.sldAYVol,SIGNAL(valueChanged(int)),ui.sbAYVol,SLOT(setValue(int)));
	connect(ui.sldGSVol,SIGNAL(valueChanged(int)),ui.sbGSVol,SLOT(setValue(int)));
	connect(ui.sldSdrvVol,SIGNAL(valueChanged(int)),ui.sbSdrvVol,SLOT(setValue(int)));
	connect(ui.sldSAAVol,SIGNAL(valueChanged(int)),ui.sbSAAVol,SLOT(setValue(int)));

	connect(ui.sbMasterVol,SIGNAL(valueChanged(int)),ui.sldMasterVol,SLOT(setValue(int)));
	connect(ui.sbBeepVol,SIGNAL(valueChanged(int)),ui.sldBeepVol,SLOT(setValue(int)));
	connect(ui.sbTapeVol,SIGNAL(valueChanged(int)),ui.sldTapeVol,SLOT(setValue(int)));
	connect(ui.sbAYVol,SIGNAL(valueChanged(int)),ui.sldAYVol,SLOT(setValue(int)));
	connect(ui.sbGSVol,SIGNAL(valueChanged(int)),ui.sldGSVol,SLOT(setValue(int)));
	connect(ui.sbSdrvVol,SIGNAL(valueChanged(int)),ui.sldSdrvVol,SLOT(setValue(int)));
	connect(ui.sbSAAVol,SIGNAL(valueChanged(int)),ui.sldSAAVol,SLOT(setValue(int)));

// dos
	connect(ui.newatb,SIGNAL(released()),this,SLOT(newa()));
	connect(ui.newbtb,SIGNAL(released()),this,SLOT(newb()));
	connect(ui.newctb,SIGNAL(released()),this,SLOT(newc()));
	connect(ui.newdtb,SIGNAL(released()),this,SLOT(newd()));

	connect(ui.loadatb,SIGNAL(released()),this,SLOT(loada()));
	connect(ui.loadbtb,SIGNAL(released()),this,SLOT(loadb()));
	connect(ui.loadctb,SIGNAL(released()),this,SLOT(loadc()));
	connect(ui.loaddtb,SIGNAL(released()),this,SLOT(loadd()));

	connect(ui.saveatb,SIGNAL(released()),this,SLOT(savea()));
	connect(ui.savebtb,SIGNAL(released()),this,SLOT(saveb()));
	connect(ui.savectb,SIGNAL(released()),this,SLOT(savec()));
	connect(ui.savedtb,SIGNAL(released()),this,SLOT(saved()));

	connect(ui.remoatb,SIGNAL(released()),this,SLOT(ejcta()));
	connect(ui.remobtb,SIGNAL(released()),this,SLOT(ejctb()));
	connect(ui.remoctb,SIGNAL(released()),this,SLOT(ejctc()));
	connect(ui.remodtb,SIGNAL(released()),this,SLOT(ejctd()));

	connect(ui.disktabs,SIGNAL(currentChanged(int)),this,SLOT(fillDiskCat()));
	connect(ui.actCopyToTape,SIGNAL(triggered()),this,SLOT(copyToTape()));
	connect(ui.actSaveHobeta,SIGNAL(triggered()),this,SLOT(diskToHobeta()));
	connect(ui.actSaveRaw,SIGNAL(triggered()),this,SLOT(diskToRaw()));
	connect(ui.tbToTape,SIGNAL(released()),this,SLOT(copyToTape()));
	connect(ui.tbToHobeta,SIGNAL(released()),this,SLOT(diskToHobeta()));
	connect(ui.tbToRaw,SIGNAL(released()),this,SLOT(diskToRaw()));
// tape
	connect(ui.tapelist,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(chablock(QModelIndex)));
	connect(ui.tapelist,SIGNAL(clicked(QModelIndex)),this,SLOT(tlistclick(QModelIndex)));
	// connect(ui.tapelist,SIGNAL(cellClicked(int,int)),this,SLOT(setTapeBreak(int,int)));
	connect(ui.tloadtb,SIGNAL(released()),this,SLOT(loatape()));
	connect(ui.tsavetb,SIGNAL(released()),this,SLOT(savtape()));
	connect(ui.tremotb,SIGNAL(released()),this,SLOT(ejctape()));
	connect(ui.blkuptb,SIGNAL(released()),this,SLOT(tblkup()));
	connect(ui.blkdntb,SIGNAL(released()),this,SLOT(tblkdn()));
	connect(ui.blkrmtb,SIGNAL(released()),this,SLOT(tblkrm()));
	connect(ui.actCopyToDisk,SIGNAL(triggered()),this,SLOT(copyToDisk()));
	connect(ui.tbToDisk,SIGNAL(released()),this,SLOT(copyToDisk()));
// hdd
	connect(ui.hm_islba,SIGNAL(stateChanged(int)),this,SLOT(hddcap()));
	connect(ui.hm_glba,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hm_gcyl,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hm_ghd,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hm_gsec,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hm_pathtb,SIGNAL(released()),this,SLOT(hddMasterImg()));
	connect(ui.hs_islba,SIGNAL(stateChanged(int)),this,SLOT(hddcap()));
	connect(ui.hs_glba,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hs_gcyl,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hs_ghd,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hs_gsec,SIGNAL(valueChanged(int)),this,SLOT(hddcap()));
	connect(ui.hs_pathtb,SIGNAL(released()),this,SLOT(hddSlaveImg()));
// external
	connect(ui.tbSDCimg,SIGNAL(released()),this,SLOT(selSDCimg()));
	connect(ui.tbsdcfree,SIGNAL(released()),ui.sdPath,SLOT(clear()));
	connect(ui.cSlotOpen,SIGNAL(released()),this,SLOT(openSlot()));
	connect(ui.cSlotEject,SIGNAL(released()),this,SLOT(ejectSlot()));
// input
	connect(ui.tbPadNew, SIGNAL(released()),this,SLOT(newPadMap()));
	connect(ui.tbPadDelete,SIGNAL(released()),this,SLOT(delPadMap()));
	connect(ui.cbPadMap, SIGNAL(currentIndexChanged(int)),this,SLOT(chaPadMap(int)));
	connect(ui.tbAddBind,SIGNAL(clicked(bool)),this, SLOT(addBinding()));
	connect(ui.tbEditBind,SIGNAL(clicked(bool)),this,SLOT(editBinding()));
	connect(ui.tbDelBind,SIGNAL(clicked(bool)),this,SLOT(delBinding()));
	connect(ui.actAddBinding,SIGNAL(triggered()),this,SLOT(addBinding()));
	connect(ui.actEditBinding,SIGNAL(triggered()),this,SLOT(editBinding()));
	connect(ui.tvPadTable,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(editBinding()));
	connect(ui.actDelBinding,SIGNAL(triggered()),this,SLOT(delBinding()));
	connect(padial, SIGNAL(bindReady(xJoyMapEntry)), this, SLOT(bindAccept(xJoyMapEntry)));
//tools
	connect(ui.umlist,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(umedit(QModelIndex)));
	connect(ui.umaddtb,SIGNAL(released()),this,SLOT(umadd()));
	connect(ui.umdeltb,SIGNAL(released()),this,SLOT(umdel()));
	connect(ui.umuptb,SIGNAL(released()),this,SLOT(umup()));
	connect(ui.umdntb,SIGNAL(released()),this,SLOT(umdn()));
// bookmark add dialog
	connect(uia.umasptb,SIGNAL(released()),this,SLOT(umaselp()));
	connect(uia.umaok,SIGNAL(released()),this,SLOT(umaconf()));
	connect(uia.umacn,SIGNAL(released()),umadial,SLOT(hide()));
// profiles manager
	connect(ui.tbNewProfile,SIGNAL(released()),this,SLOT(newProfile()));
	connect(ui.tbDelProfile,SIGNAL(released()),this,SLOT(rmProfile()));

}

void SetupWin::okay() {
	apply();
	reject();
}

void SetupWin::start(xProfile* p) {
	prof = p;
	comp = p->zx;
// machine
	ui.rsetbox->clear();
	foreach(xRomset rs, conf.rsList) {
		ui.rsetbox->addItem(QString::fromLocal8Bit(rs.name.c_str()));
	}
	ui.machbox->setCurrentIndex(ui.machbox->findData(QString::fromUtf8(comp->hw->name)));
	ui.rsetbox->setCurrentIndex(ui.rsetbox->findText(QString::fromUtf8(prof->rsName.c_str())));
	ui.resbox->setCurrentIndex(ui.resbox->findData(comp->resbank));
	setmszbox(ui.machbox->currentIndex());
	ui.mszbox->setCurrentIndex(ui.mszbox->findData(comp->mem->memSize));
	if (ui.mszbox->currentIndex() < 0) ui.mszbox->setCurrentIndex(ui.mszbox->count() - 1);
	ui.cbCpu->setCurrentIndex(ui.cbCpu->findData(comp->cpu->type));
	ui.sbFreq->setValue(comp->cpuFrq);
	ui.scrpwait->setChecked(comp->evenM1);
// video
	ui.cbFullscreen->setChecked(conf.vid.fullScreen);
	ui.cbKeepRatio->setChecked(conf.vid.keepRatio);
	ui.sbScale->setValue(conf.vid.scale);
//	ui.noflichk->setChecked(conf.vid.noFlick);
	ui.sldNoflic->setValue(conf.vid.noflic); chaflc();
	ui.grayscale->setChecked(conf.vid.grayScale);
	ui.border4T->setChecked(comp->vid->brdstep & 0x06);
	ui.contMem->setChecked(comp->contMem);
	ui.contIO->setChecked(comp->contIO);
	ui.bszsld->setValue((int)(conf.brdsize * 100));
	ui.pathle->setText(QString::fromLocal8Bit(conf.scrShot.dir.c_str()));
	ui.ssfbox->setCurrentIndex(ui.ssfbox->findText(conf.scrShot.format.c_str()));
	ui.scntbox->setValue(conf.scrShot.count);
	ui.sintbox->setValue(conf.scrShot.interval);
	ui.ssNoLeds->setChecked(conf.scrShot.noLeds);
	ui.ssNoBord->setChecked(conf.scrShot.noBorder);
	ui.geombox->clear();
	foreach(xLayout lay, conf.layList) {
		ui.geombox->addItem(QString::fromLocal8Bit(lay.name.c_str()));
	}
	ui.geombox->setCurrentIndex(ui.geombox->findText(QString::fromLocal8Bit(conf.prof.cur->layName.c_str())));
	ui.ulaPlus->setChecked(comp->vid->ula->enabled);
// sound
//	ui.gsgroup->setChecked(comp->gs->enable);
	ui.tbGS->setChecked(comp->gs->enable);
	ui.gsrbox->setChecked(comp->gs->reset);
	// setRFIndex(ui.gstereobox, comp->gs->stereo);

	ui.sdrvBox->setCurrentIndex(ui.sdrvBox->findData(comp->sdrv->type));

	ui.tbSAA->setChecked(comp->saa->enabled);
//	ui.saaEn->setChecked(comp->saa->enabled);
//	ui.saaStereo->setChecked(!comp->saa->mono);

	ui.senbox->setChecked(conf.snd.enabled);
//	ui.mutbox->setChecked(conf.snd.mute);
	ui.outbox->setCurrentIndex(ui.outbox->findText(QString::fromLocal8Bit(sndOutput->name)));
	ui.ratbox->setCurrentIndex(ui.ratbox->findData(QVariant(conf.snd.rate)));

	ui.sbMasterVol->setValue(conf.snd.vol.master);
	ui.sbBeepVol->setValue(conf.snd.vol.beep);
	ui.sbTapeVol->setValue(conf.snd.vol.tape);
	ui.sbAYVol->setValue(conf.snd.vol.ay);
	ui.sbGSVol->setValue(conf.snd.vol.gs);
	ui.sbSdrvVol->setValue(conf.snd.vol.sdrv);
	ui.sbSAAVol->setValue(conf.snd.vol.saa);

	ui.schip1box->setCurrentIndex(ui.schip1box->findData(QVariant(comp->ts->chipA->type)));
	ui.schip2box->setCurrentIndex(ui.schip2box->findData(QVariant(comp->ts->chipB->type)));
	ui.stereo1box->setCurrentIndex(ui.stereo1box->findData(QVariant(comp->ts->chipA->stereo)));
	ui.stereo2box->setCurrentIndex(ui.stereo2box->findData(QVariant(comp->ts->chipB->stereo)));
	ui.tsbox->setCurrentIndex(ui.tsbox->findData(QVariant(comp->ts->type)));
// input
	buildkeylist();
	int idx = ui.keyMapBox->findText(QString(conf.keyMapName.c_str()));
	if (idx < 1) idx = 0;
	ui.keyMapBox->setCurrentIndex(idx);
	ui.ratEnable->setChecked(comp->mouse->enable);
	ui.ratWheel->setChecked(comp->mouse->hasWheel);
	ui.cbSwapButtons->setChecked(comp->mouse->swapButtons);
	ui.cbKbuttons->setChecked(comp->joy->extbuttons);
	ui.sldDeadZone->setValue(conf.joy.dead);
	ui.lePadName->setText(conf.joy.joy ? SDL_JoystickName(0) : "none");
	padModel->update();
// dos
	ui.diskTypeBox->setCurrentIndex(ui.diskTypeBox->findData(comp->dif->type));
	ui.bdtbox->setChecked(fdcFlag & FDC_FAST);
	ui.mempaths->setChecked(conf.storePaths);
	Floppy* flp = comp->dif->fdc->flop[0];
	ui.apathle->setText(QString::fromLocal8Bit(flp->path));
		ui.a80box->setChecked(flp->trk80);
		ui.adsbox->setChecked(flp->doubleSide);
		ui.awpbox->setChecked(flp->protect);
	flp = comp->dif->fdc->flop[1];
	ui.bpathle->setText(QString::fromLocal8Bit(flp->path));
		ui.b80box->setChecked(flp->trk80);
		ui.bdsbox->setChecked(flp->doubleSide);
		ui.bwpbox->setChecked(flp->protect);
	flp = comp->dif->fdc->flop[2];
	ui.cpathle->setText(QString::fromLocal8Bit(flp->path));
		ui.c80box->setChecked(flp->trk80);
		ui.cdsbox->setChecked(flp->doubleSide);
		ui.cwpbox->setChecked(flp->protect);
	flp = comp->dif->fdc->flop[3];
	ui.dpathle->setText(QString::fromLocal8Bit(flp->path));
		ui.d80box->setChecked(flp->trk80);
		ui.ddsbox->setChecked(flp->doubleSide);
		ui.dwpbox->setChecked(flp->protect);
	fillDiskCat();
// hdd
	ui.hiface->setCurrentIndex(ui.hiface->findData(comp->ide->type));

	ui.hm_type->setCurrentIndex(ui.hm_type->findData(comp->ide->master->type));
	ATAPassport pass = ideGetPassport(comp->ide,IDE_MASTER);
	ui.hm_path->setText(QString::fromLocal8Bit(comp->ide->master->image));
	ui.hm_islba->setChecked(comp->ide->master->hasLBA);
	ui.hm_gsec->setValue(pass.spt);
	ui.hm_ghd->setValue(pass.hds);
	ui.hm_gcyl->setValue(pass.cyls);
	ui.hm_glba->setValue(comp->ide->master->maxlba);

	ui.hs_type->setCurrentIndex(ui.hm_type->findData(comp->ide->slave->type));
	pass = ideGetPassport(comp->ide,IDE_SLAVE);
	ui.hs_path->setText(QString::fromLocal8Bit(comp->ide->slave->image));
	ui.hs_islba->setChecked(comp->ide->slave->hasLBA);
	ui.hs_gsec->setValue(pass.spt);
	ui.hs_ghd->setValue(pass.hds);
	ui.hs_gcyl->setValue(pass.cyls);
	ui.hs_glba->setValue(comp->ide->slave->maxlba);
// external
	ui.sdPath->setText(QString::fromLocal8Bit(comp->sdc->image));
	ui.sdcapbox->setCurrentIndex(ui.sdcapbox->findData(comp->sdc->capacity));
	if (ui.sdcapbox->currentIndex() < 0) ui.sdcapbox->setCurrentIndex(2);	// 128M
	ui.sdlock->setChecked(comp->sdc->lock);

	ui.cSlotName->setText(comp->slot->name);
	setRFIndex(ui.cSlotType, comp->slot->mapType);
// tape
	ui.cbTapeAuto->setChecked(conf.tape.autostart);
	ui.cbTapeFast->setChecked(conf.tape.fast);
	ui.tpathle->setText(QString::fromLocal8Bit(comp->tape->path));
	buildtapelist();
// input
	buildpadlist();
	setRFIndex(ui.cbPadMap, trUtf8(conf.prof.cur->jmapName.c_str()));
// tools
	buildmenulist();
// leds
	ui.cbMouseLed->setChecked(conf.led.mouse);
	ui.cbJoyLed->setChecked(conf.led.joy);
	ui.cbKeysLed->setChecked(conf.led.keys);
	ui.cbTapeLed->setChecked(conf.led.tape);
	ui.cbDiskLed->setChecked(conf.led.disk);
	ui.cbMessage->setChecked(conf.led.message);
// profiles
	ui.defstart->setChecked(conf.defProfile);
	buildproflist();

	show();
}

void SetupWin::apply() {
// machine
	HardWare *oldmac = comp->hw;
	prof->hwName = std::string(getRFSData(ui.machbox).toUtf8().data());
	compSetHardware(prof->zx,prof->hwName.c_str());
	prof->rsName = getRFText(ui.rsetbox);
	prfSetRomset(prof, prof->rsName);
	comp->resbank = getRFIData(ui.resbox);
	memSetSize(comp->mem, getRFIData(ui.mszbox));
	cpuSetType(comp->cpu, getRFIData(ui.cbCpu));
	compSetBaseFrq(comp, ui.sbFreq->value());
	comp->evenM1 = ui.scrpwait->isChecked() ? 1 : 0;
	if (comp->hw != oldmac) compReset(comp,RES_DEFAULT);
// video
	conf.vid.fullScreen = ui.cbFullscreen->isChecked() ? 1 : 0;
	conf.vid.keepRatio = ui.cbKeepRatio->isChecked() ? 1 : 0;
	conf.vid.scale = ui.sbScale->value();
//	conf.vid.noFlick = ui.noflichk->isChecked() ? 1 : 0;
	conf.vid.noflic = ui.sldNoflic->value();
	conf.vid.grayScale = ui.grayscale->isChecked() ? 1 : 0;
	conf.scrShot.dir = std::string(ui.pathle->text().toLocal8Bit().data());
	conf.scrShot.format = getRFText(ui.ssfbox);
	conf.scrShot.count = ui.scntbox->value();
	conf.scrShot.interval = ui.sintbox->value();
	conf.scrShot.noLeds = ui.ssNoLeds->isChecked() ? 1 : 0;
	conf.scrShot.noBorder = ui.ssNoBord->isChecked() ? 1 : 0;
	conf.brdsize = ui.bszsld->value()/100.0;
	comp->vid->brdstep = ui.border4T->isChecked() ? 7 : 1;
	comp->contMem = ui.contMem->isChecked() ? 1 : 0;
	comp->contIO = ui.contIO->isChecked() ? 1 : 0;
	comp->vid->ula->enabled = ui.ulaPlus->isChecked() ? 1 : 0;
	prfSetLayout(NULL, getRFText(ui.geombox));
// sound
	std::string nname = getRFText(ui.outbox);
	conf.snd.enabled = ui.senbox->isChecked() ? 1 : 0;
//	conf.snd.mute = ui.mutbox->isChecked() ? 1 : 0;
	conf.snd.rate = getRFIData(ui.ratbox);

	conf.snd.vol.master = ui.sbMasterVol->value();
	conf.snd.vol.beep = ui.sbBeepVol->value();
	conf.snd.vol.tape = ui.sbTapeVol->value();
	conf.snd.vol.ay = ui.sbAYVol->value();
	conf.snd.vol.gs = ui.sbGSVol->value();
	conf.snd.vol.sdrv = ui.sbSdrvVol->value();
	conf.snd.vol.saa = ui.sbSAAVol->value();

	setOutput(nname.c_str());
	aymSetType(comp->ts->chipA, getRFIData(ui.schip1box));
	aymSetType(comp->ts->chipB, getRFIData(ui.schip2box));
	comp->ts->chipA->stereo = getRFIData(ui.stereo1box);
	comp->ts->chipB->stereo = getRFIData(ui.stereo2box);
	comp->ts->type = getRFIData(ui.tsbox);

//	comp->gs->enable = ui.gsgroup->isChecked() ? 1 : 0;
	comp->gs->enable = ui.tbGS->isChecked() ? 1 : 0;
	comp->gs->reset = ui.gsrbox->isChecked() ? 1 : 0;
//	comp->gs->stereo = getRFIData(ui.gstereobox);

	comp->sdrv->type = getRFIData(ui.sdrvBox);

	comp->saa->enabled = ui.tbSAA->isChecked() ? 1 : 0;
//	comp->saa->enabled = ui.saaEn->isChecked() ? 1 : 0;
//	comp->saa->mono = ui.saaStereo->isChecked() ? 0 : 1;

	sndCalibrate(comp);
// input
	comp->mouse->enable = ui.ratEnable->isChecked() ? 1 : 0;
	comp->mouse->hasWheel = ui.ratWheel->isChecked() ? 1 : 0;
	comp->mouse->swapButtons = ui.cbSwapButtons->isChecked() ? 1 : 0;
	comp->joy->extbuttons = ui.cbKbuttons->isChecked() ? 1 : 0;
	conf.joy.dead = ui.sldDeadZone->value();
	std::string kmname = getRFText(ui.keyMapBox);
	if (kmname == "none") kmname = "default";
	conf.keyMapName = kmname;
	loadKeys();
// bdi
	difSetHW(comp->dif, getRFIData(ui.diskTypeBox));
	setFlagBit(ui.bdtbox->isChecked(),&fdcFlag,FDC_FAST);
	conf.storePaths = ui.mempaths->isChecked() ? 1 : 0;

	Floppy* flp = comp->dif->fdc->flop[0];
	flp->trk80 = ui.a80box->isChecked() ? 1 : 0;
	flp->doubleSide = ui.adsbox->isChecked() ? 1 : 0;
	flp->protect = ui.awpbox->isChecked() ? 1 : 0;

	flp = comp->dif->fdc->flop[1];
	flp->trk80 = ui.b80box->isChecked() ? 1 : 0;
	flp->doubleSide = ui.bdsbox->isChecked() ? 1 : 0;
	flp->protect = ui.bwpbox->isChecked() ? 1 : 0;

	flp = comp->dif->fdc->flop[2];
	flp->trk80 = ui.c80box->isChecked() ? 1 : 0;
	flp->doubleSide = ui.cdsbox->isChecked() ? 1 : 0;
	flp->protect = ui.cwpbox->isChecked() ? 1 : 0;

	flp = comp->dif->fdc->flop[3];
	flp->trk80 = ui.d80box->isChecked() ? 1 : 0;
	flp->doubleSide = ui.ddsbox->isChecked() ? 1 : 0;
	flp->protect = ui.dwpbox->isChecked() ? 1 : 0;

// hdd
	comp->ide->type = getRFIData(ui.hiface);

	ATAPassport pass = ideGetPassport(comp->ide,IDE_MASTER);
	comp->ide->master->type = getRFIData(ui.hm_type);
	ideSetImage(comp->ide,IDE_MASTER,ui.hm_path->text().toLocal8Bit().data());
	comp->ide->master->hasLBA = ui.hm_islba->isChecked() ? 1 : 0;
	pass.spt = ui.hm_gsec->value();
	pass.hds = ui.hm_ghd->value();
	pass.cyls = ui.hm_gcyl->value();
	comp->ide->master->maxlba = ui.hm_glba->value();
	ideSetPassport(comp->ide,IDE_MASTER,pass);

	pass = ideGetPassport(comp->ide,IDE_SLAVE);
	comp->ide->slave->type = getRFIData(ui.hs_type);
	ideSetImage(comp->ide,IDE_SLAVE,ui.hs_path->text().toLocal8Bit().data());
	comp->ide->slave->hasLBA = ui.hs_islba->isChecked() ? 1 : 0;
	pass.spt = ui.hs_gsec->value();
	pass.hds = ui.hs_ghd->value();
	pass.cyls = ui.hs_gcyl->value();
	comp->ide->slave->maxlba = ui.hs_glba->value();
	ideSetPassport(comp->ide,IDE_SLAVE,pass);
// others
	sdcSetImage(comp->sdc,ui.sdPath->text().isEmpty() ? "" : ui.sdPath->text().toLocal8Bit().data());
	sdcSetCapacity(comp->sdc, getRFIData(ui.sdcapbox));
	comp->sdc->lock = ui.sdlock->isChecked() ? 1 : 0;

	comp->slot->mapType = getRFIData(ui.cSlotType);
// tape
	conf.tape.autostart = ui.cbTapeAuto->isChecked() ? 1 : 0;
	conf.tape.fast = ui.cbTapeFast->isChecked() ? 1 : 0;
// input
	conf.prof.cur->jmapName = getRFSData(ui.cbPadMap).toStdString();
// leds
	conf.led.mouse = ui.cbMouseLed->isChecked() ? 1 : 0;
	conf.led.joy = ui.cbJoyLed->isChecked() ? 1 : 0;
	conf.led.keys = ui.cbKeysLed->isChecked() ? 1 : 0;
	conf.led.tape = ui.cbTapeLed->isChecked() ? 1 : 0;
	conf.led.disk = ui.cbDiskLed->isChecked() ? 1 : 0;
	conf.led.message = ui.cbMessage->isChecked() ? 1 : 0;
// profiles
	conf.defProfile = ui.defstart->isChecked() ? 1 : 0;

	saveConfig();
	prfSave("");
}

void SetupWin::reject() {
	hide();
	emit closed();
}

// LAYOUTS

void SetupWin::layNameCheck(QString nam) {
	layUi.okButton->setEnabled(!layUi.layName->text().isEmpty());
	for (uint i = 0; i < conf.layList.size(); i++) {
		if ((QString(conf.layList[i].name.c_str()) == nam) && (eidx != (int)i)) {
			layUi.okButton->setEnabled(false);
		}
	}
}

void SetupWin::editLayout() {
	layUi.lineBox->setValue(nlay.lay.full.x);
	layUi.rowsBox->setValue(nlay.lay.full.y);
	layUi.hsyncBox->setValue(nlay.lay.blank.x);
	layUi.vsyncBox->setValue(nlay.lay.blank.y);
	layUi.brdLBox->setValue(nlay.lay.bord.x);
	layUi.brdUBox->setValue(nlay.lay.bord.y);
	layUi.intRowBox->setValue(nlay.lay.intpos.y);
	layUi.intPosBox->setValue(nlay.lay.intpos.x);
	layUi.intLenBox->setValue(nlay.lay.intSize);
	layUi.sbScrW->setValue(nlay.lay.scr.x);
	layUi.sbScrH->setValue(nlay.lay.scr.y);
	layUi.okButton->setEnabled(false);
	layUi.layWidget->setDisabled(eidx == 0);
	layUi.layName->setText(nlay.name.c_str());
	layeditor->show();
	layeditor->setFixedSize(layeditor->minimumSize());
}

void SetupWin::edLayout() {
	eidx = ui.geombox->currentIndex();
	nlay = conf.layList[eidx];
	editLayout();
}

void SetupWin::delLayout() {
	int eidx = ui.geombox->currentIndex();
	if (eidx < 1) {
		shitHappens("You can't delete this layout");
		return;
	}
	if (areSure("Do you really want to delete this layout?")) {
		conf.layList.erase(conf.layList.begin() + eidx);
		ui.geombox->removeItem(eidx);
	}
}

void SetupWin::addNewLayout() {
	eidx = -1;
	nlay = conf.layList[0];
	nlay.name = "";
	editLayout();
}

void SetupWin::layEditorChanged() {
	layUi.showField->setFixedSize(layUi.lineBox->value(),layUi.rowsBox->value());
	QPixmap pix(layUi.lineBox->value(),layUi.rowsBox->value());
	QPainter pnt;
	pnt.begin(&pix);
	pnt.fillRect(0,0,pix.width(),pix.height(),Qt::black);
	// visible screen = full - blank
	pnt.fillRect(layUi.hsyncBox->value(),layUi.vsyncBox->value(),
			layUi.lineBox->value() - layUi.hsyncBox->value(),
			layUi.rowsBox->value() - layUi.vsyncBox->value(),
			Qt::blue);
	// main screen area
	pnt.fillRect(layUi.brdLBox->value()+layUi.hsyncBox->value(), layUi.brdUBox->value()+layUi.vsyncBox->value(),
			layUi.sbScrW->value(),layUi.sbScrH->value(),
			Qt::gray);
	// INT signal
	pnt.setPen(Qt::red);
	pnt.drawLine(layUi.intPosBox->value(),
			layUi.intRowBox->value(),
			layUi.intPosBox->value() + layUi.intLenBox->value(),
			layUi.intRowBox->value());
	pnt.end();
	layUi.showField->setPixmap(pix);
	layeditor->setFixedSize(layeditor->minimumSize());
}

void SetupWin::layEditorOK() {
	std::string name = std::string(layUi.layName->text().toLocal8Bit().data());
	vLayout vlay;
	vlay.full.x = layUi.lineBox->value();
	vlay.full.y = layUi.rowsBox->value();
	vlay.bord.x = layUi.brdLBox->value();
	vlay.bord.y = layUi.brdUBox->value();
	vlay.blank.x = layUi.hsyncBox->value();
	vlay.blank.y = layUi.vsyncBox->value();
	vlay.intpos.x = layUi.intPosBox->value();
	vlay.intpos.y = layUi.intRowBox->value();
	vlay.intSize = layUi.intLenBox->value();
	vlay.scr.x = layUi.sbScrW->value();
	vlay.scr.y = layUi.sbScrH->value();
	if (eidx < 0) {
		addLayout(name, vlay);
		ui.geombox->addItem(QString::fromLocal8Bit(nlay.name.c_str()));
		ui.geombox->setCurrentIndex(ui.geombox->count() - 1);
	} else {
		if (conf.layList[eidx].name != nlay.name)
			prfChangeLayName(conf.layList[eidx].name, nlay.name);
		conf.layList[eidx].lay = vlay;
		ui.geombox->setItemText(eidx, nlay.name.c_str());
	}
	layeditor->hide();
}

// ROMSETS

void SetupWin::rmRomset() {
	int idx = ui.rsetbox->currentIndex();
	if (idx < 0) return;
	if (areSure("Do you really want to delete this romset?")) {
		conf.rsList.erase(conf.rsList.begin() + idx);		// NOTE: should be moved to xcore/romsets.cpp as delRomset?
		ui.rsetbox->removeItem(idx);
	}
}

void SetupWin::addNewRomset() {
	rseditor->edit();
}

void SetupWin::editrset() {
	eidx = ui.rsetbox->currentIndex();
	if (eidx < 0) return;
	rseditor->edit(eidx);
}

void SetupWin::rscomplete(int idx, QString nam) {
// idx < 0: new romset with name NAM created
// idx >= 0: romset IDX edited, NAM = new name
	if (idx == 0) {
		ui.rsetbox->addItem(nam);
		ui.rsetbox->setCurrentIndex(ui.rsetbox->count() - 1);
	} else {
		ui.rsetbox->setItemText(idx, nam);
		buildrsetlist();
	}
}

// lists

void SetupWin::buildpadlist() {
	QDir dir(conf.path.confDir);
	QStringList lst = dir.entryList(QStringList() << "*.pad",QDir::Files,QDir::Name);
	fillRFBox(ui.cbPadMap, lst);
}

void SetupWin::buildkeylist() {
	QDir dir(conf.path.confDir);
	QStringList lst = dir.entryList(QStringList() << "*.map",QDir::Files,QDir::Name);
	fillRFBox(ui.keyMapBox,lst);
}

std::vector<HardWare> getHardwareList() {
	std::vector<HardWare> res;
	int idx = 0;
	while (hwTab[idx].name != NULL) {
		res.push_back(hwTab[idx]);
		idx++;
	}
	return res;
}

void SetupWin::setmszbox(int idx) {
	std::vector<HardWare> list = getHardwareList();
	int t = list[idx].mask;
	QString oldText = ui.mszbox->currentText();
	ui.mszbox->clear();
	if (t == 0x00) {
		ui.mszbox->addItem("48K",48);
	} else {
		if (t & 1) ui.mszbox->addItem("128K",128);
		if (t & 2) ui.mszbox->addItem("256K",256);
		if (t & 4) ui.mszbox->addItem("512K",512);
		if (t & 8) ui.mszbox->addItem("1024K",1024);
		if (t & 16) ui.mszbox->addItem("2048K",2048);
		if (t & 32) ui.mszbox->addItem("4096K",4096);
	}
	ui.mszbox->setCurrentIndex(ui.mszbox->findText(oldText));
	if (ui.mszbox->currentIndex() < 0) ui.mszbox->setCurrentIndex(ui.mszbox->count() - 1);
}

void SetupWin::buildrsetlist() {
	if (ui.rsetbox->currentIndex() < 0) {
		ui.tvRomset->setEnabled(false);
	} else {
		ui.tvRomset->setEnabled(true);
		xRomset rset = conf.rsList[ui.rsetbox->currentIndex()];

		if (rset.file == "") {
			for (int i = 0; i < 4; i++) ui.tvRomset->showRow(i);
			ui.tvRomset->hideRow(4);
		} else {
			for (int i = 0; i < 4; i++) ui.tvRomset->hideRow(i);
			ui.tvRomset->showRow(4);
		}
		ui.tvRomset->setColumnWidth(0,100);
		ui.tvRomset->setColumnWidth(1,300);
		rsmodel->update(rset);
	}
}

void SetupWin::buildtapelist() {
	ui.tapelist->fill(comp->tape);
/*
	TapeBlockInfo* inf = new TapeBlockInfo[comp->tape->blkCount];
	tapGetBlocksInfo(comp->tape,inf);
	ui.tapelist->setRowCount(comp->tape->blkCount);
	if (comp->tape->blkCount == 0) {
		ui.tapelist->setEnabled(false);
		return;
	}
	ui.tapelist->setEnabled(true);
	QTableWidgetItem* itm;
	uint tm,ts;
	for (int i=0; i < comp->tape->blkCount; i++) {
		if (comp->tape->block == i) {
			itm = new QTableWidgetItem(QIcon(":/images/checkbox.png"),"");
			ui.tapelist->setItem(i,0,itm);
			ts = inf[i].curtime;
			tm = ts/60;
			ts -= tm * 60;
			itm = new QTableWidgetItem(QString::number(tm).append(":").append(QString::number(ts+100).right(2)));
			ui.tapelist->setItem(i,3,itm);
		} else {
			itm = new QTableWidgetItem;
			ui.tapelist->setItem(i,0,itm);
			itm = new QTableWidgetItem;
			ui.tapelist->setItem(i,3,itm);
		}
		itm = new QTableWidgetItem;
		if (inf[i].breakPoint) itm->setIcon(QIcon(":/images/cancel.png"));
		ui.tapelist->setItem(i,1,itm);
		ts = inf[i].time;
		tm = ts/60;
		ts -= tm * 60;
		itm = new QTableWidgetItem(QString::number(tm).append(":").append(QString::number(ts+100).right(2)));
		ui.tapelist->setItem(i,2,itm);
		itm = new QTableWidgetItem(QString::number(inf[i].size));
		ui.tapelist->setItem(i,4,itm);
		itm = new QTableWidgetItem(QString::fromLocal8Bit(inf[i].name));
		ui.tapelist->setItem(i,5,itm);
	}
	ui.tapelist->selectRow(0);
*/
}

void SetupWin::buildmenulist() {
	ui.umlist->setRowCount(conf.bookmarkList.size());
	QTableWidgetItem* itm;
	for (uint i = 0; i < conf.bookmarkList.size(); i++) {
		itm = new QTableWidgetItem(QString(conf.bookmarkList[i].name.c_str()));
		ui.umlist->setItem(i,0,itm);
		itm = new QTableWidgetItem(QString(conf.bookmarkList[i].path.c_str()));
		ui.umlist->setItem(i,1,itm);
	}
	ui.umlist->setColumnWidth(0,100);
	ui.umlist->selectRow(0);
};

void SetupWin::buildproflist() {
	ui.twProfileList->setRowCount(conf.prof.list.size());
	QTableWidgetItem* itm;
	for (uint i = 0; i < conf.prof.list.size(); i++) {
		itm = new QTableWidgetItem(QString::fromLocal8Bit(conf.prof.list[i]->name.c_str()));
		ui.twProfileList->setItem(i,0,itm);
		itm = new QTableWidgetItem(QString::fromLocal8Bit(conf.prof.list[i]->file.c_str()));
		ui.twProfileList->setItem(i,1,itm);
	}
}

void SetupWin::copyToTape() {
	int dsk = ui.disktabs->currentIndex();
	QModelIndexList idx = ui.disklist->selectionModel()->selectedRows();
	if (idx.size() == 0) return;
	TRFile cat[128];
	diskGetTRCatalog(comp->dif->fdc->flop[dsk],cat);
	int row;
	unsigned char* buf = new unsigned char[0xffff];
	unsigned short line,start,len;
	char name[10];
	int savedFiles = 0;
	for (int i=0; i<idx.size(); i++) {
		row = idx[i].row();
		if (diskGetSectorsData(comp->dif->fdc->flop[dsk],cat[row].trk, cat[row].sec+1, buf, cat[row].slen)) {
			if (cat[row].slen == (cat[row].hlen + ((cat[row].llen == 0) ? 0 : 1))) {
				start = (cat[row].hst << 8) + cat[row].lst;
				len = (cat[row].hlen << 8) + cat[row].llen;
				line = (cat[row].ext == 'B') ? (buf[start] + (buf[start+1] << 8)) : 0x8000;
				memset(name,0x20,10);
				memcpy(name,(char*)cat[row].name,8);
				tapAddFile(comp->tape,name,(cat[row].ext == 'B') ? 0 : 3, start, len, line, buf,true);
				savedFiles++;
			} else {
				shitHappens("File seems to be joined, skip");
			}
		} else {
			shitHappens("Can't get file data, skip");
		}
	}
	buildtapelist();
	std::string msg = std::string(int2str(savedFiles)) + " of " + int2str(idx.size()) + " files copied";
	showInfo(msg.c_str());
}

// hobeta header crc = ((105 + 257 * std::accumulate(data, data + 15, 0u)) & 0xffff))

void SetupWin::diskToHobeta() {
	QModelIndexList idx = ui.disklist->selectionModel()->selectedRows();
	if (idx.size() == 0) return;
	QString dir = QFileDialog::getExistingDirectory(this,"Save file(s) to...",QDir::homePath());
	if (dir == "") return;
	std::string sdir = std::string(dir.toLocal8Bit().data()) + SLASH;
	Floppy* flp = comp->dif->fdc->flop[ui.disktabs->currentIndex()];		// selected floppy
	int savedFiles = 0;
	for (int i=0; i<idx.size(); i++) {
		if (saveHobetaFile(flp,idx[i].row(),sdir.c_str()) == ERR_OK) savedFiles++;
	}
	std::string msg = std::string(int2str(savedFiles)) + " of " + int2str(idx.size()) + " files saved";
	showInfo(msg.c_str());
}

void SetupWin::diskToRaw() {
	QModelIndexList idx = ui.disklist->selectionModel()->selectedRows();
	if (idx.size() == 0) return;
	QString dir = QFileDialog::getExistingDirectory(this,"Save file(s) to...","",QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);
	if (dir == "") return;
	std::string sdir = std::string(dir.toLocal8Bit().data()) + SLASH;
	Floppy* flp = comp->dif->fdc->flop[ui.disktabs->currentIndex()];
	int savedFiles = 0;
	for (int i=0; i<idx.size(); i++) {
		if (saveRawFile(flp,idx[i].row(),sdir.c_str()) == ERR_OK) savedFiles++;
	}
	std::string msg = std::string(int2str(savedFiles)) + " of " + int2str(idx.size()) + " files saved";
	showInfo(msg.c_str());
}

TRFile getHeadInfo(Tape* tape, int blk) {
	TRFile res;
	TapeBlockInfo inf = tapGetBlockInfo(tape,blk);
	unsigned char* dt = (unsigned char*)malloc(inf.size + 2);
	tapGetBlockData(tape,blk,dt,inf.size+2);
	for (int i=0; i<8; i++) res.name[i] = dt[i+2];
	switch (dt[1]) {
		case 0:
			res.ext = 'B';
			res.lst = dt[12]; res.hst = dt[13];
			res.llen = dt[16]; res.hlen = dt[17];
			// autostart?
			break;
		case 3:
			res.ext = 'C';
			res.llen = dt[12]; res.hlen = dt[13];
			res.lst = dt[14]; res.hst = dt[15];
			break;
		default:
			res.ext = 0x00;
	}
	res.slen = res.hlen;
	if (res.llen != 0) res.slen++;
	free(dt);
	return res;
}

void SetupWin::copyToDisk() {
	unsigned char* dt;
	unsigned char buf[256];
	int pos;	// skip block type mark
	TapeBlockInfo inf;
	TRFile dsc;

	QModelIndexList idl = ui.tapelist->selectionModel()->selectedRows();
	if (idl.size() < 1) return;
	int blk = idl.first().row();
	if (blk < 0) return;
	int dsk = ui.disktabs->currentIndex();
	if (dsk < 0) dsk = 0;
	if (dsk > 3) dsk = 3;
	int headBlock = -1;
	int dataBlock = -1;
	if (!comp->tape->blkData[blk].hasBytes) {
		shitHappens("This is not standard block");
		return;
	}
	if (comp->tape->blkData[blk].isHeader) {
		if ((int)comp->tape->blkCount == blk + 1) {
			shitHappens("Header without data? Hmm...");
		} else {
			if (!comp->tape->blkData[blk+1].hasBytes) {
				shitHappens("Data block is not standard");
			} else {
				headBlock = blk;
				dataBlock = blk + 1;
			}
		}
	} else {
		dataBlock = blk;
		if (blk != 0) {
			if (comp->tape->blkData[blk-1].isHeader) {
				headBlock = blk - 1;
			}
		}
	}
	if (headBlock < 0) {
		const char nm[] = "FILE    ";
		memcpy(&dsc.name[0],nm,8);
		dsc.ext = 'C';
		dsc.lst = dsc.hst = 0;
		TapeBlockInfo binf = tapGetBlockInfo(comp->tape,dataBlock);
		int len = binf.size;
		qDebug() << len;
		if (len > 0xff00) {
			shitHappens("Too much data for TRDos file");
			return;
		}
		dsc.llen = len & 0xff;
		dsc.hlen = ((len & 0xff00) >> 8);
		dsc.slen = dsc.hlen;
		if (dsc.llen != 0) dsc.slen++;
	} else {
		dsc = getHeadInfo(comp->tape, headBlock);
		if (dsc.ext == 0x00) {
			shitHappens("Yes, it happens");
			return;
		}
	}
	if (!(comp->dif->fdc->flop[dsk]->insert)) newdisk(dsk);
	inf = tapGetBlockInfo(comp->tape,dataBlock);
	dt = (unsigned char*)malloc(inf.size+2);		// +2 = +mark +crc
	tapGetBlockData(comp->tape,dataBlock,dt,inf.size+2);
	switch(diskCreateDescriptor(comp->dif->fdc->flop[dsk],&dsc)) {
		case ERR_SHIT: shitHappens("Yes, it happens"); break;
		case ERR_MANYFILES: shitHappens("Too many files @ disk"); break;
		case ERR_NOSPACE: shitHappens("Not enough space @ disk"); break;
		case ERR_OK:
			pos = 0;
			while (pos < inf.size) {
				do {
					buf[pos & 0xff] = (pos < inf.size) ? dt[pos+1] : 0x00;
					pos++;
				} while (pos & 0xff);

				diskPutSectorData(comp->dif->fdc->flop[dsk],dsc.trk, dsc.sec+1, buf, 256);

				dsc.sec++;
				if (dsc.sec > 15) {
					dsc.sec = 0;
					dsc.trk++;
				}
			}
			fillDiskCat();
			showInfo("File was copied");
			break;
	}
	free(dt);
}

void SetupWin::fillDiskCat() {
	int dsk = ui.disktabs->currentIndex();
	Floppy* flp = comp->dif->fdc->flop[dsk];
	TRFile ct[128];
	QList<TRFile> cat;
	int catSize = 0;
	if (flp->insert && (diskGetType(flp) == DISK_TYPE_TRD)) {
		catSize = diskGetTRCatalog(flp, ct);
		for(int i = 0; i < catSize; i++) {
			if (ct[i].name[0] > 0x1f)
				cat.append(ct[i]);
		}
	}
	ui.disklist->setCatalog(cat);
}

// video

void SetupWin::chabsz() {ui.bszlab->setText(QString("%0%").arg(ui.bszsld->value()));}
void SetupWin::chaflc() {ui.labNoflic->setText(QString("%0%").arg(ui.sldNoflic->value() * 2));}

void SetupWin::selsspath() {
	QString fpath = QFileDialog::getExistingDirectory(this,"Screenshots folder",QString::fromLocal8Bit(conf.scrShot.dir.c_str()),QFileDialog::ShowDirsOnly);
	if (fpath!="") ui.pathle->setText(fpath);
}

// sound

/*
void SetupWin::updvolumes() {
	ui.bvlab->setText(QString::number(ui.bvsld->value()));
	ui.tvlab->setText(QString::number(ui.tvsld->value()));
	ui.avlab->setText(QString::number(ui.avsld->value()));
	ui.gslab->setText(QString::number(ui.gvsld->value()));
}
*/

// disk

void SetupWin::newdisk(int idx) {
	Floppy *flp = comp->dif->fdc->flop[idx];
	if (!saveChangedDisk(comp,idx & 3)) return;
	diskFormat(flp);
	flp->path = (char*)realloc(flp->path,sizeof(char));
	flp->path[0] = 0x00;
	flp->insert = 1;
	flp->changed = 1;
	updatedisknams();
}

void SetupWin::newa() {newdisk(0);}
void SetupWin::newb() {newdisk(1);}
void SetupWin::newc() {newdisk(2);}
void SetupWin::newd() {newdisk(3);}

void SetupWin::loada() {loadFile(comp,"",FT_DISK,0); updatedisknams();}
void SetupWin::loadb() {loadFile(comp,"",FT_DISK,1); updatedisknams();}
void SetupWin::loadc() {loadFile(comp,"",FT_DISK,2); updatedisknams();}
void SetupWin::loadd() {loadFile(comp,"",FT_DISK,3); updatedisknams();}

void SetupWin::savea() {Floppy* flp = comp->dif->fdc->flop[0]; if (flp->insert) saveFile(comp,flp->path,FT_DISK,0);}
void SetupWin::saveb() {Floppy* flp = comp->dif->fdc->flop[1]; if (flp->insert) saveFile(comp,flp->path,FT_DISK,1);}
void SetupWin::savec() {Floppy* flp = comp->dif->fdc->flop[2]; if (flp->insert) saveFile(comp,flp->path,FT_DISK,2);}
void SetupWin::saved() {Floppy* flp = comp->dif->fdc->flop[3]; if (flp->insert) saveFile(comp,flp->path,FT_DISK,3);}

void SetupWin::ejcta() {saveChangedDisk(comp,0); flpEject(comp->dif->fdc->flop[0]); updatedisknams();}
void SetupWin::ejctb() {saveChangedDisk(comp,1); flpEject(comp->dif->fdc->flop[1]); updatedisknams();}
void SetupWin::ejctc() {saveChangedDisk(comp,2); flpEject(comp->dif->fdc->flop[2]); updatedisknams();}
void SetupWin::ejctd() {saveChangedDisk(comp,3); flpEject(comp->dif->fdc->flop[3]); updatedisknams();}

void SetupWin::updatedisknams() {
	ui.apathle->setText(QString::fromLocal8Bit(comp->dif->fdc->flop[0]->path));
	ui.bpathle->setText(QString::fromLocal8Bit(comp->dif->fdc->flop[1]->path));
	ui.cpathle->setText(QString::fromLocal8Bit(comp->dif->fdc->flop[2]->path));
	ui.dpathle->setText(QString::fromLocal8Bit(comp->dif->fdc->flop[3]->path));
	fillDiskCat();
}

// tape

void SetupWin::loatape() {
	loadFile(comp,"",FT_TAPE,1);
	ui.tpathle->setText(QString::fromLocal8Bit(comp->tape->path));
	buildtapelist();
}

void SetupWin::savtape() {
	if (comp->tape->blkCount != 0) saveFile(comp,comp->tape->path,FT_TAP,-1);
}

void SetupWin::ejctape() {
	tapEject(comp->tape);
	ui.tpathle->setText(QString::fromLocal8Bit(comp->tape->path));
	buildtapelist();
}

void SetupWin::tblkup() {
	int ps = ui.tapelist->currentIndex().row();
	if (ps > 0) {
		tapSwapBlocks(comp->tape,ps,ps-1);
		buildtapelist();
		ui.tapelist->selectRow(ps-1);
	}
}

void SetupWin::tblkdn() {
	int ps = ui.tapelist->currentIndex().row();
	if ((ps != -1) && (ps < comp->tape->blkCount - 1)) {
		tapSwapBlocks(comp->tape,ps,ps+1);
		buildtapelist();
		ui.tapelist->selectRow(ps+1);
	}
}

void SetupWin::tblkrm() {
	int ps = ui.tapelist->currentIndex().row();
	if (ps != -1) {
		tapDelBlock(comp->tape,ps);
		buildtapelist();
//		ui.tapelist->selectRow(ps);
	}
}

void SetupWin::chablock(QModelIndex idx) {
	int row = idx.row();
	tapRewind(comp->tape,row);
	buildtapelist();
//	ui.tapelist->selectRow(row);
}

void SetupWin::tlistclick(QModelIndex idx) {
	int row = idx.row();
	int col = idx.column();
	if ((row < 0) || (row >= comp->tape->blkCount)) return;
	if (col != 1) return;
	comp->tape->blkData[row].breakPoint ^= 1;
	buildtapelist();
//	ui.tapelist->selectRow(row);
}

/*
void SetupWin::setTapeBreak(int row,int col) {
	if ((row < 0) || (col != 1)) return;
	comp->tape->blkData[row].breakPoint ^= 1;
	buildtapelist();
	ui.tapelist->selectRow(row);
}
*/

// hdd

void SetupWin::hddMasterImg() {
	QString path = QFileDialog::getOpenFileName(this,"Image for master HDD","","All files (*.*)",NULL,QFileDialog::DontConfirmOverwrite);
	if (path != "") ui.hm_path->setText(path);
}

void SetupWin::hddSlaveImg() {
	QString path = QFileDialog::getOpenFileName(this,"Image for slave HDD","","All files (*.*)",NULL,QFileDialog::DontConfirmOverwrite);
	if (path != "") ui.hs_path->setText(path);
}

void SetupWin::hddcap() {
	unsigned int sz;
	if (ui.hs_islba->isChecked()) {
		sz = (ui.hs_glba->value() >> 11);
	} else {
		sz = ((ui.hs_gsec->value() * (ui.hs_ghd->value() + 1) * (ui.hs_gcyl->value() + 1)) >> 11);
	}
	ui.hs_capacity->setValue(sz);
}

// external

void SetupWin::selSDCimg() {
	QString fnam = QFileDialog::getOpenFileName(this,"Image for SD card","","All files (*.*)");
	if (!fnam.isEmpty()) ui.sdPath->setText(fnam);
}

void SetupWin::openSlot() {
	QString fnam = QFileDialog::getOpenFileName(this,"MSX cartridge A","","MSX cartridge (*.rom)");
	if (fnam.isEmpty()) return;
	ui.cSlotName->setText(fnam);
	loadFile(comp, fnam.toLocal8Bit().data(), FT_SLOT_A, 0);
}

int testSlotOn(Computer*);

void SetupWin::ejectSlot() {
	sltEject(comp->slot);
	ui.cSlotName->clear();
	if (testSlotOn(comp))
		compReset(comp,RES_DEFAULT);
}

// input

void SetupWin::newPadMap() {
	QString nam = QInputDialog::getText(this,"Enter...","New gamepad map name");
	if (nam.isEmpty()) return;
	nam.append(".pad");
	std::string name = nam.toStdString();
	if (padCreate(name)) {
		ui.cbPadMap->addItem(nam, nam);
		ui.cbPadMap->setCurrentIndex(ui.cbPadMap->count() - 1);
	} else {
		showInfo("Map with that name already exists");
	}
}

void SetupWin::delPadMap() {
	if (ui.cbPadMap->currentIndex() == 0) return;
	QString name = getRFSData(ui.cbPadMap);
	if (name.isEmpty()) return;
	if (!areSure("Delete this map?")) return;
	padDelete(name.toStdString());
	ui.cbPadMap->removeItem(ui.cbPadMap->currentIndex());
	ui.cbPadMap->setCurrentIndex(0);
}

void SetupWin::chaPadMap(int idx) {
	idx--;
	if (idx < 0) {
		conf.joy.map.clear();
	} else {
		padLoadConfig(getRFSData(ui.cbPadMap).toStdString());
	}
	padModel->update();
}

void SetupWin::addBinding() {
	if (getRFSData(ui.cbPadMap).isEmpty()) return;
	bindidx = -1;
	xJoyMapEntry jent;
	jent.dev = JOY_NONE;
	jent.dev = JMAP_JOY;
	jent.key = ENDKEY;
	jent.dir = XJ_NONE;
	padial->start(jent);
}

void SetupWin::editBinding() {
	bindidx = ui.tvPadTable->currentIndex().row();
	if (bindidx < 0) return;
	padial->start(conf.joy.map[bindidx]);
}

void SetupWin::bindAccept(xJoyMapEntry ent) {
	if (ent.type == JOY_NONE) return;
	if (ent.dev == JMAP_NONE) return;
	if ((bindidx < 0) || (bindidx >= (int)conf.joy.map.size())) {
		conf.joy.map.push_back(ent);
	} else {
		conf.joy.map[bindidx] = ent;
	}
	padSaveConfig(getRFSData(ui.cbPadMap).toStdString());
	padModel->update();
}

void SetupWin::delBinding() {
	int row = ui.tvPadTable->currentIndex().row();
	if (row < 0) return;
	if (!areSure("Delete this binding?")) return;
	conf.joy.map.erase(conf.joy.map.begin() + row);
	padModel->update();
	padSaveConfig(getRFSData(ui.cbPadMap).toStdString());
}

// tools

void SetupWin::umup() {
	int ps = ui.umlist->currentRow();
	if (ps > 0) {
		swapBookmarks(ps,ps-1);
		buildmenulist();
		ui.umlist->selectRow(ps-1);
	}
}

void SetupWin::umdn() {
	int ps = ui.umlist->currentIndex().row();
	if ((ps != -1) && (ps < (int)conf.bookmarkList.size() - 1)) {
		swapBookmarks(ps, ps+1);
		buildmenulist();
		ui.umlist->selectRow(ps+1);
	}
}

void SetupWin::umdel() {
	int ps = ui.umlist->currentIndex().row();
	if (ps != -1) {
		delBookmark(ps);
		buildmenulist();
		if (ps == (int)conf.bookmarkList.size()) {
			ui.umlist->selectRow(ps-1);
		} else {
			ui.umlist->selectRow(ps);
		}
	}
}

void SetupWin::umadd() {
	uia.namele->clear();
	uia.pathle->clear();
	umidx = -1;
	umadial->show();
}

void SetupWin::umedit(QModelIndex idx) {
	umidx = idx.row();
	uia.namele->setText(ui.umlist->item(umidx,0)->text());
	uia.pathle->setText(ui.umlist->item(umidx,1)->text());
	umadial->show();
}

void SetupWin::umaselp() {
	QString fpath = QFileDialog::getOpenFileName(NULL,"Select file","","Known formats (*.sna *.z80 *.tap *.tzx *.trd *.scl *.fdi *.udi)");
	if (fpath!="") uia.pathle->setText(fpath);
}

void SetupWin::umaconf() {
	if ((uia.namele->text()=="") || (uia.pathle->text()=="")) return;
	if (umidx == -1) {
		addBookmark(std::string(uia.namele->text().toLocal8Bit().data()),std::string(uia.pathle->text().toLocal8Bit().data()));
	} else {
		setBookmark(umidx,std::string(uia.namele->text().toLocal8Bit().data()),std::string(uia.pathle->text().toLocal8Bit().data()));
	}
	umadial->hide();
	buildmenulist();
	ui.umlist->selectRow(ui.umlist->rowCount()-1);
}

// profiles

void SetupWin::newProfile() {
	QString nam = QInputDialog::getText(this,"Enter...","New profile name");
	if (nam.isEmpty()) return;
	std::string nm = std::string(nam.toLocal8Bit().data());
	std::string fp = nm + ".conf";
	if (!addProfile(nm,fp))
		shitHappens("Can't add such profile");
	buildproflist();
}

void SetupWin::rmProfile() {
	int idx = ui.twProfileList->currentRow();
	if (idx < 0) return;
	block = 1;
	if (areSure("Do you really want to delete this profile?")) {
		std::string pnam(ui.twProfileList->item(idx,0)->text().toLocal8Bit().data());
		idx = delProfile(pnam);
		switch(idx) {
			case DELP_OK_CURR:
				prfChanged = 1;
				start(conf.prof.cur);
				break;
			case DELP_ERR:
				shitHappens("Sorry, i can't delete this profile");
				break;
		}
	}
	block = 0;
	buildproflist();
}
