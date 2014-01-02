/* SeqApplication
 */
#include <algo.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <app/Roster.h>
#include <InterfaceKit.h>
#include <midi2/MidiConsumer.h>
#include <midi2/MidiRoster.h>
#include <StorageKit.h>
#include <support/Autolock.h>
#include <TranslationKit.h>
#include "ArpKernel/ArpAboutWindow.h"
#include "ArpKernel/ArpBitmapTools.h"
#include "ArpKernel/ArpBitmapWrite.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmKernel/AmDevice.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmStdFilters.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolBar.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqAboutWindow.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqEditDeviceWindow.h"
#include "Sequitur/SeqEditMotionWindow.h"
#include "Sequitur/SeqEditMultiFilterWindow.h"
#include "Sequitur/SeqEditToolWindow.h"
#include "Sequitur/SeqFilterAddOnWindow.h"
#include "Sequitur/SeqImageManager.h"
#include "Sequitur/SeqManageRosterWindows.h"
//#include "Sequitur/SeqMessageWrapper.h"
#include "Sequitur/SeqPreferences.h"
#include "Sequitur/SeqPrefWin.h"
#include "Sequitur/SeqSongWindow.h"
#include "Sequitur/SeqStudioWindow.h"
#include "Sequitur/SeqTempoViewFactory.h"
#include "AmStdFactory/AmStdViewFactory.h"
#include "AmKernel/AmFileRosters.h"
#include "AmPublic/AmGlobalsI.h"

#include "SearchPath.h"

static const char*	app_signature	= "application/x-vnd.Arp-sequitur";

static BBitmap*	defaultFilterImage = 0;

static SeqImageManager		im;
static SeqPreferences		pr;

// This directory will be located in the user settings directory.
static const char*	ARPSETTINGSDIR_STR		= "AngryRedPlanet";
// This directory will be located in ARPSETTINGSDIR_STR.
static const char*	SEQUITURSETTINGSDIR_STR	= "Sequitur";

static const char*	VERSION_STR				= "version";
static const char*	VERSION_NUMBER_STR		= "2.0";
static const char*	SETTINGS_FILE_STR		= "settings";
static const char*	STUDIO_FILE_STR			= "studio";
static const char*	INITIAL_SKIN_STR		= "Plate Metal";

static const char*	WINDOWSETTINGS_STR		= "window_settings";
static const char*	PREFERENCES_STR			= "preferences";
static const char*	AM_GLOBALS_STR			= "am_globals";
static const char*	OPEN_DOCUMENT_STR		= "open_document";

/*----------------------------------------------------------------*/
/* Implementation for SequiturDefs.h
 */
static BResourceSet* gResources = NULL;
static int32 gInitResources = 0;
static BLocker gSkinAccess;
static BSearchPath gSkinPath;
static BString gSkinFile;

BResourceSet& Resources()
{
	if (!gResources) gResources = new BResourceSet();
	if (atomic_or(&gInitResources, 1) == 0) {
		//gResources->AddResources((*void)Resources);
		if (gSkinFile.Length() > 0) {
			BAutolock _l(gSkinAccess);
			for (int32 i=0; i<gSkinPath.CountDirectories(); i++) {
				entry_ref ref;
				gSkinPath.DirectoryAt(i, &ref);
				BPath path(&ref);
				if (path.InitCheck() == B_OK) path.Append(gSkinFile.String());
				if (path.InitCheck() == B_OK) {
					BFile file(path.Path(), B_READ_ONLY);
					if (file.InitCheck() == B_OK) {
						BResources* r = new BResources;
						if (r->SetTo(&file) == B_OK) {
							gResources->AddResources(r);
							break;
						} else {
							delete r;
						}
					}
				}
			}
		}
		atomic_or(&gInitResources, 2);
	} else {
		while ((gInitResources&2) == 0) snooze(20000);
	}
	return *gResources;
}

status_t seq_make_skin_menu(BMenu* into, BMessage* baseMsg)
{
	BAutolock _l(gSkinAccess);
	gSkinPath.Rewind();
	entry_ref ref;
	while (gSkinPath.GetNextRef(&ref) == B_OK) {
		if (ref.name && *ref.name) {
			BMessage* msg = new BMessage(*baseMsg);
			msg->AddString("seq:skin", ref.name);
			BMenuItem* it = new BMenuItem(ref.name, msg);
			if (gSkinFile == ref.name) it->SetMarked(true);
			into->AddItem(it);
		}
	}
	return B_OK;
}

/*static
DocWindow *myfactory(entry_ref *ref, const char *title,
		window_look look, window_feel feel, uint32 flags, uint32 workspace)
{
	return new SeqSongWindow(wr, ref, title, look, feel, flags, workspace);
}*/

static bool		gIsQuitting = false;

bool seq_is_quitting()
{
	return gIsQuitting;
}

void seq_set_quitting(bool quitting)
{
	gIsQuitting = quitting;
}

uint32 seq_count_significant_windows()
{
	BWindow*	win;
	uint32		count = 0;
	for( int32 k = 0; (win = be_app->WindowAt(k)); k++ ) {
		SeqWindowStateI*	state = dynamic_cast<SeqWindowStateI*>(win);
		if( state && state->IsSignificant() ) count++;
	}
	return count;
}

bool seq_flag_is_on(uint32 flags)
{
	return seq_app->FlagIsOn(flags);
}

status_t seq_get_bool_preference(const char* name, bool* val, int32 n)
{
	return seq_app->GetBoolPreference(name, val, n);
}

status_t seq_get_int32_preference(const char* name, int32* val, int32 n)
{
	return seq_app->GetInt32Preference(name, val, n);
}

status_t seq_get_message_preference(const char* name, BMessage* msg, int32 n)
{
	return seq_app->GetMessagePreference(name, msg, n);
}

status_t seq_get_ref_preference(const char* name, entry_ref* ref, int32 n)
{
	return seq_app->GetRefPreference(name, ref, n);
}

status_t seq_get_string_preference(const char* name, const char** str, int32 n)
{
	return seq_app->GetStringPreference(name, str, n);
}

status_t seq_get_factory_int32_preference(	const char* fac, const char* view,
											const char* name, int32* outI32, int32 n)
{
	return seq_app->GetFactoryInt32Preference(fac, view, name, outI32, n);
}

status_t seq_set_string_preference(const char* name, const char* str, int32 n)
{
	return seq_app->SetStringPreference(name, str, n);
}

static BPath gAppPath;
status_t seq_get_app_path(BPath* path)
{
	*path = gAppPath;
	return B_OK;
}

static status_t verify_directory(const BString& pathStr)
{
	BPath		path( pathStr.String() );
	BPath		parent;
	status_t	err = path.InitCheck();
	if (err != B_OK) return err;
	if ((err = path.GetParent(&parent)) != B_OK) return err;
	BDirectory	dir( parent.Path() );
	if ((err = dir.InitCheck()) != B_OK) return err;
	if (dir.Contains(path.Leaf(), B_DIRECTORY_NODE) == true) return B_OK;
	return dir.CreateDirectory(path.Leaf(), 0);
}

/*************************************************************************
 * _ENTER-STRING-WIN
 * This window lets users type in a string.
 *************************************************************************/
class _EnterStringWin : public BWindow
{
public:
	_EnterStringWin(const BString& label, const BString& initText);

