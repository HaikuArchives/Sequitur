#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include "GlPublic/GlPlanes.h"
#include "Glasslike/GlApp.h"
#include "Glasslike/GlMainWin.h"
#include "Glasslike/GlPrefs.h"
#include "Glasslike/GlPrefWin.h"
#include <Glasslike/GlDefs.h>

// This directory will be located in the user settings directory.
static const char*	ARPSETTINGSDIR_STR			= "AngryRedPlanet";
// This directory will be located in ARPSETTINGSDIR_STR.
static const char*	GLASSLIKESETTINGSDIR_STR	= "Glasslike";
static const char*	SETTINGS_FILE_STR			= "settings";
static const char*	SETTING_STR					= "setting";

gl_image_id			GL_NODE_IMAGE_ID				= 0;
gl_image_id			GL_PIXEL_TARGET_IMAGE_ID		= 0;

gl_image_id			GL_MIDI_A_IMAGE_ID				= 0;
gl_image_id			GL_MIDI_B_IMAGE_ID				= 0;
gl_image_id			GL_MIDI_C_IMAGE_ID				= 0;
gl_image_id			GL_MIDI_Q_IMAGE_ID				= 0;
gl_image_id			GL_MIDI_RECORD_IMAGE_ID			= 0;

static GlPrefs	g_Prefs;

const char*		GlAppSig = "application/x-vnd.ARP-Glasslike";

static status_t verify_directory(const BString16& pathStr);
static GlImage* new_midi_a_img();
static GlImage* new_midi_b_img();
static GlImage* new_midi_c_img();
static GlImage* new_midi_Q_img();
static GlImage* new_midi_record_img();

static void _draw_colour_wheel(	GlPlanes& pixels,
								int32 l, int32 t, int32 r, int32 b)
{
	int32				wheelHalfW = (r - l) / 2, wheelHalfH = (b - t) / 2;
	int32				cenX = l + wheelHalfW, cenY = t + wheelHalfH;
	float				radius = float((wheelHalfW < wheelHalfH) ? wheelHalfW : wheelHalfH);
	float				radiusHalf = radius / 2;
	
	for (int32 y = t; y <= b; y++) {
		for (int32 x = l; x <= r; x++) {
			int32		pix = ARP_PIXEL(x, y, pixels.w);
			float		d = ARP_DISTANCE(cenX, cenY, x, y);
			if (d <= radius) {
				float	hue = arp_degree(float(x - cenX), float(cenY - y));
				float	saturation = 1, value = 1;
				if (d < radiusHalf) saturation = 1 - ((radiusHalf - d) / radiusHalf);
				else if (d > radiusHalf) value = 1 - ((d - radiusHalf) / radiusHalf);
				pixels.SetHsv(pix, hue, saturation, value);
//				pixels.SetHsv(pix, hue, 1, 1);
				pixels.a[pix] = 255;
			} else pixels.a[pix] = 0;
		}
	}
}

#include <GlPublic/GlPixel.h>

static GlImage* _new_colour_wheel(int32 w, int32 h)
{
	GlImage*		img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlPlanes*		p = img->LockPixels(GL_PIXEL_RGBA);
	if (p && p->HasColor()) {
		_draw_colour_wheel(*p, 0, 0, w-1, h-1);
	}
	img->UnlockPixels(p);
	return img;
}

/* FIX: Remove this when done with the testing.
 */
void _PREFS_INIT_TEMP()
{
	g_Prefs.Initialize();
}

/***************************************************************************
 * GL-APP
 ***************************************************************************/
GlApp::GlApp()
		: inherited(GlAppSig), mIsQuitting(false)
{
//	for (float x = 0; x <= 4.0; x += 0.1) printf("%f: %f\n", x, sin(x));
//	for (float x = 0; x <= 255; x += 2) printf("%f: %f\n", x, sin(x));

	InitRects();
	for (uint32 i = 0; i < _NUM_SETTINGS; i++) mSettingsChanges[i] = false;
	
	gl_pixel_init_value();
	gl_init_defs();
	g_Prefs.Initialize();
	mGlobalsImpl.Initialize();
	mSkinCache.Initialize();

	AcquireImages();

#if 0
bigtime_t				start, stop;
float					v1 = 0.0001, v2 = 0.25, v3;
start = system_time();
for (uint32 k = 0; k < 1000000; k++) v3 = v1 + v2;
stop = system_time();
printf("1 Elapsed: %f sec\n", double(stop - start) / 1000000);


int32					iv1 = 23, iv2 = 34556342, iv3;
start = system_time();
for (uint32 k = 0; k < 1000000; k++) iv3 = iv1 + iv2;
stop = system_time();
printf("2 Elapsed: %f sec\n", double(stop - start) / 1000000);

uint8					iv1x = 13, iv2x = 235, iv3x;
start = system_time();
for (uint32 k = 0; k < 1000000; k++) iv3x = iv1x + iv2x;
stop = system_time();
printf("3 Elapsed: %f sec\n", double(stop - start) / 1000000);
#endif

	LoadState();
}

