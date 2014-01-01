/* SeqEditMotionWindow.h
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
 * 2001.03.12		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQEDITMOTIONWINDOW_H
#define SEQUITUR_SEQEDITMOTIONWINDOW_H

#include "Sequitur/SeqEditRosterWindow.h"
class AmMotion;
class BButton;
class BMenuItem;
class BTextControl;
class AmMotionEditor;

/*************************************************************************
 * SEQ-EDIT-MOTION-WINDOW
 * This window allows users to edit all the properties for a single
 * Motion.
 *************************************************************************/
class SeqEditMotionWindow : public SeqEditRosterWindow
{
public:
	SeqEditMotionWindow(BRect frame,
						const BMessage* config,
						const BMessage* motionMsg);
	virtual ~SeqEditMotionWindow();

	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	void				SetMotion(const BMessage* motionMsg);
	void				SetMotion(const BString& key, const BString& path);
	
protected:
	virtual uint32		ConfigWhat() const;
	virtual	bool		Validate();
	virtual	void		SaveChanges();
	virtual const char*	EntryName() const;

private:
	typedef SeqEditRosterWindow		inherited;
	AmMotion*			mMotion;
	BTextControl*		mNameCtrl;
	AmMotionEditor*		mEditor;
	
	void				SetWindowTitle();

	BView*				NewGeneralView(BRect frame);
	BView*				NewMotionView(BRect frame);
	BView*				NewDescriptionView(BRect frame);
};

#endif