	void				MessageReceived(BMessage* msg);

private:
	typedef BWindow		inherited;
	BView*				mBg;
	BTextControl*		mTextCtrl;

	void				HandleOk();
	void				AddViews(const BString& label, const BString& initText);
};

/***************************************************************************
 * SEQ-APPLICATION
 ****************************************************************************/
SeqApplication::SeqApplication()
		: inherited(app_signature),
		  mGrossErrorHack(false)
{
	AM_LOG("SeqApplication::SeqApplication() 1\n");	
	/* The preferences must be loaded in FIRST THING.  Anything else --
	 * even starting up the status -- will cause the Resources() to be
	 * loaded without the current skin.
	 */
	BMessage		state;
	LoadState(&state);
	ApplyPreferences(&state);
	/* If the state file has no previous version, then default to my
	 * nice-looking Plate Metal skin.  The Plate Metal skin contains
	 * a lot of extranneous graphics that should not be in the default
	 * skin, because no subsequent skins could remove them; you'd have
	 * to edit the app's resource file.
	 */
	const char*		version;
	if (state.FindString(VERSION_STR, &version) != B_OK) {
		mPreferences.RemoveName(CURRENT_SKIN_PREF);
		mPreferences.AddString(CURRENT_SKIN_PREF, INITIAL_SKIN_STR);
		gSkinFile = INITIAL_SKIN_STR;
	}
	
	gSkinPath.AddSearchPath("%A/Skins");
	gSkinPath.AddDirectory(B_USER_SETTINGS_DIRECTORY, "AngryRedPlanet/Sequitur/Skins");

	am_set_startup_status_func(report_startup_status);
	am_report_startup_status("Starting up...");
	/* Construct the AngryRedPlanet/Sequitur/ directory in the config/settings/,
	 * as well as AngryRedPlanet/Sequitur/Skins/
	 */
	BPath		path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() == B_OK) {
		BString		dir(path.Path());
		dir << "/" << ARPSETTINGSDIR_STR;
		if (verify_directory(dir) == B_OK) {
			dir << "/" << SEQUITURSETTINGSDIR_STR;
			if (verify_directory(dir) == B_OK) {
				dir << "/" << "Skins";
				verify_directory(dir);
			}
		}
	}

	InitRects();
	app_info ainfo;
	if (GetAppInfo(&ainfo) == B_OK) {
		BEntry entry(&ainfo.ref,true);
		if (entry.InitCheck() == B_OK) {
			if (entry.GetPath(&gAppPath) == B_OK) {
				gAppPath.GetParent(&gAppPath);
			}
		}
	}

	am_report_startup_status("Loading UI...");
	pr.Initialize();
	
	/* Load the studio.
	 */
	am_report_startup_status("Loading studio...");
	BFile*			file;
	file = SettingsFile(B_READ_ONLY, STUDIO_FILE_STR);
	if (file) {
		mAmGlobals.Studio().Read(file);
		delete file;
	}

	am_report_startup_status("Initializing Devices...");
	AmDeviceRoster*		devices = AmDeviceRoster::Default();
	if (devices) ;
	am_report_startup_status("Initializing Motions...");
	AmMotionRoster*		motions = AmMotionRoster::Default();
	if (motions) ;
	am_report_startup_status("Initializing Tools...");
	AmToolRoster*		tools = AmToolRoster::Default();
	if (tools) ;
	
	am_report_startup_status("Initializing globals...");
	mAmGlobals.Initialize();
	defaultFilterImage = const_cast<BBitmap*>( im.FindBitmap(FILTER_BG_0) );

	am_report_startup_status("Creating view factories...");
	AmViewFactory*	fact = new AmStdViewFactory();
	if (fact) mAmGlobals.AddFactory(fact);
	fact = new SeqTempoViewFactory();
	if (fact) mAmGlobals.AddFactory(fact);

	ApplySettings(&state);
}

SeqApplication::~SeqApplication()
{
	AM_LOG("\nSeqApplication::~SeqApplication() 1\n");	
	SaveState();
	
	AM_LOG("SeqApplication::~SeqApplication() 2\n");	
	BWindow*	win;
	while ( (win = WindowAt(0)) ) {
		BMessenger m(win);
		BMessage reply;
		m.SendMessage(B_QUIT_REQUESTED, &reply);
	}

	AM_LOG("SeqApplication::~SeqApplication() 3\n");	
	AmToolRoster::ShutdownDefault();
	AM_LOG("SeqApplication::~SeqApplication() 4\n");	
	AmMotionRoster::ShutdownDefault();
	AM_LOG("SeqApplication::~SeqApplication() 5\n");	
	AmDeviceRoster::ShutdownDefault();
	AM_LOG("SeqApplication::~SeqApplication() 6\n");	
	
	im.Shutdown();
	AM_LOG("SeqApplication::~SeqApplication() 7\n");	
	delete gResources;
	AM_LOG("SeqApplication::~SeqApplication() 8\n");	
	gResources = NULL;
	mAmGlobals.Shutdown();
	AM_LOG("SeqApplication::~SeqApplication() 9\n");
	AmFilterRoster::ShutdownDefault(true);
	AM_LOG("SeqApplication::~SeqApplication() 10 (end)\n");
}

BBitmap* SeqApplication::DefaultFilterImage()
{
	return defaultFilterImage;
}

BPoint SeqApplication::FilterImageSize()
{
	return BPoint(20, 20);
}

void SeqApplication::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SHOW_PREFERENCES_MSG:
			ShowAuxiliaryWindow(PREF_WIN_INDEX, msg);
			break;
		case SHOW_FILTERS_MSG:
			ShowAuxiliaryWindow(MANAGE_FILTERS_WIN_INDEX, msg);
			break;
		case SHOW_STUDIO_MSG:
			ShowAuxiliaryWindow(STUDIO_WIN_INDEX, msg);
			break;
		case SHOW_MANAGE_DEVICES_MSG:
			ShowAuxiliaryWindow(MANAGE_DEVICES_WIN_INDEX, msg);
			break;
		case SHOW_MANAGE_MOTIONS_MSG:
			ShowAuxiliaryWindow(MANAGE_MOTIONS_WIN_INDEX, msg);
			break;
		case SHOW_MANAGE_TOOLS_MSG:
			ShowAuxiliaryWindow(MANAGE_TOOLS_WIN_INDEX, msg);
			break;
		case SHOW_EDIT_DEVICE_MSG:
			ShowAuxiliaryWindow(EDIT_DEVICE_WIN_INDEX, msg);
			break;
		case SHOW_EDIT_MOTION_MSG:
			ShowAuxiliaryWindow(EDIT_MOTION_WIN_INDEX, msg);
			break;
		case SHOW_EDIT_MULTIFILTER_MSG:
			ShowAuxiliaryWindow(EDIT_MULTIFILTER_WIN_INDEX, msg);
			break;
		case SHOW_EDIT_TOOL_MSG:
			ShowAuxiliaryWindow(EDIT_TOOL_WIN_INDEX, msg);
			break;
		case SHOW_NEW_TOOL_BAR_MSG:
			ShowNewToolBarWindow();
			break;
		case 'grhk':
			mGrossErrorHack = false;
			break;
			
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void SeqApplication::ArgvReceived(int32 argc, char** argv)
{
	inherited::ArgvReceived(argc, argv);
	ArpParseDBOpts(argc, argv);
}

