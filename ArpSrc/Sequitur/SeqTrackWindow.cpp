/* SeqTrackWindow.cpp
 */
#include <stdio.h>
#include <app/Application.h>
#include <app/Clipboard.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpMultiScrollBar.h"
#include "ArpViews/ArpRangeControl.h"
#include "ArpViews/ArpTwoStateButton.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmToolBar.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmTransport.h"
#include "Sequitur/SeqMeasureControl.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqSignatureWindow.h"
#include "Sequitur/SeqSplitterView.h"
#include "Sequitur/SeqToolBarView.h"
#include "Sequitur/SeqTrackHrzSecondaryData.h"
#include "Sequitur/SeqTrackHrzSecondaryInfo.h"
#include "Sequitur/SeqTrackWindow.h"
#include "Sequitur/SeqTrackWindowAux.h"

// TEMP!
#define SS_PLAY_TRACK			'aPlT'

static const int32 EDIT_MENU_INDEX		= 0;
static const int32 VIEW_MENU_INDEX		= 1;
static const int32 TOOL_BARS_MENU_INDEX	= 2;
static const int32 WINDOWS_MENU_INDEX	= 3;

#define ARP_NEW_DATA_VIEW_MSG		'aNdM'
#define ARP_REMOVE_VIEW_MSG			'aReV'
#define CREATE_RHYTHM_MSG	'pCrR'

static const float		_BASE_BEAT_LENGTH		= 32;
//static const char*		SZ_FILTER_CONTROL_PANEL = "filter_control_panel";
static const uint32		SHOW_SATURATION_WIN_MSG		= 'iSSw';
static const uint32		CHANGE_TO_MSG				= 'icht';
static const uint32		LOOP_MSG					= '@lop';
static const uint32		SHOW_TOOL_BAR_MSG			= 'isTB';
static const uint32		DELETE_TOOL_BAR_MSG			= 'idTB';
static const uint32		VELOCITY_FINISHED_MSG		= 'ivlf';
static const uint32		ACTIVE_TOOL_SET_CHANGED_MSG	= 'iats';

static const char*		MEASURE_CONTROL_STR		= "measure_control";
static const char*		SPLITTER_1_STR			= "splitter_1";

static const BBitmap*	gControlBg = NULL;

static const float		INITIAL_SEPARATION			= 2;

/*************************************************************************
 * SEQ-TRACK-WINDOW Class Methods
 *************************************************************************/
SeqTrackWindow* SeqTrackWindow::ClassOpen(	AmSongRef songRef,
											AmTrackRef trackRef,
											AmTime time,
											BString factorySignature)
{
	SeqTrackWindow		*tWin;
		
	tWin = new SeqTrackWindow(songRef, trackRef, time, factorySignature);
	if (!tWin) return NULL;
	tWin->Show();
	return tWin;
}

SeqTrackWindow* SeqTrackWindow::ClassOpen(	AmSongRef songRef,
											const BMessage* config)
{
	/* Any track configuration without a "primary_track" is pre-2.0,
	 * and not valid for opening.
	 */
	int32			i;
	if (config->FindInt32("primary_track", &i) != B_OK) return NULL;

	SeqTrackWindow		*win = new SeqTrackWindow(songRef, config);
	if (!win) return NULL;
	win->Show();
	return win;
}

static BRect initial_frame(BWindow* owner)
{
	BScreen		s(owner);
	if	(!s.IsValid() ) return BRect(100, 20, 540, 520);
	BRect		sf = s.Frame();
	return BRect(	sf.Width() * 0.25, sf.Height() * 0.25,
					sf.Width() * 0.75, sf.Height() * 0.75 );
}

/*************************************************************************
 * SEQ-TRACK-WINDOW
 *************************************************************************/
SeqTrackWindow::SeqTrackWindow(	AmSongRef songRef,
								AmTrackRef trackRef,
								AmTime time,
								BString factorySignature)
		: inherited(initial_frame(this),
					"Edit Track",
					B_DOCUMENT_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK),
		AmSongObserver(songRef),
		mSelections(NULL), mCachedEndTime(0),
		mFactorySignature(factorySignature),
		mDurationCtrl(NULL), mVelocityCtrl(NULL),
		mOrderedSaturation(0.55), mShadowSaturation(0.25), mLoop(false),
		mPriDataView(0), mPriInfoView(0),
		mChangeToMenu(NULL), mShowToolBarMenu(NULL), mDeleteToolBarMenu(NULL),
		mFilterItem(NULL), mTracksField(NULL),
		mSecondaryInfo(0), mSecondaryData(0),
		mBgView(0), mHsb(0),
		mInspectorFactory(0),
		mUndoItem(0), mRedoItem(0), mActiveToolSetThread(0)
{
	mTrackRefs.push_back(trackRef);
	mMtc.SetBeatLength(_BASE_BEAT_LENGTH);
	Init();
	ScrollToTime(time);

	mActiveToolSetThread = spawn_thread(ActiveToolSetThreadEntry,
										"ARP Active Tool Set",
										B_NORMAL_PRIORITY,
										this);
	if (mActiveToolSetThread > 0) resume_thread(mActiveToolSetThread);
}

SeqTrackWindow::SeqTrackWindow(	AmSongRef songRef,
								const BMessage* config)
		: inherited(initial_frame(this),
					"Edit Track",
					B_DOCUMENT_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK),
		AmSongObserver(songRef),
		mSelections(NULL), mCachedEndTime(0),
		mDurationCtrl(NULL), mVelocityCtrl(NULL),
		mOrderedSaturation(0.55), mShadowSaturation(0.25), mLoop(false),
		mPriDataView(0), mPriInfoView(0),
		mChangeToMenu(NULL), mShowToolBarMenu(NULL), mDeleteToolBarMenu(NULL),
		mFilterItem(NULL), mTracksField(NULL),
		mSecondaryInfo(0), mSecondaryData(0),
		mBgView(0), mHsb(0),
		mInspectorFactory(0),
		mUndoItem(0), mRedoItem(0), mActiveToolSetThread(0)
{
	const AmSong*		song = ReadLock();
	if (song) {
		int32			i;
		if (config->FindInt32("primary_track", &i) == B_OK) {
			AmTrackRef	ref;
			if (song->TrackRefForIndex(i, ref) == B_OK) mTrackRefs.push_back(ref);
		}
		if (mTrackRefs.size() < 1) {
			AmTrackRef	ref;
			if (song->TrackRefForIndex(0, ref) == B_OK) mTrackRefs.push_back(ref);
		}
	}
	ReadUnlock(song);

	mMtc.SetBeatLength(_BASE_BEAT_LENGTH);
	Init();
}

SeqTrackWindow::~SeqTrackWindow()
{
	if (mActiveToolSetThread > 0) kill_thread(mActiveToolSetThread);
	mActiveToolSetThread = 0;
	RemoveCommonFilter(this);
	if (mSignatureWin.IsValid() ) mSignatureWin.SendMessage(B_QUIT_REQUESTED);
	if (mSatWin.IsValid() ) mSatWin.SendMessage(B_QUIT_REQUESTED);
	if (mMotionWin.IsValid() ) mMotionWin.SendMessage(B_QUIT_REQUESTED);
	
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::~SeqTrackWindow() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) song->Transport().StopWatching(BMessenger(this) );
	ReadUnlock(song);
	// END READ SONG BLOCK
	if (mTrackRefs.size() > 0) mTrackRefs[0].RemoveObserverAll(this);
	SongRef().RemoveObserverAll(this);
	AmGlobals().RemoveObserver(AmGlobalsI::TOOL_BAR_OBS, this);
	
	delete mInspectorFactory;
	delete mSelections;
}

void SeqTrackWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	ArpASSERT(message);
	if (message->what == B_MOUSE_DOWN) {
		Activate(true);
	} else if (message->what == B_KEY_DOWN) {
		int32	key;
		if (message->FindInt32("key", &key) == B_OK ) {
// FIX:  Can't I just get rid of this now?
			AmToolRef			toolRef = AmGlobals().FindTool(key);
			if (toolRef.IsValid() ) {
				uint32	button = B_PRIMARY_MOUSE_BUTTON;
				if (modifiers()&B_SHIFT_KEY) button = B_TERTIARY_MOUSE_BUTTON;
				AmGlobals().SetTool(toolRef.ToolKey(), button, 0);
				return;
			} else if (key == 0x5b) {		// Enter on a numeric keypad
				if( SongRef().IsPlaying() ) { BMessage m(STOP_SONG_MSG); PostMessage(&m); }
				else { BMessage m(PLAY_SONG_MSG); PostMessage(&m); }
				return;
			} else if (key == 0x64) {		// 0 on a numeric keypad
				if( SongRef().IsPlaying() ) { BMessage m(STOP_SONG_MSG); PostMessage(&m); }
				else { BMessage m(SS_PLAY_TRACK); PostMessage(&m); }
				return;
			}
		}
	}
	inherited::DispatchMessage(message, handler);
}

void SeqTrackWindow::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	mInspectorFactory->SetRight(new_width);
	SetupHsb();
	BScrollBar*	sb = PrimaryVertical();
	if (sb && mPriDataView) arp_setup_vertical_scroll_bar(sb, mPriDataView);
}

static void add_tool_bar_items_to(BMenu* menu, uint32 what, uint32 mark)
{
	AmToolBarRef	toolBarRef;
	for (uint32 k = 0; (toolBarRef = AmGlobals().ToolBarAt(k)).IsValid(); k++) {
		const char*	toolBarName = toolBarRef.ToolBarName();
		ArpASSERT(toolBarName);
		if (toolBarName) {
			BMessage*	msg = new BMessage(what);
			if (msg) {
				msg->AddString("tool_bar", toolBarName);
				BMenuItem*	item = new BMenuItem(toolBarName, msg);
				if (item) {
					menu->AddItem(item);
					if (mark) {
						// READ TOOL BAR BLOCK
						const AmToolBar*	toolBar = toolBarRef.ReadLock();
						if (toolBar && toolBar->IsShowing() ) item->SetMarked(true);
						toolBarRef.ReadUnlock(toolBar);
						// END READ TOOL BAR BLOCK
					}
				}
			}
		}
	}
}

