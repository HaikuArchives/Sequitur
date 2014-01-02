/* SeqPhrasePropertyWindow.h
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
 * 2001.03.07		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQPHRASEPROPERTYWINDOW_H
#define SEQUITUR_SEQPHRASEPROPERTYWINDOW_H

#include <interface/Button.h>
#include <interface/ColorControl.h>
#include <interface/MenuField.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <interface/Window.h>
#include "AmKernel/AmPhrase.h"
#include "Sequitur/SeqWindowStateI.h"
class AmPhraseEvent;
class AmTimeView;

/*************************************************************************
 * SEQ-PHRASE-PROPERTY-WINDOW
 * This window allows users to edit all the properties for a single
 * phrase event.
 *************************************************************************/
class SeqPhrasePropertyWindow : public BWindow,
								public SeqWindowStateI
{
public:
	SeqPhrasePropertyWindow(AmSongRef songRef,
							AmPhraseEvent* event,
							const BMessage* config = NULL); 
	virtual ~SeqPhrasePropertyWindow();

	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	void				SetPhraseEvent(AmPhraseEvent* event);

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool		IsSignificant() const;
	virtual status_t	GetConfiguration(BMessage* config);
	status_t			SetConfiguration(const BMessage* config);
	
private:
	typedef BWindow		inherited;
	AmSongRef			mSongRef;
	AmPhraseEvent*		mEvent;
	
	BView*				mBg;
	BTextControl*		mNameCtrl;
	BStringView*		mTimeLabel;
	AmTimeView*			mTimeCtrl;
	BMenuField*			mColorField;
	BColorControl*		mColorCtrl;
	
	void				Refresh();
	void				RangeChanged();

	void				SetColor();
	void				GetColor();
	void				SetControls(bool enabled);
	
	void				AddViews(BRect frame);

};

#endif
