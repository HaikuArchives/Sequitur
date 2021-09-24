/* GlPrefWin.h
 * Copyright (c)2003 by Eric Hackborn.
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
 *	-� None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.04.27				hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef _GL_PREF_WIN_H
#define _GL_PREF_WIN_H

#include <InterfaceKit.h>
#include <SupportKit.h>
#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpFileNameControl.h>
#include <ArpInterface/ArpFloatControl.h>
#include <ArpInterface/ArpIntControl.h>
#include <ArpInterface/ArpMenuControl.h>
#include <Glasslike/GlMidi.h>
class GlMidiBindingsListView;

/***************************************************************************
 * GL-PREF-WIN
 ***************************************************************************/
class GlPrefWin : public BWindow
{
public:
	GlPrefWin(BRect frame, BMessage* config);
	virtual ~GlPrefWin();
	
	virtual void			MessageReceived(BMessage *msg);
	virtual	bool			QuitRequested();
	virtual	void			WindowActivated(bool state);

protected:
	friend class GlMidiBindingsListView;
	void					MidiSelectionChanged();

private:
	typedef BWindow 		inherited;
	// General controls
	BTextControl*			mCreator;
	ArpIntControl*			mKey;
	// Midi controls
	uint32					mMidiLearn;
	GlMidiBindingsListView*	mMidiListView;
	BButton*				mMidiLearnOne;
	BButton*				mMidiLearnMany;
	BButton*				mMidiDelete;
	ArpMenuControl*			mMidiRtMenu;
#if 1
	ArpFileNameControl*		mMidiFileName;
	ArpMenuControl*			mMidiRootMenu;
	ArpMenuControl*			mMidiParamMenu;
#endif
#if 0
	ArpFloatControl*		mMidiMinCtrl;
	ArpFloatControl*		mMidiMaxCtrl;
#endif
	std::vector<GlMidiEvent>		mDeletedBindings;

	void					OkGeneral();
	void					OkMidi();
	
	status_t				LearnMidi(uint32 what);
	status_t				ReceiveMidi(BMessage* event);
	status_t				DeleteMidi();
	status_t				MidiRtChanged();
	status_t				MidiFileChanged();
	status_t				MidiRootChanged();
	status_t				MidiParamChanged();
#if 0
	status_t				MidiRangeChanged();
#endif
	status_t				BuildRootMenu();
	status_t				BuildParamMenu();

	void					AddViews(const BMessage& prefs);
	BView*					NewGeneralView(BRect bounds, const BMessage& prefs);
	BView*					NewMidiView(BRect bounds, const BMessage& prefs);
};

#endif