void SeqTrackWindow::MenusBeginning()
{
	inherited::MenusBeginning();
	BMenuBar*	bar = KeyMenuBar();
	if( !bar ) return;
	BMenu*		menu;
	BMenuItem*	item;
	bool		canCopy = ( mSelections != 0 );
	bool		canPaste = mPropertiesTool.CanPaste();
	// Edit menu
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::MenusBeginning() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if (song) {
		const BUndoContext* u = song->UndoContext();
		BList context;
		for (uint32 k = 0; k < mTrackRefs.size(); k++)
			context.AddItem(mTrackRefs[k].TrackId() );
//		context.AddItem( mTrackRef.TrackId() );
		if (mUndoItem && u) {
			if (u->CountUndos(&context) > 0) {
				const char* name = u->UndoName(&context);
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
			if (u->CountRedos(&context) > 0) {
				const char* name = u->RedoName(&context);
				BString dispName = "Redo ";
				dispName << (name ? name : "Something");
				mRedoItem->SetEnabled(true);
				mRedoItem->SetLabel(dispName.String());
			} else {
				mRedoItem->SetEnabled(false);
				mRedoItem->SetLabel("Redo");
			}
		}
	}
	ReadUnlock( song );
	// END READ SONG BLOCK

	if( (menu = bar->SubmenuAt(EDIT_MENU_INDEX)) != 0 ) {
		if( (item = menu->FindItem(B_CUT)) != 0 ) item->SetEnabled( canCopy );
		if( (item = menu->FindItem(B_COPY)) != 0 ) item->SetEnabled( canCopy );
		if( (item = menu->FindItem(B_PASTE)) != 0 ) item->SetEnabled( canPaste );
	}

	// VIEW menu
	if (mChangeToMenu) {
		BMenuItem*	item;
		for (int32 k = 0; (item = mChangeToMenu->ItemAt(k)); k++) {
			if (mPriName.Compare( item->Label() ) == 0) {
				item->SetEnabled(false);
				item->SetMarked(true);
			} else {
				item->SetEnabled(true);
				item->SetMarked(false);
			}
		}
	}

	// TOOL BAR menu
	if (mShowToolBarMenu) {
		mShowToolBarMenu->RemoveItems(0, mShowToolBarMenu->CountItems(), true);
		add_tool_bar_items_to(mShowToolBarMenu, SHOW_TOOL_BAR_MSG, true);
	}
	if (mDeleteToolBarMenu) {
		mDeleteToolBarMenu->RemoveItems(0, mDeleteToolBarMenu->CountItems(), true);
		add_tool_bar_items_to(mDeleteToolBarMenu, DELETE_TOOL_BAR_MSG, false);
	}
	
	// WINDOWS menu
	if (mFilterItem) {
		if ( seq_flag_is_on(SEQ_FILTER_WINDOW_ACTIVE) ) mFilterItem->SetMarked(true);
		else mFilterItem->SetMarked(false);
	}

	// Choose track: menu field
	if (mTracksField && mTracksField->MenuBar() ) {
		_TwChooseTrackMenu*		m = dynamic_cast<_TwChooseTrackMenu*>(mTracksField->Menu() );
		if (m) m->EmptyState();
	}
}

void SeqTrackWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case AM_SCROLL_MSG:
			float		x, pri_y, sec_y;
			if (msg->FindFloat("x", &x) != B_OK) x = 0;
			if (msg->FindFloat("pri_y", &pri_y) != B_OK) pri_y = 0;
			if (msg->FindFloat("sec_y", &sec_y) != B_OK) sec_y = 0;
			if (x != 0 || pri_y != 0 || sec_y != 0)
				AmScrollBy(x, pri_y, sec_y);
			break;
		case AmGlobalsI::TOOL_BAR_OBS:
			AddToolBars();
			break;
		case AmSong::END_TIME_CHANGE_OBS:
			AmTime		endTime;
			if (find_time(*msg, "end_time", &endTime) == B_OK) {
				mCachedEndTime = endTime;
				SetupHsb();
			}
			break;
		case AmSong::TRACK_CHANGE_OBS:
			TrackChangeReceived(msg);
			break;
		case AmTrack::TITLE_CHANGE_OBS:
			const char*		str;
			if (msg->FindString( SZ_TRACK_TITLE, &str ) == B_OK)
				SetTitle(str);
			break;
		case CREATE_RHYTHM_MSG: {
			int32					initialNote = -1;
			AmSelectionsI*			selections = Selections();
			track_id				tid = OrderedTrackAt(0).TrackId();
			if (selections && tid) {
				AmPhraseEvent*		topPhrase;
				AmEvent*			event;
				for (uint32 k = 0; selections->EventAt(tid, k, &topPhrase, &event) == B_OK; k++) {
					AmNoteOn*		noteOn = NULL;
					if (event->Type() == event->NOTEON_TYPE
							&& ((noteOn = dynamic_cast<AmNoteOn*>(event)) != NULL) ) {
						initialNote = noteOn->Note();
						break;
					}
				}
			}
			if (initialNote < 0) {
				BAlert*	alert = new BAlert(	"Warning", "You must have one or more notes selected to create a motion.",
										"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
				if (alert) alert->Go();
				return;
			}
			if (mTrackRefs.size() > 0) {
				if (!mMotionWin.IsValid() ) {
					TwCreateMotionWin*	win = new TwCreateMotionWin(SongRef(), BMessenger(this), uint8(initialNote) );
					if (win) {
						mMotionWin = BMessenger(win);
						win->Show();
					}
				} else {
					BHandler*		target;
					BLooper*		looper;
					if ( (target = mMotionWin.Target(&looper)) != NULL) {
						BWindow*	win = dynamic_cast<BWindow*>(target);
						if (win) win->Activate(true);
					}
				}
//				TwCreateMotionWin*		win = new TwCreateMotionWin(SongRef(), BMessenger(this), uint8(initialNote) );
//				if (win) win->Show();
			}
		} break;
		case DUR_QUANTIZE_FINISHED_MSG: {
			if (mPriDataView) mPriDataView->Invalidate();
			if (mSecondaryData) mSecondaryData->InvalidateAll();
		} break;
		case DUR_EIGHTHS_FINISHED_MSG: {
			if (mPriDataView) mPriDataView->Invalidate();
			if (mSecondaryData) mSecondaryData->InvalidateAll();
		} break;
		case B_OBSERVER_NOTICE_CHANGE:
			ObserverMessageReceived(msg);
			break;

		case B_UNDO: {
			// WRITE TRACK BLOCK
			AmSong*	song = WriteLock();
			if (song && song->UndoContext() ) {
				BList context;
				for (uint32 k = 0; k < mTrackRefs.size(); k++)
					context.AddItem(mTrackRefs[k].TrackId() );
				AmTime	oldEnd = song->CountEndTime();
				song->UndoContext()->Undo(&context);
				AmTime	newEnd = song->CountEndTime();
				if (oldEnd != newEnd) song->EndTimeChangeNotice(newEnd);
			}
			WriteUnlock(song);
			// END WRITE TRACK BLOCK
		} break;
		case 'REDO': {
			// WRITE TRACK BLOCK
			AmSong*	song = WriteLock();
			if( song && song->UndoContext() ) {
				BList context;
				for (uint32 k = 0; k < mTrackRefs.size(); k++)
					context.AddItem(mTrackRefs[k].TrackId() );
				AmTime	oldEnd = song->CountEndTime();
				song->UndoContext()->Redo(&context);
				AmTime	newEnd = song->CountEndTime();
				if (oldEnd != newEnd) song->EndTimeChangeNotice(newEnd);
			}
			WriteUnlock(song);
			// END WRITE TRACK BLOCK
		} break;

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
		case CHANGE_TO_MSG:
			HandleChangeToMsg( msg );
			break;
		case SHOW_TOOL_BAR_MSG: {
			const char*			toolBarName;
			if (msg->FindString("tool_bar", &toolBarName) == B_OK) {
				AmGlobalsI*		globalsI = &(AmGlobals());
				if (!globalsI) printf("COULDN'T GET GLOBALS AT ALL\n");
				AmGlobalsImpl*	impl = dynamic_cast<AmGlobalsImpl*>(globalsI);
				if (impl) impl->ToggleShowing(toolBarName);
			}
		} break;
		case DELETE_TOOL_BAR_MSG: {
			const char*			toolBarName;
			if (msg->FindString("tool_bar", &toolBarName) == B_OK) {
				AmGlobalsI*		globalsI = &(AmGlobals());
				if (!globalsI) printf("COULDN'T GET GLOBALS AT ALL\n");
				AmGlobalsImpl*	impl = dynamic_cast<AmGlobalsImpl*>(globalsI);
				if (impl) impl->DeleteToolBar(toolBarName);
			}
		} break;
		case LOOP_MSG: {
			SeqMeasureControl*	mc = MeasureControl();
			bool				isLooping;
			if (mc && msg->FindBool("on", &isLooping) == B_OK) {
				mc->SetMarkerVisible(AM_LEFT_LOOP_MARKER | AM_RIGHT_LOOP_MARKER, isLooping);
			}
		} break;
		case SHOW_SATURATION_WIN_MSG: {
			if (!mSatWin.IsValid() ) {
				_TwSaturationWin*	win = new _TwSaturationWin(this, mOrderedSaturation, mShadowSaturation);
				if (win) {
					mSatWin = BMessenger(win);
					win->Show();
				}
			} else {
				BHandler*		target;
				BLooper*		looper;
				if ( (target = mSatWin.Target(&looper)) != NULL) {
					BWindow*	win = dynamic_cast<BWindow*>(target);
					if (win) win->Activate(true);
				}
			}
		} break;
		case TW_ORDERED_SAT_MSG: {
			int32	sat;
			if (msg->FindInt32( "be:value", &sat ) == B_OK) {
				float	orderedSaturation = (float)sat / 100;
				if (orderedSaturation < 0) orderedSaturation = 0;
				else if (orderedSaturation > 1) orderedSaturation = 1;
				if (mOrderedSaturation != orderedSaturation) {
					mOrderedSaturation = orderedSaturation;
					BMessage		msg(AM_SATURATION_MSG);
					msg.AddInt32("sat_type", 0);
					PostToViews(&msg, POST_TO_DATA_VIEWS);
				}
			}
		} break;
		case TW_SHADOW_SAT_MSG: {
			int32	sat;
			if (msg->FindInt32( "be:value", &sat ) == B_OK) {
				float		shadowSaturation = (float)sat / 100;
				if (shadowSaturation < 0) shadowSaturation = 0;
				else if (shadowSaturation > 1) shadowSaturation = 1;
				if (mShadowSaturation != shadowSaturation) {
					mShadowSaturation = shadowSaturation;
					BMessage		msg(AM_SATURATION_MSG);
					msg.AddInt32("sat_type", 1);
					PostToViews(&msg, POST_TO_DATA_VIEWS);
				}
			}
		} break;
		case SS_PLAY_TRACK:
			if (mTrackRefs.size() > 0) HandlePlay(mTrackRefs[0] );
			break;
		case PLAY_SONG_MSG:
			HandlePlay( AmTrackRef() );
			break;
		case STOP_SONG_MSG: {
			SongRef().StopTransport();
			if (mPriDataView) PostMessage(AM_STOP_MSG, mPriDataView);
		} break;
		case TRANSPORT_CHANGE_MSG:
			TransportChangeReceived( msg );
			break;
		case B_CUT:
			CutSelectedEvents();
			break;
		case B_COPY:
			CopySelectedEvents();
			break;
		case B_PASTE:
			PasteSelectedEvents();
			break;
		case DUPLICATE_INFO_MSG:
			DuplicateInfoReceived( msg );
			break;
		case CHANGE_INFO_MSG:
			ChangeInfoReceived( msg );
			break;
		case REMOVE_INFO_MSG:
			if (mSecondaryInfo && mSecondaryData) {
				view_id		id;
				if( msg->FindPointer( SZ_VIEW_ID, &id ) == B_OK ) {
					int32	infoIndex = mSecondaryInfo->IndexOf( id );
					if( infoIndex >= 0 ) {
						BView*	view = mSecondaryData->ItemAt( infoIndex );
						if( view ) {
							mSecondaryInfo->RemoveMiniView( id );
							mSecondaryData->RemoveMiniView( view );
						}
					}
				}
			}
			break;
		case TW_PRIMARY_TRACK_MSG: {
			track_id		tid;
			const char*		trackName;
			if (msg->FindPointer("track_id", &tid) == B_OK
					&& msg->FindString("track_name", &trackName) == B_OK)
				SetPrimaryTrack(tid, trackName);
		} break;
		case TW_SET_ORDERED_TRACK_MSG: {
			track_id		tid;
			int32			order;
			if (msg->FindPointer("track_id", &tid) == B_OK
					&& msg->FindInt32("order", &order) == B_OK)
				SetOrderedTrack(tid, order);
		} break;
		case TW_CLEAR_ORDERED_TRACK_MSG: {
			track_id		tid;
			if (msg->FindPointer("track_id", &tid) == B_OK)
				ClearOrderedTrack(tid);
		} break;
		case 'xups':
			{
				BScrollBar*	sb = PrimaryVertical();
				if (sb && mPriDataView) arp_setup_vertical_scroll_bar(sb, mPriDataView);
			}
			break;
		case ACTIVE_TOOL_SET_CHANGED_MSG:
			ActiveToolSetChanged();
			break;
		default:
			inherited::MessageReceived(msg);
		break;
	}
}

