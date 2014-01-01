/* GlApp.h
 * Copyright (c)2002-2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 04.18.99		hackborn
 * Created this file.
 */
#ifndef GLASSLIKE_GLAPP_H
#define GLASSLIKE_GLAPP_H

#include <ArpCore/StlVector.h>
#include <be/app/Application.h>
#include <be/interface/Rect.h>
#include <be/interface/Window.h>
#include <be/storage/File.h>
#include <be/support/Locker.h>
#include <Glasslike/GlGlobalsImpl.h>
#include <Glasslike/GlKeyTracker.h>
#include <Glasslike/GlMidiBinding.h>
#include <Glasslike/GlSkinCache.h>
#include <Glasslike/GlToolBar.h>

/* FIX: Remove this when done with lib testing
 */
void _PREFS_INIT_TEMP();

/***************************************************************************
 * GL-APP
 ***************************************************************************/
class GlApp : public BApplication
{
public:
	GlApp();
	virtual ~GlApp();

	virtual bool			IsQuitting() const;
	virtual void			MessageReceived(BMessage *msg);
	virtual bool			QuitRequested(void);
	virtual	void			ReadyToRun();

	GlKeyTracker&			KeyTracker();
	void					SetMidiTarget(const BMessenger& target);
	void					UnsetMidiTarget(const BMessenger& target);
	GlMidiBindingList&		MidiBindings();
	GlToolBarRoster&		ToolBars();
	GlSkinCache&			Skin();
	
	enum {
		PREF_WIN_INDEX		= 0,

		_NUM_AUX_WIN
	};	
	/* This method is used by the auxiliary windows to persist the state
	 * of each window between invocations.
	 */
	void					SetAuxiliaryWindowSettings(	uint32 index,
														const BMessage& settings);
	enum {
		MIXING_WIN_INDEX	= 0,
		
		_NUM_SETTINGS
	};
	void					SetSettings(uint32 index, const BMessage& settings);
	
private:
	typedef BApplication	inherited;
	GlMidi					mMidi;
	GlKeyTracker			mKeyTracker;
	GlMidiBindingList		mMidiBindings;
	GlGlobalsImpl			mGlobalsImpl;
	GlToolBarRoster			mToolBars;
	GlSkinCache				mSkinCache;
	vector<BMessenger>		mMixingWins;
	bool					mIsQuitting;

	/* This lock is used whenever the auxiliary windows --
	 * preferences, filters, or tool properties -- are storing
	 * their configuration.
	 */
	mutable BLocker 		mSettingsLock; 
	BMessenger				mAuxWins[_NUM_AUX_WIN];
	BRect					mAuxWinInitFrames[_NUM_AUX_WIN];
	BMessage				mAuxWinSettings[_NUM_AUX_WIN];
	BMessage				mSettings[_NUM_SETTINGS];
	bool					mSettingsChanges[_NUM_SETTINGS];

	void					ShowAuxiliaryWindow(uint32 index, BMessage* msg);
	BWindow*				NewAuxiliaryWindow(	uint32 index, BRect frame,
												BMessage* config, const BMessage* msg) const;

	void					InitRects();

	status_t				SaveState();
	void					LoadState();
	BFile*					SettingsFile(	uint32 open_mode,
											const char* fileName,
											const char* mimeType = "text/plain") const;

	void					AcquireImages();
	void					ReleaseImages();
};


/* A convenience for anyone accessing the application
 */
#define		gl_app		((GlApp*)be_app)


#endif
