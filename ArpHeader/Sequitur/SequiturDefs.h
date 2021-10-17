/* SequiturDefs.h
 * A collection of common defines for the Sequitur classes.
 */

#ifndef __ARPHEADER_SEQUITUR_SEQUITURDEFS_H__
#define __ARPHEADER_SEQUITUR_SEQUITURDEFS_H__

#include <experimental/ResourceSet.h>

class BMenu;
class BMessage;
class BPath;
class BView;

enum {
	/* Post this message to the app to show the Preferences window
	 */
	SHOW_PREFERENCES_MSG		= '#opr',
	/* Post this message to the app to show the Filters window
	 */
	SHOW_FILTERS_MSG			= '#ofl',
	SHOW_STUDIO_MSG				= '#ost',
		// "tool"				A pointer to the tool_id to show.

	SHOW_MANAGE_DEVICES_MSG		= '#omd',
	SHOW_MANAGE_MOTIONS_MSG		= '#omm',
	SHOW_MANAGE_TOOLS_MSG		= '#omt',

	SHOW_EDIT_DEVICE_MSG		= '#oed',
		// "device_unique_name"	A string, the unique name for the Device to edit.
		// "path"				A string, the local path for the object, if any.
	SHOW_EDIT_MULTIFILTER_MSG	= '#oef',
		// "key"				A string, the key for the multifilter to edit.
		// "path"				A string, the local path for the object, if any.
	SHOW_EDIT_MOTION_MSG		= '#oem',
		// "motion_unique_name"	A string, the unique name for the Motion to edit.
		// "path"				A string, the local path for the object, if any.
	SHOW_EDIT_TOOL_MSG			= '#oet',
		// "tool_key"			A string, the unique name for the Tool to edit.
		// "path"				A string, the local path for the object, if any.

	SHOW_NEW_TOOL_BAR_MSG		= '#ont',
	/* Post these to any window which can play a song.
	 */
	PLAY_SONG_MSG				= '#psp',
	PLAY_SONG_FROM_TIME_MSG		= '#psf',
		// "time"				An int64, the time at which to play -- start from
		//						zero if absent.
	STOP_SONG_MSG				= '#pst',
	RECORD_MSG					= '#prc',

	SHOWTRACKWIN_MSG			= '#otw',
		// This can be posted to any song window, to have it open a track window
		// SZ_TRACK_ID, a pointer to the track_id to open.  Required.
		// SZ_AMTIME, the AmTime to open the window at.  Optional.

	SEQ_REFRESH_WINDOW_MSG		= '#REF'
		// "flags", an int32 containing the refresh flags
};

#define UNIQUE_NAME_STR			"Unique Name"

/* State persistence
 */
enum {
	/* This is the what used by each song window to persist itself
	 * into the app's settings file.
	 */
	SONG_WINDOW_SETTING_MSG				= '#sso',
	/* This is the message that the preference window uses to
	 * persist itself in the settings file.
	 */
	PREFERENCE_WINDOW_SETTING_MSG		= '#spr',
	/* This is the message that the filter window uses to
	 * persist itself in the settings file.
	 */
	FILTER_WINDOW_SETTING_MSG			= '#sfi',
	/* These are the messages that the relevant aux windows use
	 * to persist themselves to the settings file.
	 */
	MANAGE_DEVICES_WINDOW_SETTING_MSG	= '#smd',
	MANAGE_MOTIONS_WINDOW_SETTING_MSG	= '#smp',
	MANAGE_TOOLS_WINDOW_SETTING_MSG		= '#smt',

	EDIT_DEVICE_WINDOW_SETTING_MSG		= '#sed',
	EDIT_MULTIFILTER_WINDOW_SETTING_MSG	= '#sef',
	EDIT_MOTION_WINDOW_SETTING_MSG		= '#sep',
	EDIT_TOOL_WINDOW_SETTING_MSG		= '#set',

	PHRASE_PROPERTY_WINDOW_SETTING_MSG	= '#spp',
	STUDIO_WINDOW_SETTING_MSG			= '#ssw'
};

extern const char*		SEQ_TIME_STR;


/*---------------------------------------------------------
 * IMAGE CONSTANTS.  Use the ImageManager() to access the
 * actual images.
 *---------------------------------------------------------*/
#define LEFT_LOOP_STR				"Left Loop"
#define RIGHT_LOOP_STR				"Right Loop"
#define PLAY_NORMAL_STR				"Play Button Normal"
#define PLAY_PRESSED_STR			"Play Button Pressed"
#define PLAY_FROM_START_NORMAL_STR	"Play From Start Button Normal"
#define PLAY_FROM_START_PRESSED_STR	"Play From Start Button Pressed"
#define PLAY_TRACK_NORMAL_STR		"Play Track Button Normal"
#define PLAY_TRACK_PRESSED_STR		"Play Track Button Pressed"
#define STOP_NORMAL_STR				"Stop Button Normal"
#define STOP_PRESSED_STR			"Stop Button Pressed"
#define RECORD_NORMAL_STR			"Record Button Normal"
#define RECORD_PRESSED_STR			"Record Button Pressed"
#define LOOP_NORMAL_STR				"Loop Button Normal"
#define LOOP_PRESSED_STR			"Loop Button Pressed"
#define REWIND_NORMAL_STR			"Rewind Button Normal"
#define REWIND_PRESSED_STR			"Rewind Button Pressed"
#define	SONG_POSITION_MARKER_IMAGE_STR "Song Position Marker"
#define	MUTE_ON_IMAGE_STR			"Mute On"
#define	MUTE_OFF_IMAGE_STR			"Mute Off"
#define	SOLO_ON_IMAGE_STR			"Solo On"
#define	SOLO_OFF_IMAGE_STR			"Solo Off"
#define	SEQ_PROPERTY_MENU_NORMAL_STR "Property Menu Normal"