bool SeqApplication::QuitRequested(void)
{
	if (mGrossErrorHack) return false;
	
	seq_set_quitting(true);
//	SaveState();
	bool	answer = inherited::QuitRequested();
	if (answer == true) {
		if( mAboutWin.IsValid() )			mAboutWin.SendMessage(B_QUIT_REQUESTED);
		for (int32 k = 0; k < _NUM_AUX_WIN; k++) {
			if (mAuxWins[k].IsValid() )	mAuxWins[k].SendMessage(B_QUIT_REQUESTED);
		}
//		if( mFiltersWin.IsValid() )			mFiltersWin.SendMessage(B_QUIT_REQUESTED);
//		if( mToolPropertiesWin.IsValid() )	mToolPropertiesWin.SendMessage(B_QUIT_REQUESTED);
	} else {
		seq_set_quitting(false);
	}
	return answer;
}

void SeqApplication::AboutRequested(void)
{
	if( !mAboutWin.IsValid() ) {
		BWindow*	win = new SeqAboutWindow();
		mAboutWin = BMessenger(win);
		win->Show();
	}
}

bool SeqApplication::FlagIsOn(uint32 flags) const
{
	if (flags&SEQ_DEVICE_WINDOW_ACTIVE) {
		if (!mAuxWins[MANAGE_DEVICES_WIN_INDEX].IsValid() ) return false;
	}
	if (flags&SEQ_FILTER_WINDOW_ACTIVE) {
		if (!mAuxWins[MANAGE_FILTERS_WIN_INDEX].IsValid() ) return false;
	}
	if (flags&SEQ_MOTION_WINDOW_ACTIVE) {
		if (!mAuxWins[MANAGE_MOTIONS_WIN_INDEX].IsValid() ) return false;
	}
	if (flags&SEQ_TOOL_WINDOW_ACTIVE) {
		if (!mAuxWins[MANAGE_TOOLS_WIN_INDEX].IsValid() ) return false;
	}
	return true;
}

status_t SeqApplication::GetPreferences(BMessage* preferences) const
{
	ArpASSERT(preferences);
	BAutolock l(&mPrefLock);
	if (!preferences) return B_ERROR;
	(*preferences) = mPreferences;
	return B_OK;
}

status_t SeqApplication::SetPreferences(const BMessage* preferences)
{
	BAutolock l(&mPrefLock);
	mPreferences.MakeEmpty();
	if (!preferences) return B_OK;
	mPreferences = *preferences;
	return B_OK;
}

status_t SeqApplication::GetBoolPreference(const char* name, bool* val, int32 n) const
{
	BAutolock l(&mPrefLock);
	return mPreferences.FindBool(name, n, val);
}

status_t SeqApplication::GetInt32Preference(const char* name, int32* val, int32 n) const
{
	BAutolock l(&mPrefLock);
	return mPreferences.FindInt32(name, n, val);
}

status_t SeqApplication::GetMessagePreference(const char* name, BMessage* msg, int32 n) const
{
	BAutolock l(&mPrefLock);
	return mPreferences.FindMessage(name, n, msg);
}

status_t SeqApplication::GetRefPreference(const char* name, entry_ref* ref, int32 n) const
{
	BAutolock l(&mPrefLock);
	return mPreferences.FindRef(name, n, ref);
}

status_t SeqApplication::GetStringPreference(const char* name, const char** str, int32 n) const
{
	BAutolock l(&mPrefLock);
	return mPreferences.FindString(name, n, str);
}

status_t SeqApplication::GetFactoryInt32Preference(	const char* fac, const char* view,
													const char* name, int32* outI32, int32 n) const
{
	BAutolock l(&mPrefLock);
	AmFactoryMessageWrapper	wrap(const_cast<BMessage*>(&mPreferences));
	return wrap.GetInt32Preference(fac, view, name, outI32, n);
}

status_t SeqApplication::SetStringPreference(const char* name, const char* str, int32 n)
{
	BAutolock l(&mPrefLock);
	if (mPreferences.HasString(name, n) ) return mPreferences.ReplaceString(name, n, str);
	else return mPreferences.AddString(name, str);
}

void SeqApplication::SetAuxiliaryWindowSettings(uint32 index, const BMessage& settings)
{
	BAutolock l(&mAuxWinSettingsLock);
	ArpASSERT(index < _NUM_AUX_WIN);
	mAuxWinSettings[index].MakeEmpty();
	mAuxWinSettings[index] = settings;
}

void SeqApplication::AddShutdownRef(const char* name, entry_ref* ref)
{
	BAutolock l(&mPrefLock);
	mShutdownMsg.AddRef(name, ref);
}

void SeqApplication::AddShutdownMessage(const char* name, BMessage* msg)
{
	BAutolock l(&mPrefLock);
	mShutdownMsg.AddMessage(name, msg);
}

void SeqApplication::ShowAuxiliaryWindow(uint32 index, BMessage* msg)
{
	if (!mAuxWins[index].IsValid() ) {
		BRect			frame = mAuxWinInitFrames[index];
		BAutolock 		l(&mAuxWinSettingsLock);
		BMessage*		m = ( mAuxWinSettings[index].IsEmpty() ) ? NULL : &mAuxWinSettings[index];
		BWindow* 		win = NewAuxiliaryWindow(index, frame, m, msg);
		if (win) {
			mAuxWins[index] = BMessenger(win);
			win->Show();
		}
	} else {
		BHandler*		target;
		BLooper*		looper;
		if ( (target = mAuxWins[index].Target(&looper)) != NULL) {
			BWindow*	win = dynamic_cast<BWindow*>(target);
			if (win) {
				win->PostMessage(msg);
				win->Activate(true);
			}
		}
	}
}

BWindow* SeqApplication::NewAuxiliaryWindow(uint32 index, BRect frame,
											BMessage* config, const BMessage* msg) const
{
	if (index == PREF_WIN_INDEX) return new SeqPrefWin(frame, config);
	else if (index == STUDIO_WIN_INDEX) return new SeqStudioWindow(frame, config);
	else if (index == MANAGE_DEVICES_WIN_INDEX) return new SeqManageDevicesWindow(frame, config);
	else if (index == MANAGE_FILTERS_WIN_INDEX) return new SeqFilterAddOnWindow(frame, config);
	else if (index == MANAGE_MOTIONS_WIN_INDEX) return new SeqManageMotionsWindow(frame, config);
	else if (index == MANAGE_TOOLS_WIN_INDEX) return new SeqManageToolsWindow(frame, config);
	else if (index == EDIT_DEVICE_WIN_INDEX) return new SeqEditDeviceWindow(frame, config, msg);
	else if (index == EDIT_MOTION_WIN_INDEX) return new SeqEditMotionWindow(frame, config, msg);
	else if (index == EDIT_MULTIFILTER_WIN_INDEX) return new SeqEditMultiFilterWindow(frame, config, msg);
	else if (index == EDIT_TOOL_WIN_INDEX) return new SeqEditToolWindow(frame, config, msg);
	return NULL;
}

