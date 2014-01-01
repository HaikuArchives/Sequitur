/* SeqEditDeviceWindow.h
 * Copyright (c)2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.01.31		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQEDITDEVICEWINDOW_H
#define SEQUITUR_SEQEDITDEVICEWINDOW_H

#include "Sequitur/SeqEditRosterWindow.h"
class AmBankChange;
class AmDevice;
class SeqBitmapEditor;
class _AbstractBankRow;
class _BankList;
class _ControllerList;

/*************************************************************************
 * SEQ-EDIT-DEVICE-WINDOW
 * This window allows users to edit all the properties for a single
 * device.
 *************************************************************************/
class SeqEditDeviceWindow : public SeqEditRosterWindow
{
public:
	SeqEditDeviceWindow(BRect frame,
						const BMessage* config,
						const BMessage* deviceMsg);
	virtual ~SeqEditDeviceWindow();

	virtual	void		MenusBeginning();
	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	void				SetDevice(const BMessage* deviceMsg);
	void				SetDevice(const BString& key, const BString& path);

protected:
	virtual uint32		ConfigWhat() const;
	virtual	bool		Validate();
	virtual	void		SaveChanges();
	virtual const char*	EntryName() const;

private:
	typedef SeqEditRosterWindow inherited;
	ArpRef<AmDevice>	mDevice;
	BTextControl*		mMfgCtrl;
	BTextControl*		mNameCtrl;
	BMenuField*			mBankSelectionCtrl;
	BMenuField*			mBankMenu;
	_BankList*			mBankList;
	_ControllerList*	mControllerList;
	SeqBitmapEditor*	mIconEditor;
	BMenu*				mInputFilterMenu;
	
	void				SetBankSelection(const AmBankChange* bankEvent);
	void				NewBank();
	AmBankChange*		NewBankSelection();
	void				SetWindowTitle();
	status_t			PastePatchList(_AbstractBankRow* parent, uint32 firstPatch);
	
	BView*				NewGeneralView(BRect frame);
	BView*				NewBankView(BRect frame);
	BView*				NewControllersView(BRect frame);
	BView*				NewDescriptionView(BRect frame);
	BView*				NewIconView(BRect frame);
	BView*				NewFilterView(BRect frame);
};

#endif