void SeqTrackWindow::WindowActivated(bool state)
{
	inherited::WindowActivated(state);
	if (state) AmGlobals().SelectSong(SongRef());
}

void SeqTrackWindow::Quit()
{
	RemoveCommonFilter(this);
	inherited::Quit();
}

bool SeqTrackWindow::QuitRequested()
{
	SongRef().RemoveObserverAll(this);
	if (mTrackRefs.size() > 0) mTrackRefs[0].RemoveObserverAll(this);

	SaveState();
	return true;
}

void SeqTrackWindow::ScrollToTime(AmTime time)
{
	if (!Lock() ) return;
	if (mHsb && mPriDataView) {
		float	pixel = mMtc.TickToPixel(time);
		pixel = pixel - (mPriDataView->Frame().Width() * 0.15);
		if (pixel < 0) pixel = 0;
		mHsb->SetValue(pixel);
	}
	Unlock();
}

track_id SeqTrackWindow::TrackId() const
{
	if (mTrackRefs.size() < 1) return 0;
	return mTrackRefs[0].TrackId();
}

void SeqTrackWindow::SaveState()
{
	StoreViewConfig();
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		// WRITE TRACK BLOCK
		AmSong*		song = WriteLock();
		AmTrack*	track = song ? song->Track(mTrackRefs[0] ) : NULL;
		if (track) track->SetConfiguration(&config);
		WriteUnlock(song);
		// END WRITE TRACK BLOCK
	}
}

uint32 SeqTrackWindow::CountOrderedTracks() const
{
	return mTrackRefs.size();
}

AmTrackRef SeqTrackWindow::OrderedTrackAt(uint32 index) const
{
	if (index >= mTrackRefs.size() ) return AmTrackRef();
	return mTrackRefs[index];
}

const AmTimeConverter& SeqTrackWindow::TimeConverter() const
{
	return mMtc;
}

AmSelectionsI* SeqTrackWindow::Selections() const
{
	if (mSelections) mSelections->Scrub();
	return mSelections;
}

void SeqTrackWindow::SetSelections(AmSelectionsI* selections)
{
	bool	redraw = false;
	if (mSelections) {
		redraw = true;
		if (mSelections != selections) delete mSelections;
	}
	mSelections = selections;
	if (mSelections || redraw) {
		if (mPriDataView) mPriDataView->Invalidate( mPriDataView->Bounds() );
		if (mSecondaryData) {
			BView*	view;
			for( view = mSecondaryData->ChildAt(0); view != 0; view = view->NextSibling())
				view->Invalidate( view->Bounds() );
		}
	}

	if (mInspectorFactory)
		mInspectorFactory->InstallViewFor(mSelections);
}

AmTime SeqTrackWindow::GridTime() const
{
	if (!mDurationCtrl) return PPQN;
	return mDurationCtrl->RawTicks();
}

void SeqTrackWindow::GetSplitGridTime(int32* m, AmTime* v, int32* d) const
{
	if (!mDurationCtrl) {
		*m = 1;
		*v = PPQN;
		*d = 2;
	} else mDurationCtrl->GetSplitTime(m, v, d);
}

uint8 SeqTrackWindow::Velocity() const
{
	if (!mVelocityCtrl) return AmVelocityControl::InitialVelocity();
	return mVelocityCtrl->Velocity();
}

float SeqTrackWindow::OrderedSaturation() const
{
	return mOrderedSaturation;
}

float SeqTrackWindow::ShadowSaturation() const
{
	return mShadowSaturation;
}

void SeqTrackWindow::PostMessageToDataView(BMessage& msg, view_id fromView)
{
	BLooper*		looper;
	
	/* First check to see if the info view passing this in is the primary
	 * info view -- if so, just post to the primary data view and be done.
	 */
	if (mPriInfoView == fromView) {
		if( mPriDataView
				&& ( looper = mPriDataView->Looper() )
				&& ( looper->Lock() ) ) {
			mPriDataView->MessageReceived( &msg );
			looper->Unlock();
		}
		return;
	}

	/* Otherwise, find the corresponding secondary data view for this
	 * (presumably) secondary info view.
	 */
	if (!mSecondaryInfo || !mSecondaryData) return;
	int32			index = mSecondaryInfo->IndexOf(fromView);
	if (index < 0) {
		printf("SeqTrackWindow::PostMessageToDataView() info didn't exist\n"); fflush(stdout);
		return;
	}
	BView*			targetData = mSecondaryData->ItemAt(index);
	if (!targetData) {
		printf("SeqTrackWindow::PostMessageToDataView() data didn't exist\n"); fflush(stdout);
		return;
	}
	if( (looper = targetData->Looper())
			&& ( looper->Lock() ) ) {
		targetData->MessageReceived( &msg );
		looper->Unlock();
	}
}

void SeqTrackWindow::PostMessageToInfoView(BMessage& msg, view_id fromView)
{
	BLooper*		looper;
	
	/* First check to see if the data view passing this in is the primary
	 * data view -- if so, just post to the primary info view and be done.
	 */
	if (mPriDataView == fromView) {
		if (mPriInfoView
				&& (looper = mPriInfoView->Looper() )
				&& (looper->Lock() ) ) {
			mPriInfoView->MessageReceived(&msg);
			looper->Unlock();
		}
		return;
	}

	/* Otherwise, find the corresponding secondary info view for this
	 * (presumably) secondary data view.
	 */
	if (!mSecondaryInfo || !mSecondaryData) return;
	int32			index = mSecondaryData->IndexOf(fromView);
	if (index < 0) {
		printf("SeqTrackWindow::PostMessageToInfoView() data didn't exist\n"); fflush(stdout);
		return;
	}
	BView*			targetInfo = mSecondaryInfo->ItemAt(index);
	if (!targetInfo) {
		printf("SeqTrackWindow::PostMessageToInfoView() info didn't exist\n"); fflush(stdout);
		return;
	}
	if ( (looper = targetInfo->Looper())
			&& ( looper->Lock() ) ) {
		targetInfo->MessageReceived(&msg);
		looper->Unlock();
	}
}

bool SeqTrackWindow::IsSignificant() const
{
	return false;
}