const char* SeqApplication::MessageName(uint32 index) const
{
	if (index == PREF_WIN_INDEX) return "pref_win_settings";
	else if (index == STUDIO_WIN_INDEX) return "studio_win_settings";
	else if (index == MANAGE_DEVICES_WIN_INDEX) return "manage_dev_win_settings";
	else if (index == MANAGE_FILTERS_WIN_INDEX) return "filt_win_settings";
	else if (index == MANAGE_MOTIONS_WIN_INDEX) return "manage_motions_win_settings";
	else if (index == MANAGE_TOOLS_WIN_INDEX) return "manage_tools_win_settings";
	else if (index == EDIT_DEVICE_WIN_INDEX) return "edit_device_win_settings";
	else if (index == EDIT_MOTION_WIN_INDEX) return "edit_motion_win_settings";
	else if (index == EDIT_MULTIFILTER_WIN_INDEX) return "edit_multifilter_win_settings";
	else if (index == EDIT_TOOL_WIN_INDEX) return "edit_tool_win_settings";
	return NULL;
}

void SeqApplication::InitRects()
{
	BScreen		s;
	float		w = s.Frame().Width(), h = s.Frame().Height();
	
	mAuxWinInitFrames[PREF_WIN_INDEX]				= BRect(w * 0.25, h * 0.25, w * 0.75, h * 0.75);
	mAuxWinInitFrames[STUDIO_WIN_INDEX]				= BRect(w * 0.30, h * 0.25, w * 0.70, h * 0.75);
	mAuxWinInitFrames[MANAGE_DEVICES_WIN_INDEX]		= BRect(w * 0.04, h * 0.05, w * 0.34, h * 0.6);
	mAuxWinInitFrames[MANAGE_FILTERS_WIN_INDEX]		= BRect(w * 0.08, h * 0.1, w * 0.38, h * 0.7);
	mAuxWinInitFrames[MANAGE_MOTIONS_WIN_INDEX]		= BRect(w * 0.12, h * 0.15, w * 0.42, h * 0.8);
	mAuxWinInitFrames[MANAGE_TOOLS_WIN_INDEX]		= BRect(w * 0.16, h * 0.2, w * 0.46, h * 0.9);
	mAuxWinInitFrames[EDIT_DEVICE_WIN_INDEX]		= BRect(w * 0.25, h * 0.25, w * 0.75, h * 0.75);
	mAuxWinInitFrames[EDIT_MOTION_WIN_INDEX]		= BRect(w * 0.25, h * 0.25, w * 0.75, h * 0.75);
	mAuxWinInitFrames[EDIT_MULTIFILTER_WIN_INDEX]	= BRect(w * 0.25, h * 0.25, w * 0.75, h * 0.75);
	mAuxWinInitFrames[EDIT_TOOL_WIN_INDEX]			= BRect(w * 0.25, h * 0.25, w * 0.75, h * 0.75);
}

void SeqApplication::ShowNewToolBarWindow()
{
	if (!mNewToolBarWin.IsValid() ) {
		BWindow* 		win = new _EnterStringWin("New tool bar name:", "New tool bar");
		if (win) {
			mNewToolBarWin = BMessenger(win);
			win->Show();
		}
	} else {
		BHandler*		target;
		BLooper*		looper;
		if ( (target = mNewToolBarWin.Target(&looper)) != NULL) {
			BWindow*	win = dynamic_cast<BWindow*>(target);
			if (win) win->Activate(true);
		}
	}
}

status_t SeqApplication::SaveState()
{
	BMessage	mainSettings(mShutdownMsg);
	mainSettings.AddMessage(PREFERENCES_STR, &mPreferences);
	mainSettings.RemoveName(VERSION_STR);
	mainSettings.AddString(VERSION_STR, VERSION_NUMBER_STR);
	BMessage	globalsMsg;
	if (mAmGlobals.WriteTo(&globalsMsg) == B_OK) mainSettings.AddMessage(AM_GLOBALS_STR, &globalsMsg);
	/* Save my auxiliary window settings.  These will get
	 * tossed for any of the windows that are currently open,
	 * but its easier to just save them then check which
	 * windows are open.
	 */
	for (uint32 k = 0; k < _NUM_AUX_WIN; k++) {
		const char*		msgName = MessageName(k);
		if (msgName && !mAuxWinSettings[k].IsEmpty() )
			mainSettings.AddMessage(msgName, &mAuxWinSettings[k]);
	}
	/* Save the message to my state file.
	 */
	BFile*		file;
	file = SettingsFile(B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE,
						SETTINGS_FILE_STR,
						0);
	if (file) {
		AmFlatten(mainSettings, file);
		delete file;
	}
	/* Save the studio.
	 */
	file = SettingsFile(B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE,
						STUDIO_FILE_STR,
						0);
	if (file) {
		mAmGlobals.Studio().Write(file);
		delete file;
	}

	return B_OK;
}

void SeqApplication::LoadState(BMessage* into) const
{
	/* Load the settings.
	 */
	BMessage		mainSettings;
	BFile*			file = SettingsFile(B_READ_ONLY, SETTINGS_FILE_STR);
	if (!file) {
		delete file;
		*into = BMessage();
		return;
	}
	status_t		err = into->Unflatten(file);
	delete file;
	if (err != B_OK) *into = BMessage();
}

void SeqApplication::ApplyPreferences(const BMessage* from)
{
	/* Read my preferences.
	 */
	BMessage msg;
	if (from->FindMessage(PREFERENCES_STR, &msg) == B_OK) {
		mPreferences.MakeEmpty();
		mPreferences = msg;
	}
	
	/* If the preferences are empty, supply them with the defaults.
	 */
	if (mPreferences.IsEmpty() ) {
		mPreferences.AddBool(REMEBER_OPEN_SONGS_PREF, true);
		mPreferences.AddString(OPEN_NEW_SONG_PREF, "channels");
		mPreferences.AddInt32(OPEN_NEW_SONG_CHANNEL_PREF, 2);
		mPreferences.AddInt32(UNDO_HISTORY_PREF, AM_DEFAULT_UNDO_HISTORY);
		mPreferences.AddInt32(TRACK_HEIGHT_PREF, AM_DEFAULT_TRACK_HEIGHT);
	}
	/* If there isn't even an entry for the signature choices, then
	 * go ahead and add one in.  This is so that users with existing
	 * settings files will still see the defaults.  Users can still
	 * use the signature window to set the number of entries to 0,
	 * and that will create a SIGNATURE_CHOICES_PREF that's just empty.
	 */
	if (!mPreferences.HasMessage(SIGNATURE_CHOICES_PREF) ) {
		BMessage	sigChoices;
		sigChoices.AddInt32("beats", 4);
		sigChoices.AddInt32("beat value", 4);
		sigChoices.AddInt32("beats", 2);
		sigChoices.AddInt32("beat value", 4);
		sigChoices.AddInt32("beats", 3);
		sigChoices.AddInt32("beat value", 4);
		mPreferences.AddMessage(SIGNATURE_CHOICES_PREF, &sigChoices);
	}

	mPreferences.FindString(CURRENT_SKIN_PREF, &gSkinFile);
}

