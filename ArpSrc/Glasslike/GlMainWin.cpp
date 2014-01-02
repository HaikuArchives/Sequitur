#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <StorageKit.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ArpSplitterView.h>
#include <ArpInterface/ViewTools.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlActions.h>
#include <GlPublic/GlNodeData.h>
#include <GlPublic/GlRecorder.h>
#include <GlPublic/GlRootNode.h>
#include <GlKernel/GlDefs.h>
#include <GlKernel/GlRecorderHolder.h>
#include <Glasslike/GlApp.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlMainNavView.h>
#include <Glasslike/GlMainNodesView.h>
#include <Glasslike/GlMainWin.h>
#include <Glasslike/GlMainWinAux.h>
#include <Glasslike/GlMidiBinding.h>
#include <Glasslike/GlRootInfoInspector.h>
#include <Glasslike/GlResultView.h>
#include <Glasslike/SZ.h>

static const char*		WIN_NAME				= "Glasslike";
static const uint32		CUT_MSG					= 'iCut';
static const uint32		COPY_MSG				= 'iCpy';
static const uint32		PASTE_MSG				= 'iPst';
static const uint32		SAVE_MSG				= 'iSav';
static const uint32		SAVE_AS_MSG				= 'iSvA';
static const uint32		NEW_GRID_MSG			= 'iNeG';
static const uint32		ADD_GRID_MSG			= 'iAdG';
static const uint32		REQUEST_OPEN_ROOT_MSG	= 'iROM';
static const uint32		REQUEST_SAVE_ROOT_MSG	= 'iRSM';
static const uint32		OPEN_ROOT_MSG			= 'iOpM';
static const uint32		SAVE_ROOT_AS_MSG		= 'iSaM';
static const uint32		CLEAR_INPUT_MSG			= 'iClI';
static const uint32		REWIND_RESULT_MSG		= 'iRwd';
static const char*		RESULT_FN_STR			= "result fn";

static const uint32		REQUEST_SAVE_FILE_MSG		= 'iRSF';
static const uint32		SAVE_AS_FILE_MSG			= 'iSaF';
static const uint32		REQUEST_SAVE_JPG_FILE_MSG	= 'iRjF';
static const uint32		SAVE_AS_JPG_FILE_MSG		= 'iSjF';

static const uint32		LV_REQUEST_CHOOSE_FOLDER_MSG	= 'ilRf';
static const uint32		LV_CHOOSE_FOLDER_MSG			= 'ilCf';

static const uint32		PROJECT_SETTINGS_MSG			= 'iPrS';

static const int32		RESULT_CODE						= 0;
//static const int32		PREVIEW_CODE					= 1;

static float			g_ScrollBar_X		= 0;
static float			g_ScrollBar_Y		= 0;


static void	alert_err(status_t err)
{
ArpFINISH();
/*
	if (err == B_OK) return;
	BString			s("Error: ");
	s << strerror(err);
	(new BAlert("", s.String(), "OK"))->Go();
*/
}

/***************************************************************************
 * _PLAYER-VIEW
 * I have the start and stop buttons
 ***************************************************************************/
class _PlayerView : public BView
{
public:
	_PlayerView(BRect frame, BButton** play, BButton** stop, BButton** rewind,
				BStatusBar** statusBar);
	
	virtual	void		AttachedToWindow();
	
private:
	typedef BView		inherited;
};

/***************************************************************************
 * _STATUS-VIEW
 * I display a simple text message about what's going on
 ***************************************************************************/
class _StatusView : public BView
{
public:
	_StatusView(BRect frame);
	
	virtual	void		AttachedToWindow();
	virtual void		Draw(BRect clip);
	
	void				SetText(const char* text);
	
private:
	typedef BView		inherited;
	float				mBottom;
	BString16			mString;
	
	void				DrawOn(BRect clip, BView* v);
};

/*************************************************************************
 * GL-MIXING-WIN
 *************************************************************************/