GlApp::~GlApp()
{
	ReleaseImages();
	SaveState();
}

bool GlApp::IsQuitting() const
{
	return mIsQuitting;
}

void GlApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SHOW_PREFERENCES_MSG:
			ShowAuxiliaryWindow(PREF_WIN_INDEX, msg);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}
bool GlApp::QuitRequested(void)
{
	mIsQuitting = true;
//	SaveState();
	bool	answer = inherited::QuitRequested();
	if (answer == false) mIsQuitting = false;
	return answer;
}

void GlApp::ReadyToRun()
{
	if (mMixingWins.size() < 1) {
		BWindow*	win = new GlMainWin(BRect(120, 80, 820, 920), mSettings[MIXING_WIN_INDEX]);
		if (win) {
			mMixingWins.push_back(BMessenger(win));
			win->Show();
		}
	}
	if (mMixingWins.size() < 1)
		PostMessage(B_QUIT_REQUESTED);
}

GlKeyTracker& GlApp::KeyTracker()
{
	return mKeyTracker;
}

void GlApp::SetMidiTarget(const BMessenger& target)
{
	mMidi.SetTarget(target);
}

void GlApp::UnsetMidiTarget(const BMessenger& target)
{
	mMidi.UnsetTarget(target);
}

GlMidiBindingList& GlApp::MidiBindings()
{
	return mMidiBindings;
}

GlToolBarRoster& GlApp::ToolBars()
{
	return mToolBars;
}

GlSkinCache& GlApp::Skin()
{
	return mSkinCache;
}

void GlApp::SetAuxiliaryWindowSettings(uint32 index, const BMessage& settings)
{
	ArpASSERT(index < _NUM_AUX_WIN);
	BAutolock		l(&mSettingsLock);
	mAuxWinSettings[index].MakeEmpty();
	mAuxWinSettings[index] = settings;
}

void GlApp::SetSettings(uint32 index, const BMessage& settings)
{
	ArpASSERT(index < _NUM_SETTINGS);
	BAutolock		l(&mSettingsLock);
	mSettings[index] = settings;
	mSettingsChanges[index] = true;
}