/*---------------------------------------------------------
 * Miscellaneous functions.  Right now, global behaviour
 * is spread between here and the SeqApplication.  That
 * should be addressed.
 *---------------------------------------------------------*/

BResourceSet&	Resources();
bool			seq_is_quitting();
void			seq_set_quitting(bool quitting);
/* Answer the number of windows that have defined themselves as
 * 'significant' -- typically the song windows.
 */
uint32			seq_count_significant_windows();

enum {
	SEQ_FILTER_WINDOW_ACTIVE		= 0x00000001,
	SEQ_DEVICE_WINDOW_ACTIVE		= 0x00000002,
	SEQ_MULTIFILTER_WINDOW_ACTIVE	= 0x00000004,
	SEQ_MOTION_WINDOW_ACTIVE		= 0x00000008,
	SEQ_TOOL_WINDOW_ACTIVE			= 0x00000010
};
/* Supply one or more of the flags above and receive true
 * if they are on, false otherwise.
 */
bool			seq_flag_is_on(uint32 flags);

/* These two are implemented in SeqAboutWindow.cpp
 */
void			report_startup_status(const char* msg);
void			finish_startup_status();

void			low_memory_warning();
/* This function creates an HTML page with the docs for each filter
 * and places it in app_path/Documentation/UsersGuide/.  Images for
 * each filter are placed in the images/ directory.
 */
status_t		seq_generate_filter_docs();
/* Same thing, but the tools doc.
 */
status_t		seq_generate_tool_docs();
/* Fill in the entry refs with the appropriate page of the docs.
 */
status_t		seq_get_doc_index_ref(entry_ref* ref);
status_t		seq_get_filters_ref(entry_ref* ref);
status_t		seq_get_tools_ref(entry_ref* ref);

/*---------------------------------------------------------
 * Global preferences
 *---------------------------------------------------------*/
status_t		seq_get_bool_preference(const char* name, bool* val, int32 n = 0);
status_t		seq_get_int32_preference(const char* name, int32* val, int32 n = 0);
status_t		seq_get_message_preference(const char* name, BMessage* msg, int32 n = 0);
status_t		seq_get_ref_preference(const char* name, entry_ref* ref, int32 n = 0);
status_t		seq_get_string_preference(const char* name, const char** str, int32 n = 0);
status_t		seq_get_factory_int32_preference(	const char* fac, const char* view,
													const char* name, int32* outI32, int32 n = 0);

status_t		seq_set_string_preference(const char* name, const char* str, int32 n = 0);

status_t		seq_make_skin_menu(BMenu* into, BMessage* baseMsg);
status_t		seq_get_app_path(BPath* path);

#define REMEBER_OPEN_SONGS_PREF		"Remember Open Songs"
#define	OPEN_NEW_SONG_PREF			"Open New Song"
	// blank", "channel", "file"
	// If channel, there's also an int32 of OPEN_NEW_SONG_CHANNEL_PREF
	// that tells the number of channels to create.
	// If file, there's also a ref of OPEN_NOW_SONG_FILE_PREF that
	// tells the file to use.
#define OPEN_NEW_SONG_CHANNEL_PREF	"Open New Song Channel"
#define OPEN_NEW_SONG_FILE_PREF		"Open New Song File"
#define	OPEN_FROM_QUERY_PREF		"Open From Query"
	// This preference is actually a BMessage, which in turn contains:
	// "on" -- bool, whether or not the item is currently active
	// "name" -- string, the name of the item
	// "ref" -- ref, the entry_ref where the query is located
	// "skip" -- bool, if true than single top-level directories are skipped
#define CURRENT_SKIN_PREF			"Skin"
#define SIGNATURE_CHOICES_PREF		"Signature Choices"
	// A BMessage with an equal number of "beats" and "beat value" int32s.
#define UNDO_HISTORY_PREF			"Undo History"
#define TRACK_WIN_FOLLOW_PREF		"Track Win Follow"
	// A bool
#define TRACK_WIN_PLAY_TO_END_PREF	"Track Win Play to End"
	// A bool
#define TRACK_HEIGHT_PREF			"Track Height"
	// An int32
#define PHRASE_LABEL_HEIGHT_PREF	"Phrase Label Height"
	// An int32
#define AUTHOR_PREF					"Author"
	// A string
#define EMAIL_PREF					"Email"
	// A string

// SEQ_REFRESH_WINDOW_MSG flags
enum {
	SEQ_REFRESH_SONG_WINDOW		= 0x00000001,
	SEQ_REFRESH_PHRASES			= 0x00000011,

	SEQ_REFRESH_TRACK_WINDOW	= 0x00010000
};

/*---------------------------------------------------------
 * Pipeline drawing
 *---------------------------------------------------------*/
extern float	PIPELINE_SHADE_1;
extern float	PIPELINE_SHADE_2;
extern float	PIPELINE_SHADE_3;

void			seq_draw_hrz_pipe(	BView* view, float left,
									float top, float right, float shade = 1);
rgb_color		seq_darken(rgb_color c, float f);



#endif // __ARPHEADER_SEQUITUR_SEQUITURDEFS_H__