GlMainWin::GlMainWin(BRect frame, const BMessage& config)
		: inherited(frame, WIN_NAME, B_DOCUMENT_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS),
		  mChannel(this), mPreview(0), mPathView(0), mChain(0), mNodes(0),
		  mMidiView(0), mInspector(0), mInspectTarget(0),
		  mPlay(0), mStop(0), mRewind(0), mStatusBar(0),
		  mResultView(0), mStatusView(0),
		  mPerformer(mResult), mRecording(false),
		  mInspectorCid(0), mInspectorNid(0), mInspectorVt(0), mInspectorViewMsg(0),
		  mOpenPanel(0), mSavePanel(0)
{
	GlRootRef		ref;
	ref.SetTo(gl_new_root_node(GL_IMAGE_IO));
	mPath.SetRef(ref);
	
	g_ScrollBar_X = B_V_SCROLL_BAR_WIDTH;
	g_ScrollBar_Y = B_H_SCROLL_BAR_HEIGHT;

	float			fontSize = 10;
	const BFont*	font = be_plain_font;
	if (font) font->Size();
	BRect			b = Bounds();
	BRect			f(b);
	float			menuH = fontSize + 7;
	f.bottom = menuH;
	AddMainMenu(f);

	float			padX = float(Prefs().GetInt32(ARP_PADX)), padY = float(Prefs().GetInt32(ARP_PADY));
	float			nodeY = float(Prefs().GetInt32(GL_NODE_IMAGE_Y));

	float			nodesL = padX, nodesT = padY;
	float			nodesR = frame.Width() - padX, nodesB = nodesT + 40;

	float			pathT = nodesB + padY;
	float			pathB = pathT + fontSize + 4;

	float			chainT = pathB + padY;
	float			chainB = chainT + nodeY + padY + nodeY + padY;

	float			previewL = padX, previewT = pathT;
	float			previewR = previewL + (chainB - pathT), previewB = chainB;

	float			pathL = previewR + padX, pathR = nodesR;

	float			chainL = pathL, chainR = pathR;

	float			midiL = padX, midiT = chainB + padY;
	float			midiR = nodesR, midiB = midiT + 9;

	float			inspectL = midiL, inspectT = midiB + 1;
	float			inspectR = midiR, inspectB = frame.Height() * 0.4f;
	if (inspectB < inspectT) inspectB = inspectT;

#if 0
	float			inspectL = padX, inspectT = chainB + padY;
	float			inspectR = nodesR, inspectB = frame.Height() * 0.4f;
	if (inspectB < inspectT) inspectB = inspectT;
#endif
	
	float			splitT = inspectB + 1, splitB = inspectB + 5;

	float			playerL = padX, playerT = padY;
	float			playerR = frame.Width() - padX, playerB = playerT + Prefs().GetInt32(ARP_BUTTON_Y);
	
	mResult.SetPreviewSize(int32(previewR - previewL), int32(previewB - previewT));

	BView*			topPane = new BView(BRect(0, menuH + 1, frame.Width() - 1, inspectB),
										"top", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);
	if (!topPane) return;
	AddChild(topPane);

	mNodes = new GlMainNodesView(BRect(nodesL, nodesT, nodesR, nodesB), mPath);
	if (mNodes) {
		mNodes->SetViewColor(200, 180, 160);
		topPane->AddChild(mNodes);
	}

	mPreview = new GlMainPreview(	BRect(previewL, previewT, previewR, previewB),
									mPath, mResult);
	if (mPreview) {
//		mPreview->SetViewColor(200, 180, 160);
		topPane->AddChild(mPreview);
	}

	mPathView = new GlMainPathView(BRect(pathL, pathT, pathR, pathB), mPath);
	if (mPathView) {
		mPathView->SetViewColor(200, 180, 160);
		topPane->AddChild(mPathView);
	}

	mChain = new GlMainNavView(BRect(chainL, chainT, chainR, chainB), mPath);
	if (mChain) {
		topPane->AddChild(mChain);
	}

	mMidiView = new GlMainMidiView(BRect(midiL, midiT, midiR, midiB));
	if (mMidiView) {
		mMidiView->SetViewColor(Prefs().GetColor(ARP_BG_C));
		topPane->AddChild(mMidiView);
	}

	mInspector = new BView(	BRect(inspectL, inspectT, inspectR, inspectB), "inspect",
							B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);
	if (mInspector) {
		mInspector->SetViewColor(200, 180, 160);
		topPane->AddChild(mInspector);
	}

	ArpSplitterView*	split = new ArpSplitterView(BRect(0, splitT, frame.Width() - 1, splitB), "hsplit", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW, B_HORIZONTAL);
	if (split) AddChild(split);

	BView*				bottomPane = new BView(	BRect(0, splitB + 1, frame.Width() - 1, frame.Height() - g_ScrollBar_Y),
												"bottom", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!bottomPane) return;
	AddChild(bottomPane);
	bottomPane->SetViewColor(Prefs().GetColor(ARP_BG_C));
	BView*				v = new _PlayerView(BRect(playerL, playerT, playerR, playerB),
											&mPlay, &mStop, &mRewind, &mStatusBar);
	if (v) bottomPane->AddChild(v);

	b = bottomPane->Bounds();
	mResultView = new GlResultView(BRect(playerL, playerB + padY, playerR, b.Height() - 1), mResult);
	if (mResultView) bottomPane->AddChild(mResultView);

	mStatusView = new _StatusView(BRect(0, frame.Height() - g_ScrollBar_Y + 1, frame.Width() - g_ScrollBar_X, frame.Height()));
	if (mStatusView) AddChild(mStatusView);

	if (mStop) mStop->SetEnabled(false);

	if (mStatusBar) {
		mStatusBar->SetMaxValue(1);
		mPerformer.InitStatus(BMessenger(this), RESULT_CODE, mStatusBar->Frame().Width());
	}

	ReadSettings(config);
	mChannel.SetPath(&mPath);
	RootChanged();
}