status_t SeqTrackWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT(config);
	status_t			err = GetDimensions(config, this);
	if (err != B_OK) return err;
	BRect				b(Bounds() );
	BView*				v = FindView(SPLITTER_1_STR);
	if (v) config->AddFloat(SPLITTER_1_STR, v->Frame().top / b.Height() );

	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::GetConfiguration() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) {
		if (mTrackRefs.size() > 0) {
			int32		index = song->TrackIndex(mTrackRefs[0].TrackId() );
			if (index >= 0) config->AddInt32("primary_track", index);
		}
		for (uint32 k = 1; k < mTrackRefs.size(); k++) {
			int32		index = song->TrackIndex(mTrackRefs[k].TrackId() );
			if (index >= 0) config->AddInt32("ordered_track", index);
		}
	}
	ReadUnlock(song);
	// END READ SONG BLOCK

	config->AddFloat("beat_length", mMtc.BeatLength() );
	config->AddString("factory_sig", mFactorySignature);
	config->AddFloat("ordered_sat", mOrderedSaturation);
	config->AddFloat("shadow_sat", mShadowSaturation);
	if (mDurationCtrl) {
		int32	m, d;
		AmTime	v;
		mDurationCtrl->GetSplitTime(&m, &v, &d);
		config->AddInt32("dur_m", m);
		add_time(*config, "dur_v", v);
		config->AddInt32("dur_d", d);
	}
	if (mVelocityCtrl) config->AddInt32("vel_v", mVelocityCtrl->Velocity() );
	
	if (mHsb) add_time(*config, "time", mMtc.PixelToTick(mHsb->Value() ));
	return B_OK;
}

status_t SeqTrackWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	status_t			err = SetDimensions(config, this);
	if (err != B_OK) return err;

	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::SetConfiguration() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) {
		int32			i;
		mTrackRefs.resize(1);
		for (int32 k = 0; config->FindInt32("ordered_track", k, &i) == B_OK; k++) {
			AmTrackRef	ref;
			if (song->TrackRefForIndex(i, ref) == B_OK) mTrackRefs.push_back(ref);
		}
	}
	ReadUnlock(song);
	// END READ SONG BLOCK

	float				f;
	const char*			s;
	if (config->FindFloat("beat_length", &f) == B_OK) mMtc.SetBeatLength(f);
	if (config->FindString("factory_sig", &s) == B_OK) mFactorySignature = s;
	if (config->FindFloat("ordered_sat", &f) == B_OK) mOrderedSaturation = f;
	if (config->FindFloat("shadow_sat", &f) == B_OK) mShadowSaturation = f;

	return B_OK;
}

SeqMeasureControl* SeqTrackWindow::MeasureControl() const
{
	return dynamic_cast<SeqMeasureControl*>( FindView(MEASURE_CONTROL_STR) );
}

void SeqTrackWindow::HandlePlay(const AmTrackRef& solo)
{
	ArpASSERT(mPriDataView);
	AmTime				startTime = mMtc.PixelToTick(mPriDataView->Bounds().left);
	AmTime				stopTime = mMtc.PixelToTick(mPriDataView->Bounds().right);
	SeqMeasureControl*	mc = MeasureControl();
	if (mc) mc->SetTransportLooping();
	bool				toEnd;
	if (seq_get_bool_preference(TRACK_WIN_PLAY_TO_END_PREF, &toEnd) == B_OK && toEnd)
		stopTime = -1;
	SongRef().StartPlaying(solo, startTime, stopTime);
}

void SeqTrackWindow::TransportChangeReceived(BMessage* msg)
{
	int32	state = TS_STOPPED;
	AmTime	time = -1;
	bool	playToEnd;
	msg->FindInt32(ArpTransportState, &state);
	find_time(*msg, ArpTransportTime, &time);

	/* Don't redraw view if the window isn't visible -- but still
	 * go through the motions so my state is set.
	 */
	bool	redraw = true;
	if (IsMinimized() || IsHidden()) redraw = false;

	if (msg->FindBool("play_to_end", &playToEnd) != B_OK) playToEnd = false;
	if (state == TS_STOPPED) { ;
//		printf("*** Transport stopped.\n");
		AmTrackDataView*	dv = dynamic_cast<AmTrackDataView*>(mPriDataView);
		if (dv) dv->DrawSongPosition(-1, redraw);
	} else {
//		printf("Transport now at %ld\n", time);
		SeqMeasureControl*	mc = MeasureControl();
		if (mc) mc->SetMarkerTime(AM_POSITION_MARKER, time);
		AmTrackDataView*	dv = dynamic_cast<AmTrackDataView*>(mPriDataView);
		if (dv) dv->DrawSongPosition(time, redraw);
		if (playToEnd) {
			bool	follow;
			if (seq_get_bool_preference(TRACK_WIN_FOLLOW_PREF, &follow) != B_OK) follow = false;
			if (follow) TrackSongPosition(time);
		}
	}
			
	// Reply to transport message.  You must always reply.
	// ArpTransportNextTime is the next time for which this
	// observer can display a visibly different indicator.
	BMessage reply(B_REPLY);
	add_time(reply, ArpTransportNextTime, time+(PPQN/8));
	msg->SendReply(&reply);
}

void SeqTrackWindow::TrackSongPosition(AmTime time)
{
	if (!mHsb || !mPriDataView) return;
	/* This is the number of pixels in from the right edge I'll let the song
	 * position get before I decide to scroll it.
	 */
	float		right_buffer = 15;
	AmTime		left = mMtc.PixelToTick( mPriDataView->Bounds().left );
	AmTime		right = mMtc.PixelToTick( mPriDataView->Bounds().right - right_buffer );
	if ( time < left || time > right) {
		float		newValue = mMtc.TickToPixel(time);
		if (newValue < 0) newValue = 0;
		if ( newValue != mHsb->Value() ) mHsb->SetValue(newValue);
	}
}

void SeqTrackWindow::ShowSignatureWindow(int32 measure, uint32 beats, uint32 beatValue)
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

status_t SeqTrackWindow::GetLeftSignature(AmSignature& sig)
{
	if (mTrackRefs.size() < 1) return B_ERROR;
	AmTime		left = 0;
	if (mPriDataView) left = mMtc.PixelToTick(mPriDataView->Bounds().left);
	if (left < 0) left = 0;
	status_t	err = B_ERROR;
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::GetLeftSignature() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	const AmTrack*		track = song ? song->Track(mTrackRefs[0]) : NULL;
	if (track) err = track->GetSignature(left, sig);
	ReadUnlock( song );
	// END READ TRACK BLOCK
	return err;
}

void SeqTrackWindow::SetupHsb()
{
	ArpASSERT( mHsb );
	ArpASSERT( mPriDataView );
	if ( !mHsb || !mPriDataView ) return;

	float	width = mMtc.TickToPixel( mCachedEndTime + AmGlobals().EndTimeBuffer() );
	mHsb->SetRange( 0, width );
	arp_setup_scroll_bar( *mHsb, mPriDataView->Bounds(), width );
}

ArpMultiScrollBar* SeqTrackWindow::PrimaryVertical() const
{
	if( !mPriDataView || !mPriDataView->Parent() ) return 0;
	BView*		sb = mPriDataView->Parent()->FindView( "sb" );
	return dynamic_cast<ArpMultiScrollBar*>( sb );
}

void SeqTrackWindow::ObserverMessageReceived(const BMessage* msg)
{
	int32		what;
	msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
	switch( what ) {
		case 'zoom': {
			bool	hasY = false;
			float	valueY, valueX;
			if (msg->FindFloat("y_value", &valueY) == B_OK) hasY = true;
			if (msg->FindFloat("x_value", &valueX) == B_OK) {
				// Retrieve the current tick being shown in the center of
				// the display.
				BRect b = mPriDataView->Bounds();
				const float w = b.Width()+1;
				AmTime tick = mMtc.PixelToTick(b.left + w/2);
				mMtc.SetBeatLength(valueX * _BASE_BEAT_LENGTH);
				
				// First invalidate all the views, since anything the had contained
				// is no longer useful.
				if (!hasY && mPriDataView) mPriDataView->Invalidate();
				if (mSecondaryData ) mSecondaryData->InvalidateAll();
				if (MeasureControl() ) MeasureControl()->Invalidate();
				
				// Adjust the scroll bar, and scroll it to be showing the same ticks
				// that had previously been.
				SetupHsb();
				float center = mMtc.TickToPixel(tick);
				mHsb->SetValue(center - w/2);
			}
			if (mPriInfoView && hasY) {
				BMessage	newMsg('zoom');
				newMsg.AddFloat("y_value", valueY);
				PostMessage(&newMsg, mPriInfoView);
			}
		} break;
		default:
			break;
	}
}

void SeqTrackWindow::TrackChangeReceived(const BMessage* msg)
{
	if (mTrackRefs.size() < 1) return;
	track_id	id;
	int32		status;
	if (msg->FindPointer( SZ_TRACK_ID, &id) != B_OK) return;
	if (msg->FindInt32(	SZ_STATUS, &status) != B_OK) return;
	if (status == AM_REMOVED && id == mTrackRefs[0].TrackId() )
		PostMessage(B_QUIT_REQUESTED);
}

void SeqTrackWindow::ChangeInfoReceived(const BMessage* msg)
{
	if (!mSecondaryInfo || !mSecondaryData) return;
	/* Get the info I need to do the operation.
	 */
	const char*		sig;
	const char*		viewName;
	view_id			infoId;
	if (msg->FindString(SZ_FACTORY_SIGNATURE, &sig) != B_OK
			|| msg->FindString(SZ_FACTORY_VIEW_NAME, &viewName) != B_OK
			|| msg->FindPointer(SZ_VIEW_ID, &infoId) != B_OK)
		return;
	AmViewFactory*	factory = AmGlobals().FactoryNamed( BString(sig) );
	if (!factory) return;
	/* Find the views to replace.
	 */
	int32			index = mSecondaryInfo->IndexOf(infoId);
	if (index < 0) return;
	/* Construct the replacement views.
	 */
	AmViewProperty	fakeProp( sig, viewName );
	BView*			infoView = factory->NewInfoView(SongRef(), *this, &fakeProp, SEC_VIEW);
	BView*			dataView = factory->NewDataView(SongRef(), *this, &fakeProp, SEC_VIEW);
	if( !infoView || !dataView ) {
		delete infoView;
		delete dataView;
		return;
	}
	/* Replace the views.  It is necessary that the info view be done first,
	 * because the info view always has one more view than the data view -- it
	 * has the empty view which allows users to add more views.
	 */
	if( mSecondaryInfo->ReplaceMiniView( infoView, index ) )
		if( !mSecondaryData->ReplaceMiniView( dataView, index ) )
			mSecondaryData->AddMiniView( dataView );
}

