/* SeqApplication.h
 */

#ifndef SEQUITUR_SEQAPPLICATION_H
#define SEQUITUR_SEQAPPLICATION_H

#include <app/Application.h>
#include <app/Messenger.h>
#include <SelfWritten/DocApplication.h>
#include <interface/Bitmap.h>
#include <interface/Picture.h>
#include <support/Locker.h>
#include <storage/MimeType.h>

#include "AmKernel/AmGlobalsImpl.h"

/***************************************************************************
 * SEQ-APPLICATION
 ****************************************************************************/
class SeqApplication : public DocApplication
{
private:
	typedef DocApplication	inherited;
	
public:
	SeqApplication();
	virtual ~SeqApplication();

	AmGlobalsI& AmGlobals();

	virtual void	MessageReceived(BMessage* msg);
	virtual void	ArgvReceived(int32 argc, char** argv);
	virtual bool	QuitRequested(void);
	virtual void	AboutRequested(void);
	virtual	void	FileOpen();

	// Answer the default size of the bitmaps our filters return.
	// Not sure if this is the best place for this?
	BPoint FilterImageSize();
	BBitmap* DefaultFilterImage();

	bool			FlagIsOn(uint32 flags) const;

	/* If this method returns B_OK, then the supplied preferences
	 * BMessage will have been annotated with all the current
	 * system preferences.
	 */
	status_t		GetPreferences(BMessage* preferences) const;
	/* Set the current system preferences based on those in the
	 * supplied preferences BMessage.  Supplying a NULL BMessage
	 * will wipe the preferences.
	 */
	status_t		SetPreferences(const BMessage* preferences);

	/* Conveniences.  You can also use the global preference
	 * accessing functions.  There's a reason why I have these
	 * accessing methods instead of supplying the entire preference
	 * objects:  These calls are atomic, so clients don't have to
	 * mess with locking.
	 */
	status_t		GetBoolPreference(const char* name, bool* val, int32 n = 0) const;
	status_t		GetInt32Preference(const char* name, int32* val, int32 n = 0) const;
	status_t		GetMessagePreference(const char* name, BMessage* msg, int32 n = 0) const;
	status_t		GetRefPreference(const char* name, entry_ref* ref, int32 n = 0) const;
	status_t		GetStringPreference(const char* name, const char** str, int32 n = 0) const;
	status_t		GetFactoryInt32Preference(	const char* fac, const char* view,
												const char* name, int32* outI32, int32 n = 0) const;

	status_t		SetStringPreference(const char* name, const char* str, int32 n = 0);
	/* This is a hack -- used to be that, during shutdown, the app ran
	 * through all open windows and asked them to write out their config
	 * info.  Dianne changed the procedure a little so that the windows
	 * are usually gone by the time the app is asking for this info.
	 * This method lets the windows do the one thing they were doing
	 * before, which is supply an entry ref for the settings file.
	 */
	void			AddShutdownRef(const char* name, entry_ref* ref);
	void			AddShutdownMessage(const char* name, BMessage* msg);

	enum {
		PREF_WIN_INDEX				= 0,
		STUDIO_WIN_INDEX			= 1,
		MANAGE_DEVICES_WIN_INDEX	= 2,
		MANAGE_FILTERS_WIN_INDEX	= 3,
		MANAGE_MOTIONS_WIN_INDEX	= 4,
		MANAGE_TOOLS_WIN_INDEX		= 5,
		EDIT_DEVICE_WIN_INDEX		= 6,
		EDIT_MOTION_WIN_INDEX		= 7,
		EDIT_MULTIFILTER_WIN_INDEX	= 8,
		EDIT_TOOL_WIN_INDEX			= 9,

		_NUM_AUX_WIN
	};	
	/* This method is used by the auxiliary windows to persist the state
	 * of each window between invocations.
	 */
	void			SetAuxiliaryWindowSettings(uint32 index, const BMessage& settings);

private:
	BMessenger 			mAboutWin;
	BMessenger			mNewToolBarWin;
	/* This lock is used whenever the auxiliary windows --
	 * preferences, filters, or tool properties -- are storing
	 * their configuration.
	 */
	mutable BLocker 	mAuxWinSettingsLock; 
	BMessenger			mAuxWins[_NUM_AUX_WIN];
	BRect				mAuxWinInitFrames[_NUM_AUX_WIN];
	BMessage			mAuxWinSettings[_NUM_AUX_WIN];

	void				ShowAuxiliaryWindow(uint32 index, BMessage* msg);
	BWindow*			NewAuxiliaryWindow(	uint32 index, BRect frame,
											BMessage* config, const BMessage* msg) const;
	const char*			MessageName(uint32 index) const;
	void				InitRects();
	void				ShowNewToolBarWindow();
	
	BMessage			mPreferences;
	mutable BLocker 	mPrefLock; 
	AmGlobalsImpl		mAmGlobals;
	/* See AddShutdownRef();
	 */
	BMessage			mShutdownMsg;
	bool				mGrossErrorHack;
	BFilePanel			*mLoadPanel;

	/* Save the application's state (including preferences)
	 */
	status_t	SaveState();
	
	void		LoadState(BMessage* into) const;
	void		ApplyPreferences(const BMessage* from);
	void		ApplySettings(const BMessage* from);
	
	void		ReadWindowState(const BMessage* message);
	/* Answer the file in Sequitur's settings directory that has
	 * the supplied name.  If none, create it.
	 */
	BFile*		SettingsFile(	uint32 open_mode,
								const char* fileName,
								const char* mimeType = "text/plain") const;
};

/* A convenience for anyone accessing the application
 */
#define		seq_app		((SeqApplication*)be_app)


// FilePanel filter for general access

class MIDIRefFilter : public BRefFilter {
public:
	bool MIDIRefFilter::Filter(const entry_ref *r, BNode *node, struct stat_beos *,
									   const char *mimetype);
private:
	static BMimeType typeA, typeB, typeC;
};

extern MIDIRefFilter midifileFilter;

#endif
