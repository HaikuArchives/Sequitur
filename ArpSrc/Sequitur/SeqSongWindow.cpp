/* SeqSongWindow.cpp
 */
#include <cstdio>
#include <cassert>
#include <cstring>
#include <RecentItems.h>
#include <Roster.h>
#include <experimental/BitmapButton.h>
#include <SelfWritten/DocApplication.h>
#include <InterfaceKit.h>
#include <kernel/fs_attr.h>
#include <StorageKit.h>
#include <support/Autolock.h>
#include <support/Locker.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpSafeDelivery.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpMultiScrollBar.h"
#include "ArpViews/ArpRangeControl.h"
#include "ArpViews/ArpTwoStateButton.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"

#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmStandardMidiFile.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmSongFunctionRoster.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmTransport.h"

#include "Sequitur/SeqMeasureControl.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqNavMenu.h"
#include "Sequitur/SeqPipelineMatrixView.h"
#include "Sequitur/SeqPhraseMatrixView.h"
#include "Sequitur/SeqSignatureWindow.h"
#include "Sequitur/SeqSongWindow.h"
#include "Sequitur/SeqSongTitleMatrixView.h"
#include "Sequitur/SeqSongIndexMatrixView.h"
#include "Sequitur/SeqSplitterView.h"
#include "Sequitur/SeqTempoControl.h"
#include "Sequitur/SeqTimeViews.h"
#include "Sequitur/SeqTrackWindow.h"

static const char*	SZ_TEMPO_VIEW			= "tempo";
static const char*	SZ_MEASURE_VIEW			= "measure";
static const char*	MBT_VIEW_SZ				= "mbt view";
static const char*	HMSM_VIEW_SZ			= "hmsm view";
static const char*	FILE_MENU_SZ			= "File";
static const char*	TRACKS_MENU_SZ			= "Tracks";
static const char*	SEQUITUR_CONFIGURATION_STR	= "sequitur configuration";
static const char*	FUNCTION_STR			= "function";

#define ARPMSG_REMOVETRACK			'paRt'
#define ARPMSG_EDITTRACK			'paEt'
#define ARPMSG_REQUESTTRACK			'paQt'
#define ARPMSG_ADDTRACK				'paAt'
#define ARPMSG_OPENFILTERWINDOW		'paOf'
#define ARPMSG_ABOUTSEQUITUR		'paAs'
#define ARPMSG_EXPORTMIDI			'paEx'

static const uint32		EDIT_TRACK_NAME_MSG			= 'pETN';
static const uint32		OPEN_TEMPO_MSG				= 'paOt';
static const uint32		SELECT_ALL_MSG				= 'isea';
static const uint32		SELECT_NONE_MSG				= 'isen';
static const uint32		PERFORM_FUNCTION_MSG		= 'ipfm';

static const uint32		SHOW_MANUAL_MSG				= 'ishM';
static const uint32		SHOW_FILTER_GUIDE_MSG		= 'ishF';
static const uint32		SHOW_TOOL_GUIDE_MSG			= 'ishT';
static const uint32		MERGE_PHRASES_MSG			= 'phrM';
static const uint32		SEPARATE_PHRASES_MSG		= 'phrS';
static const uint32		DELETE_SELECTION_MSG		= 'pdeS';
static const uint32		EXPAND_MARKED_RANGE_MSG		= 'iExR';
static const uint32		CONTRACT_MARKED_RANGE_MSG	= 'iCnR';
static const uint32		GROUP_MSG					= 'i_gr';
static const uint32		UNGROUP_MSG					= 'iugr';
static const uint32		MOVE_TRACK_UP_MSG			= 'itup';
static const uint32		MOVE_TRACK_DOWN_MSG			= 'itdn';
static const uint32		PANIC_MSG					= 'ipan';
static const int32		EDIT_MENU_INDEX				= 1;

static const uint32		REWIND_SONG_MSG				= 'irwn';
static const uint32		SCROLL_ARRANGE_MSG			= 'scrA';
static const char*		DELTA_STR					= "delta";
static const char*		SPLITTER_1_STR				= "splitter_1";
static const char*		SPLITTER_2_STR				= "splitter_2";
static const char*		SPLITTER_3_STR				= "splitter_3";

static const uint32		ARPMSG_TEMPOCHANGE			= 'temp';
static const float		DEFAULT_BEAT_LENGTH			= 8;

#define STR_OVERVIEW			"OverView"

#define STR_MEASURE_LAYER		"MeasureLayer"
#define STR_AMSONG_WINDOW		"VersionWindow"

static const char*		TRANSPORT_VIEW_SZ		= "transport view";
static const char*		INPUT_PANEL_STR			= "input panel";
static const char*		HRZ_INPUT_PIPELINE_STR	= "hrz input pipeline";
static const char*		PHRASE_MATRIX_STR		= "arrange panel";
static const char*		OUTPUT_PANEL_STR		= "output panel";
static const char*		HRZ_OUTPUT_PIPELINE_STR	= "hrz output pipeline";
static const char*		HRZ_DEST_PIPELINE_STR	= "hrz dest pipeline";
static const char*		RECORD_BUTTON_STR		= "record button";
static const char*		LOOP_BUTTON_STR			= "loop button";

static const BBitmap*	TRANSPORT_BM			= 0;

#define I_INITIAL_HRZ_FILTERS_WIDTH			(100)
#define I_INITIAL_HRZ_OUTPUT_FILTERS_WIDTH	(27)
#define I_INITIAL_HRZ_FILTERS_WIDTH_TOTAL	(I_INITIAL_HRZ_FILTERS_WIDTH + I_INITIAL_HRZ_OUTPUT_FILTERS_WIDTH)

static const char*		gAttrQueryString		= "_trk/qrystr";

enum {
	NEW_TRACK_MSG				= '@adt',
	NEW_TRACKS_FOR_DEVICE_MSG	= '@add',
	REMOVE_TRACK_MSG			= '@rmt'
};

static const uint32	LOOP_MSG	= '@lop';

static uint32 next_free_group(const AmSong* song);

/***************************************************************************
 * _SONG-MEASURE-CONTROL
 * This SeqMeasureControl subclass adds the ability to display and create
 * a selected time range.
 ***************************************************************************/
class _SongMeasureControl : public SeqMeasureControl
{
public:
	_SongMeasureControl(BRect frame,
						const char* name,
						AmSongRef songRef,
						AmTimeConverter& mtc,
						float leftIndent = 0,
						float rightIndent = 0,
						int32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	virtual ~_SongMeasureControl();
	
	float			ScrollDelta() const;
	void			SetScrollDelta(float delta);

	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage* msg);
	virtual	void	MouseUp(BPoint pt);

protected:
	virtual void	DrawCenterBgOn(BRect cBounds, BView* view, AmTime songEndTime);
	virtual void	DrawRightBgOn(BRect rBounds, BView* view, AmTime songEndTime);
	virtual void	DrawLeftOn(BRect lBounds, BView* view);

	void			DrawBackground(	BView* view, rgb_color c, BRect bounds );

private:
	typedef SeqMeasureControl	inherited;
	/* Protects any data that will be accesed by the scroll thread.
	 */
	mutable BLocker				mAccess;

	/* Set to true if my superclass passes on handling the mouse moved events
	 * in its mouse down -- i.e., I only handle mouse moves if my superclass doesn't.
	 */
	bool						mTrackMouse;
	/* Set to the mouse down point.
	 */
	BPoint						mOrigin;
	/* These times are determined in the mouse down.  They are the start and end
	 * of the measure that the mouse was clicked in (unless there was a previous
	 * selection range that the user is not overwriting, and then both of these
	 * values are set to the start of that range)
	 */
	AmTime						mLeftTime, mRightTime;
	/* Used during mouse tracking to store the last selected range.
	 */
	AmRange						mPrevRange;
	
	/* Fill the start and end times in with the start and end times of the measure
	 * at the current point.
	 */
	void						GetMeasureTimes(BPoint pt, AmTime* start, AmTime* end);
	void						GetMeasureTimes(const AmSong* song, AmTime time, AmTime* start, AmTime* end);
	/* Answer my window's selections.
	 */
	SeqSongSelections*			Selections() const;
	void						SetSelectionRange(AmRange range);
	bool						IsRecording() const;
	/* The thread that does the scrolling is launched in the mouse down and
	 * killed in the mouse up.
	 */
	float						mScrollDelta;
	thread_id					mScrollThread;
	static int32				ScrollThreadEntry(void *arg);
};

/***************************************************************************
 * _SONG-TRANSPORT-VIEW
 * This view displays the tempo and time MBT views.
 ***************************************************************************/
class _SongTransportView : public BView
{
public:
	_SongTransportView(BPoint topLeft, AmSongRef songRef);
	
	virtual void	Draw(BRect clip);

private:
	typedef BView		inherited;
};

/*************************************************************************
 * SEQ-SONG-WINDOW
 *************************************************************************/
static BRect figure_best_frame( BWindow* owner, const BMessage* config )
{
	BScreen	s(owner);
	if( !s.IsValid() ) return BRect( 100, 40, 700, 300 );
	BRect	sf = s.Frame();
	BRect	f(	sf.Width() * 0.25, sf.Height() * 0.25,
				sf.Width() * 0.75, sf.Height() * 0.75 );
	if( config ) {
		float	l, t, r, b;
		if( config->FindFloat( "f_left", &l ) == B_OK
				&& config->FindFloat( "f_top", &t ) == B_OK
				&& config->FindFloat( "f_right", &r ) == B_OK
				&& config->FindFloat( "f_bottom", &b ) == B_OK )
			f.Set( l * sf.Width(), t * sf.Height(), r * sf.Width(), b * sf.Height() );
	}
	return f;
}

static BString track_name_from_device(const char* mfg, const char* prod)
{
	BString		s(mfg);
	if (mfg && prod) s << " ";
	s << prod;
	return s;
}

static BString track_name_from(const BMessage& msg)
{
	const char*		str;
#if 0
	if (msg.FindString("device_label", &str) == B_OK) return str;
	const char*		str2;
	if (msg.FindString("device_mfg", &str) != B_OK) str = NULL;
	if (msg.FindString("device_product", &str2) != B_OK) str2 = NULL;
	if (str || str2) return track_name_from_device(str, str2);
#endif
	if (msg.FindString("seqf:name", &str) != B_OK) return BString(SZ_UNTITLED_TRACK);
	if (strlen(str) < 11 ) return str;
	if (strncmp(str, "/dev/midi/", 10) != 0 ) return str; 
	BString			n(str);
	n.RemoveFirst( BString("/dev/midi/") );
	return n;
}

static AmFilterAddOnHandle* associated_source(AmFilterAddOnHandle* dest, AmFilterRoster* roster, int32 N)
{
	AmFilterAddOn::type assocType = AmFilterAddOn::SOURCE_FILTER;
	BString	assocClass = dest->KeyForType(assocType);
	for (int32 k=0; k<N; k++) {
		AmFilterAddOnHandle* src = dynamic_cast<AmFilterAddOnHandle*>(roster->AddOnAt(k));
		if (src) {
			if (assocClass.Length() > 0 && assocType == src->Type() &&
					assocClass == src->Key() && dest->Name() == src->Name())
				return src;
		}
	}
	return 0;
}

/* The addonLimit tells this function to only create tracks for
 * the given number of addons.  Values less than 1 indicate that
 * the tracks should be created for each installed addon.
 */