GlMainWin::~GlMainWin()
{
	gl_app->UnsetMidiTarget(BMessenger(this));

	delete mOpenPanel;
	delete mSavePanel;
}

void GlMainWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_KEY_DOWN: {
			if (CurrentFocus() == 0) {
				BView*		v = LastMouseMovedView();
				if (v) PostMessage(msg, v);
			}
		} break;
#if 0
//		case B_KEY_DOWN: {
		case B_KEY_UP: {
			int32	key = -1;
			if (msg->FindInt32("key", &key) == B_OK) {
				// DEL on page contros, . / DEL on a numeric keypad
				if (key == 0x34 || key == 0x65) {
					if (mProjectView) mProjectView->Delete();
				}
			}
		} break;
#endif
		case 'DATA':
			if (msg->HasRef("refs")) Open(msg);
			break;
		case GL_PREVIEW_IMAGE_DROPPED: {
			NewImageInput(mResult.FreezeImage(msg));
			if (mPreview) mPreview->Process();
		} break;
		case GL_MIDI_EVENT_MSG: {
			GlMidiEvent		event;
			if (event.ReadFrom(*msg) == B_OK) mChannel.MidiEvent(event, 0);
		} break;
		case GL_DISABLE_PLAY_CONTROLS_MSG: {
			int32		code = -1;
			msg->FindInt32("c", &code);
			if (code == RESULT_CODE) {
				EnablePlayControls(false);
				if (mStatusBar) {
					mStatusBar->Reset();
					mStatusBar->SetMaxValue(1);
				}
			}
		} break;
		case GL_ENABLE_PLAY_CONTROLS_MSG: {
			int32		code = -1;
			msg->FindInt32("c", &code);
			if (code == RESULT_CODE) {
				EnablePlayControls(true);
				if (mStatusBar) mStatusBar->Reset();
			}
		} break;
		case GL_ROOT_CHANGED_MSG: {
			RootChanged();
		} break;
		case GL_PATH_CHANGED_MSG: {
			PathChanged();
		} break;
		case GL_INSPECT_NODE_MSG: {
			gl_chain_id			cid;
			gl_node_id			nid;
			int32				vt;
			if (msg->FindPointer(GL_CHAIN_ID_STR, &cid) != B_OK) cid = 0;
			if (msg->FindPointer(GL_NODE_ID_STR, &nid) != B_OK) nid = 0;
			if (msg->FindInt32(GL_NODE_VIEW_STR, &vt) != B_OK) vt = GL_INSPECTOR_VIEW;
			InspectNode(cid, nid, vt);
		} break;
		case GL_INSPECT_ROOT_INFO_MSG: {
			InspectNode(0, 0, 0, GL_INSPECT_ROOT_INFO_MSG);
		} break;

		case GL_PLAY_MSG: {
			GlRootRef		ref = mPath.Current();
			if (ref.IsValid()) mPerformer.StartRoot(&ref);
		} break;
		case GL_STOP_MSG:
			mPerformer.Stop();
			break;
		case GL_RECORD_MSG:
			Record();
			break;