void SeqApplication::ApplySettings(const BMessage* from)
{
	BMessage		msg;
	if (from->FindMessage(AM_GLOBALS_STR, &msg) == B_OK) mAmGlobals.ReadFrom(&msg);
	msg.MakeEmpty();
	/* Read the settings for my auxiliary windows.  NOTE:  It's very
	 * important to do this before reading the saved windows, because
	 * these settings will get replaced by any aux win that was actually
	 * open when the app last closed.
	 */
	for (uint32 k = 0; k < _NUM_AUX_WIN; k++) {
		const char*		msgName = MessageName(k);
		if (msgName) {
			if (from->FindMessage(msgName, &mAuxWinSettings[k]) != B_OK)
				mAuxWinSettings[k].MakeEmpty();
		}
	}
	/* Read all my saved windows.
	 */
	for( int32 k = 0; (from->FindMessage(WINDOWSETTINGS_STR, k, &msg)) == B_OK; k++ ) {
		ReadWindowState( &msg );
		msg.MakeEmpty();
	}
	/* Read all my open documents
	 */
	bool		restoreDocuments = false;
	if( mPreferences.FindBool(REMEBER_OPEN_SONGS_PREF, &restoreDocuments) == B_OK && restoreDocuments ) {
		mGrossErrorHack = true;
		BMessage	refs;
		entry_ref	ref;
		for( int32 k = 0; from->FindRef(OPEN_DOCUMENT_STR, k, &ref) == B_OK; k++ ) {
			BEntry		entry( &ref );
			if( entry.Exists() ) refs.AddRef( "refs", &ref );
		}
		if( !(refs.IsEmpty()) ) RefsReceived( &refs );
		PostMessage('grhk');
	}
}

void SeqApplication::ReadWindowState(const BMessage* message)
{
	uint32			index = _NUM_AUX_WIN;
	if (message->what == SONG_WINDOW_SETTING_MSG) ;
	else if (message->what == PREFERENCE_WINDOW_SETTING_MSG) index = PREF_WIN_INDEX;
	else if (message->what == FILTER_WINDOW_SETTING_MSG) index = MANAGE_FILTERS_WIN_INDEX;
	else if (message->what == MANAGE_DEVICES_WINDOW_SETTING_MSG) index = MANAGE_DEVICES_WIN_INDEX;
	else if (message->what == MANAGE_MOTIONS_WINDOW_SETTING_MSG) index = MANAGE_MOTIONS_WIN_INDEX;
	else if (message->what == MANAGE_TOOLS_WINDOW_SETTING_MSG) index = MANAGE_TOOLS_WIN_INDEX;
	else if (message->what == STUDIO_WINDOW_SETTING_MSG) index = STUDIO_WIN_INDEX;

	if (index != _NUM_AUX_WIN) {
		SetAuxiliaryWindowSettings(index, *message);
		ShowAuxiliaryWindow(index, NULL);
	}
}

BFile* SeqApplication::SettingsFile(uint32 open_mode,
									const char* fileName,
									const char* mimeType) const
{
	BPath		path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() != B_OK) return NULL;
	BString		strPath(path.Path());
	strPath << "/" << ARPSETTINGSDIR_STR << "/" << SEQUITURSETTINGSDIR_STR;
	if (verify_directory(strPath) != B_OK) return NULL;

	strPath << "/" << fileName;
	BFile	*file = new BFile(strPath.String(), open_mode);
	if (!file) return NULL;
	if (file->InitCheck() != B_OK) {
		delete file;
		return NULL;
	}

	if (open_mode&B_CREATE_FILE) {
		// Set the new file's MIME type
		BNodeInfo	nodeInfo(file);
		nodeInfo.SetType(mimeType);
	}
	return file;
}

// #pragma mark -

/*************************************************************************
 * _ENTER-STRING-WIN
 *************************************************************************/