void SeqTrackWindow::DuplicateInfoReceived(const BMessage* msg)
{
	if (!mSecondaryInfo || !mSecondaryData) return;
	/* Get the info I need to do the operation.
	 */
	const char*		sig;
	const char*		viewName;
	view_id			infoId;
	if (msg->FindString(SZ_FACTORY_SIGNATURE, &sig) != B_OK
			|| msg->FindString(SZ_FACTORY_VIEW_NAME, &viewName) != B_OK
			|| msg->FindPointer(SZ_VIEW_ID, &infoId) != B_OK)
		return;
	AmViewFactory*	factory = AmGlobals().FactoryNamed( BString(sig) );
	if (!factory) return;
	/* Find the view to duplicate.
	 */
	int32			index = mSecondaryInfo->IndexOf(infoId);
	if (index < 0) return;
	/* Construct the duplicate views.
	 */
	AmViewProperty	fakeProp(sig, viewName);
	BView*			infoView = factory->NewInfoView(SongRef(), *this, &fakeProp, SEC_VIEW );
	BView*			dataView = factory->NewDataView(SongRef(), *this, &fakeProp, SEC_VIEW );
	if (!infoView || !dataView) {
		delete infoView;
		delete dataView;
		return;
	}
	/* Replace the views.
	 */
	if (mSecondaryData->InsertMiniView(dataView, index) )
		mSecondaryInfo->InsertMiniView(infoView, index);
}

void SeqTrackWindow::AmScrollBy(float x, float pri_y, float sec_y)
{
	if( x && mHsb ) {
		float	new_x = mHsb->Value() + x;
		if( new_x < 0 ) new_x = 0;
		if( new_x != mHsb->Value() ) mHsb->SetValue( new_x );
	}
	BScrollBar*		sb = PrimaryVertical();
	if( pri_y && sb ) {
		float	new_y = sb->Value() + pri_y;
		if( new_y < 0 ) new_y = 0;
		if( new_y != sb->Value() ) sb->SetValue( new_y );
	}
}

void SeqTrackWindow::SetPrimaryTrack(track_id tid, const char* trackName)
{
	/* First see if this track is already in the ordered track
	 * list -- if it is, then I don't need to lock the song.
	 */
	bool		foundTid = false;
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == tid) {
			if (k == 0) return;
			mTrackRefs[0] = mTrackRefs[k];
			mTrackRefs.erase(mTrackRefs.begin() + k);
			foundTid = true;
			break;
		}
	}
	if (!foundTid) {
		// READ SONG BLOCK
		const AmSong*		song = SongRef().ReadLock();
		if (!song) return;
		AmTrackRef			trackRef;
		if (song->TrackRefForId(tid, trackRef) == B_OK) {
			if (mTrackRefs.size() > 0) mTrackRefs[0] = trackRef;
			else mTrackRefs.push_back(trackRef);
		}
		SongRef().ReadUnlock(song);
		// END READ SONG BLOCK
	}
	if (mTracksField && mTracksField->MenuItem() )
		mTracksField->MenuItem()->SetLabel(trackName);
	SetTitle(trackName);
	/* Now update all my data and info views about the change.
	 */
	BMessage		msg(AM_ORDERED_TRACK_MSG);
	msg.AddPointer("track_id", tid);
	msg.AddInt32("track_order", 0);
	PostToViews(&msg);

	if (mInspectorFactory)
		mInspectorFactory->SetTrackRef(mTrackRefs[0], mSelections);
}

void SeqTrackWindow::SetOrderedTrack(track_id tid, uint32 order)
{
	ArpASSERT(order > 0);
	AmTrackRef	trackRef;
	/* First see if this track is already in the ordered track
	 * list -- if it is, then I don't need to lock the song.
	 */
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == tid) {
			if (k == order) return;
			trackRef = mTrackRefs[k];
			mTrackRefs.erase(mTrackRefs.begin() + k);
			break;
		}
	}
	if (!trackRef.IsValid() ) {
		// READ SONG BLOCK
		const AmSong*		song = SongRef().ReadLock();
		if (song) song->TrackRefForId(tid, trackRef);
		SongRef().ReadUnlock(song);
		// END READ SONG BLOCK
	}
	if (trackRef.IsValid() ) {
		if (order >= mTrackRefs.size() ) mTrackRefs.push_back(trackRef);
		else mTrackRefs[order] = trackRef;
		BMessage		msg(AM_ORDERED_TRACK_MSG);
		msg.AddPointer("track_id", tid);
		msg.AddInt32("track_order", order);
		PostToViews(&msg);
	}
}

void SeqTrackWindow::ClearOrderedTrack(track_id tid)
{
	if (mSelections) mSelections->RemoveTrack(tid);
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == tid) {
			if (k == 0) return;
			mTrackRefs.erase(mTrackRefs.begin() + k);
			BMessage		msg(AM_ORDERED_TRACK_MSG);
			msg.AddPointer("track_id", tid);
			PostToViews(&msg);
			return;
		}
	}
}

void SeqTrackWindow::StoreViewConfig()
{
	AmTrackDataView*		view = dynamic_cast<AmTrackDataView*>(mPriDataView);
	if (view) {
		AmViewProperty		prop(view->FactorySignature().String(), view->ViewName().String() );
		prop.SetConfiguration( view->ConfigurationData() );
		if (mTrackRefs.size() > 0) {
			// WRITE TRACK BLOCK
			AmSong*		song = WriteLock();
			AmTrack*	track = song ? song->Track(mTrackRefs[0] ) : NULL;
			if (track) {
				track->SetProperty(&prop, PRI_VIEW);
				if (mSecondaryData) mSecondaryData->StoreViewProperties(track);
			}
			WriteUnlock(song);
			// END WRITE TRACK BLOCK
		}
	}
}

void SeqTrackWindow::PostToViews(BMessage* msg, uint32 flags)
{
	ArpASSERT(msg);
	if (mPriDataView && flags&POST_TO_DATA_VIEWS) PostMessage(msg, mPriDataView);
	if (mPriInfoView && flags&POST_TO_INFO_VIEWS) PostMessage(msg, mPriInfoView);
	if (mSecondaryData && flags&POST_TO_DATA_VIEWS) {
		BView*		v;
		for (int32 k = 0; (v = mSecondaryData->ItemAt(k)) != NULL; k++)
			PostMessage(msg, v);
	}
	if (mSecondaryInfo && flags&POST_TO_INFO_VIEWS) {
		BView*		v;
		for (int32 k = 0; (v = mSecondaryInfo->ItemAt(k)) != NULL; k++)
			PostMessage(msg, v);
	}
}

void SeqTrackWindow::CutSelectedEvents()
{
	if (mTrackRefs.size() < 1) return;
	if (!Selections() ) return;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		mPropertiesTool.Cut(song, Selections(), BMessenger() );
		SetSelections(NULL);
	}
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void SeqTrackWindow::CopySelectedEvents()
{
	if (!Selections() ) return;
	mPropertiesTool.Copy(Selections() );
}

void SeqTrackWindow::PasteSelectedEvents()
{
	if (mTrackRefs.size() < 1) return;
	if (!mPriDataView || !mPropertiesTool.CanPaste() ) return;
	AmTime			newTime = mMtc.PixelToTick( mPriDataView->Bounds().left );
	AmSelectionsI*	selections = AmSelectionsI::NewSelections();

	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		mPropertiesTool.Paste(song, *this, newTime, selections, BMessenger(), 0);
		SetSelections(selections);
	}
	WriteUnlock( song );
	// END WRITE SONG BLOCK
}

status_t SeqTrackWindow::AddChangeToMenuItem(BMenu* toMenu)
{
	mChangeToMenu = new BMenu("Change to");
	if (!mChangeToMenu) return B_NO_MEMORY;
	AmViewFactory*	fact = AmGlobals().FactoryNamed( BString( mFactorySignature ) );
	if (!fact) return B_ERROR;
	BString			mt1(fact->EventMetaType());
	BString			mt2;
	
	for (uint32 k = 0; (fact = AmGlobals().FactoryAt(k) ) != 0; k++) {
		mt2 = fact->EventMetaType();
		if (mt1 == mt2) {
			BString		name;
			for (uint32 k = 0; fact->DataNameAt(k, PRI_VIEW, name) == B_OK; k++) {
				BMessage*	msg = new BMessage( CHANGE_TO_MSG );
				BMenuItem*	item = new BMenuItem( name.String(), msg, 0 );
				if( msg && item ) {
					msg->AddString( "signature", fact->Signature().String() );
					msg->AddString( "name", name.String() );
					mChangeToMenu->AddItem( item );
				}
				name = (const char*)NULL;
			}
		}
	}
	BMenuItem*	superitem = new BMenuItem(mChangeToMenu);
	if( !superitem ) {
		delete mChangeToMenu;
		mChangeToMenu = NULL;
		return B_NO_MEMORY;
	}
	toMenu->AddItem(superitem);
	return B_OK;
}

