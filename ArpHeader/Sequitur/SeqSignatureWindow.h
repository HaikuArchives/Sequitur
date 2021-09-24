/* SeqSignatureWindow.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 10.17.00		hackborn
 * Created this file
 */
#ifndef SEQUITUR_SEQSIGNATUREWINDOW_H
#define SEQUITUR_SEQSIGNATUREWINDOW_H

#include <interface/Button.h>
#include <interface/CheckBox.h>
#include <interface/MenuField.h>
#include <interface/RadioButton.h>
#include <interface/StringView.h>
#include <interface/Window.h>
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmTrackRef.h"
class ArpIntControl;
class AmSignature;

/***************************************************************************
 * _SEQ-SIG-POP-UP-ENTRY
 * The data model behind a single row of the popup choices area.
 ****************************************************************************/
class _SeqSigPopUpEntry
{
public:
	_SeqSigPopUpEntry();
	_SeqSigPopUpEntry(	BRadioButton* rb,
						ArpIntControl* ic,
						BMenuField* mf,
						BStringView* s,
						float bottom);

	_SeqSigPopUpEntry&	operator=(const _SeqSigPopUpEntry &e);
	void				Remove();
	bool				IsOn() const;
	void				SetEnabled(bool enable);
	status_t			GetSignature(AmSignature& signature) const;
	void				Set(uint32 beats, uint32 beatValue);
	
	BRadioButton*	mRadio;
	ArpIntControl*	mIntCtrl;
	BMenuField*		mField;
	BStringView*	mSlash;
	float			mBottom;
};
typedef std::vector<_SeqSigPopUpEntry>		sigpopup_vec;

/***************************************************************************
 * SEQ-SIGNATURE-WINDOW
 * Allow users to change the signature for a single measure.  Works on
 * both song and track signatures.
 ****************************************************************************/
class SeqSignatureWindow : public BWindow,
						   public AmSongObserver
{
public:
	SeqSignatureWindow(AmSongRef songRef, BRect frame); 
	SeqSignatureWindow(AmSongRef songRef, AmTrackRef trackRef, BRect frame); 
	~SeqSignatureWindow();
	
	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	void				SetMeasure(int32 measure, uint32 beats = 4, uint32 beatValue = 4);

private:
	typedef BWindow		inherited;
	AmTrackRef			mTrackRef;
	BView*				mBg;
	BCheckBox*			mLockToSong;
	BButton*			mOkButton;
	BButton*			mCancelButton;
	BButton*			mMoreButton;
	BButton*			mFewerButton;
	BView*				mBottomLine;
	sigpopup_vec		mEntries;
	float				mChoicesTop;
	
	void				SetSignature();
	void				SetPreferences();
	void				AddPopUpChoice();
	void				RemovePopUpChoice();
	
	void				AddViews(bool showLockToSong);
	float				AddSigRow(	BView* toView, float top, uint32 beats, uint32 beatValues,
									bool on = false);
	/* Fill in the supplied signature with the values of the
	 * currently selected row.
	 */
	status_t			GetSignature(AmSignature& signature) const;
};

#endif