static void tracks_for_each_device(AmSong* song, uint32 channelLimit, int32 addonLimit = 0)
{
	/* Create tracks for the first four channels of each installed
	 * destination.
	 */
	AmFilterRoster* roster = AmFilterRoster::Default();
	if (roster) {
		BAutolock _l(roster->Locker());
		int32 N = addonLimit;
		if (N < 1) N = roster->CountAddOns();
		BMessenger me(song);
		for (int32 i=0; i<N; i++) {
			AmFilterAddOnHandle* h =
				dynamic_cast<AmFilterAddOnHandle*>(roster->AddOnAt(i));
			/* If I can find one, get a source filter that matches the
			 * destination filter.
			 */
			AmFilterAddOnHandle* srcH = associated_source(h, roster, N);
			if (h && h->Type() == AmFilterAddOn::DESTINATION_FILTER
					&& h->Subtype() == AmFilterAddOn::NO_SUBTYPE) {
				BMessage	msg('null');
				BMessage	srcMsg('null');
				if (h->GetArchiveTemplate(&msg) == B_OK) {
					if( srcH ) if( srcH->GetArchiveTemplate(&srcMsg) != B_OK ) srcH = 0;
					ArpRef<AmFilterAddOn> addon;
					ArpRef<AmFilterAddOn> srcAddon = 0;
					if (roster) addon = roster->FindFilterAddOn(&msg);
					if (addon) {
						if (srcH) srcAddon = roster->FindFilterAddOn(&srcMsg);
						BMessage	config(msg);
						BMessage	srcConfig(srcMsg);
						bool		more = true, srcMore = true;
						if (!srcAddon) srcMore = false;
						BString		trackName = track_name_from(msg);
						for( uint32 k = 0; k < channelLimit && more; k++ ) {
							more = false;
							BString	tn( trackName );
							tn << " - " << k + 1;
							AmTrack* track = new AmTrack( song, tn.String() );
							if (track) {
								int32		trackHeight;
								if (seq_get_int32_preference(TRACK_HEIGHT_PREF, &trackHeight) != B_OK) trackHeight = AM_DEFAULT_TRACK_HEIGHT;
								track->SetPhraseHeight(trackHeight);
								if (song->AddTrack(track) == B_OK) {

									track->InsertFilter(addon, DESTINATION_PIPELINE, -1, &config);
									AmFilterHolderI* holder = track->Filter(DESTINATION_PIPELINE);
									if (holder && holder->Filter()) {
										config.MakeEmpty();
										if (holder->Filter()->GetNextConfiguration(&config) == B_OK) {
											more = true;
										}
									}
									/* Add in the source filter.  Get its next config.  If it doesn't
									 * have one, then set the flag to stop adding any more of this
									 * source filter.
									 */
									if( srcAddon && srcMore ) {
										track->InsertFilter(srcAddon, SOURCE_PIPELINE, -1, &srcConfig);
										AmFilterHolderI* srcHolder = track->Filter(SOURCE_PIPELINE);
										if (holder && holder->Filter()) {
											srcConfig.MakeEmpty();
											if (srcHolder->Filter()->GetNextConfiguration(&srcConfig) == B_OK) {
												srcMore = true;
											}
										} else srcMore = false;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	song->ClearDirty();
}

// #pragma mark -

SeqSongWindow::SeqSongWindow(DocApplication *wr, entry_ref *ref, const char *title,
					 window_look /*look*/, window_feel /*feel*/,
					 uint32 flags, uint32 workspace)
	: inherited(wr, ref, figure_best_frame(this, NULL),
				title, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				flags | B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK, workspace),
	  AmSongObserver(AmSongRef()),
	  mIsRecording(false), mSelections(NULL), mOpenMenu(0), mUndoItem(0),
	  mRedoItem(0), mMergeItem(NULL), mSeparateItem(NULL), mDeleteItem(NULL),
	  mExpandItem(NULL), mContractItem(NULL),
	  mDeviceItem(NULL), mFilterItem(NULL), mMotionItem(NULL), mToolItem(NULL),
	  mImportPanel(NULL), mSavePanel(NULL), mQuitAfterSave(false),
	  mAddedRefToSettings(false)
{
	AmSongRef	songRef;
	int32		undoHistory;
	if (seq_get_int32_preference(UNDO_HISTORY_PREF, &undoHistory) != B_OK) undoHistory = AM_DEFAULT_UNDO_HISTORY;

	if ( (AmGlobals().NewSong(songRef, undoHistory)) != B_OK ) {
		printf("SeqSongWindow::SeqApplication() couldn't perform NewSong()\n");
	}
	InitData(songRef);

	const char*		pref;
	if (seq_get_string_preference(OPEN_NEW_SONG_PREF, &pref) == B_OK) {
		if (strcmp(pref, "channels") == 0 && !ref) {
			int32	channels;
			am_report_startup_status("Creating initial song...");
			if (seq_get_int32_preference(OPEN_NEW_SONG_CHANNEL_PREF, &channels) != B_OK) channels = 1;
			// WRITE SONG BLOCK
			AmSong*		song = WriteLock();
			if (song) {
				song->SetSuppressUndo(true);
				tracks_for_each_device(song, channels);
				song->SetSuppressUndo(false);
			}
			WriteUnlock( song );
			// END WRITE SONG BLOCK
		} else if (!ref && strcmp(pref, "file") == 0) {
			entry_ref	ref;
			if (seq_get_ref_preference(OPEN_NEW_SONG_FILE_PREF, &ref) == B_OK) {
				BString msg("Loading song ");
				msg += ref.name;
				msg += "...";
				am_report_startup_status(msg.String());
				BEntry		entry(&ref);
				if (entry.InitCheck() == B_OK) Load(&entry, false, NO_CONFIGURATION);
			}
		} else if (ref) {
			BString msg("Loading song ");
			msg += ref->name;
			msg += "...";
			am_report_startup_status(msg.String());
			BEntry		entry(ref);
			if (entry.InitCheck() == B_OK) Load(&entry);
		}
	}
	
	finish_startup_status();
	CheckForGuides();
}

void SeqSongWindow::CheckForGuides()
{
	entry_ref ref;
	if (seq_get_filters_ref(&ref) != B_OK)
		seq_generate_filter_docs();
	if (seq_get_tools_ref(&ref) != B_OK)
		seq_generate_tool_docs();
}

void SeqSongWindow::InitData(AmSongRef songRef)
{
	SetSongRef(songRef);
	mSelections = 0;
	mOverViewHeight = 23;
	mTrackSongPosition = false;
	mOpenMenu = NULL;
	mUndoItem = NULL;
	mRedoItem = NULL;
	mImportPanel = mSavePanel = NULL;
	mLastMmcTime = 0;
	
	SetName(STR_AMSONG_WINDOW);
	
	mMtc.SetBeatLength(DEFAULT_BEAT_LENGTH);

	AddMainMenu();
	AddViews();
	AddCommonFilter(this);
	
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		song->EndTimeChangeNotice();
		song->Transport().StartWatching( BMessenger(this) );
		song->SetWindow(BMessenger(this));
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

SeqSongWindow::~SeqSongWindow()
{
	RemoveCommonFilter(this);
	if ( mSignatureWin.IsValid() ) mSignatureWin.SendMessage(B_QUIT_REQUESTED);
	CloseTrackWindows();
	SongRef().StopTransport();
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::~SeqSongWindow() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if( song ) {
		song->Transport().StopWatching( BMessenger(this) );
	}
	ReadUnlock( song );
	// END READ SONG BLOCK
	delete mSelections;
	delete mImportPanel;
	delete mSavePanel;
}

bool SeqSongWindow::IsDirty()
{
	bool dirty = false;
	
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::IsDirty() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if( song ) dirty = song->IsDirty();
	ReadUnlock( song );
	// END READ SONG BLOCK
	
	printf("Returning dirty=%d\n", dirty);
	
	return dirty;
}

static bool is_syx_file(entry_ref& ref)
{
	/* Let the sniffer determine the type, and check it against
	 * the syx MIME type.
	 */
	BMimeType	guess;
	if (BMimeType::GuessMimeType(&ref, &guess) == B_OK) {
		if (strcmp(guess.Type(), "application/x-syx") == 0)
			return true;
	}
	/* Find the node's MIME type, and check it against the syx type.
	 */
	BNode		node(&ref);
	if (node.InitCheck() != B_OK) return false;
	BNodeInfo	info(&node); 
	if (info.InitCheck() != B_OK) return false;
	char		mime[B_MIME_TYPE_LENGTH];
	if (info.GetType(mime) != B_OK) return false;
	if (strcmp(mime, "application/x-syx") == 0)
		return true;
	/* Check to see if the file ends with .syx.
	 */
	BPath		path(&ref);
	if (path.InitCheck() != B_OK) return false;
	BString		leaf( path.Leaf() );
	if (leaf.Length() < 4) return false;
	leaf.Remove(0, leaf.Length() - 4);
	if (leaf.ICompare(".syx") == 0) return true;
	return false;
}

status_t SeqSongWindow::Load(BEntry *ent, bool merge, uint32 flags)
{
	entry_ref	ref;
	const char	*op = "reading";
	BString		message;
	BString		filename;
	
	op = "finding";
	status_t err = ent->GetRef(&ref);
	if (err == B_OK && is_syx_file(ref) ) {
		return LoadSyx(ref, merge, flags);
	}
	
	BFile f;
	if( !err ) {
		op = "opening";
		err = f.SetTo(&ref, B_READ_ONLY);
	}
	
	AmStandardMidiFile smf;
	if( !err ) {
		op = "reading";
		err = smf.ReadFile(&f);
	}
	
	if (err == B_OK) {
		AmSong* song = WriteLock();
		if (song) {
			if( !merge ) song->StartLoad();
			AmPhraseEvent* ev = smf.DetachTempoTrack();
			if (ev) {
				if ( ev->Phrase() ) {
					AmNode* node = ev->Phrase()->HeadNode();
					while (node) {
						song->AddEvent(node->Event());
						node = node->next;
					}
					ev->Phrase()->SetList();
				}
				ev->Delete();
			}
			ev = smf.DetachSignatureTrack();
			if (ev) {
				if ( ev->Phrase() ) {
					AmNode* node = ev->Phrase()->HeadNode();
					while (node) {
						song->AddEvent(node->Event());
						node = node->next;
					}
					ev->Phrase()->SetList();
				}
				ev->Delete();
			}
			BMessage				inputConnections, outputConnections;
			while (smf.CountTracks() > 0) {
				smf_track_info trackInfo;
				ev = smf.RemoveTrack(0, &trackInfo);
				if (ev) {
					AmTrack* track = new AmTrack(song, trackInfo.title.String());
					song->AddTrack(track);
					AmNode*			n = ev->Phrase()->HeadNode();
					while (n) {
						AmNode*		n2 = n->next;
						AmEvent*	e = n->Event();
#if 0
						if (e) {
							if (e->Type() == e->PHRASE_TYPE) {
								ev->Phrase()->Remove(e);
								track->AddEvent(NULL, e);
							} else e->Delete();
						}
#endif
						if (e) {
							ev->Phrase()->Remove(e);
							if (e->Type() == e->PHRASE_TYPE) track->AddEvent(NULL, e);
							else e->Delete();
						}
						n = n2;
					}
					delete ev;

//					printf("Source filter: "); trackInfo.input_device.PrintToStream();
//					printf("Input filter: "); trackInfo.input_filters.PrintToStream();
					if (trackInfo.has_output_filters)
						track->UnflattenFilters(&trackInfo.output_filters, OUTPUT_PIPELINE);
					if (trackInfo.has_output_device)
						track->UnflattenFilters(&trackInfo.output_device, DESTINATION_PIPELINE);
					else if (trackInfo.channel >= 0) {
						ArpRef<AmFilterAddOn> addon =
							AmFilterRoster::Default()->FindFilterAddOn("arp:MidiConsumer");
						if (addon) {
							BMessage config;
							config.AddInt32("channel", trackInfo.channel);
							track->InsertFilter(addon, DESTINATION_PIPELINE,
												-1, &config);
						}
					}
					if (trackInfo.has_input_filters)
						track->UnflattenFilters(&trackInfo.input_filters, INPUT_PIPELINE);
					if (trackInfo.has_input_device)
						track->UnflattenFilters(&trackInfo.input_device, SOURCE_PIPELINE);
					if (trackInfo.has_edit_filters)
						track->UnflattenFilters(&trackInfo.edit_filters, THROUGH_PIPELINE);
					if (trackInfo.has_track_settings)
						track->PutSettings(&trackInfo.track_settings);
					if (trackInfo.has_input_connections)
						inputConnections = trackInfo.input_connections;
					if (trackInfo.has_output_connections)
						outputConnections = trackInfo.output_connections;
				}
			}
			if (!inputConnections.IsEmpty() ) song->UnflattenConnections(&inputConnections, INPUT_PIPELINE);
			if (!outputConnections.IsEmpty() ) song->UnflattenConnections(&outputConnections, OUTPUT_PIPELINE);
			if (!merge) song->FinishLoad();
			song->EndTimeChangeNotice();
			WriteUnlock(song);
		}

		/* Read in the file's configuration.
		 */
		if( !(flags&NO_CONFIGURATION) ) {
			attr_info	attInfo;
			if( f.GetAttrInfo( SEQUITUR_CONFIGURATION_STR, &attInfo ) == B_OK
					&& attInfo.type == B_MESSAGE_TYPE ) {
				char*		buffer = (char*)malloc( sizeof(char) * (attInfo.size + 10) );
				if( buffer ) {
					ssize_t	size = f.ReadAttr( SEQUITUR_CONFIGURATION_STR, B_MESSAGE_TYPE, 0, buffer, attInfo.size + 5 );
					if( size == attInfo.size ) {
						BMessage	config;
						if( config.Unflatten(buffer) == B_OK ) SetConfiguration( &config );
					}
					free(buffer);
				}
			}
		}
	}
	
	if( err < B_OK ) {
		BString str("Error ");
		str += op;
		str += " file";
		if( filename.Length() > 0 ) {
			str += "\n";
			str += filename;
		}
		str += ":\n";
		if( message.Length() > 0 ) {
			str += message;
		} else {
			str += strerror(err);
		}
		str += ".";
		(new BAlert(Title(), str.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}

	return err;
}

status_t SeqSongWindow::Save(BEntry *ent, const BMessage* /*args*/)
{
	/* Force all my track windows to persist themselves.
	 */
	for (uint32 k = 0; k < mTrackWins.size(); k++) {
		if (mTrackWins[k].IsValid() ) {
			BHandler*			target;
			BLooper*			looper;
			if ( (target = mTrackWins[k].Target(&looper)) != NULL) {
				SeqTrackWindow*	win = dynamic_cast<SeqTrackWindow*>(target);
				if (win && win->Lock() ) {
					win->SaveState();
					win->Unlock();
				}
			}
		}
	}
	AmStandardMidiFile smf;
	{
		#ifdef AM_TRACE_LOCKS
		printf("SeqSongWindow::Save() read lock 1\n"); fflush(stdout);
		#endif
		const AmSong* song = ReadLock();
		if (song) {
			if (song->TempoPhrase() ) smf.SetTempoTrack(new AmRootPhraseEvent( *(song->TempoPhrase()) ));
			else smf.SetTempoTrack(new AmRootPhraseEvent());
			smf.SetSignatureTrack(new AmRootPhraseEvent(song->Signatures()));
			
			for (size_t i=0; i<song->CountTracks(); i++) {
				const AmTrack*	track = song->Track(i);
				if (!track) continue;

				AmEvent*		events = track->RawPlaybackList(0, -1, PLAYBACK_IGNORE_MUTE);
				AmPhraseEvent*	phrase = new AmRootPhraseEvent;
				if (events) {
					AmNode*		head = NULL;
					AmNode*		tail = NULL;
					while (events) {
						AmEvent* ev = events;
						events = events->RemoveEvent();
						AmNode* node = new AmNode(ev);
						if (!tail) {
							head = tail = node;
						} else {
							tail->InsertNext(node);
							tail = node;
						}
					}
					phrase->Phrase()->SetList(head);
				}
				
				smf_track_info trackInfo;
				trackInfo.title = track->Title();
				track->FlattenFilters(&trackInfo.output_device, DESTINATION_PIPELINE);
				track->FlattenFilters(&trackInfo.output_filters, OUTPUT_PIPELINE);
				track->FlattenFilters(&trackInfo.input_device, SOURCE_PIPELINE);
				track->FlattenFilters(&trackInfo.input_filters, INPUT_PIPELINE);
				track->FlattenFilters(&trackInfo.edit_filters, THROUGH_PIPELINE);
				track->GetSettings(&trackInfo.track_settings);
				if (i == 0) {
					song->FlattenConnections(&trackInfo.input_connections, INPUT_PIPELINE);
					song->FlattenConnections(&trackInfo.output_connections, OUTPUT_PIPELINE);	
				}
				/* Write out my phrase info.
				 */
				AmNode*			n = track->Phrases().HeadNode();
				while (n && n->Event() ) {
					AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(n->Event() );
					if (n->Event()->Type() == n->Event()->PHRASE_TYPE && pe && pe->Phrase() ) {
						BMessage	msg;
						AmRange		range = pe->TimeRange();
						if (range.IsValid() && add_time(msg, AM_START_TIME_STR, range.start) == B_OK
								&& add_time(msg, AM_END_TIME_STR, range.end) == B_OK
								&& pe->Phrase()->GetProperties(msg) == B_OK) {
							trackInfo.phrases.AddMessage("phrase", &msg);
						}
					}
					n = n->next;
				}
				
//				printf("Source filter: "); trackInfo.input_device.PrintToStream();
//				printf("Input filter: "); trackInfo.input_filters.PrintToStream();
				
				smf.AddTrack(phrase, trackInfo);
			}
			ReadUnlock(song);
		}
	}
	
	BFile		f(ent, B_READ_WRITE|B_ERASE_FILE);
	status_t	err = B_OK;

	// more code ripped from StyledEdit
	// maybe this code should be factored out in DocWindow
	if((err = f.InitCheck()) == B_NO_ERROR)
	{
		mode_t perms = 0;
		f.GetPermissions(&perms);
	
		if((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		{
			(new BAlert(B_EMPTY_STRING, "File is read-only.", "OK"))->Go();
			return err;
		}
	}
	else
	{
		BDirectory dir;
		ent->GetParent(&dir);

		entry_ref ref;
		ent->GetRef(&ref);

		dir.CreateFile(ref.name, &f);

		err = f.InitCheck();
		if( !err ) {
			BNodeInfo	ni;
			if(ni.SetTo(&f) == B_OK)
//				ni.SetType("audio/x-midi");
				ni.SetType("audio/midi");
		}
	}
	
	if (err == B_OK) {
		err = smf.WriteFile(&f);
		/* Write my configuration as an attribute on the file
		 */
		BMessage	config;
		if( GetConfiguration( &config ) == B_OK ) {
			BMallocIO	data;
			if( AmFlatten(config,&data) == B_OK ) {
				f.WriteAttr( SEQUITUR_CONFIGURATION_STR, B_MESSAGE_TYPE, 0,
								data.Buffer(), data.BufferLength() );
			}
		}
	}
	if (err == B_OK) {
		// Song is saved!
		#ifdef AM_TRACE_LOCKS
		printf("SeqSongWindow::Save() read lock 2\n"); fflush(stdout);
		#endif
		const AmSong* song = ReadLock();
		if (song) {
			song->ClearDirty();
			ReadUnlock(song);
		}
		entry_ref ref;
		if (ent->GetRef(&ref) == B_OK) {
			be_roster->AddToRecentDocuments(&ref);
			if (ref != fileref) {
				fileref = ref;
				SetTitle(ref.name);
				untitled = false;
			}
		}
	}
	
	if( err != B_OK ) {
		BString str("Error writing file");
		BPath path;
		if( ent->GetPath(&path) == B_OK ) {
			str << " " << path.Path();
		}
		str << ":\n" << strerror(err) << ".";
		(new BAlert(Title(), str.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}
	
	return err;
}


static MIDIRefFilter savefilefilter;

void SeqSongWindow::SaveAs() {
		if (!mSavePanel) {
			BMessenger msngr(this);	// only temporary...
			mSavePanel = new BFilePanel(B_SAVE_PANEL, &msngr);
			mSavePanel->SetRefFilter(&midifileFilter);
		}
		BEntry ent(&fileref);	// if anything was previously loaded
		if (ent.InitCheck() == B_OK) {
			BEntry dir;
			ent.GetParent(&dir);
			mSavePanel->SetPanelDirectory(&dir);
		}
		mSavePanel->Show();
}


void SeqSongWindow::WindowFrame(BRect *proposed)
{
	BAutolock l(this);
	
	// Should load preferred frame from a settings file.
	proposed->right = proposed->left + 800;
	proposed->bottom = proposed->top + 500;
}

static BMenuItem* remove_nav_menu(BMenu* menu)
{
	BMenuItem*	item = 0;
	for( int32 k = 0; (item = menu->ItemAt(k)); k++ ) {
		SeqNavMenu*		nav = dynamic_cast<SeqNavMenu*>( item->Submenu() );
		if( nav ) return menu->RemoveItem(k);
	}
	return 0;
}

static void remove_nav_menus(BMenu* menu)
{
	BMenuItem*		item;
	while( (item = remove_nav_menu(menu)) ) delete item;
}

static void track_item_label(	const char* prefix, uint32 selectionCount,
								const char* postfix, BMenuItem* item)
{
	ArpASSERT(prefix);
	BString		label(prefix);
	if (selectionCount > 0) {
		if (selectionCount == 1) label << "1 track";
		else label << selectionCount << " tracks";
		if (postfix) label << postfix;
		item->SetLabel(label.String() );
		item->SetEnabled(true);
	} else {
		label << "track";
		if (postfix) label << postfix;
		item->SetLabel(label.String() );
		item->SetEnabled(false);
	}
}

static bool any_selections_grouped(const AmSong* song, SeqSongSelections* selections)
{
	if (!selections) return false;
	uint32		count = selections->CountTracks();
	for (uint32 k = 0; k < count; k++) {
		const AmTrack*	track = song->Track(selections->TrackAt(k));
		if (track && track->Groups() > 0) return true;
	}
	return false;
}

void SeqSongWindow::MenusBeginning()
{
	SeqSongTitleMatrixView*	titles = HeaderManager();
	if (titles) titles->StopEdit();
//	if (titles) titles->StopAllEdits();

	uint32			nextGroup = 0;
	bool			ungroupEnabled = false;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::MenusBeginning() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if (song) {
		if (mSelections) nextGroup = next_free_group(song);
		const BUndoContext* u = song->UndoContext();
		if (mUndoItem && u) {
			if (u->CountUndos() > 0) {
				const char* name = u->UndoName();
				BString dispName = "Undo ";
				dispName << (name ? name : "Something");
				mUndoItem->SetEnabled(true);
				mUndoItem->SetLabel(dispName.String());
			} else {
				mUndoItem->SetEnabled(false);
				mUndoItem->SetLabel("Undo");
			}
		}
		if (mRedoItem && u) {
			if (u->CountRedos() > 0) {
				const char* name = u->RedoName();
				BString dispName = "Redo ";
				dispName << (name ? name : "Something");
				mRedoItem->SetEnabled(true);
				mRedoItem->SetLabel(dispName.String());
			} else {
				mRedoItem->SetEnabled(false);
				mRedoItem->SetLabel("Redo");
			}
		}
		ungroupEnabled = any_selections_grouped(song, mSelections);
	}
	ReadUnlock(song);
	// END READ SONG BLOCK

	if (mSelections && !(mSelections->IsEmpty()) ) {
		if (mMergeItem) mMergeItem->SetEnabled(true);
		if (mSeparateItem) mSeparateItem->SetEnabled(true);
		if (mDeleteItem) mDeleteItem->SetEnabled(true);
	} else {
		if (mMergeItem) mMergeItem->SetEnabled(false);
		if (mSeparateItem) mSeparateItem->SetEnabled(false);
		if (mDeleteItem) mDeleteItem->SetEnabled(false);
	}

	/* Expand / Contract edit items.
	 */
	bool			ecOn = false;
	SeqMeasureControl*	mc = MeasureControl();
	if (mc && mc->MarkerVisible(mc->LEFT_LOOP_MARKER)
			&& mc->MarkerVisible(mc->RIGHT_LOOP_MARKER)
			&& mc->MarkerTime(mc->RIGHT_LOOP_MARKER) > mc->MarkerTime(mc->LEFT_LOOP_MARKER))
		ecOn = true;
	if (mExpandItem) mExpandItem->SetEnabled(ecOn);
	if (mContractItem) mContractItem->SetEnabled(ecOn);
	
	/* Track menu items.
	 */	
	BMenuBar*	bar = KeyMenuBar();
	if( !bar ) return;
	BMenuItem*	keyItem, *item;
	BMenu*		sub;
	if ( (keyItem = bar->FindItem(TRACKS_MENU_SZ)) && (sub = keyItem->Submenu()) ) {
		uint32		selectionCount = 0;
		if (mSelections) selectionCount = mSelections->CountTracks();
		if ( (item = sub->FindItem(REMOVE_TRACK_MSG)) )
			track_item_label("Delete ", selectionCount, NULL, item);
		if ( (item = sub->FindItem(EDIT_TRACK_NAME_MSG)) ) {
			if (selectionCount == 1) item->SetEnabled(true);
			else item->SetEnabled(false);
		}
		if ( (item = sub->FindItem(MOVE_TRACK_UP_MSG)) )
			track_item_label("Move ", selectionCount, " up", item);
		if ( (item = sub->FindItem(MOVE_TRACK_DOWN_MSG)) )
			track_item_label("Move ", selectionCount, " down", item);

		if ( (item = sub->FindItem(GROUP_MSG)) ) {
			if (nextGroup > 0) item->SetEnabled(true);
			else item->SetEnabled(false);
		}
		if ( (item = sub->FindItem(UNGROUP_MSG)) )
			item->SetEnabled(ungroupEnabled);
	}

	if( (keyItem = bar->FindItem(FILE_MENU_SZ)) && (sub = keyItem->Submenu()) ) {
		remove_nav_menus( sub );
		int32		index = 2;
		/* Add a dynamic menu item for each valid Open From Query preference.
		 */
		BMessage	owq;
		for( int32 k = 0; seq_get_message_preference(OPEN_FROM_QUERY_PREF, &owq, k) == B_OK; k++ ) {
			bool		on;
			entry_ref	ref;
			if( owq.FindBool("on", &on) != B_OK ) on = true;
			if( on && owq.FindRef("ref", &ref) == B_OK ) {
				BString		name("Open ");
				const char*	str;
				if( owq.FindString("name", &str) == B_OK ) name << str;
				else name << "<no name>";
				BNode		node( &ref );
				attr_info	info;
				if ( node.InitCheck() == B_OK) {
					if (node.GetAttrInfo( gAttrQueryString, &info) == B_OK) {
						BString		predicate;
						if (node.ReadAttr(	gAttrQueryString, B_STRING_TYPE, 0,
											predicate.LockBuffer((int32)info.size),
											(size_t)info.size ) == info.size) {
							predicate.UnlockBuffer();
							SeqNavMenu*		nm = new SeqNavMenu( name.String(), BMessenger(be_app) );
							if( nm && (item = new BMenuItem( nm ) ) ) {
								nm->SetPredicate( predicate.String() );
								nm->SetSkipLoneDirectory( true );
								sub->AddItem( item, index );
								index++;
							}
						}
					} else if ( node.IsDirectory() ) {
						BPath	path(&ref);
						const char*		pathStr;
						if ( path.InitCheck() == B_OK && (pathStr = path.Path()) ) {
							SeqNavMenu*		nm = new SeqNavMenu( name.String(), BMessenger(be_app) );
							if( nm && (item = new BMenuItem( nm ) ) ) {
								nm->SetPath( pathStr );
								nm->SetSkipLoneDirectory( true );
								sub->AddItem( item, index );
								index++;
							}
						}
					}
				}
			}
		}	
	}
	if (mDeviceItem) {
		if ( seq_flag_is_on(SEQ_DEVICE_WINDOW_ACTIVE) ) mDeviceItem->SetMarked(true);
		else mDeviceItem->SetMarked(false);
	}
	if (mFilterItem) {
		if ( seq_flag_is_on(SEQ_FILTER_WINDOW_ACTIVE) ) mFilterItem->SetMarked(true);
		else mFilterItem->SetMarked(false);
	}
	if (mMotionItem) {
		if ( seq_flag_is_on(SEQ_MOTION_WINDOW_ACTIVE) ) mMotionItem->SetMarked(true);
		else mMotionItem->SetMarked(false);
	}
	if (mToolItem) {
		if ( seq_flag_is_on(SEQ_TOOL_WINDOW_ACTIVE) ) mToolItem->SetMarked(true);
		else mToolItem->SetMarked(false);
	}
}

void SeqSongWindow::MenusEnded()
{
	BMenuBar*	bar = KeyMenuBar();
	if( !bar ) return;
	BMenuItem*	file = bar->FindItem( "File" );
	if( !file ) return;
	BMenu*		sub = file->Submenu();
	if( !sub ) return;
	BMenuItem*	item;
	for( int32 k = 0; (item = sub->ItemAt( k )); k++ ) {
		SeqNavMenu*		nm = dynamic_cast<SeqNavMenu*>( item->Submenu() );
		if( nm ) nm->Rewind();
	}
}

void SeqSongWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	if (message->what == B_MOUSE_DOWN) {
		Activate(true);
	} else if (message->what == B_KEY_DOWN) {
		int32	key;
		if (message->FindInt32( "key", &key ) == B_OK) {
#if 0
BString		logs("SeqSongWindow::DispatchMessage key ");
logs << key << "\n";
AM_BLOG(logs);
#endif
			if (key == 0x5b) {			// Enter on a numeric keypad
				if( SongRef().IsPlaying() ) { BMessage m(STOP_SONG_MSG); PostMessage(&m); }
				else { BMessage m(PLAY_SONG_MSG); PostMessage(&m); }
				return;
			} else if (key == 0x64) {	// 0 on a numeric keypad
//AM_LOG("SeqSongWindow::DispatchMessage key is numeric 0\n");
				BMessage		m(REWIND_SONG_MSG);
				PostMessage(&m);
				return;
			} else if (key == 0x65) {	// . / DEL on a numeric keypad
				ArpTwoStateButton*	ctrl = dynamic_cast<ArpTwoStateButton*>( FindView(RECORD_BUTTON_STR) );
				if( ctrl ) {
					( ctrl->ButtonState() ) ? ctrl->SetButtonState(false) : ctrl->SetButtonState(true);
				}
				return;
			}
		}
	}
	inherited::DispatchMessage(message, handler);
}

void SeqSongWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			ObserverMessageReceived(msg);
			break;
		case CHANGE_SIGNATURE_MSG: {
			int32		measure = 1, beats = 4, beatValue = 4;
			status_t	m_err = msg->FindInt32("measure", &measure);
			status_t	b_err = msg->FindInt32("beats", &beats);
			status_t	v_err = msg->FindInt32("beat value", &beatValue);
			AmSignature		sig;
			if (GetLeftSignature(sig) == B_OK) {
				if (m_err != B_OK) measure = sig.Measure();
				if (b_err != B_OK) beats = sig.Beats();
				if (v_err != B_OK) beatValue = sig.BeatValue();
			}
			ShowSignatureWindow(measure, beats, beatValue);
			} break;
		case SHOWTRACKWIN_MSG:
			HandleDoubleClickTrack(msg);
			break;
		case SCROLL_ARRANGE_MSG:
			float		delta;
			if( msg->FindFloat( DELTA_STR, &delta ) == B_OK ) ScrollArrangeBy( delta * 0.5 );
			break;
		case ARPMSG_TEMPOCHANGE: {
			float		tempo;
			if (msg->FindFloat("tempo", &tempo) == B_OK) {
				// WRITE SONG BLOCK
				AmSong*		song = WriteLock();
				if (song) {
					song->Transport().SetBPM(tempo);
					song->SetBPM(tempo);
				}
				WriteUnlock(song);
				// END WRITE SONG BLOCK
			}
		} break;
		case TRANSPORT_CHANGE_MSG:
			TransportChangeReceived(msg);
			break;
		case SEQ_REFRESH_WINDOW_MSG: {
			int32		flags;
			if (msg->FindInt32("flags", &flags) == B_OK) {
				if (flags&SEQ_REFRESH_PHRASES) {
					SeqPhraseMatrixView*	v = ArrangeManager();
					if (v) v->Invalidate();
				}
			}
		} break;
		// File menu
		case PERFORM_FUNCTION_MSG:
			PerformSongFunction(msg);
			break;
		case SHOW_MANUAL_MSG: {
//AM_LOG("SeqSongWindow::MessageReceived() SHOW_MANUAL_MSG\n");
			entry_ref	ref;
			if (seq_get_doc_index_ref(&ref) == B_OK) be_roster->Launch(&ref);
		} break;
		case SHOW_FILTER_GUIDE_MSG: {
//AM_LOG("SeqSongWindow::MessageReceived() SHOW_FILTER_GUIDE_MSG\n");
			seq_generate_filter_docs();
			entry_ref	ref;
			if (seq_get_filters_ref(&ref) == B_OK) be_roster->Launch(&ref);
		} break;
		case SHOW_TOOL_GUIDE_MSG: {
//AM_LOG("SeqSongWindow::MessageReceived() SHOW_TOOL_GUIDE_MSG\n");
			seq_generate_tool_docs();
			entry_ref	ref;
			if (seq_get_tools_ref(&ref) == B_OK) be_roster->Launch(&ref);
		} break;
		case ARPMSG_ABOUTSEQUITUR:
			be_app->AboutRequested();
			break;
		// Edit menu
		case B_UNDO: {
			AmSong*		song = WriteLock();
			if( song && song->UndoContext() ) {
				AmTime	oldEnd = song->CountEndTime();
				song->UndoContext()->Undo();
				AmTime	newEnd = song->CountEndTime();
				if (oldEnd != newEnd) song->EndTimeChangeNotice(newEnd);
			}
			WriteUnlock( song );
		} break;
		case 'REDO': {
			AmSong*		song = WriteLock();
			if( song && song->UndoContext() ) {
				AmTime	oldEnd = song->CountEndTime();
				song->UndoContext()->Redo();
				AmTime	newEnd = song->CountEndTime();
				if (oldEnd != newEnd) song->EndTimeChangeNotice(newEnd);
			}
			WriteUnlock( song );
		} break;
		case SELECT_ALL_MSG:
			SelectAll();
			break;
		case SELECT_NONE_MSG:
			SetSelections(0);
			break;
		case MERGE_PHRASES_MSG:
			MergePhrases();
			break;
		case SEPARATE_PHRASES_MSG:
			SeparatePhrases();
			break;
		case DELETE_SELECTION_MSG:
			DeleteSelection();
			break;
		case EXPAND_MARKED_RANGE_MSG:
			ExpandMarkedRange();
			break;
		case CONTRACT_MARKED_RANGE_MSG:
			ContractMarkedRange();
			break;
		case PANIC_MSG:
			HandleStopSong(true);
			break;
		// Tracks menu
		case NEW_TRACK_MSG:
			NewTrack();
			break;
		case NEW_TRACKS_FOR_DEVICE_MSG:
			NewTrack(msg);
			break;
		case EDIT_TRACK_NAME_MSG:
			EditTrackName();
			break;
		case REMOVE_TRACK_MSG:
			DeleteSelectedTracks();
			break;
		case MOVE_TRACK_UP_MSG:
			MoveTracksBy(-1);
			break;
		case MOVE_TRACK_DOWN_MSG:
			MoveTracksBy(1);
			break;
		case GROUP_MSG:
			GroupTracks();
			break;
		case UNGROUP_MSG:
			UngroupTracks();
			break;
		// Windows menu
		case ARPMSG_OPENFILTERWINDOW:
			be_app->PostMessage( SHOW_FILTERS_MSG );
			break;
		case OPEN_TEMPO_MSG:
			OpenTempoWindow();
			break;
		// Transport buttons
		case REWIND_SONG_MSG: {
			HandleSetSongPosition(0);
		} break;
		case AM_PLAY_MMC:
		case PLAY_SONG_MSG:
			HandlePlaySong();
			break;
		case PLAY_SONG_FROM_TIME_MSG:
			AmTime			time;
			if (msg->FindInt64(SEQ_TIME_STR, &time) != B_OK) time = 0;
			HandleSetSongPosition(time);
//			HandlePlaySong();
			break;
		case AM_STOP_MMC:
		case STOP_SONG_MSG:
			HandleStopSong();
			break;
		case AM_RECORD_MMC:
		case RECORD_MSG: {
			bool	isRecording;
			if (msg->FindBool("on", &isRecording) == B_OK) {
				SetIsRecording(isRecording);
//				HandlePlaySong();
			}
		} break;
		case AM_REWIND_MMC:
			HandleMoveSongPosition(-PPQN);
#if 0
			{ bigtime_t			t = system_time();
//			printf("old %lld cur %lld\n", mLastMmcTime, t);
			if (t - mLastMmcTime > 1000) HandleMoveSongPosition(-PPQN);
			mLastMmcTime = t;
			}
#endif
			break;
		case AM_FASTFORWARD_MMC:
			HandleMoveSongPosition(PPQN);
			break;
		case LOOP_MSG: {
			SeqMeasureControl*	mc = MeasureControl();
			bool				isLooping;
			if (mc && msg->FindBool("on", &isLooping) == B_OK) {
				mc->SetMarkerVisible(AM_LEFT_LOOP_MARKER | AM_RIGHT_LOOP_MARKER, isLooping);
			}
		} break;
		case B_SAVE_REQUESTED: {
			entry_ref r;
			const char * name;
			msg->FindRef("directory", &r);
			BDirectory dir(&r);
			msg->FindString("name", &name);
			BEntry ent(&dir, name);
			Save(&ent);
			if (mQuitAfterSave) Quit();
		}
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

bool SeqSongWindow::QuitRequested()
{
	if (IsDirty()) {
		BString alertStr("Save changes to \"");
		alertStr << Title() << "\"?";
		BAlert *quitAlert = new BAlert("Alert", alertStr.String(),
			 "Cancel", "Don't save", "Save",
			 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		int32 res = quitAlert->Go();
		switch (res) {
			case 2:
					if(untitled) {
						SaveAs();
						mQuitAfterSave = true;
						return false;	// until actually saved
					} else {
						BEntry ent(&fileref);
						Save(&ent);
					}
					// fall through
			case 1:	return true;
			case 0: return false;
		}
	}
	return true;
}


void SeqSongWindow::Quit()
{
	RemoveCommonFilter(this);
	if( (seq_is_quitting() == false)
			&& (seq_count_significant_windows() <= 1) ) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		AddRefToSettings();
	}
	if( seq_is_quitting() == false ) {
		AmGlobals().RemoveSong( SongRef().SongId() );
	} else {
		AddRefToSettings();
	}
	inherited::Quit();
}

void SeqSongWindow::WindowActivated(bool state)
{
	inherited::WindowActivated(state);
	if (state) AmGlobals().SelectSong(SongRef());
}

void SeqSongWindow::ScrollArrangeBy(float delta)
{
	BScrollBar*		sb = ArrangeScrollBar();
	if (!sb) return;
	float			newValue = sb->Value() + delta;
	if (newValue < 0) newValue = 0;
	if ( newValue != sb->Value() ) sb->SetValue(newValue);
}

void SeqSongWindow::ScrollArrangeTo(float value)
{
	BScrollBar*		sb = ArrangeScrollBar();
	if (!sb) return;
	if (value < 0) value = 0;
	if (value != sb->Value()) sb->SetValue(value);
}

SeqSongSelections* SeqSongWindow::Selections() const
{
	return mSelections;
}

static void add_track(std::vector<track_id>& vec, track_id tid)
{
	for (uint32 k = 0; k < vec.size(); k++)
		if (vec[k] == tid) return;
	vec.push_back(tid);
}

void SeqSongWindow::SetSelections(SeqSongSelections* selections)
{
	AmRange				oldRange, newRange;
	std::vector<track_id>	tracks;
	if (mSelections) {
		oldRange = mSelections->TimeRange();
		newRange = oldRange;
		uint32		count = mSelections->CountTracks();
		for( uint32 k = 0; k < count; k++ ) {
			add_track(tracks, mSelections->TrackAt(k) );
		}
	}
	if (selections != mSelections) delete mSelections;
	mSelections = selections;

	if (mSelections) {
		newRange = oldRange + mSelections->TimeRange();
		uint32		count = mSelections->CountTracks();
		for (uint32 k = 0; k < count; k++) {
			add_track(tracks, mSelections->TrackAt(k) );
		}
	}
	if (ArrangeManager() ) {
		ArrangeManager()->InvalidateSelections(newRange, tracks);
	}
	SeqMeasureControl*	mc = MeasureControl();
	if (mc) mc->Invalidate();
	/* TEST:  Let's try this out:  Playthrough and Record states are
	 * set automatically based on the current selection.
	 */
	// WRITE SONG BLOCK
	AmSong*			song = WriteLock();
	if (song) {
		AmTrack*	track;
		for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
			if (mSelections && mSelections->IncludesTrack(track->Id() )) {
				track->SetRecordMode(AmTrack::RECORD_REPLACE_MODE);
			} else {
				track->SetRecordMode(AmTrack::RECORD_OFF_MODE);
			}
		}
		if (mSelections) song->SetRecordRange(mSelections->TimeRange() );
		else song->SetRecordRange( AmRange(-1, -1) );
		
		if (song->UndoContext() ) {
			song->UndoContext()->SetUndoName("Set Modes");
			song->UndoContext()->CommitState();
		}
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK

	/* Redisplay any track index views that were affected.
	 */
	SeqSongIndexMatrixView*		matrix = IndexMatrix();
	if (matrix) matrix->InvalidateTracks(tracks);
}

bool SeqSongWindow::IsRecording() const
{
	return mIsRecording;
}

void SeqSongWindow::SelectAll()
{
	SeqSongSelections*	selections = SeqSongSelections::New();
	if (!selections) return;

	AmTime				endTime = 0;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::SelectAll() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) {
		const AmTrack*	track = NULL;		
		for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
			selections->AddTrack(track->Id() );
		}
		endTime = song->CountEndTime();
	}
	ReadUnlock( song );
	// END READ SONG BLOCK

 	selections->SetTimeRange( AmRange(0, endTime) );
 	SetSelections(selections);
}

bool SeqSongWindow::IsSignificant() const
{
	return true;
}

status_t SeqSongWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT(config);
	config->what = SONG_WINDOW_SETTING_MSG;
	/* Store my window layout.
	 */
	status_t					err = GetDimensions(config, this);
	if (err != B_OK) return err;
	BRect						b(Bounds() );
	BView*						v = FindView(SPLITTER_1_STR);
	if (v) config->AddFloat(SPLITTER_1_STR, v->Frame().left / b.Width() );
	v = FindView(SPLITTER_2_STR);
	if (v) config->AddFloat(SPLITTER_2_STR, v->Frame().left / b.Width() );
	v = FindView(SPLITTER_3_STR);
	if (v) config->AddFloat(SPLITTER_3_STR, v->Frame().left / b.Width() );

	/* Store my open tracks.
	 */
	for (uint32 k = 0; k < mTrackWins.size(); k++) {
		if (mTrackWins[k].IsValid() ) {
			BHandler*			target;
			BLooper*			looper;
			if ( (target = mTrackWins[k].Target(&looper)) != NULL) {
				SeqTrackWindow*	win = dynamic_cast<SeqTrackWindow*>(target);
				if (win && win->Lock() ) {
					BMessage	trackMsg;
					if (win->GetConfiguration(&trackMsg) == B_OK)
						config->AddMessage("track_win", &trackMsg);
					win->Unlock();
				}
			}
		}
	}

	return B_OK;
}

status_t SeqSongWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	/* Restore my layout.
	 */
	status_t		err = SetDimensions(config, this);
	if (err != B_OK) return err;
	BRect			b(Bounds() );
	float			f;
	if (config->FindFloat(SPLITTER_1_STR, &f) == B_OK) {
		SeqSplitterView*	v = dynamic_cast<SeqSplitterView*>(FindView(SPLITTER_1_STR) );
		if (v) v->MoveVerticalSplitter(f * b.Width() );
	}
	if (config->FindFloat(SPLITTER_2_STR, &f) == B_OK) {
		SeqSplitterView*	v = dynamic_cast<SeqSplitterView*>(FindView(SPLITTER_2_STR) );
		if (v) v->MoveVerticalSplitter(f * b.Width() );
	}
	if (config->FindFloat(SPLITTER_3_STR, &f) == B_OK) {
		SeqSplitterView*	v = dynamic_cast<SeqSplitterView*>(FindView(SPLITTER_3_STR) );
		if (v) v->MoveVerticalSplitter(f * b.Width() );
	}
	/* Restore my ordered tracks.
	 */
	BMessage		trackMsg;
	for (int32 k = 0; config->FindMessage("track_win", k, &trackMsg) == B_OK; k++) {
		BWindow*	win = SeqTrackWindow::ClassOpen(SongRef(), &trackMsg);
		if (win) mTrackWins.push_back(BMessenger(win) );
		trackMsg.MakeEmpty();
	}
	return B_OK;
}

static void add_song_functions(BMenu* menu)
{
	if (!menu) return;
	AmSongFunctionRoster*	roster = AmSongFunctionRoster::Default();
	if (!roster) return;

	BMenu*		submenu = new BMenu("Functions", B_ITEMS_IN_COLUMN);
	if (!submenu) return;
	AmSongFunctionI*	function = NULL;	
	for(uint32 k = 0; (function = roster->FunctionAt(k)); k++) {
		if ( function->Name() ) {
			BMessage*		msg = new BMessage(PERFORM_FUNCTION_MSG);
			if (msg) {
				msg->AddString( FUNCTION_STR, function->Name() );
				add_menu_item(submenu, function->Name(), msg, 0);
			}
		}
	}
	BMenuItem*	item = new BMenuItem(submenu);
	if (item) menu->AddItem(item);
}

void SeqSongWindow::AddMainMenu()
{
	BMenuBar*	menuBar;
	BMenu*		menu;
	BMenuItem*	item;
	BRect		rect = Bounds();
	
	rect.bottom = rect.top + Prefs().Size(MAINMENU_Y);
	menuBar = new BMenuBar(rect, NULL, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
			B_ITEMS_IN_ROW, FALSE);

	/* File Menu
	 */
	menu = new BMenu(FILE_MENU_SZ, B_ITEMS_IN_COLUMN);
	item = new BMenuItem("New", new BMessage(DOC_APP_NEW_WINDOW), 'N');
	item->SetTarget(be_app);
	menu->AddItem(item);
	mOpenMenu = new BMenuItem(BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
								NULL, NULL, be_app,
								20, false, NULL, "application/x-vnd.Arp-sequitur"),
								new BMessage(DOC_APP_OPEN));
	mOpenMenu->SetTarget(be_app);
	mOpenMenu->SetShortcut('O', B_COMMAND_KEY);
	menu->AddItem(mOpenMenu);
	/* This is where all the dynamic Open From Query menu items are placed.
	 * However, they aren't generated here, but in MenusBeginning(), so that
	 * the see any changes that have been made to the preferences since the
	 * song window's been opened.
	 */
	menu->AddSeparatorItem();
	
	add_menu_item( menu, "Save", DOC_WIN_SAVE, 'S' );
	add_menu_item( menu, "Save as" B_UTF8_ELLIPSIS, DOC_WIN_SAVE_AS, 0);
	add_menu_item( menu, "Close", B_QUIT_REQUESTED, 'W' );
	
	menu->AddSeparatorItem();
	item = new BMenuItem("Preferences" B_UTF8_ELLIPSIS, new BMessage(SHOW_PREFERENCES_MSG), 'P');
	item->SetTarget(be_app);
	menu->AddItem(item);
	add_song_functions(menu);
	menu->AddSeparatorItem();

	item = new BMenuItem("Documentation", new BMessage(SHOW_MANUAL_MSG));
	item->SetTarget(this); 
	menu->AddItem(item);
	entry_ref	docref;
	if (seq_get_doc_index_ref(&docref) != B_OK) item->SetEnabled(false);
#if 0	
	item = new BMenuItem("User\'s guide", new BMessage(SHOW_MANUAL_MSG)); 
	item->SetTarget(this); 
	menu->AddItem(item);
	entry_ref	docref;
	if (seq_get_users_guide_ref(&docref) != B_OK) item->SetEnabled(false);
#endif
	item = new BMenuItem("Filter guide", new BMessage(SHOW_FILTER_GUIDE_MSG));
	item->SetTarget(this); 
	menu->AddItem(item);
//	if (seq_get_filters_ref(&docref) != B_OK) item->SetEnabled(false);
	item = new BMenuItem("Tool guide", new BMessage(SHOW_TOOL_GUIDE_MSG));
	item->SetTarget(this); 
	menu->AddItem(item);
//	if (seq_get_tools_ref(&docref) != B_OK) item->SetEnabled(false);

	menu->AddSeparatorItem();
	add_menu_item( menu, "About Sequitur" B_UTF8_ELLIPSIS, ARPMSG_ABOUTSEQUITUR, 0 );
	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 0);
	
	/* Edit Menu
	 */
	menu = new BMenu("Edit", B_ITEMS_IN_COLUMN);
	mUndoItem = add_menu_item( menu, "Undo", B_UNDO, 'Z', B_COMMAND_KEY );
	mRedoItem = add_menu_item( menu, "Redo", 'REDO', 'Z', B_SHIFT_KEY|B_COMMAND_KEY );
	menu->AddSeparatorItem();
	add_menu_item( menu, "Select all", SELECT_ALL_MSG, 'A' );
	add_menu_item( menu, "Select none", SELECT_NONE_MSG, 'D' );
	menu->AddSeparatorItem();
	mMergeItem = add_menu_item( menu, "Merge phrases", MERGE_PHRASES_MSG, 'M' );
	mSeparateItem = add_menu_item( menu, "Separate phrases", SEPARATE_PHRASES_MSG, 'E' );
	mDeleteItem = add_menu_item( menu, "Delete selection", DELETE_SELECTION_MSG, 0 );
	menu->AddSeparatorItem();
	mExpandItem = add_menu_item( menu, "Expand marked range", EXPAND_MARKED_RANGE_MSG, 0 );
	mContractItem = add_menu_item( menu, "Contract marked range", CONTRACT_MARKED_RANGE_MSG, 0 );
	menu->AddSeparatorItem();
	add_menu_item( menu, "Set time signature" B_UTF8_ELLIPSIS, CHANGE_SIGNATURE_MSG, 0);
	menu->AddSeparatorItem();
	add_menu_item(menu, "Stop all notes", PANIC_MSG, 0);
	item = new BMenuItem(menu);
	menuBar->AddItem( item, EDIT_MENU_INDEX );

	/* Tracks Menu
	 */
	menu = new BMenu(TRACKS_MENU_SZ, B_ITEMS_IN_COLUMN);
	add_menu_item(menu, "Add track", NEW_TRACK_MSG, 'T');
	
	/* Create a submenu of all destination filters
	 */
	BMenu* submenu = new BMenu("Add tracks for", B_ITEMS_IN_COLUMN);
	AmFilterRoster* roster = AmFilterRoster::Default();
	if (roster) {
		BAutolock _l(roster->Locker());
		const int32 N = roster->CountAddOns();
		BMessenger me(this);
		for (int32 i=0; i<N; i++) {
			AmFilterAddOnHandle* h =
				dynamic_cast<AmFilterAddOnHandle*>(roster->AddOnAt(i));
			if (h && h->Type() == AmFilterAddOn::DESTINATION_FILTER) {
				BMessage* msg = new BMessage(NEW_TRACKS_FOR_DEVICE_MSG);
				if (h->GetArchiveTemplate(msg) == B_OK) {
					item = new BMenuItem(h->Name().String(), msg);
					item->SetTarget(me);
					submenu->AddItem(item);
				} else {
					delete msg;
				}
			}
		}
	}
	item = new BMenuItem(submenu);
	menu->AddItem(item);
	menu->AddSeparatorItem();
	add_menu_item(menu, "Edit name", EDIT_TRACK_NAME_MSG, 0);
	add_menu_item(menu, "Delete track", REMOVE_TRACK_MSG, 'R');
	menu->AddSeparatorItem();
	add_menu_item(menu, "Move track up", MOVE_TRACK_UP_MSG, B_UP_ARROW);
	add_menu_item(menu, "Move track down", MOVE_TRACK_DOWN_MSG, B_DOWN_ARROW);
#if 0
	menu->AddSeparatorItem();
	add_menu_item(menu, "Group", GROUP_MSG, 0);
	add_menu_item(menu, "Ungroup", UNGROUP_MSG, 0);
#endif
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 2);

	/* Windows Menu
	 */
	menu = new BMenu("Windows", B_ITEMS_IN_COLUMN);
	mDeviceItem = new BMenuItem("Devices", new BMessage(SHOW_MANAGE_DEVICES_MSG), 0);
	mDeviceItem->SetTarget(be_app);
	menu->AddItem(mDeviceItem);

	mFilterItem = add_menu_item(menu, "Filters", ARPMSG_OPENFILTERWINDOW, 0);

	mMotionItem = new BMenuItem("Motions", new BMessage(SHOW_MANAGE_MOTIONS_MSG), 0);
	mMotionItem->SetTarget(be_app);
	menu->AddItem(mMotionItem);

	item = new BMenuItem("Studio", new BMessage(SHOW_STUDIO_MSG), 0);
	item->SetTarget(be_app);
	menu->AddItem(item);

	add_menu_item(menu, "Tempo", OPEN_TEMPO_MSG, 0);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 3);

	mToolItem = new BMenuItem("Tools", new BMessage(SHOW_MANAGE_TOOLS_MSG), 0);
	mToolItem->SetTarget(be_app);
	menu->AddItem(mToolItem);

	/* Temp Menu
	 */
#if 0
	menu = new BMenu("Temp", B_ITEMS_IN_COLUMN);
	add_menu_item( menu, "Test Media", 'tmpM', 0 );
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 4);
#endif

	AddChild(menuBar);
	SetKeyMenuBar(menuBar);
}

void SeqSongWindow::AddViews()
{
	float	top = Prefs().Size( MAINMENU_Y ) + 1,
//			transportHeight = 37,
			transportHeight = 39,
			measureHeight = 25;
	InitializeControlLayer(top, transportHeight);
	
	top += (transportHeight + 1);
	InitializeMeasureLayer(top, top + measureHeight);

	top += (measureHeight + 1);
	InitializeTrackLayer(top);
}

static const BBitmap*	gControlBg = NULL;

class _AmControlView : public BView
{
public:
	_AmControlView(BRect frame, const char* name, uint32 resizeMask, uint32 flags)
			: BView(frame, name, resizeMask, flags)
	{
		if (!gControlBg) gControlBg = ImageManager().FindBitmap(AM_SONG_CONTROL_BG_STR);
	}

	virtual void AttachedToWindow()
	{
		BView::AttachedToWindow();
		SetViewColor(Prefs().Color(AM_CONTROL_BG_C) );
		if (gControlBg) SetViewBitmap(gControlBg);
	}
	
	virtual void Draw(BRect clip)
	{
		BView::Draw(clip);
		BRect	b = Bounds();
		if (clip.top <= b.top) {
			SetHighColor(0, 0, 0);
			StrokeLine( BPoint(clip.left, b.top), BPoint(clip.right, b.top) );
		}
		if (clip.top <= b.top + 1) {
			SetHighColor( tint_color(ViewColor(), B_LIGHTEN_2_TINT) );
			StrokeLine( BPoint(clip.left, b.top + 1), BPoint(clip.right, b.top + 1) );
		}
		if (clip.bottom >= (b.bottom - 1) ) {
			SetHighColor( tint_color(ViewColor(), B_DARKEN_2_TINT) );
			StrokeLine( BPoint(clip.left, b.bottom - 1), BPoint(clip.right, b.bottom - 1) );
		}
		if (clip.bottom >= b.bottom) {
			SetHighColor(0, 0, 0);
			StrokeLine( BPoint(clip.left, b.bottom), BPoint(clip.right, b.bottom) );
		}
	}
};

static BRect add_transport_button(	BRect frame, const char* name, uint32 what,
									const char* bmNormal, const char* bmPressed,
									BView* toView)
{
	const BBitmap*	bmN = ImageManager().FindBitmap( bmNormal );
	const BBitmap*	bmP = ImageManager().FindBitmap( bmPressed );
	if( !bmN || !bmP ) return frame;
	BMessage*		msg = new BMessage( what );
	if( !msg ) return frame;
	BRect		b = bmN->Bounds();
	frame.right = frame.left + b.Width();
	frame.bottom = frame.top + b.Height();
	BBitmapButton*	butt = new BBitmapButton( frame, name, 0, msg, bmN, bmN, bmP, bmP, bmP );
	if (butt) {
		/* Right now there's a bug in the buttons that causes them to resizes themslves
		 * based on label info, even if they don't have a label.  This compensates.
		 */
		butt->ResizeToPreferred();
		butt->SetValue(B_CONTROL_OFF);
		toView->AddChild( butt );
		frame.OffsetBy( BPoint( b.Width() + 1, 0 ) );
	}
	return frame;
}

static BRect add_two_state_button(	BRect frame, const char* name, uint32 what,
									const char* bmNormal, const char* bmPressed,
									BView* toView)
{
	const BBitmap*	bmN = ImageManager().FindBitmap( bmNormal );
	const BBitmap*	bmP = ImageManager().FindBitmap( bmPressed );
	if( !bmN || !bmP ) return frame;
	BMessage*		msg = new BMessage( what );
	if( !msg ) return frame;
	BRect		b = bmN->Bounds();
	frame.right = frame.left + b.Width();
	frame.bottom = frame.top + b.Height();
	ArpTwoStateButton*	butt = new ArpTwoStateButton( frame, name, 0, msg, bmN, bmN, bmP, bmP, bmP );
	if( butt ) {
		toView->AddChild( butt );
		frame.OffsetBy( BPoint( b.Width() + 1, 0 ) );
	}
	return frame;
}

void SeqSongWindow::InitializeControlLayer(float top, float height)
{
	BRect		b = Bounds();
	mControlBg = new _AmControlView(BRect(0, top, b.right, top + height),
									"control bg", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW );
	if (!mControlBg ) return;
	AddChild( mControlBg );
	
	BRect		f(4, 11, 4, 11);
	f = add_transport_button( f, "rewind", REWIND_SONG_MSG, REWIND_NORMAL_STR, REWIND_PRESSED_STR, mControlBg );
	f = add_transport_button( f, "play", PLAY_SONG_MSG, PLAY_NORMAL_STR, PLAY_PRESSED_STR, mControlBg );
//	f = add_transport_button( f, "play_start", PLAY_SONG_FROM_TIME_MSG, PLAY_FROM_START_NORMAL_STR, PLAY_FROM_START_PRESSED_STR, mControlBg );
	f = add_transport_button( f, "stop", STOP_SONG_MSG, STOP_NORMAL_STR, STOP_PRESSED_STR, mControlBg );
	f = add_two_state_button( f, RECORD_BUTTON_STR, RECORD_MSG, RECORD_NORMAL_STR, RECORD_PRESSED_STR, mControlBg );
	f = add_two_state_button( f, LOOP_BUTTON_STR, LOOP_MSG, LOOP_NORMAL_STR, LOOP_PRESSED_STR, mControlBg );

	float	left = f.left + 4;
	_SongTransportView*	stv = new _SongTransportView( BPoint(left, 4 ), SongRef() );
	if (stv) mControlBg->AddChild(stv);
}

int32 SeqSongWindow::InitializeMeasureLayer(float top, float bottom)
{
	BRect				b = Bounds();
	SeqMeasureControl*	ctrl;

	float	separatorWidth = 5,
			titleViewWidth = 60;

	b.Set(0, top, b.right, bottom);
	if ( (ctrl = new _SongMeasureControl(b, SZ_MEASURE_VIEW, SongRef(), mMtc, titleViewWidth + separatorWidth, 120)) != 0)
		AddChild(ctrl);
	return 0;
}

int32 SeqSongWindow::InitializeTrackLayer(float top)
{
	BRect		bounds = Bounds();
	BRect		rect;
	BView*		splitter;

	float		titleViewWidth = 130,
				inputW = 40,
				filterW = I_INITIAL_HRZ_FILTERS_WIDTH_TOTAL;
	float		splitterW = 5;
	
	SetSizeLimits(	filterW + titleViewWidth + ((Prefs().Size(V_SCROLLBAR_X) + 2) * 3),
					9000, 30, 9000);

	rect.left = 0;
	rect.top = top;
	rect.right = titleViewWidth;
	rect.bottom = bounds.bottom;
	BView*				headerPanel = NewHeaderPanel(rect);
	rect.left = rect.right + splitterW;
	rect.right = rect.left + inputW;
	BView*				inputPanel = NewInputPanel(rect);
	rect.left = rect.right + splitterW;
	rect.right = bounds.right - Prefs().Size( V_SCROLLBAR_X ) - 2 - filterW - splitterW;
	/* This is because of that FUCKING scroll bar bug that sets the orientation
	 * based on the initial frame -- frame has to be something arbitrary, then
	 * I can't actually set the correct frame until the view is attached.
	 */
	BScrollBar*			arrangeHsb = NULL;
	BRect				arrangeHsbF;
	BView*				arrangePanel = NewArrangePanel(rect, &arrangeHsb, &arrangeHsbF);
	rect.left = rect.right + splitterW;
	rect.right = bounds.right - Prefs().Size( V_SCROLLBAR_X ) - 2;
	BView*				outputPanel = NewOutputPanel( rect );
	if (!headerPanel || !inputPanel || !arrangePanel || !outputPanel) {
		delete headerPanel;
		delete inputPanel;
		delete arrangePanel;
		delete outputPanel;
		return (int32)top;
	}
	AddChild(headerPanel);
	splitter = NewSplitter(headerPanel->Frame(), inputPanel->Frame(), SPLITTER_1_STR, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_VERTICAL );
	if (splitter) AddChild(splitter);
	AddChild(inputPanel);
	splitter = NewSplitter(inputPanel->Frame(), arrangePanel->Frame(), SPLITTER_2_STR, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_VERTICAL );
	if (splitter) AddChild(splitter);
	AddChild(arrangePanel);
	/* FSB (fucking scrollbar bug) */
	if (arrangeHsb) {
		arrangeHsb->MoveTo(arrangeHsbF.left, arrangeHsbF.top);
		arrangeHsb->ResizeTo(arrangeHsbF.Width(), arrangeHsbF.Height() );
	}
	splitter = NewSplitter(arrangePanel->Frame(), outputPanel->Frame(), SPLITTER_3_STR, B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM, B_VERTICAL );
	if (splitter) AddChild(splitter);
	AddChild( outputPanel );

	/* Add in the vertical scroll bar.  It's important to do this last, after
	 * the manager views have been constructed.
	 */
	rect.left = bounds.right - Prefs().Size(V_SCROLLBAR_X) - 1;
	rect.right = bounds.right;
	rect.bottom = bounds.bottom - Prefs().Size(H_SCROLLBAR_Y) - 1;
	BView*		vsb = NewVerticalScrollBar(rect);
	if (vsb) AddChild(vsb);

	return 0;
}

BView* SeqSongWindow::NewHeaderPanel(BRect frame) const
{
	BScrollBar*				sb;
	SeqSongIndexMatrixView*	indexes;
	SeqSongTitleMatrixView*	header;
	BRect					rect;
	float					indexW = 42;
		
	if (mControlBg) indexW = 4 + mControlBg->StringWidth("999G9") + 12 + 4;

	// Create my track index view...
	rect.left = rect.top = 0;
	rect.right = indexW;
	rect.bottom = frame.Height() - Prefs().Size( H_SCROLLBAR_Y ) - 2;
	indexes = new SeqSongIndexMatrixView(rect, SongRef() );
	// Create my track header view...
	rect.left = indexW + 1;
	rect.right = frame.Width();
	header = new SeqSongTitleMatrixView(rect, SongRef() );
	// ...add a scroll bar...
	rect.left = 0;
	rect.bottom = frame.Height();
	rect.top = rect.bottom - Prefs().Size( H_SCROLLBAR_Y ) - 1;
	sb = new BScrollBar(rect, "HSB", header, 0, 0, B_HORIZONTAL);
	// ...wrap them up in a view.
	BView*				view = new BView( frame, "header panel", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW );
	if (!indexes || !header || !sb || !view) {
		delete indexes;
		delete header;
		delete sb;
		delete view;
		return 0;
	}
	sb->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	indexes->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM );
	header->SetResizingMode(B_FOLLOW_ALL);
	header->SetHorizontalScrollBar(sb);
	view->AddChild(indexes);
	view->AddChild(header);
	view->AddChild(sb);
	return view;
}

BView* SeqSongWindow::NewInputPanel(BRect frame) const
{
	BScrollBar*				sb;
	SeqPipelineMatrixView*	input;
	BRect					rect;
	
	// Create my filter view...
	rect.left = rect.top = 0;
	rect.right = frame.Width();
	rect.bottom = frame.Height() - Prefs().Size(H_SCROLLBAR_Y) - 2;
	input = new SeqSongPipelineMatrixView(rect, HRZ_INPUT_PIPELINE_STR, SongRef(), INPUT_PIPELINE, SongRef() );
	// ...add a scroll bar...
	rect.left = 0;
	rect.bottom = frame.Height();
	rect.top = rect.bottom - Prefs().Size(H_SCROLLBAR_Y) - 1;
	sb = new BScrollBar(rect, "HSB", 0, 0, 0, B_HORIZONTAL);
	// ...wrap them up in a view.
	BView*				view = new BView(frame, INPUT_PANEL_STR, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW);

	if (!input || !sb || !view) {
		delete input;
		delete sb;
		delete view;
		return 0;
	}
	input->SetResizingMode(B_FOLLOW_ALL);
	input->SetHorizontalScrollBar(sb);
	view->SetViewColor( Prefs().Color(AM_DATA_BACKDROP_C) );
	view->AddChild(input);
	view->AddChild(sb);
	return view;
}

BView* SeqSongWindow::NewArrangePanel(BRect frame, BScrollBar** outSb, BRect* outSbF)
{
	SeqPhraseMatrixView*	matrix = NULL;
	BScrollBar*				sb = NULL;
	ArpRangeControl*		zoom = NULL;	
	BRect					rect;
	BRect					zoomF;
	// Create my arrange view...
	rect.left = rect.top = 0;
	rect.right = frame.Width();
	rect.bottom = frame.Height() - Prefs().Size(H_SCROLLBAR_Y) - 2;
	matrix = new SeqPhraseMatrixView(rect, SongRef(), mMtc);
	// ...add a zoom control...
	zoomF.Set(	rect.right - ARP_ZOOM_CONTROL_WIDTH, frame.Height() - Prefs().Size(H_SCROLLBAR_Y) - 1,
				rect.right, frame.Height() );
	if ((zoom = new ArpZoomControl(zoomF, "zoom")) != NULL) {
		zoom->AddHorizontalBand( 0.25,	0.25,	10 );
		zoom->AddHorizontalBand( 0.26,	0.49,	10 );
		zoom->AddHorizontalBand( 0.50,	0.50,	10 );
		zoom->AddHorizontalBand( 0.51,	0.74,	10 );
		zoom->AddHorizontalBand( 0.75,	0.75,	10 );
		zoom->AddHorizontalBand( 0.76,	0.99,	10 );
		zoom->AddHorizontalBand( 1,		1,		10 );
		zoom->AddHorizontalBand( 1.01,	1.49,	10 );
		zoom->AddHorizontalBand( 1.50,	1.50,	10 );
		zoom->AddHorizontalBand( 1.51,	1.99,	10 );
		zoom->AddHorizontalBand( 2,		2,		10 );
		zoom->AddHorizontalBand( 2.01,	2.99,	10 );
		zoom->AddHorizontalBand( 3.00,	3.00,	10 );
		zoom->AddHorizontalBand( 3.01,	4.00,	10 );

		zoom->AddVerticalBand(	 4.00,	3.01,	10 );
		zoom->AddVerticalBand(	 3.00,	3.00,	10 );
		zoom->AddVerticalBand(	 2.99,	2.01,	10 );
		zoom->AddVerticalBand(	 2.00,	2.00,	10 );
		zoom->AddVerticalBand(	 1.99,	1.01,	10 );
		zoom->AddVerticalBand(	 1,		1,		10 );

		zoom->SetUpdatedMessage( new BMessage('zoom') );
		zoom->StartWatching(this, 'zoom');
		zoom->SetZoomX(mMtc.BeatLength() / DEFAULT_BEAT_LENGTH);
	}
	// ...add a scroll bar...
	rect.top = zoomF.top;
	rect.bottom = zoomF.bottom;
	rect.right = zoomF.left - 1;
//	sb = new BScrollBar(rect, "HSB", matrix, 0, 0, B_HORIZONTAL);
	sb = new BScrollBar(BRect(-100, -100, 0, -90), "HSB", matrix, 0, 0, B_HORIZONTAL);
	*outSb = sb;
	*outSbF = rect;
	// ...wrap them up in a view.
	BView*				view = new BView(frame, PHRASE_MATRIX_STR, B_FOLLOW_ALL, B_WILL_DRAW);

	if (!matrix || !sb || !zoom || !view) {
		delete matrix;
		delete sb;
		delete zoom;
		delete view;
		return NULL;
	}
	matrix->SetHorizontalScrollBar(sb);
	view->AddChild(matrix);
	view->AddChild(sb);
	view->AddChild(zoom);
	
	SeqMeasureControl*		measureView = MeasureControl();
	if (measureView) matrix->SetMeasureView(measureView);
	return view;
}

BView* SeqSongWindow::NewOutputPanel(BRect frame) const
{
	BScrollBar*				sb;
	SeqPipelineMatrixView*	output;
	SeqPipelineMatrixView*	dest;
	BRect					rect;
	float					destW = I_INITIAL_HRZ_OUTPUT_FILTERS_WIDTH;
	
	// Create my output pipeline...
	rect.left = rect.top = 0;
	rect.right = frame.Width() - destW - 1;
	rect.bottom = frame.Height() - Prefs().Size( H_SCROLLBAR_Y ) - 2;
	output = new SeqSongPipelineMatrixView(rect, HRZ_OUTPUT_PIPELINE_STR, SongRef(), OUTPUT_PIPELINE, SongRef() );

	// Create my destination pipeline...
	rect.left = rect.right + 1;
	rect.right = frame.Width();
	dest = new SeqSongPipelineMatrixView(rect, HRZ_DEST_PIPELINE_STR, SongRef(), DESTINATION_PIPELINE, SongRef() );
	
	// ...add a scroll bar...
	rect.left = 0;
	rect.bottom = frame.Height();
	rect.top = rect.bottom - Prefs().Size(H_SCROLLBAR_Y) - 1;
	sb = new BScrollBar(rect, "HSB", 0, 0, 0, B_HORIZONTAL);
	// ...wrap them up in a view.
	BView*				view = new BView( frame, OUTPUT_PANEL_STR, B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW );

	if (!output || !dest || !sb || !view) {
		delete output;
		delete dest;
		delete sb;
		delete view;
		return 0;
	}
	output->SetResizingMode(B_FOLLOW_ALL);
	output->SetHorizontalScrollBar(sb);
	dest->SetResizingMode( B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM );
	view->SetViewColor( Prefs().Color(AM_DATA_BACKDROP_C) );
	view->AddChild(output);
	view->AddChild(dest);
	view->AddChild(sb);
	return view;
}

BView* SeqSongWindow::NewSplitter(	BRect leftTop, BRect rightBottom,
									const char* name,
									uint32 resizeMask, orientation direction) const
{
	BRect	b;
	if( direction == B_VERTICAL ) {
		b.left = leftTop.right + 1;
		b.top = leftTop.top;
		b.right = rightBottom.left - 1;
		b.bottom = rightBottom.bottom;
	} else {
		b.left = leftTop.left;
		b.top = leftTop.bottom + 1;
		b.right = rightBottom.right;
		b.bottom = rightBottom.top - 1;
	}
	SeqSplitterView*	sv = new SeqSplitterView(b, name, resizeMask, B_WILL_DRAW, direction);
	if (!sv) return NULL;
	sv->SetViewColor(Prefs().Color(AM_DATA_BACKDROP_C) );
	return sv;
}

BView* SeqSongWindow::NewVerticalScrollBar(BRect frame)
{
	SeqSongIndexMatrixView*	indexes = IndexMatrix();
	SeqSongTitleMatrixView*	header = HeaderManager();
	SeqPipelineMatrixView*	input = InputManager();
	SeqPhraseMatrixView*	arrange = ArrangeManager();
	SeqPipelineMatrixView*	output = OutputManager();
	SeqPipelineMatrixView*	dest = DestinationManager();
	
	if (!indexes || !header || !input || !arrange || !output || !dest) return NULL;
	ArpMultiScrollBar*		vsb;
	vsb = new ArpMultiScrollBar( frame, "VSB", header, 0, 0, B_VERTICAL );
	if (!vsb) return NULL;
	
//	header->SetVerticalScrollBar(vsb);
	arrange->SetVerticalScrollBar(vsb);
	
	vsb->AddTarget(indexes);
	vsb->AddTarget(input);
	vsb->AddTarget(arrange);
	vsb->AddTarget(output);
	vsb->AddTarget(dest);

	if (Lock() ) {
		header->SetupScrollBars();
		arrange->SetupScrollBars();
		Unlock();
	}
	return vsb;
}

#if 0
BPopUpMenu* SeqSongWindow::BuildContextMenuFor(AmTrack *track)
{
	BPopUpMenu	*menu;
	bool		enabled = (track != 0);
	
	menu = new BPopUpMenu("Context", true, true, B_ITEMS_IN_COLUMN);
	if (menu == 0) return 0;

	AddViewFactoryMenuItems(menu, track);
	add_menu_item(ARPMSG_REMOVETRACK, "Delete Track", enabled, menu);
	menu->AddSeparatorItem();
	add_menu_item(ARPMSG_EDITTRACK, "Edit Track", enabled, menu);
	menu->SetTargetForItems(this);
	return menu;
}

bool SeqSongWindow::AddViewFactoryMenuItems(BMenu *toMenu, AmTrack *track)
{
	BList	*factories = seq_app->ViewFactories();
	if (factories == 0) return false;
	if (factories->IsEmpty()) return false;
	int32	size = factories->CountItems();
	
	// If there's only one factory, which only has one over view, we only need
	// one simple menu item.
	if (size == 1) {
		AmViewFactory	*factory = (AmViewFactory*)(factories->ItemAt(0));
		if (factory == 0) return false;
		BList			&names = factory->OverViewNames();
		uint32			nameSize = names.CountItems();
		if (nameSize == 0) return false;
		if (nameSize == 1) {
			BString		*str = (BString*)names.ItemAt(0);
			BMessage	*msg = new BMessage(ARPMSG_REQUESTTRACK);
			if (msg == 0) return false;
			msg->AddPointer(STR_AMVIEWFACTORY, (void*)factory);
			msg->AddString(STR_OVERVIEW, str->String());
			if (track != 0) msg->AddPointer(STR_AMTRACK, (void*)track);
			return AddMenuItem(msg, "Add Track", true, toMenu);
		}
		BMenu		*submenu = new BMenu("Add Track");
		if (submenu == 0) return false;
		for (uint32 k=0; k<nameSize; k++) {
			BString		*str = (BString*)names.ItemAt(k);
			BMessage	*msg = new BMessage(ARPMSG_REQUESTTRACK);
			if (msg == 0) return false;
			msg->AddPointer(STR_AMVIEWFACTORY, (void*)factory);
			msg->AddString(STR_OVERVIEW, str->String());
			if (track != 0) msg->AddPointer(STR_AMTRACK, (void*)track);
			AddMenuItem(msg, str->String(), true, submenu);
		}
		BMenuItem	*item = new BMenuItem(submenu);
		if (item == 0) {
			delete submenu;
			return false;
		}
		toMenu->AddItem(item);
		return true;
	}
	
	// FIX:  Finish off with the case of multiple view factories.
	return false;
}
#endif

status_t SeqSongWindow::SetIsRecording(bool isRecording)
{
	if (isRecording == mIsRecording) return B_ERROR;
	mIsRecording = isRecording;
	if (mSelections && !mSelections->IsEmpty() ) {
		/* This is the format the views expect the tracks in, sigh, so
		 * I'll just go with it for now.
		 */
		std::vector<track_id>			tracks;
		uint32						count = mSelections->CountTracks();
		for (uint32 k = 0; k < count; k++) tracks.push_back(mSelections->TrackAt(k));

		SeqPhraseMatrixView*		mv = ArrangeManager();
		if (mv) mv->InvalidateSelections(mSelections->TimeRange(), tracks);
		SeqMeasureControl*			mc = MeasureControl();
		if (mc) mc->Invalidate();
		SeqSongIndexMatrixView*		matrix = IndexMatrix();
		if (matrix) matrix->InvalidateTracks(tracks);

		if (mIsRecording) return B_OK;
	}
	return B_ERROR;
}

void SeqSongWindow::TransportChangeReceived(BMessage* msg)
{
	int32	state = TS_STOPPED;
	AmTime	time = -1;
	float	tempo = -1;
	bool	playToEnd;
	msg->FindInt32(ArpTransportState, &state);
	find_time(*msg, ArpTransportTime, &time);
	msg->FindFloat(ArpTransportTempo, &tempo);
	if (IsRecording() || msg->FindBool("play_to_end", &playToEnd) != B_OK) playToEnd = true;
	if (state == TS_STOPPED) {
		mTrackSongPosition = false;
		if ( ArrangeManager() ) ArrangeManager()->DrawSongPosition(-1);
//		printf("*** Transport stopped.\n");
	} else {
//		printf("Transport now at %lld\n", time);
		if ( ArrangeManager() ) ArrangeManager()->DrawSongPosition(time);
		SeqMeasureControl*	mc = MeasureControl();
		if (mc) mc->SetMarkerTime(AM_POSITION_MARKER, time);

		SeqTimeMbtView*		mbt = TimeMbtView();
		if (mbt) mbt->SetTime(time);

		SeqTimeHmsmView*	hmsm = TimeHmsmView();
		if (hmsm) hmsm->SetTime(time);

		if (playToEnd) TrackSongPosition(time);
	}
	
	if (tempo > 0) {
		SeqTempoControl*	tc = TempoControl();
		if( tc && !tc->IsTracking() ) tc->SetTempo(tempo);
	}
	
	// Draw it all right now, dammit.
	UpdateIfNeeded();
	
	// Reply to transport message.  You must always reply.
	// ArpTransportNextTime is the next time for which this
	// observer can display a visibly different indicator.
	BMessage reply(B_REPLY);
	add_time(reply, ArpTransportNextTime, time+(PPQN/4));
	msg->SendReply(&reply);
}

void SeqSongWindow::TrackSongPosition(AmTime time)
{
//	if (!mTrackSongPosition) return;
	SeqPhraseMatrixView*	arrange = ArrangeManager();
	if (!arrange) return;
	/* This is the number of pixels in from the right edge I'll let the song
	 * position get before I decide to scroll it.
	 */
	float		right_buffer = 15;
	AmTime		left = mMtc.PixelToTick( arrange->Bounds().left );
	AmTime		right = mMtc.PixelToTick( arrange->Bounds().right - right_buffer );
	if( time < left || time > right ) {
		BScrollBar*		sb = ArrangeScrollBar();
		if( sb ) {
			sb->SetValue( mMtc.TickToPixel( time ) );
		}
	}
}

void SeqSongWindow::MergePhrases()
{
	if (!mSelections || mSelections->IsEmpty() ) return;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track;
	if (song) {
		uint32		count = mSelections->CountTracks();
		AmRange		r(mSelections->TimeRange() );
		if (!r.IsValid() ) {
			r.start = 0;
			r.end = song->CountEndTime();
		}
		for( uint32 k = 0; k < count; k++ ) {
			if ( (track = song->Track( mSelections->TrackAt(k) )) ) {
				MergeTrackPhrases(track, r);
			}
		}
		if ( song->UndoContext() ) {
			song->UndoContext()->SetUndoName("Merge phrases");
			song->UndoContext()->CommitState();
		}
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

static void merge(	AmTrack* track, AmPhraseEvent* srcPhraseEvent, AmPhrase* destPhrase,
					std::vector<AmPhraseEvent*>& removes)
{
	AmPhrase*	srcPhrase = srcPhraseEvent->Phrase();
	if (!srcPhrase) return;
	AmNode*		n = srcPhrase->HeadNode();
	AmNode*		nextN = NULL;

	while (n) {
		nextN = n->next;
		AmEvent*	copy = n->Event()->Copy();
		if (copy) {
			track->RemoveEvent(srcPhrase, n->Event() );
			track->AddEvent(destPhrase, copy);
		}
		n = nextN;
	}
	ArpASSERT(srcPhrase->IsEmpty() );
	if (srcPhrase->IsEmpty() ) removes.push_back(srcPhraseEvent);
}

void SeqSongWindow::MergeTrackPhrases(AmTrack* track, AmRange range)
{
	AmPhraseEvent*			firstPe = NULL;
	AmNode*					n = track->Phrases().HeadNode();
	std::vector<AmPhraseEvent*>	removes;
	while (n && (n->StartTime() <= range.end) ) {
		AmNode*				nextN = n->next;
		if (n->Event()
				&& n->Event()->Type() == n->Event()->PHRASE_TYPE
				&& n->EndTime() >= range.start) {
			if (!firstPe) firstPe = dynamic_cast<AmPhraseEvent*>(n->Event() );
			else {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(n->Event() );
				if (pe) merge(track, pe, firstPe->Phrase(), removes);
			}
		}
		n = nextN;
	}
	for (uint32 k = 0; k < removes.size(); k++) track->RemoveEvent(NULL, removes[k]);
}

void SeqSongWindow::SeparatePhrases()
{
	if (!mSelections || mSelections->IsEmpty() ) return;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track;
	if (song) {
		uint32		count = mSelections->CountTracks();
		AmRange		r(mSelections->TimeRange() );
		if (!r.IsValid() ) {
			r.start = 0;
			r.end = song->CountEndTime();
		}
		for( uint32 k = 0; k < count; k++ ) {
			if( (track = song->Track( mSelections->TrackAt(k) )) ) {
				SeparateTrackPhrases(track, r);
			}
		}
		if ( song->UndoContext() ) {
			song->UndoContext()->SetUndoName("Separate phrases");
			song->UndoContext()->CommitState();
		}
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void SeqSongWindow::SeparateTrackPhrases(AmTrack* track, AmRange range)
{
	AmPhraseEvent*		mergedPhrase = new AmRootPhraseEvent();
	if (!mergedPhrase) return;
	AmPhraseEvent*		pe;
	AmNode*				n = track->Phrases().HeadNode();
	AmNode*				nNext;
	AmRange				change = range;
	std::vector<AmPhraseEvent*> removes;
	std::vector<AmPhraseEvent*> adds;
	BMessage			properties;
	bool				hasProperties = false;
	while( n && (n->StartTime() <= range.end) ) {
		/* TRICK:  During the course of removing events from the phrase that I'm
		 * iterating over (the 'n' var), this might cause the n to get rehashed from
		 * the phrase that contains it ( track->Phrases() ), which would invalidate
		 * n and cause a crash if you tried to find its next.  For that reason,
		 * I get the next here, at the start of the iteration.
		 */
		nNext = n->next;
		if ( (n->EndTime() >= range.start)
				&& (pe = dynamic_cast<AmPhraseEvent*>( n->Event() ))
				&& pe->Phrase() ) {
			if (!hasProperties) {
				pe->Phrase()->GetProperties(properties);
				hasProperties = true;
			}
			change += pe->TimeRange();
			/* For any events that start in the selection range, remove
			 * them from the phrase and place them in the mergedPhrase.
			 * Always operate on a copy because the undo state created
			 * by the remove will cause that event to get deleted.
			 */
			AmNode*		peNode = pe->Phrase()->FindNode(range.start);
			AmNode*		peNext = 0;
			AmRange		eventRange;
			while( peNode && (eventRange = pe->EventRange(peNode->Event())).start >= range.start
					&& eventRange.start <= range.end ) {
				peNext = peNode->next;
				AmEvent*	copy = peNode->Event()->Copy();
				if (copy) {
					track->RemoveEvent( pe->Phrase(), peNode->Event() );
					mergedPhrase->Phrase()->Add(copy);
				}
				peNode = peNext;
			}
			/* For any events that start after the selection range, remove
			 * them from the phrase and place them all together in a new phrase.
			 */
			peNode = pe->Phrase()->FindNode(range.end + 1);
			AmPhraseEvent*	splitPhrase = NULL;
			while (peNode) {
				peNext = peNode->next;
				if (!splitPhrase) splitPhrase = new AmRootPhraseEvent();
				if (splitPhrase && splitPhrase->Phrase() ) {
					AmEvent*	copy = peNode->Event()->Copy();
					if (copy) {
						track->RemoveEvent( pe->Phrase(), peNode->Event() );
						splitPhrase->Phrase()->Add(copy);
					}
				}
				peNode = peNext;
			}
			if( splitPhrase && !splitPhrase->IsEmpty() ) adds.push_back( splitPhrase );
			/* If this phrase has been completely emptied out, tag it to be removed.
			 */
			if( pe->IsEmpty() ) {
				removes.push_back( pe );
			}
		}
		n = nNext;
	}

	/* Remove any of the phrases that were emptied out.
	 */
	for( uint32 k = 0; k < removes.size(); k++ ) {
		status_t	err = track->RemoveEvent(NULL, removes[k]);
		ArpASSERT(err == B_OK);
	}
	/* Add any phrases that were created.
	 */
	for( uint32 k = 0; k < adds.size(); k++ ) {
		if (hasProperties) adds[k]->Phrase()->SetProperties(properties);
		track->AddEvent(NULL, adds[k]);
	}
	/* Add the newly created phrase, if any.
	 */	
	if ( mergedPhrase->IsEmpty() ) mergedPhrase->Delete();
	else {
		if (hasProperties) mergedPhrase->Phrase()->SetProperties(properties);
		track->AddEvent(NULL, mergedPhrase);
	}
}

void SeqSongWindow::DeleteSelection()
{
	if ( !mSelections || mSelections->IsEmpty() ) return;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track;
	if (song) {
		AmTime		endTime = song->CountEndTime();
		uint32		count = mSelections->CountTracks();
		AmRange		r(mSelections->TimeRange() );
		if (!r.IsValid() ) {
			r.start = 0;
			r.end = endTime;
		}
		for (uint32 k = 0; k < count; k++) {
			if ( (track = song->Track( mSelections->TrackAt(k) )) )
				Delete(track, r);
		}
		if ( song->UndoContext() ) {
			song->UndoContext()->SetUndoName("Delete selection");
			song->UndoContext()->CommitState();
		}
		AmTime		newEndTime = song->CountEndTime();
		if (endTime != newEndTime) song->EndTimeChangeNotice(newEndTime);
	}
	WriteUnlock( song );
	// END WRITE SONG BLOCK
	SetSelections(0);
}

static bool delete_one_phrase(AmTrack* track, AmRange range)
{
	const AmPhrase& 	phrases = track->Phrases();
	AmNode*				n = phrases.HeadNode();
	while (n) {
		if (n->StartTime() > range.end) return false;
		if (n->StartTime() >= range.start && n->EndTime() <= range.end) {
			AmEvent*		event = n->Event();
			track->RemoveEvent(NULL, event);
			return true;
		}
		n = n->next;
	}
	return false;
}

static void delete_events_contained_by(	AmTrack* track, AmPhraseEvent& topPhrase,
										AmPhrase* phrase, AmRange range)
{
	AmNode*		n = phrase->HeadNode();
	AmNode*		next;
	while ( n && n->Event() ) {
		next = n->next;
		AmRange	eventRange = topPhrase.EventRange(n->Event());
		if (eventRange.start > range.end) return;
		if (eventRange.start >= range.start && eventRange.end <= range.end) {
			AmEvent*	event = n->Event();
			track->RemoveEvent(phrase, event);
		}
		n = next;
	}
	return;
}

void SeqSongWindow::Delete(AmTrack* track, AmRange range)
{
	ArpASSERT(track);
	/* Delete all phrases that are contained wholly within the range.
	 */
	while( delete_one_phrase(track, range) ) ;
	/* Delete any events that are contained in the range.
	 */
	const AmPhrase& 	phrases = track->Phrases();
	AmNode*				n = phrases.HeadNode();
	AmNode*				next;
	while( n ) {
		next = n->next;
		if (n->StartTime() > range.end) break;
		if (n->Event()->Type() == n->Event()->PHRASE_TYPE
				&& n->EndTime() >= range.start) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe && pe->Phrase() ) delete_events_contained_by(track, *pe, pe->Phrase(), range);
		}
		n = next;
	}
}

void SeqSongWindow::ExpandMarkedRange()
{
	SeqMeasureControl*	mc = MeasureControl();
	if (!mc) return;
	if (!mc->MarkerVisible(mc->LEFT_LOOP_MARKER) || !mc->MarkerVisible(mc->RIGHT_LOOP_MARKER))
		return;
	AmTime		l = mc->MarkerTime(mc->LEFT_LOOP_MARKER),
				r = mc->MarkerTime(mc->RIGHT_LOOP_MARKER);
	if (l < 0 || r < 0 || r <= l) return;
	AmTime		expand = r - l;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		AmTime			endTime = song->CountEndTime();
		AmTrack*		track;
		BUndoContext*	undoContext = song->UndoContext();
		for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
			if (!mSelections || mSelections->IsEmpty() || mSelections->IncludesTrack(track->Id()))
				Expand(track, l, expand, undoContext);
		}
		if (undoContext) {
			undoContext->SetUndoName("Expand marked range");
			undoContext->CommitState();
		}
		AmTime		newEndTime = song->CountEndTime();
		if (endTime != newEndTime) song->EndTimeChangeNotice(newEndTime);
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void SeqSongWindow::ContractMarkedRange()
{
	SeqMeasureControl*	mc = MeasureControl();
	if (!mc) return;
	if (!mc->MarkerVisible(mc->LEFT_LOOP_MARKER) || !mc->MarkerVisible(mc->RIGHT_LOOP_MARKER))
		return;
	AmRange		r(	mc->MarkerTime(mc->LEFT_LOOP_MARKER),
					mc->MarkerTime(mc->RIGHT_LOOP_MARKER));
	if (r.start < 0 || r.end < 0 || r.end <= r.start) return;
	AmTime		expand = -(r.end - r.start);
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		AmTime			endTime = song->CountEndTime();
		AmTrack*		track;
		BUndoContext*	undoContext = song->UndoContext();
		for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
			if (!mSelections || mSelections->IsEmpty() || mSelections->IncludesTrack(track->Id())) {
				Delete(track, r);
				Expand(track, r.start, expand, undoContext);
			}
		}
//		song->OffsetSignatures(r.end -1, expand + 1);
		if (undoContext) {
			undoContext->SetUndoName("Contract marked range");
			undoContext->CommitState();
		}
		AmTime		newEndTime = song->CountEndTime();
		if (endTime != newEndTime) song->EndTimeChangeNotice(newEndTime);
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void SeqSongWindow::Expand(	AmTrack* track, AmTime start, AmTime expand,
							BUndoContext* undoContext)
{
	ArpASSERT(track);
	const AmPhrase& 	phrases = track->Phrases();
	AmNode*				n = phrases.HeadNode();
	AmNode*				next;
	AmChangeEventUndo*	undo = NULL;
	if (undoContext) undo = new AmChangeEventUndo(track);
	while (n) {
		next = n->next;
		if (n->EndTime() >= start) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(n->Event());
			AmPhrase*		topPhrase;
			if (pe && (topPhrase = pe->Phrase()) != NULL) {
				AmNode*		n2 = pe->Phrase()->HeadNode();
				AmNode*		next2;
				while (n2) {
					next2 = n2->next;
					AmTime		s = n2->StartTime();
					AmEvent*	e = n2->Event();
					ArpASSERT(e);
					if (e && s >= start) {
						if (undo) undo->EventChanging(topPhrase, e);
					}
					n2 = next2;
				}
			}
		}
		n = next;
	}
	if (undo) {
		if (undo->CountEvents() > 0) {
			AmEvent*		e = NULL;
			AmPhrase*		c = NULL;
			for (int32 k = 0; (e = undo->EventAt(k, &c)) != NULL; k++) {
				c->SetEventStartTime(e, e->StartTime() + expand);
				undo->EventChanged(e);
			}
			undoContext->AddOperation(undo, BResEditor::B_ANY_UNDO_MERGE);
		}
		else delete undo;
	}

}

void SeqSongWindow::EditTrackName()
{
	if (!mSelections) return;
	SeqSongTitleMatrixView*	titles = HeaderManager();
	if (!titles) return;
	if (mSelections->CountTracks() != 1) return;

	titles->StartEdit( mSelections->TrackAt(0) );
}

void SeqSongWindow::PerformSongFunction(const BMessage* msg)
{
	const char*				name;
	if (msg->FindString(FUNCTION_STR, &name) != B_OK) return;
	AmSongFunctionRoster*	roster = AmSongFunctionRoster::Default();
	if (!roster) return;
	AmSongFunctionI*		function = roster->FindFunction(name);
	if (!function) return;

	if (function->WriteMode() ) {
		// WRITE SONG BLOCK
		AmSong*		song = WriteLock();
		if (song) function->WriteSong(song);
		WriteUnlock(song);
		// END WRITE SONG BLOCK
	} else {
		// READ SONG BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqSongWindow::PerformSongFunction() read lock\n"); fflush(stdout);
		#endif
		const AmSong*	song = ReadLock();
		if (song) function->ReadSong(song);
		ReadUnlock(song);
		// END READ SONG BLOCK
	}	
}

void SeqSongWindow::NewTrack(const BMessage* device)
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		int32		trackHeight;
		if (seq_get_int32_preference(TRACK_HEIGHT_PREF, &trackHeight) != B_OK) trackHeight = AM_DEFAULT_TRACK_HEIGHT;
		uint32	count = 0;
		if (!device) {
			song->AddTrack(trackHeight);
			count++;
		} else {
			AmFilterRoster* roster = AmFilterRoster::Default();
			ArpRef<AmFilterAddOn> addon;
			if (roster) addon = roster->FindFilterAddOn(device);
			if (addon) {
				BMessage	config(*device);
				bool		more;
				BString		trackName = track_name_from(config);
				int32		k = 1;
				do {
					more = false;
					BString	tn( trackName );
					tn << " - " << k;
					AmTrack* track = new AmTrack( song, tn.String() );
					if (track) {
						track->SetPhraseHeight(trackHeight);
						if (song->AddTrack(track) == B_OK) {
							count++;
							track->InsertFilter(addon, DESTINATION_PIPELINE,
												-1, &config);
							AmFilterHolderI* holder = track->Filter(DESTINATION_PIPELINE);
							if (holder && holder->Filter()) {
								config.MakeEmpty();
								if (holder->Filter()->GetNextConfiguration(&config) == B_OK) {
									more = true;
								}
							}
						}
					}
					k++;
				} while (more);
			}
		}
		if ( count > 0 && song->UndoContext() ) {
			BString  name("Add ");
			if (count == 1) name << "1 track";
			else name << count << " tracks";
			song->UndoContext()->SetUndoName( name.String() );
			song->UndoContext()->CommitState();
		}
	}
	WriteUnlock( song );
	// END WRITE SONG BLOCK
}

static void build_ids(const AmSong* song, SeqSongSelections* selections, std::vector<track_id>& ids)
{
	for (uint32 k = 0; k < selections->CountTracks(); k++) {
		const AmTrack*		track = song->Track(selections->TrackAt(k) );
		if (track) ids.push_back(track->Id() );
	}
}

void SeqSongWindow::DeleteSelectedTracks()
{
	uint32		count;
	if( !mSelections || (count = mSelections->CountTracks()) < 1 ) return;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		/* Fix:  The stupid selection object stores items by index, rather
		 * than id.  This can cause a problem during deleting, since obviously
		 * the indexes change as you delete.  For now, I build up an id list
		 * based on the indexes, but once the selection object is storing by
		 * id this won't be necessary.
		 */
		std::vector<track_id>	ids;
		build_ids(song, mSelections, ids);
		for (uint32 k = 0; k < ids.size(); k++)
			song->RemoveTrack(ids[k] );
		if (song->UndoContext() ) {
			BString	undoName("Delete ");
			if (count == 1) undoName << "1 track";
			else undoName << count << " tracks";
			song->UndoContext()->SetUndoName( undoName.String() );
			song->UndoContext()->CommitState();
		}
	}
	WriteUnlock( song );
	// END WRITE SONG BLOCK		
	SetSelections(NULL);
}

static void move_tracks_by(SeqSongSelections* selections, AmSong* song, int32 delta)
{
	ArpASSERT(delta != 0 && selections && song);
	/* If the new index for any track will be out of bounds then this
	 * operation won't continue.
	 */
	uint32					selCount = selections->CountTracks();
	uint32					songCount = song->CountTracks();
	std::map<uint32, track_id> 	tracks;
	for (uint32 k = 0; k < selCount; k++) {
		AmTrack*	track = song->Track(selections->TrackAt(k));
		if (track) {
			int32		ti = track->SongIndex() + delta;
			if (ti < 0 || ti >= int32(songCount) ) return;
			tracks[ti] = track->Id();
		}
	}

	if (delta > 0) {
		std::map<uint32, track_id>::iterator i = tracks.end();
		do {
			--i;
			song->MoveTrackBy(i->second, delta);
		} while (i != tracks.begin());
	} else {
		for (std::map<uint32, track_id>::iterator i = tracks.begin(); i != tracks.end(); ++i) {
			song->MoveTrackBy(i->second, delta);
		}
	}
		
	if (song->UndoContext() ) {
		BString	undoName("Move ");
		if (selCount == 1) undoName << "1 track";
		else undoName << selCount << " tracks";
		song->UndoContext()->SetUndoName(undoName.String() );
		song->UndoContext()->CommitState();
	}
}

void SeqSongWindow::MoveTracksBy(int32 delta)
{
	uint32		count;
	if (!mSelections || (count = mSelections->CountTracks()) < 1) return;

	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) move_tracks_by(mSelections, song, delta);
	WriteUnlock(song);
	// END WRITE SONG BLOCK		
}

void SeqSongWindow::GroupTracks()
{
	uint32		count;
	if (!mSelections || (count = mSelections->CountTracks()) < 1) return;

	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		for (uint32 k = 0; k < count; k++) {
			AmTrack*	track = song->Track(mSelections->TrackAt(k));
			track->SetGroups(0);
		}
		uint32		nextGroup = next_free_group(song);
		if (nextGroup > 0) {
			for (uint32 k = 0; k < count; k++) {
				AmTrack*	track = song->Track(mSelections->TrackAt(k));
				track->SetGroups(nextGroup);
			}
		}
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK		
}

void SeqSongWindow::UngroupTracks()
{
	uint32		count;
	if (!mSelections || (count = mSelections->CountTracks()) < 1) return;

	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		for (uint32 k = 0; k < count; k++) {
			AmTrack*	track = song->Track(mSelections->TrackAt(k));
			track->SetGroups(0);
		}
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK		
}

void SeqSongWindow::OpenTempoWindow()
{
	AmTrackRef		trackRef;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::OpenTempoWindow() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if( song ) trackRef = song->TempoRef();
	ReadUnlock( song );
	// END READ SONG BLOCK
	BWindow*	win = SeqTrackWindow::ClassOpen( SongRef(), trackRef, 0, BString("arp:Tempo") );
	if (win) mTrackWins.push_back( BMessenger(win) );
}

void SeqSongWindow::HandlePlaySong()
{
	mTrackSongPosition = true;
	if ( SongRef().IsPlaying() || SongRef().IsRecording() ) return;
	
	AmTime				start = 0;
	SeqMeasureControl*	mc = MeasureControl();
	if (mc) {
		start = mc->MarkerTime(mc->POSITION_MARKER);
		mc->SetTransportLooping();
	}
	TrackSongPosition(start);
	if (mIsRecording) SongRef().StartRecording(start);
	else SongRef().StartPlaying(start);
}		

void SeqSongWindow::HandleSetSongPosition(AmTime time)
{
	bool				isPlaying = SongRef().IsPlaying();
	if (isPlaying) SongRef().StopTransport();

	SeqMeasureControl*	mc = MeasureControl();
	if (time < 0) time = 0;
	if (mc) mc->SetMarkerTime(mc->POSITION_MARKER, time);
//	ScrollArrangeTo(mMtc.TickToPixel(time));

	if (isPlaying) HandlePlaySong();
}

void SeqSongWindow::HandleMoveSongPosition(AmTime offset)
{
	SeqMeasureControl*	mc = MeasureControl();
	if (!mc) return;
		
	bool			isPlaying = SongRef().IsPlaying();
	if (isPlaying) SongRef().StopTransport();

	AmTime			time = mc->MarkerTime(mc->POSITION_MARKER) + offset;
	if (time < 0) time = 0;
	mc->SetMarkerTime(mc->POSITION_MARKER, time);

	if (isPlaying) HandlePlaySong();
	else ScrollArrangeTo(mMtc.TickToPixel(time));
}

void SeqSongWindow::HandleStopSong(bool panic)
{
	if (panic) SongRef().PanicStop();
	else SongRef().StopTransport();
	ArpTwoStateButton*	ctrl = dynamic_cast<ArpTwoStateButton*>( FindView(RECORD_BUTTON_STR) );
	if (ctrl) ctrl->SetButtonState(false);
}

void SeqSongWindow::HandleDoubleClickTrack( const BMessage* msg )
{
	AmTime			time;
	track_id		id;
	if ( msg->FindPointer( SZ_TRACK_ID, &id ) != B_OK ) return;

	if (find_time(*msg, SZ_AMTIME, &time) != B_OK) {
		BScrollBar*		sb = ArrangeScrollBar();
		if( sb ) time = mMtc.PixelToTick( sb->Value() );
		else time = 0;
	}
	AmTrackRef		trackRef;
	status_t		status = B_ERROR;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::HandleDoubleClickTrack() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if (song) status = song->TrackRefForId( id, trackRef );
	ReadUnlock(song);
	// END READ SONG BLOCK
	if (status != B_OK) return;
	
	/* If I already have a window opened for the supplied track, just
	 * bring it forward at the requested time.
	 */
	for (uint32 k = 0; k < mTrackWins.size(); k++) {
		if (mTrackWins[k].IsValid() ) {
			BHandler*		target;
			BLooper*		looper;
			if ( (target = mTrackWins[k].Target(&looper)) != NULL) {
				SeqTrackWindow*	win = dynamic_cast<SeqTrackWindow*>(target);
				if (win && win->TrackId() == id) {
					win->ScrollToTime(time);
					win->Activate(true);
					return;
				}
			}
		}
	}
	/* FIX:  Right now, the signature of the factory creating the
	 * window is hardcoded.  That will change once the menu for
	 * opening tracks is build dynamically.
	 */
	BWindow*	win = SeqTrackWindow::ClassOpen( SongRef(), trackRef, time, DEFAULT_FACTORY_SIGNATURE );
	if (win) mTrackWins.push_back( BMessenger(win) );
}

void SeqSongWindow::ObserverMessageReceived(const BMessage* msg)
{
	int32		what;
	msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
	switch (what) {
		case 'zoom': {
			bool					hasX = false, hasY = false;
			AmTime					tick = 0;
			float					w = 1;
			float					valueY, valueX;
			SeqPhraseMatrixView*	matrix = ArrangeManager();
			if (msg->FindFloat("x_value", &valueX) == B_OK && matrix) {
				hasX = true;
				// Retrieve the current tick being shown in the center of
				// the display.
				BRect				b = matrix->Bounds();
				w = b.Width() + 1;
				tick = mMtc.PixelToTick(b.left + w / 2);
				mMtc.SetBeatLength(valueX * DEFAULT_BEAT_LENGTH);
			}
			if (msg->FindFloat("y_value", &valueY) == B_OK) {
				hasY = true;
				uint32						count;
				if (mSelections && (count = mSelections->CountTracks()) > 0) {
					float					height = 23 * valueY;
					SeqSongIndexMatrixView*	iv = IndexMatrix();
					SeqSongTitleMatrixView*	tv = HeaderManager();
					SeqPipelineMatrixView*	p1 = InputManager();
					SeqPipelineMatrixView*	p2 = OutputManager();
					SeqPipelineMatrixView*	p3 = DestinationManager();
					// WRITE SONG BLOCK
					AmSong* song = WriteLock();
					if (song) {
						for (uint32 k = 0; k < count; k++) {
							AmTrack*		track = song->Track(mSelections->TrackAt(k));
							if (track) track->SetPhraseHeight(height);
						}
						if (iv) iv->FillMetrics(song);
						if (tv) tv->FillMetrics(song);
						if (p1) p1->FillMetrics(song);
						if (matrix) matrix->FillTrackMetrics(song);
						if (p2) p2->FillMetrics(song);
						if (p3) p3->FillMetrics(song);
					}
					WriteUnlock(song);
					// END WRITE SONG BLOCK
					if (iv) iv->Invalidate();
					if (tv) tv->Invalidate();
					if (p1) p1->Invalidate();
					if (p2) p2->Invalidate();
					if (p3) p3->Invalidate();
				}
			}
			if (hasX || hasY) {
				matrix->Invalidate();
				matrix->SetupScrollBars(hasX, hasY);
			}
			if (hasX) {
				float	center = mMtc.TickToPixel(tick);
				BScrollBar*	hsb = ArrangeScrollBar();
				if (hsb) hsb->SetValue(center - w / 2);
			}

		} break;
		default:
			break;
	}
}

#if 0
void SeqSongWindow::HandleContextMenuFor(AmTrack *track, AmTime time)
{
	if (backgroundView == 0) return;
	BPopUpMenu	*m = BuildContextMenuFor(track);
	if (m == 0) return;
	
	uint32		buttons;
	BPoint		where;
	backgroundView->GetMouse(&where, &buttons, false);
	BMenuItem	*item;	
	ConvertToScreen(&where);
	item = m->Go(where);
	if (item == 0) {
		delete m;
		return;
	}
	BMessage	*msg = item->Message();
	if (msg == 0) {
		delete m;
		return;
	}
	// It would be nice to share this with the MessageReceived stuff
	if (msg->what == ARPMSG_EDITTRACK) {
//		SeqTrackWindow::ClassOpen( SongRef(), mTrack, time);
	} else if (msg->what == ARPMSG_REQUESTTRACK) {
		RequestTrack(msg);
	}

	delete m;
}
#endif

#if 0
	status_t	GetManualRef(entry_ref* ref);
status_t SeqSongWindow::GetManualRef(entry_ref* ref)
{
	status_t err;
	app_info ai;
	if( (err=be_app->GetAppInfo(&ai)) != 0 ) {
		return err;
	}
	BEntry entry(&ai.ref);
	if( (err=entry.InitCheck()) != 0 ) {
		return err;
	}
	BDirectory dir;
	if( (err=entry.GetParent(&dir)) != 0 ) {
		return err;
	}
	if( (err=dir.FindEntry("Documentation/UsersGuide/index.html", &entry)) != 0 ) {
		return err;
	}
	if( ref ) err = entry.GetRef(ref);
	return err;
}
#endif

void SeqSongWindow::ShowSignatureWindow(int32 measure, uint32 beats, uint32 beatValue)
{
	if ( !mSignatureWin.IsValid() ) {
		BRect			frameRect( BPoint(80, 20), BPoint(300, 350));
		SeqSignatureWindow* win = new SeqSignatureWindow(SongRef(), frameRect);
		if (win) {
			win->SetMeasure(measure, beats, beatValue);
			mSignatureWin = BMessenger(win);
			win->Show();
		}
	} else {
		BHandler*		target;
		BLooper*		looper;
		if ((target = mSignatureWin.Target(&looper)) != NULL) {
			SeqSignatureWindow*	win = dynamic_cast<SeqSignatureWindow*>(target);
			if (win) {
				win->SetMeasure(measure, beats, beatValue);
				win->Activate( true );
			}
		}
	}
}

status_t SeqSongWindow::GetLeftSignature(AmSignature& sig)
{
	AmTime		left = LeftTick();
	status_t	err = B_ERROR;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongWindow::GetLeftSignature() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) err = song->GetSignature(left, sig);
	ReadUnlock( song );
	// END READ SONG BLOCK
	return err;
}

status_t SeqSongWindow::LoadSyx(entry_ref& ref, bool merge, uint32 flags)
{
	BFile		file(&ref, B_READ_ONLY);
	status_t	err = file.InitCheck();
	if (err != B_OK) return err;
	off_t	fileSize;
	if ( (err = file.GetSize(&fileSize)) != B_OK) return err;
	uint8*	data = (uint8*)malloc(sizeof(uint8) * fileSize);
	if (!data) return B_NO_MEMORY;

	size_t	bufferSize = 256;
	uint8	buffer[bufferSize];
	off_t	pos = 0;
	ssize_t	read;
	while ( (read = file.ReadAt(pos, (void*)buffer, bufferSize)) > 0) {
		for (ssize_t k = 0; k < read; k++) data[pos + k] = buffer[k];
		pos += read;
	}
	AmSystemExclusive*	sysex = new AmSystemExclusive(data, pos, PPQN * 4);
	free(data);
	if (!sysex) return B_NO_MEMORY;

	AmSong* song = WriteLock();
	if (song) {
		if (!merge) song->StartLoad();
		tracks_for_each_device(song, 1, 1);
		AmTrack*	track = song->Track((uint32)0);
		if (!track) err = B_ERROR;
		else {
			AmRootPhraseEvent*	pe = new AmRootPhraseEvent();
			if (!pe) err = B_NO_MEMORY;
			else {
				track->AddEvent(NULL, pe);
				track->AddEvent(pe->Phrase(), sysex);
				err = B_OK;
			}
		}
		if (!merge) song->FinishLoad();
		song->EndTimeChangeNotice();
	}
	WriteUnlock(song);

	return err;
}

// Request the name for a new track, then generate the appropriate new
// overview and add it.
void SeqSongWindow::RequestTrack(BMessage *msg) {
#if 0
	const char			*str;
	AmViewFactory	*factory;
	if ( (msg->FindString(STR_OVERVIEW, &str) != B_NO_ERROR)
			|| (msg->FindPointer(STR_AMVIEWFACTORY, (void**)&factory) != B_NO_ERROR) ) {
		return;
	}
	BMessage	*notherMsg;
	notherMsg = new BMessage(ARPMSG_ADDTRACK);
	if (notherMsg == 0) return;	
	notherMsg->AddPointer(STR_AMVIEWFACTORY, (void*)factory);
	notherMsg->AddString(STR_OVERVIEW, str);
	
	// If we have a track that this track is being requested from, add in
	// position information
	AmTrack	*track;
	if (msg->FindPointer(STR_AMTRACK, (void**)&track) == B_NO_ERROR)
		notherMsg->AddInt32(STR_POSITION, version->IndexOfTrack(track));

	ArpTextRequestWindow	*trw;
	BRect	rect;
	rect.Set(100,100,300,200);
	trw = new ArpTextRequestWindow(rect, this, notherMsg, "New track name:",
			"New track");
	trw->Show();
#endif
}

SeqTempoControl* SeqSongWindow::TempoControl() const
{
	if( !mControlBg ) return 0;
	_SongTransportView*	tv = dynamic_cast<_SongTransportView*>( mControlBg->FindView( TRANSPORT_VIEW_SZ ) );
	if( !tv ) return 0;
	return dynamic_cast<SeqTempoControl*>( tv->FindView(SZ_TEMPO_VIEW) );
}

SeqMeasureControl* SeqSongWindow::MeasureControl() const
{
	return dynamic_cast<SeqMeasureControl*>( FindView(SZ_MEASURE_VIEW) );
}

SeqSongIndexMatrixView* SeqSongWindow::IndexMatrix() const
{
	BView*		view = FindView("header panel");
	if (!view) return NULL;
	return dynamic_cast<SeqSongIndexMatrixView*>( view->ChildAt(0) );
}

SeqSongTitleMatrixView* SeqSongWindow::HeaderManager() const
{
	BView*		view = FindView("header panel");
	if (!view) return NULL;
	return dynamic_cast<SeqSongTitleMatrixView*>( view->ChildAt(1) );
}

SeqPipelineMatrixView* SeqSongWindow::InputManager() const
{
	BView*		view = FindView(INPUT_PANEL_STR);
	if (!view) return NULL;
	return dynamic_cast<SeqPipelineMatrixView*>( view->FindView(HRZ_INPUT_PIPELINE_STR) );
}

SeqPhraseMatrixView* SeqSongWindow::ArrangeManager() const
{
	BView*		view = FindView(PHRASE_MATRIX_STR);
	if (!view) return NULL;
	return dynamic_cast<SeqPhraseMatrixView*>( view->ChildAt(0) );
}

SeqPipelineMatrixView* SeqSongWindow::OutputManager() const
{
	BView*		view = FindView( OUTPUT_PANEL_STR );
	if (!view) return NULL;
	return dynamic_cast<SeqPipelineMatrixView*>( view->FindView(HRZ_OUTPUT_PIPELINE_STR) );
}

SeqPipelineMatrixView* SeqSongWindow::DestinationManager() const
{
	BView*		view = FindView(OUTPUT_PANEL_STR);
	if (!view) return NULL;
	return dynamic_cast<SeqPipelineMatrixView*>( view->FindView(HRZ_DEST_PIPELINE_STR) );
}

BScrollBar* SeqSongWindow::ArrangeScrollBar() const
{
	BView*		view = FindView( "arrange panel" );
	if (!view) return 0;
	return dynamic_cast<BScrollBar*>( view->FindView( "HSB" ) );
}

SeqTimeMbtView* SeqSongWindow::TimeMbtView() const
{
	if (!mControlBg) return 0;
	_SongTransportView*	tv = dynamic_cast<_SongTransportView*>( mControlBg->FindView(TRANSPORT_VIEW_SZ) );
	if (!tv) return 0;
	return dynamic_cast<SeqTimeMbtView*>( tv->FindView(MBT_VIEW_SZ) );
}

SeqTimeHmsmView* SeqSongWindow::TimeHmsmView() const
{
	if (!mControlBg) return 0;
	_SongTransportView*	tv = dynamic_cast<_SongTransportView*>( mControlBg->FindView(TRANSPORT_VIEW_SZ) );
	if (!tv) return 0;
	return dynamic_cast<SeqTimeHmsmView*>( tv->FindView(HMSM_VIEW_SZ) );
}

AmTime SeqSongWindow::LeftTick() const
{
	SeqPhraseMatrixView*	arrange = ArrangeManager();
	if (!arrange) return 0;
	return mMtc.PixelToTick(arrange->Bounds().left);
}

void SeqSongWindow::AddRefToSettings()
{
	if( mAddedRefToSettings ) return;
	mAddedRefToSettings = true;
	entry_ref	ref = FileRef();
	seq_app->AddShutdownRef( "open_document", &ref );
}

void SeqSongWindow::CloseTrackWindows()
{
	BMessage		msg(B_QUIT_REQUESTED);
	for (uint32 k = 0; k < mTrackWins.size(); k++) {
		if ( mTrackWins[k].IsValid() )
			SafeSendMessage(mTrackWins[k], &msg);
	}
}

// #pragma mark -

/***************************************************************************
 * _SONG-MEASURE-CONTROL
 ***************************************************************************/
_SongMeasureControl::_SongMeasureControl(	BRect frame,
											const char* name,
											AmSongRef songRef,
											AmTimeConverter& mtc,
											float leftIndent,
											float rightIndent,
											int32 resizeMask)
		: inherited( frame, name, songRef, mtc, leftIndent, rightIndent, resizeMask),
		  mTrackMouse(false), mOrigin(0, 0), mLeftTime(-1), mRightTime(-1),
		  mScrollDelta(0), mScrollThread(0)
{
}

_SongMeasureControl::~_SongMeasureControl()
{
	if( mScrollThread > 0 ) kill_thread( mScrollThread );
}

float _SongMeasureControl::ScrollDelta() const
{
	BAutolock	l(mAccess);
	return mScrollDelta;
}

void _SongMeasureControl::SetScrollDelta(float delta)
{
	BAutolock	l(mAccess);
	mScrollDelta = delta;
}

void _SongMeasureControl::MouseDown(BPoint pt)
{
	inherited::MouseDown( pt );
	mTrackMouse = false;
	mLeftTime = mRightTime = -1;
	SetScrollDelta( 0 );
	mScrollThread = 0;
	if(mMouseDown != NO_MARKER) return;
	/* My superclass will launch a popup on the secondary
	 * button, so I shouldn't handle that.
	 */
	BPoint		where;
	uint32		buttons;
	GetMouse(&where, &buttons, false);
	if (buttons&B_SECONDARY_MOUSE_BUTTON) return;
	/* User's can't change the selection if they start outside of the
	 * center area.
	 */
	if (pt.x < mLeftIndent || pt.x  > Bounds().right - mRightIndent) return;

	mTrackMouse = true;
	mOrigin = pt;

	if ( (modifiers() & B_SHIFT_KEY) ) {
		SeqSongSelections*	selections = Selections();
		if (selections && !selections->IsEmpty() ) {
			mLeftTime = mRightTime = selections->TimeRange().start;
			mPrevRange = selections->TimeRange();
		}
	}
	if( mLeftTime < 0 || mRightTime < 0 ) {
		GetMeasureTimes( pt, &mLeftTime, &mRightTime );
		mPrevRange.MakeInvalid();
	}

	/* Launch the scrolling thread.
	 */
	if( mLeftTime >= 0 && mRightTime >= 0 ) {
		mScrollThread = spawn_thread(	ScrollThreadEntry,
										"ARP Scroll Song Window",
										B_NORMAL_PRIORITY,
										this);
		if( mScrollThread > 0 ) resume_thread( mScrollThread );
	}
}

void _SongMeasureControl::MouseMoved(	BPoint pt,
										uint32 code,
										const BMessage* msg)
{
	if( !mTrackMouse || mLeftTime < 0 || mRightTime < 0 ) {
		inherited::MouseMoved( pt, code, msg );
		return;
	}
	/* If the mouse is outside the center range, then scroll
	 * the view.
	 */
	if( pt.x < mLeftIndent ) {
		float	delta = 0;
		if( pt.x < 0 ) delta = fabs(pt.x - mLeftIndent);
		else delta = fabs(mLeftIndent - pt.x);
		SetScrollDelta( 0 - delta );
	} else if( pt.x > Bounds().right - mRightIndent ) {
		float	delta = pt.x - (Bounds().right - mRightIndent);
		SetScrollDelta( delta );
	} else {
		SetScrollDelta( 0 );
	}
	/* Create the current range based on the mouse position.
	 */
	AmTime		currStart = -1, currEnd = -1;
	AmRange		newRange;
	GetMeasureTimes(pt, &currStart, &currEnd);
	if (currStart < 0 || currEnd < 0) return;
	
	if (pt.x >= mOrigin.x) {
		newRange.start = mLeftTime;
		newRange.end = currEnd;
	} else {
		newRange.start = currStart;
		newRange.end = mRightTime;
	}
	if (newRange.end < newRange.start) {
		AmTime	t = newRange.end;
		newRange.end = newRange.start;
		newRange.start = t;
	}
	if (newRange != mPrevRange) {
		SetSelectionRange(newRange);
		mPrevRange = newRange;
	}
}

void _SongMeasureControl::MouseUp(BPoint pt)
{
	uint32			mouseDown = mMouseDown;

	if (mScrollThread > 0) kill_thread(mScrollThread);
	mScrollThread = 0;
	inherited::MouseUp(pt);
	mTrackMouse = false;
	mOrigin.Set(0, 0);
	mLeftTime = mRightTime = -1;

	if (mouseDown == POSITION_MARKER) {
		BWindow*		win = Window();
		if (win) {
			BMessage	msg(PLAY_SONG_FROM_TIME_MSG);
			msg.AddInt64(SEQ_TIME_STR, MarkerTime(POSITION_MARKER));
			win->PostMessage(&msg);
		}
	}
}

void _SongMeasureControl::DrawCenterBgOn(BRect cBounds, BView* view, AmTime songEndTime)
{
	inherited::DrawCenterBgOn(cBounds, view, songEndTime);

	SeqSongSelections*	selections = Selections();
	if (!selections || selections->IsEmpty() ) return;
	AmRange		r = selections->TimeRange();
	BRect		b(Bounds() );
	float		leftX = mMtc.TickToPixel(0) - mScrollX + mLeftIndent;
	float		rightX = b.right;
	if (r.IsValid() ) {
		leftX = mMtc.TickToPixel(r.start) - mScrollX + mLeftIndent;
		rightX = mMtc.TickToPixel(r.end) - mScrollX + mLeftIndent;
	}

	rgb_color	c;
	if (IsRecording() ) c = Prefs().Color(AM_SONG_RECORD_SELECTION_C);
	else c = Prefs().Color(AM_SONG_SELECTION_C);

	DrawBackground( view, c, BRect( leftX, cBounds.top, rightX, cBounds.bottom ) );
}

void _SongMeasureControl::DrawRightBgOn(BRect rBounds, BView* view, AmTime songEndTime)
{
	inherited::DrawRightBgOn(rBounds, view, songEndTime);
	// If it's fixed than I just let the center view take care of drawing it.
//	if ( IsRightFixed() ) return;

	SeqSongSelections*	selections = Selections();
	if (!selections || selections->IsEmpty() ) return;

	AmTime				centerRightTime = CenterRightTime();
	AmRange				r = selections->TimeRange();
	if (r.end >= 0 && r.end <= centerRightTime) return;
	
	AmTime				timeWidth = songEndTime - centerRightTime;
	AmTimeConverter		mtc( mRightIndent / ((float)timeWidth / (float)PPQN) );

	float				leftX = rBounds.left;
	float				rightX = rBounds.right;
	if (r.IsValid() ) {
		leftX = mtc.TickToPixel(r.start - centerRightTime) + rBounds.left;
		rightX = mtc.TickToPixel(r.end - centerRightTime) + rBounds.left;
		if (leftX < rBounds.left) leftX = rBounds.left;
		if (rightX > rBounds.right) rightX = rBounds.right;
	}
	rgb_color	c;
	if (IsRecording() ) c = Prefs().Color(AM_SONG_RECORD_SELECTION_C);
	else c = Prefs().Color(AM_SONG_SELECTION_C);
	DrawBackground( view, c, BRect(leftX, rBounds.top, rightX, rBounds.bottom) );
}

void _SongMeasureControl::DrawLeftOn(BRect lBounds, BView* view)
{
	// If it's fixed than I just let the center view take care of drawing it.
	if ( IsLeftFixed() ) return;
	if (mLeftBg) view->DrawBitmapAsync( mLeftBg, BPoint(0, 1) );
	/* Draw any selection area on top of the background.
	 */
	SeqSongSelections*	selections = Selections();
	if (selections && !(selections->IsEmpty()) ) {
		AmTime				rightTick = mMtc.PixelToTick(mScrollX);
		AmRange				r = selections->TimeRange();
		if (r.start <= rightTick) {
			AmTimeConverter		mtc( mLeftIndent / ((float)rightTick / (float)PPQN) );
			float			leftX = lBounds.left;
			float			rightX = lBounds.right;
			if (r.IsValid() ) {
				leftX = mtc.TickToPixel(r.start);
				rightX = mtc.TickToPixel(r.end);
				if (rightX > lBounds.right) rightX = lBounds.right;
			}
			rgb_color	c;
			if (IsRecording() ) c = Prefs().Color(AM_SONG_RECORD_SELECTION_C);
			else c = Prefs().Color(AM_SONG_SELECTION_C);
			BRect		b(leftX, lBounds.top, rightX, lBounds.bottom);
			DrawBackground(view, c, b);
		}
	}
	
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("_SongMeasureControl::DrawLeftOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) LockedDrawLeftOn(song->Signatures(), lBounds, view);
	ReadUnlock(song);
	// END READ SONG BLOCK
}

void _SongMeasureControl::DrawBackground(BView* view, rgb_color c, BRect bounds)
{
	BPoint	left(bounds.left, bounds.top + 1);
	BPoint	right(bounds.right, bounds.top + 1);

	int32	rChange = (c.red > 127) ? -2 : 2;
	int32	gChange = (c.green > 127) ? -2 : 2;
	int32	bChange = (c.blue > 127) ? -2 : 2;

	while (left.y < bounds.bottom) {
		view->SetHighColor(c);
		view->StrokeLine(left, right);
		if (c.red + rChange >= 0 && c.red + rChange <= 255) c.red += rChange;
		if (c.green + gChange >= 0 && c.green + gChange <= 255) c.green += gChange;
		if (c.blue + bChange >= 0 && c.blue + bChange <= 255) c.blue += bChange;
		left.y++;
		right.y++;
	}
}

void _SongMeasureControl::GetMeasureTimes(BPoint pt, AmTime* start, AmTime* end)
{
	AmTime		time = mMtc.PixelToTick( pt.x + mScrollX - mLeftIndent );
	/* If the time being requested is less than the first measure, constrain
	 * it to the first measure.
	 */
	if( time < 0 ) time = 0;
	
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("_SongMeasureControl::GetMeasureTimes() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if( !song ) return;
	GetMeasureTimes( song, time, start, end );
	ReadUnlock( song );
	// END READ SONG BLOCK
}

void _SongMeasureControl::GetMeasureTimes(const AmSong* song, AmTime time, AmTime* start, AmTime* end)
{
	const AmPhrase&		signatures = song->Signatures();
	AmNode*				node = signatures.FindNode( time, BACKWARDS_SEARCH );
	if( !node ) return;
	AmSignature*		nodeSig = dynamic_cast<AmSignature*>( node->Event() );
	if( !nodeSig ) return;
	AmTime				s = nodeSig->StartTime(), e = nodeSig->EndTime();
	AmTime				duration = e - s;
	while( e < time ) {
		s = e + 1;
		e = s + duration;
	}
	*start = s;
	*end = e;
}

SeqSongSelections* _SongMeasureControl::Selections() const
{
	SeqSongWinPropertiesI*	win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	if (!win) return NULL;
	return win->Selections();
}

void _SongMeasureControl::SetSelectionRange(AmRange range)
{
	SeqSongWinPropertiesI*	win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	if( !win ) return;
	SeqSongSelections*	selections = win->Selections();
	if( selections ) selections = selections->Copy();
	else selections = SeqSongSelections::New();
	if( !selections ) return;

	/* If there are no tracks selected, then select them all.
	 */
	if (selections->IsEmpty() ) {
		// READ SONG BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("_SongMeasureControl::SetSelectionRange() read lock\n"); fflush(stdout);
		#endif
		const AmSong*		song = ReadLock();
		if (song) {
			const AmTrack*	track = NULL;
			for (uint32 k = 0; (track = song->Track(k)) != NULL; k++)
				selections->AddTrack(track->Id() );
		}
		ReadUnlock(song);
		// END READ SONG BLOCK
	}
	
 	selections->SetTimeRange(range);
 	win->SetSelections(selections);
}

bool _SongMeasureControl::IsRecording() const
{
	SeqSongWinPropertiesI*	win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	if (!win) return false;
	return win->IsRecording();
}

int32 _SongMeasureControl::ScrollThreadEntry(void* arg)
{
	DB(DBALL, std::cerr << "_SongMeasureControl: Enter ScrollThreadEntry." << std::endl);
	_SongMeasureControl*	ctrl = (_SongMeasureControl*)arg;
	if( !ctrl ) return B_ERROR;
	
	while( true ) {
		snooze(20000);
		float	delta = ctrl->ScrollDelta();
		if( delta != 0 ) {
			BMessage	msg( SCROLL_ARRANGE_MSG );
			msg.AddFloat( DELTA_STR, delta );
			if( ctrl->Window() ) {
				ctrl->Window()->PostMessage( &msg );
			}
		}
	}

	DB(DBALL, std::cerr << "_SongMeasureControl: Exit ScrollThreadEntry." << std::endl);
	return B_OK;
}

/***************************************************************************
 * _SONG-TRANSPORT-VIEW
 * This view displays the tempo and time MBT views.
 ***************************************************************************/
_SongTransportView::_SongTransportView(BPoint topLeft, AmSongRef songRef)
		: inherited(BRect(topLeft.x, topLeft.y, topLeft.x, topLeft.y),
					TRANSPORT_VIEW_SZ,
					B_FOLLOW_LEFT | B_FOLLOW_TOP,
					B_WILL_DRAW)
{
	if (!TRANSPORT_BM) TRANSPORT_BM = Resources().FindBitmap("Transport LCD");

	if (TRANSPORT_BM) {
		BRect		b = TRANSPORT_BM->Bounds();
		ResizeTo( b.Width(), b.Height() );
	}
	SetViewColor(B_TRANSPARENT_COLOR);

	SeqTempoControl*	tc = new SeqTempoControl(	BPoint(10, 9),
													SZ_TEMPO_VIEW,
													new BMessage(ARPMSG_TEMPOCHANGE),
													B_FOLLOW_LEFT | B_FOLLOW_TOP,
													1, 400,
													120 );
	if (tc) AddChild(tc);

	BRect	f(93, 16, 210, 27);
	SeqTimeMbtView*		mbt = new SeqTimeMbtView(f, MBT_VIEW_SZ, songRef);
	if (mbt) AddChild(mbt);

	f.Set(229, 16, 346, 27);
	SeqTimeHmsmView*		hmsm = new SeqTimeHmsmView(f, HMSM_VIEW_SZ, songRef);
	if (hmsm) AddChild(hmsm);
}

void _SongTransportView::Draw(BRect clip)
{
	if (!gControlBg) {
		SetHighColor(Prefs().Color(AM_CONTROL_BG_C) );
		FillRect(clip);
	}
	
	drawing_mode			mode = DrawingMode();
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	if (gControlBg) {
		arp_tile_bitmap_on(this, clip, gControlBg, Frame().LeftTop() );
	}
	if (TRANSPORT_BM) {
		BRect		f(clip);
		BRect		b = TRANSPORT_BM->Bounds();
		if (f.right > b.right) f.right = b.right;
		if (f.bottom > b.bottom) f.bottom = b.bottom;
		DrawBitmapAsync(TRANSPORT_BM, f, f);
	}

	SetDrawingMode(mode);
}

/***************************************************************************
 * Miscellaneous functions
 ***************************************************************************/
static uint32 next_free_group(const AmSong* song)
{
	uint32			groups = 0;
	const AmTrack*	track;
	for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
		groups |= track->Groups();
	}
	if (!(groups&0x00000001)) return 0x00000001;
	else if (!(groups&0x00000002)) return 0x00000002;
	else if (!(groups&0x00000004)) return 0x00000004;
	else if (!(groups&0x00000008)) return 0x00000008;
	else if (!(groups&0x00000010)) return 0x00000010;
	else if (!(groups&0x00000020)) return 0x00000020;
	else if (!(groups&0x00000040)) return 0x00000040;
	else if (!(groups&0x00000080)) return 0x00000080;
	else if (!(groups&0x00000100)) return 0x00000100;
	else return 0;
}
