/* SeqStudioWindow.h
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

#ifndef SEQUITUR_SEQSTUDIOWINDOW_H
#define SEQUITUR_SEQSTUDIOWINDOW_H

#include <interface/MenuField.h>
#include <interface/TextControl.h>
#include <interface/Window.h>
#include "Sequitur/SeqWindowStateI.h"

/*************************************************************************
 * SEQ-STUDIO-WINDOW
 * This window allows users to associate MIDI consumers with devices.
 *************************************************************************/
class SeqStudioWindow : public BWindow,
						public SeqWindowStateI
{
public:
	SeqStudioWindow(BRect frame,
					const BMessage* config = NULL); 
	virtual ~SeqStudioWindow();

	virtual	void		MenusBeginning();
	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();
	virtual thread_id	Run();

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool		IsSignificant() const;
	virtual status_t	GetConfiguration(BMessage* config);
	status_t			SetConfiguration(const BMessage* config);
	
private:
	typedef BWindow		inherited;
	BView*				mBg;
	BMenu*				mPortMenu;
	BTextControl*		mLabelCtrl;
	BMenuField*			mDeviceCtrl;
	
	void				ToggleColumn(const char* name);

	void				AddMainMenu(BRect frame);
	void				AddViews(BRect frame);
};

#endif