_EnterStringWin::_EnterStringWin(	const BString& label,
									const BString& initText)
		: inherited(BRect(0, 0, 0, 0), "Create Tool Bar",
					B_TITLED_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
		  mBg(NULL), mTextCtrl(NULL)
{
	AddViews(label, initText);
	if (mBg) ResizeTo(mBg->Bounds().Width(), mBg->Bounds().Height());
	BScreen		s(this);
	if (s.IsValid() ) {
		BRect	b( Bounds() );
		BRect	sf = s.Frame();
		MoveTo( (sf.Width() - b.Width()) / 2, (sf.Height() - b.Height()) / 2);
	}
}

void _EnterStringWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'ok__':
			HandleOk();
			break;			
		case 'cncl':
			PostMessage(B_QUIT_REQUESTED);
			break;			
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void _EnterStringWin::HandleOk()
{
	if (!mTextCtrl) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	AmGlobalsI*		globalsI = &(AmGlobals());
	if (!globalsI) printf("COULDN'T GET GLOBALS AT ALL\n");
	AmGlobalsImpl*	globals = dynamic_cast<AmGlobalsImpl*>(globalsI);
	if (!globals) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	AmToolBar*		bar = new AmToolBar(mTextCtrl->Text(), true);
	status_t		err = globals->AddToolBar(bar);
	if (err == B_OK) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	delete bar;
	BString		t;
	if (err == B_NAME_IN_USE)
		t << "There is already a tool bar named " << '\'' << mTextCtrl->Text() << '\'';
	else
		t << "An unknown error occurred";
	(new BAlert("", t.String(), "OK"))->Go(); 
}

void _EnterStringWin::AddViews(const BString& label, const BString& initText)
{
	mBg = new BView(BRect(0, 0, 0, 0), "bg", B_FOLLOW_NONE, 0);
	if (!mBg) return;
	mBg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	/* Layout the widgets.
	 */
	float		divider = mBg->StringWidth(label.String() ) + 10;
	float		textW = 80;
	float		fontH = arp_get_font_height(mBg);
	float		space = 5;
	BRect		textF(space, space, space + divider + textW, space + fontH + 5);
	float		buttonH = fontH + 10;
	float		buttonW = 60;		
	float		bSpace = 8;
	BRect		cnclF(bSpace, textF.bottom + bSpace, bSpace + buttonW, textF.bottom + bSpace + buttonH);
	BRect		okF(cnclF);
	okF.OffsetBy(BPoint(buttonW + bSpace, 0));
	if (okF.right < textF.right) {
		float	offset = textF.right - okF.right;
		cnclF.OffsetBy(BPoint(offset, 0));
		okF.OffsetBy(BPoint(offset, 0));
	} else if (textF.right < okF.right) {
		textF.right = okF.right;
	}
	/* Add the controls.
	 */
	mTextCtrl = new BTextControl(	textF, "text_ctrl", label.String(),
									initText.String(), NULL, B_FOLLOW_NONE);
	if (mTextCtrl) {
		mTextCtrl->SetDivider(divider);
		mBg->AddChild(mTextCtrl);
	}
	BButton*	button = new BButton(cnclF, "cancel_button", "Cancel", new BMessage('cncl'), B_FOLLOW_NONE);
	if (button) mBg->AddChild(button);
	button = new BButton(okF, "ok_button", "OK", new BMessage('ok__'), B_FOLLOW_NONE);
	if (button) {
		mBg->AddChild(button);
		button->MakeDefault(true);
	}
	mBg->ResizeTo(okF.right + bSpace, okF.bottom + bSpace);
	AddChild(mBg);
}

// #pragma mark -

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
class _HtmlFilterEntry
{
public:
	_HtmlFilterEntry()
			: mIsMulti(false)																	{ }
	_HtmlFilterEntry(const _HtmlFilterEntry& o)
			: mLabel(o.mLabel), mFileName(o.mFileName), mIndex(o.mIndex), mIsMulti(o.mIsMulti)	{ }
	_HtmlFilterEntry(const char* label, const char* fileName, int32 index, bool isMulti)
			: mLabel(label), mFileName(fileName), mIndex(index), mIsMulti(isMulti)				{ }

	_HtmlFilterEntry&	operator=(const _HtmlFilterEntry& o)
	{
		mLabel = o.mLabel;
		mFileName = o.mFileName;
		mIndex = o.mIndex;
		mIsMulti = o.mIsMulti;
		return *this;
	}
	
	BString		mLabel;
	BString		mFileName;
	int32		mIndex;
	bool		mIsMulti;
};

static const char*	filter_html_header();
static const char*	filter_html_footer();
static const char*	tool_html_header();
static const char*	tool_html_footer();
static void			build_tool_filter_list(const AmTool* tool, vector<_HtmlFilterEntry>& list);
static void			tool_filter_list_add_mf(AmMultiFilter* filter, vector<_HtmlFilterEntry>& list);
static void			tool_filter_list_add_f(AmFilterI* filter, vector<_HtmlFilterEntry>& list);
static void			add_tool_filter_link(BString& html, vector<_HtmlFilterEntry>& list, uint32 index);

static bool sort_filter_entries(const _HtmlFilterEntry& entry1, const _HtmlFilterEntry& entry2)
{
	if (entry1.mLabel.Length() < 1) return false;
	if (entry2.mLabel.Length() < 1) return true;
	if (entry1.mLabel.ICompare(entry2.mLabel) <= 0) return true;
	return false;
}

static void build_filter_list(vector<_HtmlFilterEntry>& list)
{
	AmFilterRoster*		roster1 = AmFilterRoster::Default();
	if (roster1) {
		BAutolock		_l(roster1->Locker());
		int32			N = roster1->CountAddOns();
		bool			doneMidiProducer = false, doneMidiConsumer = false;
		for (int32 i = 0; i < N; i++) {
			AmFilterAddOnHandle* h = dynamic_cast<AmFilterAddOnHandle*>(roster1->AddOnAt(i));
			if (h) {
				bool	skip = false;
				BString	fileName = h->Key();
				if (h->Key() == PRODUCER_CLASS_NAME_STR) {
					if (doneMidiProducer) skip = true;
					else {
						doneMidiProducer = true;
						fileName = "MIDI In";
					}
				} else if (h->Key() == CONSUMER_CLASS_NAME_STR) {
					if (h->Name() == BE_MIDI_SYNTH_STR) fileName = BE_MIDI_SYNTH_STR;
					else {
						if (doneMidiConsumer) skip = true;
						else {
							doneMidiConsumer = true;
							fileName = "MIDI Out";
						}
					}
				}
				if (!skip) list.push_back(_HtmlFilterEntry(h->Name().String(), fileName.String(), i, false));
			}
		}
	}

	AmMultiFilterRoster*		roster2 = AmMultiFilterRoster::Default();
	if (roster2) {
		ArpRef<AmMultiFilterAddOn>	addon = NULL;
		for (uint32 k = 0; (addon = roster2->FilterAt(k)) != NULL; k++) {
			BString			label;
			if (addon->GetLabel(label) != B_OK) label = addon->Name();
			list.push_back(_HtmlFilterEntry(label.String(), addon->Key().String(), k, true));
		}
	}

	sort(list.begin(), list.end(), sort_filter_entries);
}

static void write_filter_properties(BString& toHtml, AmFilterAddOn::type type,
									AmFilterAddOn::subtype subtype, int32 maxCnn,
									int32 majorV, int32 minorV)
{
	toHtml << "<EM>Type:";

	if (subtype == AmFilterAddOn::TOOL_SUBTYPE) toHtml << " Tool";
	else if (subtype == AmFilterAddOn::MULTI_SUBTYPE) toHtml << " Multi";

	if (type == AmFilterAddOn::THROUGH_FILTER) toHtml << " Through";
	else if (type == AmFilterAddOn::SOURCE_FILTER) toHtml << " Input";
	else if (type == AmFilterAddOn::DESTINATION_FILTER) toHtml << " Output";
	toHtml << "<BR>";

	toHtml << "Maximum connections: ";
	if (maxCnn < 0) toHtml << "Unlimited";
	else toHtml << maxCnn;
	toHtml << "<BR>";

	toHtml << "Version: " << majorV << "." << minorV << "</EM><BR>";
}

status_t seq_generate_filter_docs()
{
	const char*			IMAGE_DIR = "images";
	BPath				htmlPath;
	status_t			err = seq_get_app_path(&htmlPath);
	if (err != B_OK) return err;
	if ((err = htmlPath.InitCheck()) != B_OK) return err;
	if ((err = htmlPath.Append("Documentation")) != B_OK) return err;
	if ((err = htmlPath.Append("UsersGuide")) != B_OK) return err;
	BString				str = htmlPath.Path();
	if ((err = verify_directory(str)) != B_OK) return err;
	BPath				imagePath(htmlPath);
	if ((err = imagePath.Append(IMAGE_DIR)) != B_OK) return err;
	str = imagePath.Path();
	if ((err = verify_directory(str)) != B_OK) return err;
	/* Well, now I have my directories.  Generate the data.
	 */
	AmFilterRoster*			roster1 = AmFilterRoster::Default();
	if (!roster1) return B_ERROR;
	AmMultiFilterRoster*	roster2 = AmMultiFilterRoster::Default();
	if (!roster2) return B_ERROR;

	vector<_HtmlFilterEntry>	entries;
	build_filter_list(entries);

	BString			html;
	html << filter_html_header();

	BAutolock		_l(roster1->Locker());
	for (uint32 k = 0; k < entries.size(); k++) {
		if (entries[k].mIsMulti) {
			ArpRef<AmMultiFilterAddOn>	addon = roster2->FilterAt(entries[k].mIndex);
			if (addon) {
				BString			imageName;
				BBitmap*		image = addon->Image(BPoint(20, 20));
				if (image) {
					if (entries[k].mFileName.Length() > 0) {
						entries[k].mFileName = convert_to_filename(entries[k].mFileName);
						if (entries[k].mFileName.Length() > 0) {
							imageName << IMAGE_DIR << "/" << "gen_filt_" << entries[k].mFileName.String() << ".png";
							BPath	p(htmlPath);
							p.Append(imageName.String());
							/* save_bitmap_as_png() will delete the bitmap for us. */
							save_bitmap_as_png(image, p.Path());
						} else delete image;
					} else delete image;
				}
				BString		name, descr;
				addon->LongDescription(name, descr);
				if (name.Length() < 1) name = "<unnamed filter>";
				html << "<table><tr>\n";
				html << "<td><H3><a name =\"Section_A_" << addon->Key().String() << "\">"
					<< "A." << k + 1 << ".&nbsp;" << "</A></H3></td>\n";
				if (imageName.Length() > 0) html << "<td align=\"center\"><img src=\"" << imageName.String() << "\">&nbsp;</td>\n";
				html << "<td><H3>" << name.String() << "</H3></td>\n";
				html << "</tr> </table>\n";
				#if 0
				html << "<P><table><tr> <td><B><a name =\"Section_A_2_" << addon->Key().String() << "\">A.2." << k + 1 << ". " << name.String() << "</A></B>&nbsp;</td>";
				if (imageName.Length() > 0) html << "<td align\"center\"><img src=\"" << imageName.String() << "\"></td>";
				html << "</tr> </table>";
				#endif
				int32		majorV, minorV;
				addon->GetVersion(&majorV, &minorV);
				html << "<blockquote>\n";
				write_filter_properties(html, addon->Type(), addon->Subtype(), addon->MaxConnections(), majorV, minorV);
				if (descr.Length() > 0) html << descr.String();
				html << "</blockquote>\n";
			}
		} else {
			AmFilterAddOnHandle* h = dynamic_cast<AmFilterAddOnHandle*>(roster1->AddOnAt(entries[k].mIndex));
			if (h) {
				BString			imageName;
				BBitmap*		image = h->RawImage(BPoint(20, 20));
				if (image) {
					/* The bitmap stream deletes the bitmap so I don't have
					 * to worry about that.
					 */
					if (entries[k].mFileName.Length() > 0) {
						entries[k].mFileName = convert_to_filename(entries[k].mFileName);
						if (entries[k].mFileName.Length() > 0) {
							imageName << IMAGE_DIR << "/" << "gen_filt_" << entries[k].mFileName.String() << ".png";
							BPath	p(htmlPath);
							p.Append(imageName.String());
							/* save_bitmap_as_png() will delete the bitmap for us. */
							save_bitmap_as_png(image, p.Path());
						} else delete image;
					} else delete image;
				}

				BString		name, descr;
				h->LongDescription(name, descr);
				if (name.Length() < 1) name = "<unnamed filter>";
				html << "<table><tr>\n";
				html << "<td><H3><a name =\"Section_A_" << h->Key().String() << "\">"
					<< "A." << k + 1 << ".&nbsp;" << "</A></H3></td>\n";
				if (imageName.Length() > 0) html << "<td align=\"center\"><img src=\"" << imageName.String() << "\">&nbsp;</td>\n";
				html << "<td><H3>" << name.String() << "</H3></td>\n";
				html << "</tr> </table>\n";
				#if 0
				html << "<P><table><tr> <td><B><a name =\"Section_A_2_" << h->Key().String() << "\">A.2." << k + 1 << ". " << name.String() << "</A></B>&nbsp;</td>";
				if (imageName.Length() > 0) html << "<td align\"center\"><img src=\"" << imageName.String() << "\"></td>";
				html << "</tr> </table>";
				#endif
				int32		majorV, minorV;
				h->GetVersion(&majorV, &minorV);
				html << "<blockquote>\n";
				write_filter_properties(html, h->Type(), h->Subtype(), h->MaxConnections(), majorV, minorV);
				if (descr.Length() > 0) html << descr.String();
				html << "</blockquote>\n";
			}
		}
	}
	html << filter_html_footer();
	{
		htmlPath.Append("a_filters_generated.html");
		BFile			file(htmlPath.Path(),  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK) {
			file.Write(html.String(), html.Length() );
			BNodeInfo	nodeInfo(&file);
			nodeInfo.SetType("text/html");
		}
	}
	return B_OK;
}

static const char* filter_html_header()
{
	return "<HTML>
<TITLE> Appendix A.  The Filters</TITLE> 
<BODY bgcolor=b4b4b4> 
<H1>Appendix A.  The Filters </H1>
<HR width = \"575\"><BR>
<EM><p>This appendix is generated each time it is accessed from the File -> Filter Guide menu item
in a Sequitur song window.</p>

<p>The images on this page are in PNG format.  If your browser does not display the images
correctly, it is probably missing a PNG viewer plugin.</EM><BR>";
}

static const char* filter_html_footer()
{
	return "<P><P><HR width = \"575\"><BR><CENTER> 
<A HREF = \"16_prefs.html\"><IMG SRC = \"images/left.jpg\" BORDER=0></A> 
<A HREF = \"index.html\"><IMG SRC = \"images/top.jpg\" BORDER=0></A> 
<A HREF = \"b_tools_generated.html\"><IMG SRC = \"images/right.jpg\" BORDER=0></A> 
</CENTER> 
<BR><BR> 
</BODY> 
</HTML>";
}

#if 0
static void write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa)
{
	uint8*		pixel = (uint8*)( ((uint8*)bm->Bits()) + (uint32)(x * pa.bpp() ) + (uint32)(y * bm->BytesPerRow() ) );
	pa.write(pixel, c);
}
#endif

