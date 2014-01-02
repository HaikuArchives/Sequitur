/* AmDefs.h
 * Global definitions for the ARP MIDI classes.
 */

#ifndef AMPUBLIC_AMDEFS_H
#define AMPUBLIC_AMDEFS_H

#include <SupportDefs.h>
#include <support/String.h>
#include <limits.h>
class BMessage;

// In 4/4 time:
// Measure 1, Beat 1 = 0,
// Measure 1, Beat 2 = 6720,
// Measure 2, Beat 1 = 26880, etc
// Why this particular value?  Well,
// 6720 == 2*2*2*2*2*2*3*5*7
#define PPQN	(6720)

// Type for an event time.  Using the above factor, this gives us about
// 560,000 4-beat measures of music.
typedef int64 AmTime;
#define AM_TIME_MIN LONGLONG_MIN
#define AM_TIME_MAX LONGLONG_MAX

// A string to identify my time (when being passed in a BMessage, for example)
#define SZ_AMTIME				"AmTime"
#define AM_START_TIME_STR		"start_time"
#define AM_END_TIME_STR			"end_time"

// Covers for using AmTime values in messages
status_t add_time(BMessage& msg, const char* name, AmTime timeVal);
status_t find_time(const BMessage& msg, const char* name, int32 i, AmTime* timeVal);
status_t find_time(const BMessage& msg, const char* name, AmTime* timeVal);

// Covers for using AmTime values in messages, which are independent of
// the current PPQN.
status_t add_rel_time(BMessage& msg, const char* name, AmTime timeVal);
status_t find_rel_time(const BMessage& msg, const char* name, int32 i, AmTime* timeVal);
status_t find_rel_time(const BMessage& msg, const char* name, AmTime* timeVal);

// Startup status reports.
void am_report_startup_status(const char* msg);

typedef void (*am_startup_status_func)(const char* msg);
void am_set_startup_status_func(am_startup_status_func func);

/* Input filters process all data as it streams in from outside the
 * application, output filters process data before it is sent off
 * for realization.
 */
enum AmPipelineType {
	INPUT_PIPELINE,
	THROUGH_PIPELINE,
	OUTPUT_PIPELINE,
	NULLINPUTOUTPUT_PIPELINE,
	_NUM_PIPELINE,
	/* The following are not actually their own pipelines, but are
	 * shortcuts to logical pipelines.  The SOURCE_PIPELINE is the
	 * head of the INPUT_PIPELINE, the DESTINATION_PIPELINE is the
	 * tail of the OUTPUT_PIPELINE.
	 */
	SOURCE_PIPELINE,
	DESTINATION_PIPELINE
};

/* Number of control numbers.
 */
#define AM_CONTROLLER_SIZE			(128)

enum AmMarker {
		AM_NO_MARKER			= 0x00000000,
		AM_POSITION_MARKER		= 0x00000001,
		AM_LEFT_LOOP_MARKER		= 0x00000002,
		AM_RIGHT_LOOP_MARKER	= 0x00000004,
		_AM_NUM_MARKER
};

enum AmMotionMode {
	RHYTHM_MODE			= 1,
	PROGRESSION_MODE	= 2,
	ENVELOPE_MODE		= 3
};

/* These are the available types of views.
 *		ARRANGE_VIEW is the view used in the Arrange window.
 *		PRI_VIEW is the view used in the top half of the
 *				track edit window.
 *		SEC_VIEW is the view used in the bottom half of the
 *				track edit window.
 */
enum TrackViewType {
	ARRANGE_VIEW,
	PRI_VIEW,
	SEC_VIEW,
	
	_NUM_VIEW
};

enum {
	AM_ORDERED_TRACK_MSG		= 'aOrT',
		// Pointer "track_id".  This is send to the data and info views
		// when the primary track in the window changes.
		// Int32 "track_order".  The new position of this track.  If it's
		// zero, then this is now the primary track.  If it's below zero
		// or  not here, then this track has been cleared (which will never
		// happen to the primary track).
	AM_SATURATION_MSG			= 'aSat',
		// int32 "sat_type".  0 indicates the ordered track saturation changed,
		// 1 indicates the shadow track saturation changed.

	AM_PLAY_MMC					= 'aMcP',
	AM_STOP_MMC					= 'aMcS',
	AM_RECORD_MMC				= 'aMcR',
	AM_FASTFORWARD_MMC			= 'aMcF',
	AM_REWIND_MMC				= 'aMcW'
};

	/*---------------------------------------------------------
	 * CHANGE NOTIFICATION
	 * Change notification happens from AmTrack and AmSong
	 * implementation classes.  The available messages an
	 * observer can subscribe to are described in those classes.
	 * There are also a few details in common between the two
	 * that are described here.
	 *---------------------------------------------------------*/

/* Whenever a song or track sends out a prechange or postchange notice,
 * the client that caused the change notice has the option of supplying
 * itself as the sender argument in the message.
 */
#define SZ_SENDER		"sender"
/* When clients are notified about changes to events in songs and tracks
 * via the PRECHANGE_* and POSTCHANGE_* observer messages, the message
 * will also contain the additional field
 *		"status"	An int32 that describes the state of the event.
 */
#define SZ_STATUS		"status"

enum AmStatus {
	AM_NO_STATUS			= 0,	// This should never actually occur, it's more a safety measure
	AM_CHANGED,						// A catch-all
	AM_ADDED,						// The event has been added to the domain object
	AM_REMOVED						// The event has been removed from the domain object
};

/* Various objects in the system can be referenced by their ID's.
 */