void SeqTrackWindow::HandleChangeToMsg(BMessage* msg)
{
	if (!msg || !mPriDataView || !mPriInfoView) return;
	ArpMultiScrollBar*	sb = PrimaryVertical();
	if (!sb) return;
	const char*		signature;
	const char*		name;
	if (msg->FindString("signature", &signature) != B_OK) return;
	if (msg->FindString("name", &name ) != B_OK) return;
	AmViewFactory*	fact = AmGlobals().FactoryNamed( BString(signature) );
	if (!fact) return;

	AmViewProperty	fakeProp(signature, name);
	/* If the data view is being swapped, preserve it's configuration and
	 * supply it to the new view.  This lets similar views maintain whatever
	 * configuration the user might have done, although it does mean that
	 * views need to be aware of this, and not make use of data that might
	 * conflict with their settings.
	 */
	#if 1
	AmTrackDataView*	tdv = dynamic_cast<AmTrackDataView*>(mPriDataView);
	if (tdv) {
		BMessage*	config = tdv->ConfigurationData();
		if (tdv) fakeProp.SetConfiguration(config);
		delete config;
	}
	#endif

	BView*			dv = fact->NewDataView(SongRef(), *this, &fakeProp, PRI_VIEW);
	BView*			iv = fact->NewInfoView(SongRef(), *this, &fakeProp, PRI_VIEW);
	if (!dv || !iv) {
		delete dv;
		delete iv;
		return;
	}
	mPriName = name;
	iv->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	/* Resize the newly created views to the same frames as the old primary views.
	 */
	BRect		f = mPriDataView->Frame();
	dv->MoveTo(f.left, f.top);
	dv->ResizeTo( f.Width(), f.Height() );
	f = mPriInfoView->Frame();
	iv->MoveTo( f.left, f.top );
	iv->ResizeTo( f.Width(), f.Height() );
	/* Assign the new primary views.
	 */
	if( mPriDataView->Parent() && mPriInfoView->Parent() ) {
		BeginViewTransaction();
		BView*		oldData = mPriDataView;
		BView*		oldInfo = mPriInfoView;
		BView*		dataParent = mPriDataView->Parent();
		BView*		infoParent = mPriInfoView->Parent();
		dataParent->RemoveChild(oldData);
		dataParent->AddChild(dv);
		infoParent->RemoveChild(oldInfo);
		infoParent->AddChild(iv);
		sb->ClearTargets();
		sb->SetTarget(dv);
		sb->AddTarget(iv);
		arp_setup_vertical_scroll_bar(sb, dv);
		sb->SetValue(0);
		if (mHsb) {
			mHsb->SetTarget(dv);
			mHsb->SetValue( mHsb->Value() );
		}
		mPriDataView = dv;
		mPriInfoView = iv;
		delete oldData;
		delete oldInfo;
		EndViewTransaction();
	}
}

bool SeqTrackWindow::HasStripViews() const
{
	AmViewFactory*	fact = AmGlobals().FactoryNamed( BString(mFactorySignature) );
	if (!fact) return false;
	BString			mt1(fact->EventMetaType());
	BString			mt2;
	
	for (uint32 k = 0; (fact = AmGlobals().FactoryAt(k) ) != 0; k++) {
		mt2 = fact->EventMetaType();
		if (mt1 == mt2) {
			BString		name;
			if (fact->DataNameAt(k, SEC_VIEW, name) == B_OK)
				return true;
		}
	}
	return false;
}

void SeqTrackWindow::Init()
{
	if (!gControlBg) gControlBg = ImageManager().FindBitmap(AM_TRACK_CONTROL_BG_STR);
	AddCommonFilter(this);
	BString				trackTitle;
	bool				isTempo = false;
	AmTrackRef			trackRef;
	BMessage			config;
	if (mTrackRefs.size() > 0) trackRef = mTrackRefs[0];
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::SeqTrackWindow() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) {
		song->Transport().StartWatching( BMessenger(this) );
		mCachedEndTime = song->CountEndTime();
		const AmTrack*	track = song->Track(trackRef);
		if (track) {
			trackTitle = track->Title();
			if (track == song->TempoTrack() ) isTempo = true;
			track->GetConfiguration(&config);
		}
	}
	ReadUnlock(song);
	// END READ TRACK BLOCK
	
	if (!config.IsEmpty() ) SetConfiguration(&config);
	BRect			b(Bounds() );

	SetTitle(trackTitle.String() );
	AddMainMenu();

	float			menuFontSize = 10;
	{
		const BFont*	menuFont = be_plain_font;
		if (menuFont	) menuFontSize = menuFont->Size();
		/* Bastard!  This value is used to increase the size of the inspector
		 * area to give it room for menu fields.  But the menu fields don't increase
		 * in size directly proportional to the size of the font, oh no.  They grow
		 * faster.  Since the user is severely restricted in what values are allowable
		 * right now, I don't have to worry much about this.  Still, hopefully things
		 * work a little better in the future.
		 */
		 if (menuFontSize > 10) menuFontSize++;
	}
	mToolBarLeftTop.Set(-1, -1);
	if (AddBackgroundView() ) {
		BPoint	end;
		end = AddButtons( BPoint(4, 7) );
		float			fh = arp_get_font_height(mBgView);
		/* I have to create a StringView for the label as the menu field.  I
		 * can't just use its label because, oh glories, if that control has
		 * a view bitmap, then whenever the control is pressed the bitmap is
		 * cleared.
		 */
		const char*		tracksL = "Tracks:";
		float			labelW = mBgView->StringWidth(tracksL) + 5;
		float			fudge = fh * 0.5;
		BRect			f(end.x + 5, 7 + fudge, end.x + 5 + labelW, 7 + fh + fudge);
		BStringView*	sv = new BStringView(f, "tracks_label", tracksL);
		if (sv) {
			mBgView->AddChild(sv);
			if (gControlBg) {
				BRect	src(gControlBg->Bounds() );
				BRect	dest(src);
				dest.OffsetBy(-f.left, -f.top);
				sv->SetViewBitmap(gControlBg, src, dest, B_FOLLOW_NONE);
			}
		}

		BMenu*			menu = new _TwChooseTrackMenu(trackTitle.String(), SongRef(), *this, BMessenger(this));
		if (menu) {
			BRect		f2(f.right + 1, 7, b.right - 5, 7 + 10);
			mTracksField = new BMenuField(f2, "choose_track_field", tracksL, menu);
			if (mTracksField) {
				mTracksField->SetDivider(0);
				mBgView->AddChild(mTracksField);
			}
		}
		
		float	top = end.y + 8;
		end = AddActiveTools( BPoint(4, top) );
		mToolBarLeftTop.Set(end.x + 8, top + 2);
		end = AddToolBars();

		float	measureY = end.y + 4;
		BRect	r(0, measureY, b.Width(), measureY + 17 + menuFontSize);
		mInspectorFactory = new AmInspectorFactory(SongRef(), trackRef, mBgView, r);
		AddDataViews(measureY + 18 + menuFontSize);

		/* This is a hack -- the tracks field will be generated for all the tracks
		 * even for the tempo window.  If I was actually going to pursue this, I'd
		 * probably want to have some way of letting tracks identify their types.
		 * Right now that's all just in the view factories and view properties.
		 * In this scheme, the menu field would only display tracks that were the
		 * same type.
		 */
		if (mTracksField && isTempo) mTracksField->SetEnabled(false);
	}

	trackRef.AddObserver(this, AmTrack::TITLE_CHANGE_OBS );
	SongRef().AddObserver(this, AmSong::END_TIME_CHANGE_OBS );
	SongRef().AddObserver(this, AmSong::TRACK_CHANGE_OBS );
	AmGlobals().AddObserver(AmGlobalsI::TOOL_BAR_OBS, this);

	/* Technically this should be part of the SetConfiguration() method, but
	 * this stuff has to be done after the views are constructed, and yet to
	 * construct the views I need a lot of what happens in SetConfiguration(),
	 * so I broke it up.
	 */
	float		f;
	if (config.FindFloat(SPLITTER_1_STR, &f) != B_OK) f = 0.80;
	SeqSplitterView*	hrz_s = dynamic_cast<SeqSplitterView*>(FindView(SPLITTER_1_STR) );
	if (hrz_s) hrz_s->MoveHorizontalSplitter(f * b.Height() );
	
	AmTime			t;
	if (mHsb && mPriDataView && find_time(config, "time", &t) == B_OK) {
		float		pixel = mMtc.TickToPixel(t);
		if (pixel < 0) pixel = 0;
		mHsb->SetValue(pixel);
	}
	if (mDurationCtrl) {
		int32	m, d;
		AmTime	v;
		if (config.FindInt32("dur_m", &m) == B_OK
				&& find_time(config, "dur_v", &v) == B_OK
				&& config.FindInt32("dur_d", &d) == B_OK)
			mDurationCtrl->SetValue(m, v, d);
	}
	if (mVelocityCtrl) {
		int32	v;
		if (config.FindInt32("vel_v", &v) == B_OK)
			mVelocityCtrl->SetZoomY(v);
	}
}

void SeqTrackWindow::AddMainMenu()
{
	BMenuBar	*menuBar;
	BMenu		*menu;
	BMenuItem	*menuItem;
	BRect		rect = Bounds();
	
	rect.right++;
	rect.bottom = rect.top + 16;
	menuBar = new BMenuBar(rect, NULL, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
			B_ITEMS_IN_ROW, FALSE);

	// EDIT menu
	menu = new BMenu("Edit", B_ITEMS_IN_COLUMN);
	mUndoItem = add_menu_item( menu, "Undo", B_UNDO, 'Z', B_COMMAND_KEY );
	mRedoItem = add_menu_item( menu, "Redo", 'REDO', 'Z', B_SHIFT_KEY|B_COMMAND_KEY );
	menu->AddSeparatorItem();
	add_menu_item( menu, "Cut", B_CUT, 'X' );
	add_menu_item( menu, "Copy", B_COPY, 'C' );
	add_menu_item( menu, "Paste", B_PASTE, 'V' );
	menu->AddSeparatorItem();
	add_menu_item(menu, "Create motion...", CREATE_RHYTHM_MSG, 0);
	menu->AddSeparatorItem();
	add_menu_item(menu, "Set time signature" B_UTF8_ELLIPSIS, CHANGE_SIGNATURE_MSG, 0);
	menuItem = new BMenuItem(menu);
	menuBar->AddItem( menuItem, EDIT_MENU_INDEX );

	// VIEW menu
	menu = new BMenu("View", B_ITEMS_IN_COLUMN);
	AddChangeToMenuItem(menu);
	add_menu_item(menu, "Saturation...", SHOW_SATURATION_WIN_MSG, 0);
	menuItem = new BMenuItem(menu);
	menuBar->AddItem( menuItem, VIEW_MENU_INDEX );

	// TOOLBARS menu
	menu = new BMenu("Tool bars", B_ITEMS_IN_COLUMN);
	mShowToolBarMenu = new BMenu("Show");
	if (menu) {
		menuItem = new BMenuItem(mShowToolBarMenu);
		if (menuItem) menu->AddItem(menuItem);
	}
	mDeleteToolBarMenu = new BMenu("Delete");
	if (menu) {
		menuItem = new BMenuItem(mDeleteToolBarMenu);
		if (menuItem) menu->AddItem(menuItem);
	}
	menuItem = new BMenuItem("New", new BMessage(SHOW_NEW_TOOL_BAR_MSG), 0);
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);
	menuItem = new BMenuItem(menu);
	menuBar->AddItem(menuItem, TOOL_BARS_MENU_INDEX);

	// WINDOWS menu
	menu = new BMenu("Windows", B_ITEMS_IN_COLUMN);
	mFilterItem = new BMenuItem("Filters", new BMessage(SHOW_FILTERS_MSG), 'F');
	mFilterItem->SetTarget(be_app);
	menu->AddItem(mFilterItem);

	menuItem = new BMenuItem("Tools", new BMessage(SHOW_MANAGE_TOOLS_MSG), 0);
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuItem = new BMenuItem(menu);
	menuBar->AddItem( menuItem, WINDOWS_MENU_INDEX );
	
	AddChild(menuBar);
	SetKeyMenuBar(menuBar);
}