status_t seq_generate_tool_docs()
{
	const char*			IMAGE_DIR = "images";
	BPath				htmlPath;
	status_t			err = seq_get_app_path(&htmlPath);
	if (err != B_OK) return err;
	if ((err = htmlPath.InitCheck()) != B_OK) return err;
	if ((err = htmlPath.Append("Documentation")) != B_OK) return err;
	if ((err = htmlPath.Append("UsersGuide")) != B_OK) return err;
	BString				str = htmlPath.Path();
	if ((err = verify_directory(str)) != B_OK) return err;
	BPath				imagePath(htmlPath);
	if ((err = imagePath.Append(IMAGE_DIR)) != B_OK) return err;
	str = imagePath.Path();
	if ((err = verify_directory(str)) != B_OK) return err;
	/* Well, now I have my directories.  Generate the data.
	 */
	BString				html;
	html << tool_html_header();

	/* First sort all the tools.
	 */
	vector<_HtmlFilterEntry>	tools;
	AmToolRef					toolRef;
	for (uint32 k = 0; (toolRef = AmGlobals().ToolAt(k)).IsValid(); k++) {
		// READ TOOL BLOCK
		const AmTool*	tool = toolRef.ReadLock();
		if (tool) tools.push_back(_HtmlFilterEntry(tool->Label().String(), tool->Key().String(), k, false) );
		toolRef.ReadUnlock(tool);
		// END READ TOOL BLOCK
	}

	sort(tools.begin(), tools.end(), sort_filter_entries);

	for (uint32 k = 0; k < tools.size(); k++) {
		toolRef = AmGlobals().ToolAt(tools[k].mIndex);
		// READ TOOL BLOCK
		const AmTool*	tool = toolRef.ReadLock();
		if (tool) {
			BString			fileName = convert_to_filename(tool->Key() );
			BString			iconName;
			if (fileName.Length() > 0) {
				BBitmap*	icon = tool->Icon() ? (new BBitmap(tool->Icon())) : NULL;
				if (icon) {
					iconName << IMAGE_DIR << "/" << "gen_tool_" << fileName.String() << ".png";
					BPath	p(htmlPath);
					p.Append(iconName.String());
					/* save_bitmap_as_png() will delete the bitmap for us. */
					save_bitmap_as_png(icon, p.Path());
				}
			}
			BString			label = tool->Label();
			if (label.Length() < 1) label = "<unnamed tool>";
			html << "<table><tr>\n";
			html << "<td><H3><a name =\"Section_B_" << tool->Key().String() << "\">"
				<< "B." << k + 1 << ".&nbsp;" << "</A></H3></td>\n";
			if (iconName.Length() > 0) html << "<td align=\"center\"><img src=\"" << iconName.String() << "\">&nbsp;</td>\n";
			html << "<td><H3>" << label.String() << "</H3></td>\n";
			html << "</tr> </table>\n";
			#if 0
			html << "<P><table><tr> <td><B>B." << k + 1 << ". " << label.String() << "</B>&nbsp;</td>";
			if (iconName.Length() > 0) html << "<td align = \"center\"> <img src=\"" << iconName.String() << "\" ></td>"; 
			html << "</tr> </table>";
			#endif
			BString			longDesc;
			tool->LongDescription(longDesc);
			html << "<blockquote>\n";

			vector<_HtmlFilterEntry>	filters;
			build_tool_filter_list(tool, filters);
			if (filters.size() == 1) {
				html << "<em>Filters: ";
				add_tool_filter_link(html, filters, 0);
				html << ".</em>";
			} else if (filters.size() > 1) {
				html << "<em>Filters: ";
				for (uint32 k = 0; k < filters.size(); k++) {
					add_tool_filter_link(html, filters, k);
					if (k == filters.size() - 2) html << " and ";
					else if (k != filters.size() - 1) html << ", ";
				}
				html << ".</em>";
			}

			if (longDesc.Length() > 0) html << longDesc.String();

			html << "</blockquote>\n";
		}
		toolRef.ReadUnlock(tool);
		// END READ TOOL BLOCK
	}
	html << tool_html_footer();
	{
		htmlPath.Append("b_tools_generated.html");
		BFile			file(htmlPath.Path(),  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK) {
			file.Write(html.String(), html.Length() );
			BNodeInfo	nodeInfo(&file);
			nodeInfo.SetType("text/html");
		}
	}
	return B_OK;
}