void GlApp::ShowAuxiliaryWindow(uint32 index, BMessage* msg)
{
	if (!mAuxWins[index].IsValid() ) {
		BRect			frame = mAuxWinInitFrames[index];
		BAutolock 		l(&mSettingsLock);
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

BWindow* GlApp::NewAuxiliaryWindow(	uint32 index, BRect frame,
									BMessage* config, const BMessage* msg) const
{
	if (index == PREF_WIN_INDEX) return new GlPrefWin(frame, config);
	return NULL;
}

void GlApp::InitRects()
{
	BScreen		s;
	float		w = s.Frame().Width(), h = s.Frame().Height();
	
	mAuxWinInitFrames[PREF_WIN_INDEX]				= BRect(w * 0.25f, h * 0.25f, w * 0.75f, h * 0.75f);
}

status_t GlApp::SaveState()
{
//printf("SaveState\n");
	BMessage	settings;
	bool		changes = false;

	if (mKeyTracker.IsDirty()) changes = true;
	mKeyTracker.WriteTo(settings);

//	if (mMidiBindings.IsDirty()) {
//printf("\tbindings dirty\n");
		mMidiBindings.WriteTo(settings);
		changes = true;
//	}
	for (uint32 k = 0; k < _NUM_SETTINGS; k++) {
		BMessage	m;
		m.AddInt32("index", k);
		m.AddMessage("m", &(mSettings[k]));
		settings.AddMessage(SETTING_STR, &m);
		changes = true;
	}
	
	if (!changes) return B_OK;

	/* Save the message to my state file.
	 */
	BFile*		file;
	file = SettingsFile(B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE,
						SETTINGS_FILE_STR, 0);
	if (file) {
		settings.Flatten(file);
		delete file;
	}
	return B_OK;
}

void GlApp::LoadState()
{
	/* Load the settings.
	 */
	BFile*			file = SettingsFile(B_READ_ONLY, SETTINGS_FILE_STR);
	if (!file) {
		delete file;
		return;
	}
	BMessage		settings;
	status_t		err = settings.Unflatten(file);
	delete file;
	if (err != B_OK) return;

	mKeyTracker.ReadFrom(settings);
	
	mMidiBindings.ReadFrom(settings);
//printf("There are %ld bindings\n", mMidiBindings.Size());
	int32			msgIndex = 0;
	BMessage		m;
	while (settings.FindMessage(SETTING_STR, msgIndex, &m) == B_OK) {
		int32		index;
		BMessage	m2;
		if (m.FindInt32("index", &index) == B_OK && m.FindMessage("m", &m2) == B_OK) {
			if (index >= 0 && index < _NUM_SETTINGS)
				mSettings[index] = m2;
		}
		m.MakeEmpty();
		msgIndex++;
	}
}

BFile* GlApp::SettingsFile(	uint32 open_mode,
							const char* fileName,
							const char* mimeType) const
{
ArpPOLISH("Not there yet");
// FIX:  How to deal with settings in windows?
return NULL;
#if 0
	BPath			path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() != B_OK) return NULL;
	BString16		strPath(path.Path());
	strPath << "/" << ARPSETTINGSDIR_STR << "/" << GLASSLIKESETTINGSDIR_STR;
	if (verify_directory(strPath) != B_OK) return NULL;

	strPath << "/" << fileName;
	BFile*			file = new BFile(strPath.String(), open_mode);
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
#endif
}

void GlApp::AcquireImages()
{
	int32				x, y;
	/* Build the default node image
	 */
	GlImage*			img = _new_colour_wheel(32, 32);
//	gl_image_id			iid = mGlobalsImpl.AcquireImage("/boot/home/Glasslike.png");
//	const GlImage*		img = mGlobalsImpl.SourceImage(iid);
	if (img) {
		GlImage*		newImg = img->AsScaledImage(Prefs().GetInt32(GL_NODE_IMAGE_X),
													Prefs().GetInt32(GL_NODE_IMAGE_Y), 1);
		if (newImg) GL_NODE_IMAGE_ID = mGlobalsImpl.AcquireImage(newImg);
	}
//	mGlobalsImpl.ReleaseImage(iid);
	delete img;

	/* Build the pixel target control image
	 */
	x = Prefs().GetInt32(GL_NODE_CONTROL_X);
	y = Prefs().GetInt32(GL_NODE_CONTROL_Y);
	img = _new_colour_wheel(x * 4, x * 4);
	if (img) {
		GlImage*		newImg = img->AsScaledImage(x, y, 1);
		if (newImg) GL_PIXEL_TARGET_IMAGE_ID = mGlobalsImpl.AcquireImage(newImg);
		delete img;
	}

	if ((img = new_midi_a_img()) != 0)
		GL_MIDI_A_IMAGE_ID = mGlobalsImpl.AcquireImage(img);
	if ((img = new_midi_b_img()) != 0)
		GL_MIDI_B_IMAGE_ID = mGlobalsImpl.AcquireImage(img);
	if ((img = new_midi_c_img()) != 0)
		GL_MIDI_C_IMAGE_ID = mGlobalsImpl.AcquireImage(img);
	if ((img = new_midi_Q_img()) != 0)
		GL_MIDI_Q_IMAGE_ID = mGlobalsImpl.AcquireImage(img);
	if ((img = new_midi_record_img()) != 0)
		GL_MIDI_RECORD_IMAGE_ID = mGlobalsImpl.AcquireImage(img);
}

void GlApp::ReleaseImages()
{
	mGlobalsImpl.ReleaseImage(GL_NODE_IMAGE_ID);
	mGlobalsImpl.ReleaseImage(GL_PIXEL_TARGET_IMAGE_ID);
	mGlobalsImpl.ReleaseImage(GL_MIDI_A_IMAGE_ID);
	mGlobalsImpl.ReleaseImage(GL_MIDI_B_IMAGE_ID);
	mGlobalsImpl.ReleaseImage(GL_MIDI_C_IMAGE_ID);
	mGlobalsImpl.ReleaseImage(GL_MIDI_Q_IMAGE_ID);
	mGlobalsImpl.ReleaseImage(GL_MIDI_RECORD_IMAGE_ID);
}

// #pragma mark -

/*************************************************************************
 * Misc
 *************************************************************************/
static status_t verify_directory(const BString16& pathStr)
{
ArpFINISH();
return B_ERROR;
/*
	BPath		path( pathStr.String() );
	BPath		parent;
	status_t	err = path.InitCheck();
	if (err != B_OK) return err;
	if ((err = path.GetParent(&parent)) != B_OK) return err;
	BDirectory	dir( parent.Path() );
	if ((err = dir.InitCheck()) != B_OK) return err;
	if (dir.Contains(path.Leaf(), B_DIRECTORY_NODE) == true) return B_OK;
	return dir.CreateDirectory(path.Leaf(), 0);
*/
}

static GlImage* new_midi_a_img()
{
	GlImage*	img = new GlImage(10, 7);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlPlanes*	p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32	size = p->w * p->h;
		memset(p->r,		0,		size);
		memset(p->g,		0,		size);
		memset(p->b,		0,		size);
		memset(p->a,		255,	size);
		p->a[13] = 0;	p->a[14] = 0;	p->a[15] = 0;	p->a[16] = 0;
		p->a[22] = 0;	p->a[23] = 0;	p->a[26] = 0;	p->a[27] = 0;
 		p->a[32] = 0;	p->a[33] = 0;	p->a[34] = 0;	p->a[35] = 0;	p->a[36] = 0;	p->a[37] = 0;
		p->a[42] = 0;	p->a[43] = 0;	p->a[46] = 0;	p->a[47] = 0;
		p->a[52] = 0;	p->a[53] = 0;	p->a[56] = 0;	p->a[57] = 0;
		img->UnlockPixels(p);
	}
	return img;
}