class _AmTrackControlView : public BView
{
public:
	_AmTrackControlView(BRect frame, const char* name, uint32 resizeMask, uint32 flags)
			: BView(frame, name, resizeMask, flags)
	{
	}

	virtual void AttachedToWindow()
	{
		BView::AttachedToWindow();
		SetViewColor( Prefs().Color(AM_CONTROL_BG_C) );
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
		if (!gControlBg && clip.bottom >= (b.bottom - 1) ) {
			SetHighColor( tint_color(ViewColor(), B_DARKEN_2_TINT) );
			StrokeLine( BPoint(clip.left, b.bottom - 1), BPoint(clip.right, b.bottom - 1) );
		}
		if (clip.bottom >= b.bottom) {
			SetHighColor(0, 0, 0);
			StrokeLine( BPoint(clip.left, b.bottom), BPoint(clip.right, b.bottom) );
		}
	}
};

bool SeqTrackWindow::AddBackgroundView()
{
	BRect	r = Bounds();
	BView*	prev = ChildAt(0);
	if (prev) r.top = prev->Bounds().bottom + 1;
	r.bottom -= (Prefs().Size(H_SCROLLBAR_Y) + 2);
	mBgView = new _AmTrackControlView(	r,
										"BACKGROUND_VIEW",
										B_FOLLOW_ALL,
										B_WILL_DRAW | B_FRAME_EVENTS);
	if (!mBgView) return false;
	AddChild(mBgView);
	return true;
}