static const char* tool_html_header()
{
	return "<HTML>
<TITLE> Appendix B.  The Tools</TITLE> 
<BODY bgcolor=b4b4b4> 
<H1>Appendix B.  The Tools</H1>
<HR width = \"575\"><BR>
<EM><p>This appendix is generated each time it is accessed from the File -> Tool Guide menu item
in a Sequitur song window.</p>

<p>The images on this page are in PNG format.  If your browser does not display the images
correctly, it is probably missing a PNG viewer plugin.</EM><BR>";
}

static const char* tool_html_footer()
{
	return "</table><P><P><HR width = \"575\"><BR><CENTER> 
<A HREF = \"a_filters_generated.html\"><IMG SRC = \"images/left.jpg\" BORDER=0></A> 
<A HREF = \"index.html\"><IMG SRC = \"images/top.jpg\" BORDER=0></A> 
<A HREF = \"c_controls.html\"><IMG SRC = \"images/right.jpg\" BORDER=0></A> 
</CENTER> 
<BR><BR> 
</BODY> 
</HTML>";
}

static void build_tool_filter_list(const AmTool* tool, vector<_HtmlFilterEntry>& list)
{
	ArpASSERT(tool);
	uint32					count = tool->CountPipelines();
	for (uint32 k = 0; k < count; k++) {
		pipeline_id			pid = tool->PipelineId(k);
		AmFilterHolderI*	f = tool->Filter(pid, NULLINPUTOUTPUT_PIPELINE);
		while (f) {
			AmMultiFilter*	mf = dynamic_cast<AmMultiFilter*>(f->Filter() );
			if (mf) tool_filter_list_add_mf(mf, list);
			else tool_filter_list_add_f(f->Filter(), list);
			f = f->NextInLine();
		}
	}
	sort(list.begin(), list.end(), sort_filter_entries);
}

static void tool_filter_list_add_mf(AmMultiFilter* filter, vector<_HtmlFilterEntry>& list)
{
	ArpASSERT(filter);
	if (filter->Flags()&AmFilterI::HIDE_PROPERTIES_FLAG) return;

	uint32			count = filter->CountPipelines();
	for (uint32 k = 0; k < count; k++) {
		AmFilterHolderI*	holder = filter->Filter(filter->PipelineId(k), NULLINPUTOUTPUT_PIPELINE);
		while (holder) {
			if (holder->Filter() ) {
				AmMultiFilter*		mf = dynamic_cast<AmMultiFilter*>(holder->Filter() );
				if (mf) tool_filter_list_add_mf(mf, list);
				else tool_filter_list_add_f(holder->Filter(), list);
			}
			holder = holder->NextInLine();
		}
	}
}

static void tool_filter_list_add_f(AmFilterI* filter, vector<_HtmlFilterEntry>& list)
{
	ArpASSERT(filter);
	if (filter->Flags()&AmFilterI::HIDE_PROPERTIES_FLAG) return;

	BString		key = filter->AddOn()->Key();
	if (key == NULL_INPUT_KEY) return;
	if (key == NULL_OUTPUT_KEY) return;
	for (uint32 k = 0; k < list.size(); k++) {
		if (list[k].mFileName == key) return;
	}

	list.push_back(_HtmlFilterEntry(filter->Name().String(), key.String(), 0, false) );
}

static void add_tool_filter_link(BString& html, vector<_HtmlFilterEntry>& list, uint32 index)
{
	ArpASSERT(index < list.size() );
	html << "<A HREF = \"a_filters_generated.html#Section_A_"
		<< list[index].mFileName.String() << "\">" << list[index].mLabel.String() << "</A>";
}

status_t seq_get_doc_index_ref(entry_ref* ref)
{
	BPath				htmlPath;
	status_t			err = seq_get_app_path(&htmlPath);
	if (err != B_OK) return err;
	BDirectory			dir(htmlPath.Path() );
	if ( (err = dir.InitCheck() ) != B_OK) return err;
	BEntry			entry;
	if ( (err = dir.FindEntry("Documentation/index.html", &entry)) != B_OK) return err;
	if (ref) err = entry.GetRef(ref);
	return err;
}
#if 0
status_t		seq_get_users_guide_ref(entry_ref* ref);
status_t seq_get_users_guide_ref(entry_ref* ref)
{
	BPath				htmlPath;
	status_t			err = seq_get_app_path(&htmlPath);
	if (err != B_OK) return err;
	BDirectory			dir(htmlPath.Path() );
	if ( (err = dir.InitCheck() ) != B_OK) return err;
	BEntry			entry;
	if ( (err = dir.FindEntry("Documentation/UsersGuide/index.html", &entry)) != B_OK) return err;
	if (ref) err = entry.GetRef(ref);
	return err;
}
#endif

status_t seq_get_filters_ref(entry_ref* ref)
{
	BPath				htmlPath;
	status_t			err = seq_get_app_path(&htmlPath);
	if (err != B_OK) return err;
	BDirectory			dir(htmlPath.Path() );
	if ( (err = dir.InitCheck() ) != B_OK) return err;
	BEntry			entry;
	if ( (err = dir.FindEntry("Documentation/UsersGuide/a_filters_generated.html", &entry)) != B_OK) return err;
	if (ref) err = entry.GetRef(ref);
	return err;
}

status_t seq_get_tools_ref(entry_ref* ref)
{
	BPath				htmlPath;
	status_t			err = seq_get_app_path(&htmlPath);
	if (err != B_OK) return err;
	BDirectory			dir(htmlPath.Path() );
	if ( (err = dir.InitCheck() ) != B_OK) return err;
	BEntry			entry;
	if ( (err = dir.FindEntry("Documentation/UsersGuide/b_tools_generated.html", &entry)) != B_OK) return err;
	if (ref) err = entry.GetRef(ref);
	return err;
}