static GlImage* new_midi_b_img()
{
	GlImage*	img = new GlImage(10, 7);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlPlanes*	p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32	size = p->w * p->h;
		memset(p->r,		0,		size);
		memset(p->g,		0,		size);
		memset(p->b,		0,		size);
		memset(p->a,		255,	size);
		p->a[12] = 0;	p->a[13] = 0;	p->a[14] = 0;	p->a[15] = 0;	p->a[16] = 0;
		p->a[22] = 0;	p->a[23] = 0;	p->a[26] = 0;	p->a[27] = 0;
 		p->a[32] = 0;	p->a[33] = 0;	p->a[34] = 0;	p->a[35] = 0;	p->a[36] = 0;
		p->a[42] = 0;	p->a[43] = 0;	p->a[46] = 0;	p->a[47] = 0;
		p->a[52] = 0;	p->a[53] = 0;	p->a[54] = 0;	p->a[55] = 0;	p->a[56] = 0;
		img->UnlockPixels(p);
	}
	return img;
}

static GlImage* new_midi_c_img()
{
	GlImage*	img = new GlImage(10, 7);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlPlanes*	p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32	size = p->w * p->h;
		memset(p->r,		0,		size);
		memset(p->g,		0,		size);
		memset(p->b,		0,		size);
		memset(p->a,		255,	size);
		p->a[13] = 0;	p->a[14] = 0;	p->a[15] = 0;	p->a[16] = 0;
		p->a[22] = 0;	p->a[23] = 0;	p->a[26] = 0;	p->a[27] = 0;
 		p->a[32] = 0;	p->a[33] = 0;
		p->a[42] = 0;	p->a[43] = 0;	p->a[46] = 0;	p->a[47] = 0;
		p->a[53] = 0;	p->a[54] = 0;	p->a[55] = 0;	p->a[56] = 0;
		img->UnlockPixels(p);
	}
	return img;
}

static GlImage* new_midi_Q_img()
{
	GlImage*	img = new GlImage(10, 7);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlPlanes*	p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32	size = p->w * p->h;
		memset(p->r,		0,		size);
		memset(p->g,		0,		size);
		memset(p->b,		0,		size);
		memset(p->a,		255,	size);
		p->a[13] = 0;	p->a[14] = 0;	p->a[15] = 0;	p->a[16] = 0;
		p->a[22] = 0;	p->a[23] = 0;	p->a[26] = 0;	p->a[27] = 0;
 		p->a[35] = 0;	p->a[36] = 0;
 		p->a[44] = 0;	p->a[45] = 0;
 		p->a[54] = 0;	p->a[55] = 0;
 		img->UnlockPixels(p);
	}
	return img;
}

static GlImage* new_midi_record_img()
{
	GlImage*	img = new GlImage(10, 10);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlPlanes*	p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32	size = p->w * p->h;
		memset(p->r,		255,	size);
		memset(p->g,		0,		size);
		memset(p->b,		0,		size);
		memset(p->a,		255,	size);
 		img->UnlockPixels(p);
	}
	return img;
}