typedef void*			device_id;
typedef void*			event_id;
typedef void*			filter_id;
typedef void*			pipeline_id;
typedef void*			pipeline_matrix_id;
typedef void*			song_id;
typedef void*			tool_id;
typedef void*			tool_bar_id;
typedef void*			track_id;
/* This is the signature for the default view factory.  This factory can
 * always be accessed by calling AmGlobals().FactoryNamed( BString(DEFAULT_FACTORY_SIGNATURE) );
 */
#define DEFAULT_FACTORY_SIGNATURE	"arp:Default"

/* Defines a single endpoint uniquely in the studio.
 */
enum AmEndpointType {
	AM_PRODUCER_TYPE	= 1,
	AM_CONSUMER_TYPE	= 2
};
	
struct am_studio_endpoint {
	BString				name;				// the name of the endpoint in the midi roster
	AmEndpointType		type;				// producer or consumer
	int32				id;					// the endpoint's id in the midi roster
	int32				channel;

	am_studio_endpoint();
	am_studio_endpoint(const am_studio_endpoint& o);
	am_studio_endpoint(const char* name, AmEndpointType type, int32 id, uchar channel);
	~am_studio_endpoint();

	am_studio_endpoint&	operator=(const am_studio_endpoint& o);
	bool				operator==(const am_studio_endpoint& o) const;

private:
	bool operator!=(const am_studio_endpoint& o) const;
};

/* Answer the default MIDI control name for the given control number, 0 - 127.
 * The answer will be NULL if you go out of range.
 */
const char* am_control_name(uint8 controlNumber);

/* Types of sysex commands a device can store.
 */
enum AmCommandType
{
	AM_COMMAND_NONE,
	AM_COMMAND_SINGLE,
	AM_COMMAND_MULTI
};

	/*---------------------------------------------------------
	 * ERRORS
	 *---------------------------------------------------------*/

#define AM_ERROR_BASE				B_ERRORS_END + 0x1000

enum {
	ARPERR_PARTIAL_OPERATION = AM_ERROR_BASE
};

	/*---------------------------------------------------------
	 * MESSAGES
	 *---------------------------------------------------------*/
enum {
	AM_DRAG_TOOL_MSG		= 'aDrT'
	// String, the tool's unique name
};

	/*---------------------------------------------------------
	 * IMAGE CONSTANTS.  Use the ImageManager() to access the
	 * actual images.
	 *---------------------------------------------------------*/

#define AM_PHRASE_BG_STR			"Phrase BG"
#define AM_SONG_CONTROL_BG_STR		"Song Control BG"
#define AM_TRACK_CONTROL_BG_STR		"Track Control BG"
#define SLICE_INFO_TOP_BG			"Slice Info Top BG"
#define SLICE_PROPERTY_MENU_NORMAL_IMAGE_STR "Slice Property Menu Normal"
#define SLICE_PROPERTY_MENU_PRESSED_IMAGE_STR "Slice Property Menu Pressed"
#define CHANNEL_01_IMAGE_STR		"Channel 01"
#define CHANNEL_02_IMAGE_STR		"Channel 02"
#define CHANNEL_03_IMAGE_STR		"Channel 03"
#define CHANNEL_04_IMAGE_STR		"Channel 04"
#define CHANNEL_05_IMAGE_STR		"Channel 05"
#define CHANNEL_06_IMAGE_STR		"Channel 06"
#define CHANNEL_07_IMAGE_STR		"Channel 07"
#define CHANNEL_08_IMAGE_STR		"Channel 08"
#define CHANNEL_09_IMAGE_STR		"Channel 09"
#define CHANNEL_10_IMAGE_STR		"Channel 10"
#define CHANNEL_11_IMAGE_STR		"Channel 11"
#define CHANNEL_12_IMAGE_STR		"Channel 12"
#define CHANNEL_13_IMAGE_STR		"Channel 13"
#define CHANNEL_14_IMAGE_STR		"Channel 14"
#define CHANNEL_15_IMAGE_STR		"Channel 15"
#define CHANNEL_16_IMAGE_STR		"Channel 16"
#define FILTER_BG_PREFIX			"Filter BG "
#define FILTER_BG_0					"Filter BG 0"
#define FILTER_BORDER_OVER			"Filter Border Over"
#define FILTER_ADDON_MISSING		"Filter AddOn Missing"
#define MIDI_PORT_FILTER_IMAGE_STR	"Midi Port Filter"
#define AM_TOOL_BORDER_NORMAL		"Tool Border Normal"
#define AM_TOOL_BORDER_OVER			"Tool Border Over"
#define AM_TOOL_BORDER_PRESSED		"Tool Border Pressed"
#define AM_INITIAL_TOOL_NORMAL		"Initial Tool Normal"
#define AM_VELOCITY_FF_STR			"Velocity - ff"
#define AM_VELOCITY_F_STR			"Velocity - f"
#define AM_VELOCITY_MF_STR			"Velocity - mf"
#define AM_VELOCITY_MP_STR			"Velocity - mp"
#define AM_VELOCITY_P_STR			"Velocity - p"
#define AM_VELOCITY_PP_STR			"Velocity - pp"
#define AM_VELOCITY_PPP_STR			"Velocity - ppp"

/*----------------- The standard view factory images ----------*/
#define	TREBLE_CLEF_IMAGE_STR		"treble_clef"
#define	BASS_CLEF_IMAGE_STR			"bass_clef"

//#define		AM_LOGGING		(1)

void am_log_bstr(BString& str);
void am_log_cstr(const char* str);

#ifdef AM_LOGGING
#define AM_LOG(str) am_log_cstr(str);
#define AM_BLOG(str) am_log_bstr(str);
#else
#define AM_LOG(str) ;
#define AM_BLOG(str) ;
#endif

#endif
