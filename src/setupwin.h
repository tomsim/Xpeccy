#ifndef _SETWIN_H
#define _SETWIN_H

#include <QDialog>
#include <QModelIndex>
#include "xcore/xcore.h"

#include "ui_rsedit.h"
#include "ui_setupwin.h"
#include "ui_umadial.h"
#include "ui_layedit.h"

class SetupWin : public QDialog {
	Q_OBJECT
	public:
		SetupWin(QWidget*);
	private:
		xConfig* conf;
		ZXComp* comp;

		Ui::SetupWin setupUi;
		Ui::UmaDial uia;
		Ui::RSEdialog rseUi;
		Ui::LayEditor layUi;

		QDialog* rseditor;
		QDialog* layeditor;

		std::vector<VidLayout> lays;
		std::vector<RomSet> rsl;
	signals:
		void closed();
	public slots:
		void start(xConfig*, ZXComp*);
	private slots:
		void reject();
		void apply();
		void okay();
		void buildrsetlist();
		void setmszbox(int);
		void selsspath();
		void chabsz();
		void chabrg();
		void updatedisknams();
		void loada(); void loadb(); void loadc(); void loadd();
		void savea(); void saveb(); void savec(); void saved();
		void ejcta(); void ejctb(); void ejctc(); void ejctd();
		void newa(); void newb(); void newc(); void newd(); void newdisk(int);
		void loatape(); void savtape(); void ejctape();
		void tblkup(); void tblkdn(); void tblkrm();
		void hddMasterImg(); void hddSlaveImg();
		void umadd(); void umdel(); void umup(); void umdn();
		void umedit(QModelIndex);
		void umaselp(); void umaconf();
		void updvolumes();
		void updfrq();
		void chablock(QModelIndex);
		void setTapeBreak(int,int);
		void hddcap();
		void selSDCimg();
		void editrset();
		void setrpart();
		void addNewRomset();
		void rmRomset();
		void recheck_single(bool);
		void recheck_separate(bool);
		void fillDiskCat();
		void copyToTape();
		void copyToDisk();
		void diskToHobeta();
		void diskToRaw();
		void newProfile();
		void rmProfile();

		void editLayout();
		void addNewLayout();
		void delLayout();
		void layEditorChanged();
		void layEditorOK();
	private:
		QDialog *umadial;
		void buildtapelist();
		void buildmenulist();
		void buildkeylist();
		// void buildjmaplist();
		void buildproflist();
		int umidx;
};

#endif