#if 0
		case REWIND_RESULT_MSG:
			mResult.Rewind();
			break;
		case NEW_GRID_MSG:
			mPath.Unset();
			InspectNode(0, 0, 0);
			if (mProjectView) mProjectView->New();
			break;
#endif

		case ADD_GRID_MSG: {
			AddRoot();
		} break;
		case REQUEST_OPEN_ROOT_MSG:
			if (!mOpenPanel) mOpenPanel = new BFilePanel(B_OPEN_PANEL, 0, 0, 0, false);
			if (mOpenPanel->IsShowing()) break;
//			if (mOpenPanel->Window())
//				mOpenPanel->Window()->SetTitle("Select File" B_UTF8_ELLIPSIS);
			mOpenPanel->SetTarget(BMessenger(this));
			mOpenPanel->SetMessage(new BMessage(OPEN_ROOT_MSG) );
			mOpenPanel->Show();
			break;
		case OPEN_ROOT_MSG:
			Open(msg);
			break;
		case REQUEST_SAVE_ROOT_MSG:
			if (!mSavePanel) mSavePanel = new BFilePanel(B_SAVE_PANEL, 0, 0, 0, false);
			if (mSavePanel->IsShowing()) break;
//			if (mSavePanel->Window())
//				mSavePanel->Window()->SetTitle("Select File" B_UTF8_ELLIPSIS);
			mSavePanel->SetTarget(BMessenger(this));
			mSavePanel->SetMessage(new BMessage(SAVE_ROOT_AS_MSG) );
			mSavePanel->Show();
			break;
		case SAVE_ROOT_AS_MSG: {
			entry_ref			ref;
			const char*			name;
			if (msg->FindRef("directory", &ref) == B_OK
					&& msg->FindString("name", &name) == B_OK) {
				BDirectory		dir(&ref);
				if (dir.InitCheck() == B_OK) {
					BEntry		entry(&dir, name);
					if (entry.InitCheck() == B_OK) alert_err(Save(entry, name));
				}
			}
		} break;

		case REQUEST_SAVE_FILE_MSG:
			if (!mSavePanel) mSavePanel = new BFilePanel(B_SAVE_PANEL, 0, 0, 0, false);
			if (mSavePanel->IsShowing()) break;
//			if (mSavePanel->Window())
//				mSavePanel->Window()->SetTitle("Select File" B_UTF8_ELLIPSIS);
			mSavePanel->SetTarget(BMessenger(this));
			mSavePanel->SetMessage(new BMessage(SAVE_AS_FILE_MSG) );
			mSavePanel->Show();
			break;
		case SAVE_AS_FILE_MSG:
			SaveFile(msg, ARP_PNG_FORMAT);
			break;
		case REQUEST_SAVE_JPG_FILE_MSG:
			if (!mSavePanel) mSavePanel = new BFilePanel(B_SAVE_PANEL, 0, 0, 0, false);
			if (mSavePanel->IsShowing()) break;
//			if (mSavePanel->Window())
//				mSavePanel->Window()->SetTitle("Select File" B_UTF8_ELLIPSIS);
			mSavePanel->SetTarget(BMessenger(this));
			mSavePanel->SetMessage(new BMessage(SAVE_AS_JPG_FILE_MSG) );
			mSavePanel->Show();
			break;
		case SAVE_AS_JPG_FILE_MSG:
			SaveFile(msg, ARP_JPG_FORMAT);
			break;

#if 0
		case CLEAR_INPUT_MSG:
			if (mProjectView) mProjectView->ClearInputHack();
			break;