static BRect add_transport_button(	BRect frame, const char* name, uint32 what,
									const char* bmNormal, const char* bmPressed,
									BView* toView)
{
	const BBitmap*	bmN = ImageManager().FindBitmap(bmNormal);
	const BBitmap*	bmP = ImageManager().FindBitmap(bmPressed);
	if (!bmN || !bmP) return frame;
	BMessage*		msg = new BMessage( what );
	if (!msg) return frame;
	BRect		b = bmN->Bounds();
	frame.right = frame.left + b.Width();
	frame.bottom = frame.top + b.Height();
	BBitmapButton*	butt = new BBitmapButton(frame, name, 0, msg, bmN, bmN, bmP, bmP, bmP);
	if (butt) {
		/* Right now there's a bug in the buttons that causes them to resizes themslves
		 * based on label info, even if they don't have a label.  This compensates.
		 */
		butt->ResizeToPreferred();
		butt->SetValue(B_CONTROL_OFF);
		toView->AddChild(butt);
		frame.OffsetBy( BPoint(b.Width() + 1, 0) );
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

BPoint SeqTrackWindow::AddButtons(BPoint start)
{
	BRect			f(start.x, start.y + 3, start.x, start.y + 3);
	f = add_transport_button( f, "play", PLAY_SONG_MSG, PLAY_NORMAL_STR, PLAY_PRESSED_STR, mBgView );
	f = add_transport_button( f, "play_track", SS_PLAY_TRACK, PLAY_TRACK_NORMAL_STR, PLAY_TRACK_PRESSED_STR, mBgView );
	f = add_transport_button( f, "stop", STOP_SONG_MSG, STOP_NORMAL_STR, STOP_PRESSED_STR, mBgView );
	f = add_two_state_button( f, "loop", LOOP_MSG, LOOP_NORMAL_STR, LOOP_PRESSED_STR, mBgView );

	f.top = start.y;
	f.left += 8;
	mDurationCtrl = new AmDurationControl(BPoint(f.left, f.top), "quantize_ctrl", NULL, 0);
	if (mDurationCtrl) {
		mBgView->AddChild(mDurationCtrl);
		f.left = mDurationCtrl->Frame().right;
	}

	f.left += 8;
	mVelocityCtrl = new AmVelocityControl(	BPoint(f.left, f.top), "velocity_ctrl",
											B_FOLLOW_LEFT | B_FOLLOW_TOP, new BMessage(VELOCITY_FINISHED_MSG) );
	if (mVelocityCtrl) {
		mBgView->AddChild(mVelocityCtrl);
		f.left = mVelocityCtrl->Frame().right;
	}
	return BPoint(f.left + 1, f.bottom + 1);
}

BPoint SeqTrackWindow::AddActiveTools(BPoint start)
{
	_TwActiveToolView*	atv = new _TwActiveToolView(start);
	if (!atv) return start;
	atv->ResizeToPreferred();
	mBgView->AddChild(atv);
	BRect				f( atv->Frame() );
	return BPoint(f.right, f.bottom);
}

BPoint SeqTrackWindow::AddToolBars()
{
	ArpASSERT(mBgView);
	if (mToolBarLeftTop.x == -1 || mToolBarLeftTop.y == -1) return mToolBarLeftTop;

	BView*		v;
	while ( (v = mBgView->FindView("tool_bar_view")) != NULL) mBgView->RemoveChild(v);

	AmToolBarRef			toolBarRef;
	vector<AmToolBarRef>	refs;
	for (uint32 k = 0; (toolBarRef = AmGlobals().ToolBarAt(k)).IsValid(); k++) {
		// READ TOOL BAR BLOCK
		const AmToolBar*	toolBar = toolBarRef.ReadLock();
		if (toolBar && toolBar->IsShowing() ) refs.push_back(toolBarRef);
		toolBarRef.ReadUnlock(toolBar);
		// END READ TOOL BAR BLOCK
	}
	/* FIX: Sort the showing tool bars by name.
	 */
	BPoint		answer(mToolBarLeftTop);
	BPoint		cache(mToolBarLeftTop);
	for (uint32 k = 0; k < refs.size(); k++) {
		SeqToolBarView*		tbv = new SeqToolBarView(cache, "tool_bar_view", refs[k]);
		tbv->ResizeToPreferred();
		mBgView->AddChild(tbv);
		BRect				f = tbv->Frame();
		if (f.right > answer.x) answer.x = f.right;
		if (f.bottom > answer.y) answer.y = f.bottom;
		cache.x = f.right + 10;
	}
	return answer;
}

void SeqTrackWindow::AddDataViews(float topArg)
{
	BRect	bounds = Bounds();
	BRect	rect = Bounds();
	BView*	splitter;
	BView*	measure;
	bool	hasStrips = HasStripViews();

	float	measureTop = topArg + Prefs().Size( MAINMENU_Y );
	float	measureBottom = measureTop + 25;
	float	hsbLeft = 0,
			hsbRight = bounds.right - Prefs().Size(V_SCROLLBAR_X) - ARP_ZOOM_CONTROL_WIDTH - 1,
			hsbTop = bounds.bottom - Prefs().Size(H_SCROLLBAR_Y) - 1,
			hsbBottom = bounds.bottom;
	float	splitterH = 5;
	float	divider = 60;
	float	priH = 189;
	
	measure = AddMeasureView(	BRect(0, measureTop, bounds.right, measureBottom),
								divider, Prefs().Size(V_SCROLLBAR_X) );
	mBgView->ResizeTo( bounds.Width(), topArg - 1 );
	mBgView->SetResizingMode( B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
	
	rect.Set(0, measureBottom + 1, bounds.right, measureBottom + 1 + priH);
	if (!hasStrips) rect.bottom = bounds.bottom - Prefs().Size( H_SCROLLBAR_Y ) - 2;
	BView*			priPanel = NewPrimaryPanel( rect, divider );
	rect.top = rect.bottom + splitterH;
	rect.bottom = bounds.bottom - Prefs().Size( H_SCROLLBAR_Y ) - 2;
	/* This is because of that FUCKING scroll bar bug that sets the orientation
	 * based on the initial frame -- frame has to be something arbitrary, then
	 * I can't actually set the correct frame until the view is attached.
	 */
	BScrollBar*		secVsb = NULL;
	BRect			secVsbF;
	BView*			secPanel = NewSecondaryPanel(rect, divider, &secVsb, &secVsbF);
	if (!priPanel || !secPanel) {
		delete priPanel;
		delete secPanel;
		return;
	}
	AddChild(priPanel);

	if (hasStrips) {
		splitter = NewSplitter(priPanel->Frame(), secPanel->Frame(), SPLITTER_1_STR, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_HORIZONTAL);
		if (splitter) AddChild(splitter);
		AddChild(secPanel);
		if (secVsb) {
			secVsb->MoveTo(secVsbF.left, secVsbF.top);
			secVsb->ResizeTo(secVsbF.Width(), secVsbF.Height() );
		}
	} else {
		delete secPanel;
		secPanel = NULL;
		mSecondaryInfo = NULL;
		mSecondaryData = NULL;
	}
	/* Add the bottom horizontal scroll bar.
	 */
	mHsb = new ArpMultiScrollBar(	BRect(hsbLeft, hsbTop, hsbRight, hsbBottom),
									0, mPriDataView, 0, 0, B_HORIZONTAL);
	if (measure) mHsb->AddTarget(measure);
	if (hasStrips && mSecondaryData) mSecondaryData->SetHorizontalScrollBar(mHsb);
	AddChild(mHsb);
	SetupHsb();

	/* Add the zoom control.
	 */
	BRect				frame( hsbRight + 1, hsbTop, hsbRight + 1 +  ARP_ZOOM_CONTROL_WIDTH, hsbTop + ARP_ZOOM_CONTROL_HEIGHT);
	ArpRangeControl*	zoom = new ArpZoomControl(frame, "zoom");
	if (zoom) {
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
		zoom->AddVerticalBand(	 0.99,	0.87,	10 );
		zoom->AddVerticalBand(	 0.86,	0.86,	10 );
		zoom->AddVerticalBand(	 0.85,	0.72,	10 );
		zoom->AddVerticalBand(	 0.71,	0.71,	10 );
		zoom->AddVerticalBand(	 0.70,	0.57,	10 );

		zoom->SetUpdatedMessage( new BMessage('zoom') );
		zoom->StartWatching(this, 'zoom');
		zoom->SetZoomX(mMtc.BeatLength() / _BASE_BEAT_LENGTH);
		AddChild(zoom);
	}
}

SeqMeasureControl* SeqTrackWindow::AddMeasureView(BRect r, float left, float right)
{
	
	SeqMeasureControl	*v;
	v = new SeqMeasureControl(r, MEASURE_CONTROL_STR, SongRef(), this, mMtc, left, right);
	if (v) {
		v->SetMarkerEnabled(AM_POSITION_MARKER, false);
		v->SetMarkerVisible(AM_POSITION_MARKER, false);
		AddChild(v);
	}
	return(v);
}

BView* SeqTrackWindow::NewPrimaryPanel(BRect frame, float divider)
{
	BRect				normal( 0, 0, frame.Width(), frame.Height() );
	// Create my data view...
	BRect				r(normal);
	r.left = divider;
	r.right -= Prefs().Size( V_SCROLLBAR_X ) + 1;
	BView*				data = NewPrimaryDataView( r );
	// ...and my info view...
	r = normal;
	r.right = divider - 1;
	BView*				info = NewPrimaryInfoView( r );
	if( !data || !info ) {
		delete data;
		delete info;
		return 0;
	}
	// ...add a scrollbar...
	r = normal;
	r.left = r.right - Prefs().Size( V_SCROLLBAR_X );
	ArpMultiScrollBar*	sb = new ArpMultiScrollBar( r, "sb", data, 0, 0, B_VERTICAL );
	// ...and wrap it all in a view.
	BView*				view = new BView( normal, "pri panel", B_FOLLOW_ALL, B_WILL_DRAW );
	if( !sb || !view ) {
		delete data;
		delete info;
		delete sb;
		delete view;
		return 0;
	}
	// Setup the scrollbar
	sb->AddTarget( info );
	sb->SetResizingMode( B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM );
	arp_setup_vertical_scroll_bar(sb, data);
	// Add the views to the panel and we're done
	view->AddChild( data );
	view->AddChild( info );
	view->AddChild( sb );
	view->MoveTo( frame.left, frame.top );
	return view;
}

BView* SeqTrackWindow::NewSecondaryPanel(BRect frame, float divider, BScrollBar** outSb, BRect* outSbF)
{
	BRect				normal( 0, 0, frame.Width(), frame.Height() );
	// Create my data view...
	BRect				r(normal);
	r.left = divider;
	r.right -= Prefs().Size(V_SCROLLBAR_X) + 1;
	BView*				data = NewSecondaryData( r );
	// ...and my info view...
	r = normal;
	r.right = divider - 1;
	SeqTrackHrzSecondaryInfo* info = NewSecondaryInfo(r);
	if( !data || !info ) {
		delete data;
		delete info;
		return 0;
	}
	// ...add a scrollbar...
	r = normal;
	r.left = r.right - Prefs().Size(V_SCROLLBAR_X);
	ArpMultiScrollBar*	sb = new ArpMultiScrollBar(BRect(-100, -100, -90, 0), "sb", data, 0, 0, B_VERTICAL );
	*outSb = sb;
	*outSbF = r;
	// ...and wrap it all in a view.
	BView*				view = new BView( normal, "sec panel", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW );
	if (!sb || !view) {
		delete data;
		delete info;
		delete sb;
		delete view;
		return 0;
	}
	// Setup the scrollbar
	info->SetVerticalScrollBar( sb );
	sb->AddTarget( info );
	// Add the views to the panel and we're done
	view->AddChild( data );
	view->AddChild( info );
	view->AddChild( sb );
	view->MoveTo( frame.left, frame.top );
	return view;
}

BView* SeqTrackWindow::NewSplitter(	BRect leftTop, BRect rightBottom,
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
	SeqSplitterView*	sv = new SeqSplitterView( b, name, resizeMask, B_WILL_DRAW, direction );
	if (!sv) return NULL;
	sv->SetViewColor(Prefs().Color(AM_DATA_BACKDROP_C) );
	return sv;
}

BView* SeqTrackWindow::NewPrimaryDataView(BRect frame)
{
	if (mTrackRefs.size() < 1) return NULL;
	BView*				dv = NULL;
	AmViewFactory*		fact = NULL;
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::NewPrimaryDataView() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackRefs[0]) : NULL;
	if (track) {
		const AmViewPropertyI*	prop = track->Property(PRI_VIEW);
		if (prop && (fact = AmGlobals().FactoryNamed( prop->Signature() ) ) ) {
			dv = fact->NewDataView(SongRef(), *this, prop, PRI_VIEW);
			mPriName = prop->Name();
		}
	}
	ReadUnlock (song);
	// END READ TRACK BLOCK
	if (!dv) {
		/* The data view that the track claims to have doesn't actually exist.
		 * Now look for the preferred data view in my current factory, then the
		 * default factory.
		 */
		fact = AmGlobals().FactoryNamed( BString(mFactorySignature) );
		if (!fact) fact = AmGlobals().FactoryNamed( BString(DEFAULT_FACTORY_SIGNATURE) );
		if (!fact) return 0;
		AmViewProperty		fakeProp( DEFAULT_FACTORY_SIGNATURE, fact->PreferredDataName().String() );
		dv = fact->NewDataView(SongRef(), *this, &fakeProp, PRI_VIEW);
		mPriName = fakeProp.Name();
	}
	if (!dv) return NULL;

	dv->ResizeTo(frame.Width(), frame.Height() );
	dv->MoveTo(frame.left, frame.top);
	mPriDataView = dv;
	return mPriDataView;
}

BView* SeqTrackWindow::NewPrimaryInfoView(BRect frame)
{
	if (mTrackRefs.size() < 1) return NULL;
	BView*				iv = NULL;
	AmViewFactory*		fact = NULL;
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTrackWindow::NewPrimaryInfoView() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackRefs[0] ) : NULL;
	if (track) {
		const AmViewPropertyI*	prop = track->Property(PRI_VIEW);
		if (prop && (fact = AmGlobals().FactoryNamed( prop->Signature() ) ) ) {
			iv = fact->NewInfoView(SongRef(), *this, prop, PRI_VIEW);
		}
	}
	ReadUnlock(song);
	// END READ TRACK BLOCK
	if (!iv) {
		/* The data view that the track claims to have doesn't actually exist.
		 * Now look for the preferred data view in my current factory, then the
		 * default factory.
		 */
		fact = AmGlobals().FactoryNamed( BString(mFactorySignature) );
		if (!fact) fact = AmGlobals().FactoryNamed( BString(DEFAULT_FACTORY_SIGNATURE) );
		if (!fact) return 0;
		AmViewProperty		fakeProp(DEFAULT_FACTORY_SIGNATURE, fact->PreferredDataName().String() );
		iv = fact->NewInfoView(SongRef(), *this, &fakeProp, PRI_VIEW);
	}
	if (!iv) return NULL;

	iv->ResizeTo(frame.Width(), frame.Height() );
	iv->MoveTo(frame.left, frame.top);
	iv->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	mPriInfoView = iv;
	return mPriInfoView;
}

SeqTrackHrzSecondaryInfo* SeqTrackWindow::NewSecondaryInfo(BRect frame)
{
	mSecondaryInfo = new SeqTrackHrzSecondaryInfo(SongRef(), *this, frame, mFactorySignature, INITIAL_SEPARATION);
	return mSecondaryInfo;
}

SeqTrackHrzSecondaryData* SeqTrackWindow::NewSecondaryData(BRect frame)
{
	mSecondaryData = new SeqTrackHrzSecondaryData(SongRef(), *this, frame, INITIAL_SEPARATION);
	return mSecondaryData;
}

int32 SeqTrackWindow::ActiveToolSetThreadEntry(void* castToTrackWin)
{
	printf("Begin SeqTrackWindow::ActiveToolSetThreadEntry\n");	
	SeqTrackWindow*			tw = (SeqTrackWindow*)castToTrackWin;
	if (!tw) return B_ERROR;

	int32					display = 0; // 0 standard, 1 control
	while (true) {
		if (modifiers()&B_CONTROL_KEY) {
			if (display != 1) tw->PostMessage(ACTIVE_TOOL_SET_CHANGED_MSG);
			display = 1;
		} else {
			if (display != 0) tw->PostMessage(ACTIVE_TOOL_SET_CHANGED_MSG);
			display = 0;
		}
		snooze(20000);
	}

	printf("End SeqTrackWindow::ActiveToolSetThreadEntry\n");
	return B_OK;
}

void SeqTrackWindow::ActiveToolSetChanged()
{
	if (!mBgView) return;
	BView*		v = mBgView->FindView(ACTIVE_TOOL_VIEW_NAME);
	if (!v) return;
	v->Invalidate();
}