#endif
		case GL_SET_STATUS_MSG: {
			if (mStatusView) {
				const char*		text;
				if (msg->FindString(GL_TEXT_STR, &text) != B_OK) text = 0;
				mStatusView->SetText(text);
			}
		} break;
		case GL_STATUS_MSG: {
			int32		code = -1;
			msg->FindInt32("c", &code);
			if (code == RESULT_CODE && mStatusBar) {
				float		f;
				if (msg->FindFloat("f", &f) == B_OK) {
					f = ARP_MAX(0, f - mStatusBar->CurrentValue());
					mStatusBar->Update(f);
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void GlMainWin::Quit()
{
	GlApp*	app = dynamic_cast<GlApp*>(be_app);
	if (app && !(app->IsQuitting()) ) {
		WriteSettings();
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	inherited::Quit();
}

#if 0
bool GlMainWin::QuitRequested()
{
	GlApp*	app = dynamic_cast<GlApp*>(be_app);
	if (app && !(app->IsQuitting()) ) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return false;
	}

	return true;
}
#endif

void GlMainWin::WindowActivated(bool state)
{
	if (state) gl_app->SetMidiTarget(BMessenger(this));
	else gl_app->UnsetMidiTarget(BMessenger(this));
}

status_t GlMainWin::SaveFile(const BMessage* msg, int32 format)
{
	if (!mResultView) return B_ERROR;
	entry_ref			ref;
	const char*			name;
	status_t			err;
	if ((err = msg->FindRef("directory", &ref)) != B_OK) return err;
	if ((err = msg->FindString("name", &name)) != B_OK) return err;

	BDirectory		dir(&ref);
	if ((err = dir.InitCheck()) != B_OK) return err;
	BPath		path(&dir, name);
	if ((err = path.InitCheck()) != B_OK) return err;
	BString16	filename(path.Path());
	alert_err(mResultView->SaveImage(filename, format));
	return B_OK;
}

status_t GlMainWin::NewImageInput(ArpBitmap* bm)
{
	if (mPreview) mPreview->TakeBitmap(mResult.NewPreviewBitmap());
	if (mResultView) mResultView->TakeBitmap(bm);
	else delete bm;

	const GlRootNode*	root = mPath.Current().ReadLock();
	if (root) {
		const GlChain*	chain = root->ChainAt(0);
		if (mChain) mChain->SetContext(chain);
		if (mNodes) mNodes->SetContext(chain);
	}
	mPath.Current().ReadUnlock(root);

	return B_OK;
}

status_t GlMainWin::AddRoot()
{
	GlRootNode*		root = gl_new_root_node(GL_IMAGE_IO);
	if (!root) return B_ERROR;
	GlRootRef		ref(root);
	status_t		err = mPath.AddRef(GlRootRef(root));
	if (err != B_OK) return err;
	mPath.SetCurrent(ref.NodeId());
	PostMessage(GL_ROOT_CHANGED_MSG);
	return B_OK;
}

status_t GlMainWin::RootChanged()
{
	if (mPathView) PostMessage(GL_ROOT_CHANGED_MSG, mPathView);
	if (mPreview) PostMessage(GL_ROOT_CHANGED_MSG, mPreview);

	status_t				err = B_ERROR;
	const GlRootNode*		root = mPath.Current().ReadLock();
	if (root) {
		const GlChain*		chain = root->ChainAt(0);
		if (chain) {
			if (mChain) mChain->SetContext(chain);
			if (mNodes) mNodes->SetContext(chain);
			err = B_OK;
		}
	}
	mPath.Current().ReadUnlock(root);
	if (err != B_OK) {
		if (mChain) mChain->SetContext(0);
		if (mNodes) mNodes->SetContext(0);
	}
	InspectNode(0, 0, 0);

	return err;
}

status_t GlMainWin::PathChanged()
{
	if (mPathView) PostMessage(GL_RECACHE_MSG, mPathView);
	status_t				err = B_ERROR;
	const GlRootNode*		root = mPath.Current().ReadLock();
	if (root) {
		const GlChain*		chain = mPath.Tail(root);
//		if (!chain) chain = root->ChainAt(0);
		if (chain) {
			if (mChain) mChain->SetContext(chain);
			if (mNodes) mNodes->SetContext(chain);
			err = B_OK;
		}
	}
	mPath.Current().ReadUnlock(root);
	if (err != B_OK) {
		if (mChain) mChain->SetContext(0);
		if (mNodes) mNodes->SetContext(0);
	}
	InspectNode(0, 0, 0);

	return err;
}

/* This is pretty hacked up because of the 'viewMsg' thing which supercededs
 * some of the other logic.  Bleh.  Clean this up.
 */
status_t GlMainWin::InspectNode(gl_chain_id cid, gl_node_id nid, int32 viewType,
								int32 viewMsg)
{
	if (mInspectorCid == cid && mInspectorNid == nid
			&& mInspectorVt == viewType && mInspectorViewMsg == viewMsg)
		return B_OK;
//printf("Inspect\n");
	mInspectorCid = cid;
	mInspectorNid = nid;
	mInspectorVt = viewType;
	mInspectorViewMsg = viewMsg;
	
	mChannel.DeleteTargets();
	mRecording = false;
	if (!mInspector) {
		if (mMidiView) mMidiView->SetState(0, viewType);
		return B_ERROR;
	}
	
	if (viewMsg == 0 && (!cid || !nid)) {
		if (mInspectTarget) {
			mInspectTarget->RemoveSelf();
			delete mInspectTarget;
			mInspectTarget = 0;
		}
		if (mMidiView) mMidiView->SetState(0, viewType);
		return B_OK;
	}

	BView*				v = 0;
	BRect				f(mInspector->Bounds());
	gl_new_view_params	params;
	params.ref = mPath.Current();
	params.viewType = viewType;
	params.frame = f;
	params.channel = &mChannel;
	const GlRootNode*	root = params.ref.ReadLock();
	if (root) {
		if (viewMsg == GL_INSPECT_ROOT_INFO_MSG) v = new GlRootInfoInspector(f, root);
		else {
			const GlNode*	node = root->FindNode(cid, nid);
			if (node) {
				v = node->NewView(params);
			}
		}
	}
	params.ref.ReadUnlock(root);

	if (!v) {
		if (mMidiView) mMidiView->SetState(0, viewType);
		return B_NO_MEMORY;
	}
	/* FIX: Backwards compatibility hack.  In the new model, the
	 * new views are given correct bounds, but right now none of
	 * the code (other than config views) handles that.
	 */
	if (viewMsg == 0 && viewType != GL_CONFIG_VIEW) {
		float				w, h;
		v->GetPreferredSize(&w, &h);
		if (f.Width() != w || f.Height() != h) v->ResizeTo(w, h);
	}
	if (mInspectTarget) {
		mInspectTarget->RemoveSelf();
		delete mInspectTarget;
	}
	mInspectTarget = v;
	mInspector->AddChild(v);

	if (mMidiView) mMidiView->SetState(&mChannel, viewType);
	return B_OK;
}

status_t GlMainWin::Record()
{
	status_t		err = B_OK;
	if (mRecording) mChannel.StopRecording();
	else err = mChannel.StartRecording();

	if (err != B_OK) return err;
	mRecording = !mRecording;
	return B_OK;
}

status_t GlMainWin::Open(BMessage* msg)
{
	ArpVALIDATE(msg, return B_ERROR);

	const GlNodeAddOn*	addon = GlGlobals().GetAddOn(GL_ROOT_KEY);
	if (!addon) return B_ERROR;

	mPath.MakeEmpty();

	entry_ref				ref;
	BEntry 					entry;
	BPath					path;
	BString16				name;
	gl_id					current = 0;

	if ( (msg->FindRef("refs", &ref) == B_OK)
			&& (entry.SetTo(&ref) == B_OK)
			&& entry.IsFile()
			&& entry.GetPath(&path) == B_OK
			&& path.GetParent(&path) == B_OK
			&& entry.GetName(name) == B_OK) {
		status_t			err;
		BMessage			projectMsg;
		BFile				file(&entry, B_READ_ONLY);
		if ((err = file.InitCheck()) != B_OK) return err;
		if ((err = projectMsg.Unflatten(&file)) != B_OK) return err;
		/* The file format has a series of BMessages storing matrices.
		 */
//printf("Path: %s\n", path.Path());
		int32				msgIndex = 0;
		BMessage			rootMsg;
		while (projectMsg.FindMessage(GL_ROOT_STR, msgIndex, &rootMsg) == B_OK) {
			if (rootMsg.what == GL_ROOT_MSG) {
				GlRootNode*	root = (GlRootNode*)(addon->NewInstance(&rootMsg));
				if (!root) return B_ERROR;
				root->IncRefs();
				mPath.AddRef(GlRootRef(root));
				if (!current) current = root->Id();
				root->DecRefs();
			}
			rootMsg.MakeEmpty();
			msgIndex++;
		}
	}
	mPath.SetCurrent(current);
	RootChanged();

#if 0
	if (first >= 0 && first < int32(mData.mEntries.size()) && mData.mEntries[first]) {
		mPath.SetRef(mData.mEntries[first]->ref);
		if (Window()) Window()->PostMessage(GL_INSPECT_ROOT_MSG);
	}
#endif
	return B_OK;
}

static void _assign_root_info(	GlRootNode* root, uint32 rootCount,
								const char* name)
{
	/* If I'm saving a single ref, and the root doesn't
	 * have a label, give it the file name as the label.
	 */
	if (rootCount == 1 && name && root->HasLabel() == false)
		root->SetLabel(name);

	GlKeyTracker&		tracker = gl_app->KeyTracker();
	BString16			creator;
	if (tracker.GetCurrent(creator, 0) != B_OK) return;
	
	if (root->HasCreator() == false)
		root->SetCreator(creator);

	if (root->HasKey() == false) {
		int32			key;
		if (tracker.IncKey(&key) == B_OK)
			root->SetKey(key);
	}

	if (root->HasKey() && root->HasLabel() == false) {
		BString16		lbl;
		lbl << root->Key();
		root->SetLabel(lbl);
	}
}

status_t GlMainWin::Save(const BEntry& entry, const char* name)
{
	ArpASSERT(name);
	uint32						projectSize = mPath.Size();
	BMessage					fileMsg;
	status_t					err = B_ERROR;
	for (uint32 k = 0; k < projectSize; k++) {
		GlRootRef				ref = mPath.At(k);
		if (ref.IsValid()) {
			BMessage			msg;
			GlRootNode*			root = ref.WriteLock();
			if (!root) return B_ERROR;

			_assign_root_info(root, projectSize, name);

			err = root->WriteTo(msg);
			ref.WriteUnlock(root);
	
			msg.what = GL_ROOT_MSG;
			err = fileMsg.AddMessage(GL_ROOT_STR, &msg);
			if (err != B_OK) return err;
		}
	}
	if (err != B_OK) return err;
		
	BFile				file(&entry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ((err = file.InitCheck()) != B_OK) return err;
	return fileMsg.Flatten(&file);
}

status_t GlMainWin::ReadSettings(const BMessage& config)
{
	BString16			fn;
	if (config.FindString16(RESULT_FN_STR, &fn) == B_OK)
		NewImageInput(mResult.FreezeImage(fn));
	return B_OK;
}

status_t GlMainWin::WriteSettings()
{
	GlApp*				app = dynamic_cast<GlApp*>(be_app);
	if (!app) return B_ERROR;

	BMessage			msg;
	BString16			str;
	if (mResult.GetFileName(str) == B_OK)
		msg.AddString16(RESULT_FN_STR, str);

	app->SetSettings(app->MIXING_WIN_INDEX, msg);

	return B_OK;
}

void GlMainWin::AddMainMenu(BRect frame)
{
	BMenuBar*	menuBar;
	BMenu*		menu;
	BMenuItem*	item;

	menuBar = new BMenuBar(	frame, NULL, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_ITEMS_IN_ROW, FALSE);
	if (!menuBar) return;

	/* File Menu
	 */
	menu = new BMenu(SZ(SZ_File), B_ITEMS_IN_COLUMN);
	add_menu_item(menu, SZ(SZ_New), NEW_GRID_MSG, 'N');
//	add_menu_item(menu, "Open" B_UTF8_ELLIPSIS, REQUEST_OPEN_ROOT_MSG, 'O');
//	add_menu_item(menu, "Save As" B_UTF8_ELLIPSIS, REQUEST_SAVE_ROOT_MSG, 'A');
	add_menu_item(menu, SZ(SZ_OpenEll), REQUEST_OPEN_ROOT_MSG, 'O');
	add_menu_item(menu, SZ(SZ_Save_AsEll), REQUEST_SAVE_ROOT_MSG, 'A');
	menu->AddSeparatorItem();	// ----
//	item = new BMenuItem("Preferences" B_UTF8_ELLIPSIS, new BMessage(SHOW_PREFERENCES_MSG), 'P');
	item = new BMenuItem(SZ(SZ_PreferencesEll), new BMessage(SHOW_PREFERENCES_MSG), 'P');
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();	// ----
	item = new BMenuItem(SZ(SZ_Quit), new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 0);

	/* Edit Menu
	 */
	menu = new BMenu(SZ(SZ_Edit), B_ITEMS_IN_COLUMN);
	add_menu_item(menu, SZ(SZ_Add_Grid), ADD_GRID_MSG, 'A');
	menu->AddSeparatorItem();
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 1);

	/* TEMP Menu
	 */
	menu = new BMenu(SZ(SZ_Temp), B_ITEMS_IN_COLUMN);
//	add_menu_item(menu, "Run", PERFORM_MSG, 'R');
//	add_menu_item(menu, "Save Preview (PNG) As" B_UTF8_ELLIPSIS, REQUEST_SAVE_FILE_MSG, 0);
	add_menu_item(menu, SZ(SZ_Save_Preview_PNG), REQUEST_SAVE_FILE_MSG, 0);
	add_menu_item(menu, SZ(SZ_Clear_Input), CLEAR_INPUT_MSG, 0);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, 2);


	AddChild(menuBar);
	SetKeyMenuBar(menuBar);
}

void GlMainWin::EnablePlayControls(bool enable)
{
	if (enable) {
		if (mPlay && mPlay->IsEnabled() == false) mPlay->SetEnabled(true);
		if (mStop && mStop->IsEnabled()) mStop->SetEnabled(false);
		if (mRewind && mRewind->IsEnabled() == false) mRewind->SetEnabled(true);
	} else {
		if (mPlay && mPlay->IsEnabled()) mPlay->SetEnabled(false);
		if (mStop && mStop->IsEnabled() == false) mStop->SetEnabled(true);
		if (mRewind && mRewind->IsEnabled()) mRewind->SetEnabled(false);
	}
}

// #pragma mark -

/*************************************************************************
 * PLAYER-VIEW
 *************************************************************************/
//static const char*			PLAY_STR = "Play";
//static const char*			STOP_STR = "Stop";
//static const char*			REWIND_STR = "Rewind";

_PlayerView::_PlayerView(	BRect frame, BButton** play, BButton** stop,
							BButton** rewind, BStatusBar** statusBar)
		: inherited(frame, "player", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS)
{
	ArpASSERT(play && stop && rewind && statusBar);
	float			playW = this->StringWidth(SZ(SZ_Play)),
					stopW = this->StringWidth(SZ(SZ_Stop)),
					rewindW = this->StringWidth(SZ(SZ_Rewind));
	float			w = ARP_MAX(playW, ARP_MAX(stopW, rewindW)) + 10;
	float			padX = float(Prefs().GetInt32(ARP_PADX));
	BRect			r(padX, 0, 0, 0);
	r.bottom = frame.Height();
	r.right = padX + w;

	// FIX:  Tmp, the buttons should have images, although I might
	// want help text, too, right?
//	BString16		playStr(PLAY_STR), stopStr(STOP_STR), rewindStr(REWIND_STR);

	*play = new BButton(r, NULL, SZ(SZ_Play), new BMessage(GL_PLAY_MSG));
	if (*play) AddChild(*play);

	r.left = r.right + padX;
	r.right = r.left + w;
	*stop = new BButton(r, NULL, SZ(SZ_Stop), new BMessage(GL_STOP_MSG));
	if (*stop) AddChild(*stop);

	r.left = r.right + padX;
	r.right = r.left + w;
	*rewind = new BButton(r, NULL, SZ(SZ_Rewind), new BMessage(REWIND_RESULT_MSG));
	if (*rewind) AddChild(*rewind);

	r.left = r.right + padX;
	r.right = frame.Width() - padX;
	*statusBar = new BStatusBar(r, "sb");
	if (*statusBar) AddChild(*statusBar);
}

void _PlayerView::AttachedToWindow()
{
	inherited::AttachedToWindow();
//	SetViewColor(B_TRANSPARENT_COLOR);
	SetViewColor(Prefs().GetColor(ARP_BG_C));
}

// #pragma mark -

/*************************************************************************
 * _STATUS-VIEW
 *************************************************************************/
_StatusView::_StatusView(BRect frame)
		: inherited(frame, "status", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	mBottom = Bounds().bottom - 2;
}

void _StatusView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void _StatusView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void _StatusView::SetText(const char* text)
{
	if (!text) mString = "";
	else mString = text;
	this->Invalidate();
}

void _StatusView::DrawOn(BRect clip, BView* v)
{
	v->SetLowColor(Prefs().GetColor(ARP_BG_C));
	v->SetHighColor(Prefs().GetColor(ARP_BG_C));
	v->FillRect(clip);

	if (mString.Length() > 0) {
		v->SetHighColor(0, 0, 0);
		v->DrawString(mString.String(), BPoint(2, mBottom));
	}
}
