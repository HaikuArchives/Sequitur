#include <be/StorageKit.h>
#include <be/support/Autolock.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpPopUpMenu.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ViewTools.h>
#include <GlPublic/GlChain.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlRootNode.h>
#include <GlPublic/GlStrainedParamList.h>
#include <GlPublic/GlUtils.h>
#include <GlKernel/GlDefs.h>
#include <Glasslike/GlApp.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlMainWinAux.h>
#include <Glasslike/GlMainWinLists.h>
#include <Glasslike/GlResultCache.h>
#include <Glasslike/SZ.h>

static const int32	ROOT_CODE			= 1;
static const int32	_NODE_CODE			= 2;
static const int32	ROOT_INFO_ID		= 0;
static const int32	MANAGE_CTRL_ID		= 1;

static const uint32 CHANGE_ROOT_MSG	= 'iChR';

/***************************************************************************
 * GL-PATH-CONTROL-LAYER
 ***************************************************************************/
GlPathControlLayer::GlPathControlLayer()
		: index(-1), mRootPropsExp(0), mRootPropsOvr(0),
		  mManageExp(0), mManageOvr(0)
{
	float				x = float(Prefs().GetInt32(GL_NODE_CONTROL_X)),
						y = float(Prefs().GetInt32(GL_NODE_CONTROL_Y));
	mRootPropsExp = new ArpBitmap(x, y);
	mRootPropsOvr = new ArpBitmap(x, y);
	mManageExp = new ArpBitmap(x, y);
	mManageOvr = new ArpBitmap(x, y);
	
	AddControl(ROOT_INFO_ID, mRootPropsExp, mRootPropsOvr);
	AddControl(MANAGE_CTRL_ID, mManageExp, mManageOvr);
}

GlPathControlLayer::~GlPathControlLayer()
{
	delete mRootPropsExp;
	delete mRootPropsOvr;
	delete mManageExp;
	delete mManageOvr;
}

void GlPathControlLayer::Arrange(BRect frame, int32 inIndex)
{
	index = inIndex;
	float			l = frame.left, t = frame.top;
//					r = frame.right, b = frame.bottom;
	
	SetControlProps(ROOT_INFO_ID,		BPoint(l + 8, t - 6), true);
	SetControlProps(MANAGE_CTRL_ID,		BPoint(l - 8, t - 6), true);
}

// #pragma mark -

/***************************************************************************
 * GL-MIDI-STATE
 ***************************************************************************/
GlMidiState::GlMidiState()
{
	Clear();
}

void GlMidiState::Clear()
{
	mRecordable = false;
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) mMidi[k] = false;
}
	
void GlMidiState::SetRecordable()
{
	mRecordable = true;
}

void GlMidiState::SetMidi(int32 letter)
{
	ArpVALIDATE(letter >= 0 && letter < GL_MIDI_SIZE, return);
	mMidi[letter] = true;
}

// #pragma mark -

/********************************************************
 * GL-MAIN-PATH-VIEW
 ********************************************************/
GlMainPathView::GlMainPathView(BRect frame, GlProjectList& path)
		: inherited(frame, "path", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mPath(path), mNodes(new GlMainLabels()),
		  mMouseDownIndex(-1)
{
	Recache();
}

GlMainPathView::~GlMainPathView()
{
	delete mNodes;
}

void GlMainPathView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case GlNotifier::CHANGE_MSG: {
			BMessage			info;
			if (gl_get_notifier_msg(*msg, GlNotifier::INFO_CODE, info) == B_OK) {
				const char*	s;
				if (info.FindString(GL_NODE_LABEL_STR, &s) == B_OK)
					Recache();
			}
		} break;
		case GL_ROOT_CHANGED_MSG:
			RootChanged();
			break;
		case CHANGE_ROOT_MSG: {
			int32		index;
			if (msg->FindInt32(GL_I_STR, &index) == B_OK)
				ChangeRoot(index);
		} break;
		case GL_INVALIDATE_MSG:
			this->Invalidate();
			break;
		case GL_RECACHE_MSG:
			Recache();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void GlMainPathView::MouseDown(BPoint where)
{
	ArpVALIDATE(mNodes, return);
	mMouseDownIndex = -1;
	int32						index;
	const GlMainLabelEntry*		pn = mNodes->EntryAt(where, &index);
	/* Controls take precedence over everything else.
	 */
	int32						i32;
	if (mControls.IsEnabled() && mControls.ControlAt(where, &i32) == B_OK) {
		if (i32 == ROOT_INFO_ID) {
			if (Window()) Window()->PostMessage(GL_INSPECT_ROOT_INFO_MSG, this);
		} else if (i32 == MANAGE_CTRL_ID) printf("Manage\n");
		return;
	}
	
	if (pn) {
		if (pn->code == ROOT_CODE) RootPressed(index);
		mMouseDownIndex = index;
	}
}

void GlMainPathView::MouseMoved(BPoint where, uint32 code,
								const BMessage* dragMessage)
{
	ArpVALIDATE(mNodes, return);
	bool						redraw = false;

	int32						index;
	const GlMainLabelEntry*		pn = mNodes->EntryAt(where, &index);
	if (pn && pn->code == ROOT_CODE) {
		if (mControls.IsEnabled() == false) {
			mControls.Arrange(pn->frame, index);
			mControls.SetEnabled(true);
			redraw = true;
		}
	} else {
		if (mControls.IsEnabled()) {
			mControls.SetEnabled(false);
			redraw = true;
		}
	}

	if (redraw) this->Invalidate();
}

void GlMainPathView::MouseUp(BPoint where)
{
	ArpVALIDATE(mNodes, return);
	int32						index;
	const GlMainLabelEntry*		pn = mNodes->EntryAt(where, &index);
	if (pn && pn->code != ROOT_CODE && index == mMouseDownIndex) {
		PathPressed(index);
	}
	mMouseDownIndex = -1;
}

void GlMainPathView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

status_t GlMainPathView::PathPressed(int32 index)
{
	ArpVALIDATE(mNodes && index >= 0 && index < int32(mNodes->e.size()), return B_ERROR);
	if (mPath.Current().IsValid() == false) return B_OK;
	uint32				s = mPath.PathSize();
	/* The + 1 accounts for the fact that I don't display a
	 * label for the root chain.
	 */
	if (index + 1 >= int32(s)) return B_OK;
	if (mPath.PopTo(index + 1) == B_OK) {
		if (Window()) Window()->PostMessage(GL_PATH_CHANGED_MSG, this);
	}
	return B_OK;
}

status_t GlMainPathView::RootPressed(int32 index)
{
	ArpVALIDATE(mNodes && index >= 0 && index < int32(mNodes->e.size()), return B_ERROR);
//	printf("Root pressed\n");

	ArpPopUpMenu*				m = new ArpPopUpMenu(SZ(SZ_Roots));
	if (!m) return B_NO_MEMORY;

	gl_node_id					curId = mPath.Current().NodeId();
	GlRootRef					ref;
	BString16					label;
	for (uint32 k = 0; (ref = mPath.At(k)).NodeId() != 0; k++) {
		gl_get_root_label(ref, label);
		if (label.Length() > 0) {
//			printf("%ld: '%s'\n", k, label.String());
			BMessage*			msg = new BMessage(CHANGE_ROOT_MSG);
			if (msg && msg->AddInt32(GL_I_STR, k) == B_OK) {
				BMenuItem*		item = new BMenuItem(label.String(), msg);
				if (item) {
					if (curId == ref.NodeId()) item->SetMarked(true);
					m->AddItem(item);
				} else delete msg;
			} else delete msg;
		}
	}

	m->SetTargetForItems(this);
	m->GoAndDeliver(mNodes->e[index]->frame.LeftTop(), *this);

	return B_OK;
}

status_t GlMainPathView::ChangeRoot(int32 index)
{
	GlRootRef		ref = mPath.At(index);
	if (ref.NodeId() == 0) return B_ERROR;
	
	status_t		err = mPath.SetCurrent(ref.NodeId());
	if (err != B_OK) return err;
	if (Window()) Window()->PostMessage(GL_ROOT_CHANGED_MSG);
	return B_OK;
}

status_t GlMainPathView::RootChanged()
{
	Recache();
	GlRootRef		ref = mPath.Current();
	return ref.AddObserver(GlNotifier::INFO_CODE, BMessenger(this));
}

void GlMainPathView::DrawOn(BRect clip, BView* v)
{
	v->SetLowColor(ViewColor());
	v->SetHighColor(ViewColor());
	v->FillRect(clip);
	v->SetHighColor(0, 0, 0);
	ArpVALIDATE(mNodes, return);
	
	bool					first = true;
//	BPoint					pt(0, Bounds().bottom);
//	const char*				sep = "/";
//	float					sepW = arp_get_string_width(this, sep);
	for (uint32 k = 0; k < mNodes->e.size(); k++) {
		GlMainLabelEntry*	pn = mNodes->e[k];
		if (pn && pn->label.Length() > 0) {
			if (!first) {
//				v->DrawString(sep, pt);
//				pt.x += sepW;
			}
			v->DrawString(pn->label.String(), pn->frame.LeftBottom());
#if 0
			pt.x += arp_get_string_width(this, pn->mNodeLabel.String());
			if (pn->mCid && pn->mChains > 1 && pn->mChainLabel.Length() > 0) {
				BString16		str;
				str << " (" << pn->mChainLabel.String() << ")";
				v->DrawString(str.String(), pt);
				pt.x += arp_get_string_width(this, str.String());
			}
#endif
			first = false;
		}
	}

	mControls.DrawOn(clip, v);
}

//static uint32 gCount = 0;

status_t GlMainPathView::Recache()
{
	ArpVALIDATE(mNodes, return B_NO_MEMORY);
	mNodes->Free();
	vector<GlMainLabelEntry*>		vec;
	gl_node_id						nid = 0;
	gl_chain_id						cid = 0;
	status_t						err = mPath.GetTail(&nid, &cid);
	BString16						lbl;
//printf("%ld\n", gCount);
//gCount++;
	if (err == B_OK && nid) {
		GlRootRef					ref = mPath.Current();
		gl_chain_id					rootCid = ref.ChainId();
		const GlRootNode*			root = ref.ReadLock();
		GlMainLabelEntry*			e;
		if (root) {
			gl_get_root_label(root, lbl);
			const GlNode*			n = root->FindNode(0, nid);
			const GlChain*			chain = 0;
			if (n && cid) chain = n->FindChain(cid, false);
			while (n) {
				if (chain && chain->Id() == rootCid) break;
				BString16			lbl = n->AddOn()->Label();
//printf("\t%s\n", lbl.String());
				if (chain && n->ChainSize() > 1) lbl << " (" << chain->Label() << ")";
				if ((e = new GlMainLabelEntry(lbl, _NODE_CODE)) != 0)
					vec.push_back(e);
				chain = n->Parent();
				if (chain) n = chain->Parent();
				else n = 0;
			}
		}
		ref.ReadUnlock(root);
		if (mPath.At(mPath.Current().NodeId()).IsValid()) {
			if (lbl.Length() < 1) lbl = "<root>";
			if ((e = new GlMainLabelEntry(lbl, ROOT_CODE)) != 0)
				vec.push_back(e);
		}
	}
	
	float						padX = 5;
	float						fH = arp_get_font_height(this);
	BPoint						pt(padX, Bounds().bottom);

	for (int32 k = int32(vec.size()) - 1; k >= 0; k--) {
		float					w = StringWidth(&(vec[k]->label));
		vec[k]->frame.Set(pt.x, pt.y - fH, pt.x + w, pt.y);
		pt.x += w + padX;
		mNodes->e.push_back(vec[k]);
	}
#if 0
printf("mNodes size %ld\n", mNodes->e.size());
for (uint32 j = 0; j < mNodes->e.size(); j++) {
	printf("Thing: %s\n", mNodes->e[j]->label.String());
}
#endif
	vec.resize(0);

	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
	return B_OK;
}

// #pragma mark -

/********************************************************
 * GL-MAIN-MIDI-VIEW
 ********************************************************/
GlMainMidiView::GlMainMidiView(BRect frame)
		: inherited(frame, "midi", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mQImage(0), mRecordBm(0), mMouseDown(-1)
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) mImages[k] = 0;

	/* FIX:  The image pool should store bitmaps, also,
	 * so these should be acquired, not copied.
	 */
	mImages[0] = GlGlobals().CloneBitmap(GL_MIDI_A_IMAGE_ID);
	mImages[1] = GlGlobals().CloneBitmap(GL_MIDI_B_IMAGE_ID);
	mImages[2] = GlGlobals().CloneBitmap(GL_MIDI_C_IMAGE_ID);
	mQImage = GlGlobals().CloneBitmap(GL_MIDI_Q_IMAGE_ID);
	mRecordBm = GlGlobals().CloneBitmap(GL_MIDI_RECORD_IMAGE_ID);
}

GlMainMidiView::~GlMainMidiView()
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) delete mImages[k];
	delete mQImage;
	delete mRecordBm;
}

void GlMainMidiView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case GL_INVALIDATE_MSG:
			this->Invalidate();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void GlMainMidiView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void GlMainMidiView::MouseDown(BPoint where)
{
	mMouseDown = -1;
	if (OnRecord(where)) mMouseDown = 0;
}

void GlMainMidiView::MouseMoved(BPoint where, uint32 code,
								const BMessage* dragMessage)
{
}

void GlMainMidiView::MouseUp(BPoint where)
{
	int32		md = mMouseDown;
	mMouseDown = -1;
	if (md != 0 || !OnRecord(where)) return;

	if (Window()) Window()->PostMessage(GL_RECORD_MSG);
}

void GlMainMidiView::SetState(	GlControlChannel* channel,
								int32 viewType)
{
	mState.Clear();
	if (channel && viewType == GL_INSPECTOR_VIEW) channel->SetState(mState);
	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
}

void GlMainMidiView::DrawOn(BRect clip, BView* v)
{
	v->SetHighColor(ViewColor());
	v->FillRect(clip);

	drawing_mode		mode = v->DrawingMode();
	v->SetDrawingMode(B_OP_ALPHA);
	v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	BPoint				origin(2, 1);

	if (mRecordBm) {
		if (mState.mRecordable) v->DrawBitmap(mRecordBm->mBitmap, origin);
		origin.x += (2 + mRecordBm->Width());
	}

	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) {
		if (mState.mMidi[k]) {
			const ArpBitmap*	bm = mImages[k];
			if (!bm) bm = mQImage;
			if (bm) {
				v->DrawBitmap(mImages[k]->mBitmap, origin);
				origin.x += (2 + mImages[k]->Width());
			}
		}
	}
	v->SetDrawingMode(mode);
}

bool GlMainMidiView::OnRecord(BPoint where)
{
	if (!mRecordBm || !mState.mRecordable) return false;
	BRect			f(2, 1, 0, 0);
	f.right = f.left + mRecordBm->Width();
	f.bottom = f.top + mRecordBm->Height();
	return f.Contains(where);
}

// #pragma mark -

/********************************************************
 * GL-MAIN-CONTROL-CHANNEL
 ********************************************************/
GlMainControlChannel::GlMainControlChannel(BMessenger window)
		: mWindow(window), mPath(0)
{
}

GlMainControlChannel::~GlMainControlChannel()
{
	DeleteTargets();
}

void GlMainControlChannel::SetPath(GlProjectList* path)
{
	mPath = path;
}

status_t GlMainControlChannel::ParamEvent(	gl_param_key key, int32 code,
											const GlParamWrap& wrap)
{
	/* Send the event to my targets -- typically all my targets,
	 * but who receives can change depending on the flags.
	 */
	uint32				k;
	bool				exclusive = false;
	for (k = 0; k < mTargets.size(); k++) {
		ArpASSERT(mTargets[k]);
		if (mTargets[k]->Flags()&GlControlTarget::EXCLUSIVE_F) {
			exclusive = true;
			break;
		}
	}

	for (k = 0; k < mTargets.size(); k++) {
		if (!exclusive || mTargets[k]->Flags()&GlControlTarget::EXCLUSIVE_F) {
			mTargets[k]->ParamEvent(key, code, wrap);
		}
	}
	return B_OK;
}

status_t GlMainControlChannel::MidiEvent(GlMidiEvent event, bigtime_t time)
{
	/* Handle MIDI Machine Control event
	 */
	if (event.type == event.MMC) {
		if (event.value2 == 1) return mWindow.SendMessage(GL_STOP_MSG);
		else if (event.value2 == 2) return mWindow.SendMessage(GL_PLAY_MSG);
		else if (event.value2 == 6) return mWindow.SendMessage(GL_RECORD_MSG);
		return B_ERROR;
	}

	GlMidiBinding*		binding = gl_app->MidiBindings().At(event);
	if (!binding) return B_ERROR;

	/* Handle event that's configured to create a new node
	 */
	if (binding->rt < GL_MIDI_A || binding->rt > GL_MIDI_Z)
		return CreateRoot(binding, event.ScaledValue());

	/* Handle event that's configured to control a parameter
	 */
	if (mTargets.size() < 1) return B_ERROR;

	/* Send the event to my targets -- typically all my targets,
	 * but who receives can change depending on the flags.
	 */
	uint32				k;
	bool				exclusive = false;
	for (k = 0; k < mTargets.size(); k++) {
		ArpASSERT(mTargets[k]);
		if (mTargets[k]->Flags()&GlControlTarget::EXCLUSIVE_F) {
			exclusive = true;
			break;
		}
	}
	for (k = 0; k < mTargets.size(); k++) {
		if (!exclusive || mTargets[k]->Flags()&GlControlTarget::EXCLUSIVE_F) {
			mTargets[k]->MidiEvent(event, binding->rt, time);
		}
	}

	return B_OK;
}

status_t GlMainControlChannel::Add(GlControlTarget* target)
{
	ArpVALIDATE(target, return B_ERROR);
	mTargets.push_back(target);
	return B_OK;
}

status_t GlMainControlChannel::StartRecording()
{
	for (uint32 k = 0; k < mTargets.size(); k++) {
		ArpASSERT(mTargets[k]);
		status_t		err = mTargets[k]->StartRecording();
		if (err != B_OK) return err;
	}
	return B_OK;
}

void GlMainControlChannel::StopRecording()
{
	for (uint32 k = 0; k < mTargets.size(); k++) {
		ArpASSERT(mTargets[k]);
		mTargets[k]->StopRecording();
	}
}

void GlMainControlChannel::Populate(GlControlTargetPopulator& p)
{
	for (uint32 k = 0; k < mTargets.size(); k++) {
		ArpASSERT(mTargets[k]);
		mTargets[k]->Populate(p);
	}
}

void GlMainControlChannel::SetState(GlControlState& s) const
{
	for (uint32 k = 0; k < mTargets.size(); k++) {
		ArpASSERT(mTargets[k]);
		mTargets[k]->SetState(s);
	}
}

void GlMainControlChannel::DeleteTargets()
{
	for (uint32 k = 0; k < mTargets.size(); k++) delete mTargets[k];
	mTargets.resize(0);
}

static GlNode* _find_marked(GlChain* c, BString16 creator, int32 key)
{
	GlNode*			n;
	for (uint32 k = 0; (n = c->NodeAt(k)) != 0; k++) {
		if (n->Tags()&GL_MARKED_F && n->AddOn()) {
			if (n->GetMatch(creator, key) > 0.99999) return n;
		}
	}
	return 0;
}

static bool _can_install(int32 chain, int32 node)
{
	/* FIX: Probably a better way than this
	 */
	if (node&GL_1D_IO && chain&GL_1D_IO) return true;
	if (node&GL_2D_IO && chain&GL_2D_IO) return true;
	if (node&GL_IMAGE_IO && chain&GL_IMAGE_IO) return true;
	if (node&GL_TEXT_IO && chain&GL_TEXT_IO) return true;
	if (node&GL_NUMBER_IO && chain&GL_NUMBER_IO) return true;
	return false;
}

status_t GlMainControlChannel::CreateRoot(GlMidiBinding* b, float v)
{
	ArpVALIDATE(b, return B_ERROR);
	if (!mPath || b->root == 0) return B_ERROR;
	status_t			err = B_ERROR;
	GlRootRef			cur = mPath->Current();
	GlRootNode*			root = cur.WriteLock();
	if (!root) return err;
	GlChain*			c = mPath->Tail(root);
	gl_chain_id			cid = 0;
	gl_node_id			nid = 0;
	if (c) {
		cid = c->Id();
		GlNode*			n = _find_marked(c, b->root->Creator(), b->root->Key());
		if (!n && _can_install(c->Io(), b->root->Io())) {
			BMessage	msg;
			if (b->root->WriteTo(msg) == B_OK) {
				n = GlGlobals().NewNode(GL_WRAPPER_KEY, &msg);
				if (n) {
					n->SetTags(n->Tags()|GL_MARKED_F);
					/* FIX: This is a hack -- force the node to generate an
					 * image.  Not sure where the best place for this would
					 * be, but at some point nodes will be able to store images,
					 * so they will be part of the ReadFrom(), WriteTo().  When
					 * that's in place, probably construct the image when the
					 * root is loaded into the binding.
					 */
					n->Image();
					err = c->AddNode(n);
				}
			}
		}
		if (n) {
			err = SetParamValue(v, b->paramIndex, n);
			nid = n->Id();
		}
	}
	cur.WriteUnlock(root);

	/* Inspect the node -- this makes MIDI params active for the
	 * node, so conceivably a user could construct an entire change,
	 * alter all its parameters, and run the root all from a controller.
	 */
	if (err == B_OK && mWindow.IsValid()) {
		BMessage	msg(GL_INSPECT_NODE_MSG);
		msg.AddPointer(GL_CHAIN_ID_STR, cid);
		msg.AddPointer(GL_NODE_ID_STR, nid);
		mWindow.SendMessage(&msg);
	}
	
	return err;
}

static float _float_value(GlStrainedParam& param, float v)
{
	ArpASSERT(param.pt->Type() == GL_FLOAT_TYPE);
	const GlFloatParamType*	pt = (const GlFloatParamType*)param.pt;
	const GlFloatParam*		p = (param.p) ? ((const GlFloatParam*)param.p) : 0;
	float					min = pt->Min(), max = pt->Max();
	if (p) p->GetRange(&min, &max);
	float					r = max - min;
	return min + (r * v);
}

static int32 _int32_value(GlStrainedParam& param, float v)
{
	ArpASSERT(param.pt->Type() == GL_INT32_TYPE);
	GlInt32ParamType*		pt = (GlInt32ParamType*)param.pt;
	int32					r = pt->Max() - pt->Min();
	return int32(pt->Min() + (r * v));
}

status_t GlMainControlChannel::SetParamValue(float v, int32 paramIndex, GlNode* root) const
{
	ArpASSERT(root);
	GlStrainedParamList			params;
	root->GetParams(params);
	GlStrainedParam				p;
	int32						paramIndexCount = 0;
	uint32						size = params.Size();
	for (uint32 k = 0; k < size; k++) {
		if (params.At(k, p) == B_OK) {
			ArpASSERT(p.nid && p.pt);
			if (paramIndex == paramIndexCount) {
				GlNode*			n = root->FindNode(0, p.nid);
				if (n) {
					int32		key = p.pt->Key();
					if (p.pt->Type() == GL_FLOAT_TYPE) {
						n->Params().SetValue(key, GlFloatWrap(_float_value(p, v)));
						n->ParamChanged(gl_param_key(p.nid, key, p.index));
					} else if (p.pt->Type() == GL_INT32_TYPE) {
						n->Params().SetValue(key, GlInt32Wrap(_int32_value(p, v)));
						n->ParamChanged(gl_param_key(p.nid, key, p.index));
					}
					return B_OK;
				}
				return B_ERROR;
			}
			paramIndexCount++;
		}
	}
	return B_ERROR;
}

// #pragma mark -

/*************************************************************************
 * GL-PERFORMER
 *************************************************************************/
class _GlPerformerData
{
public:
	GlAlgo*				a;
	GlNodeDataList		list;


	_GlPerformerData() : a(0)		{ }
	~_GlPerformerData()
	{
		delete a;
		list.DeleteContents();
	}
};

GlPerformer::GlPerformer(GlResultCache& result)
		: mResult(result), mArgsFlags(0), mCode(0),
		  mState(ROOT_WAITING), mThread(0), mData(0)
{
}

GlPerformer::~GlPerformer()
{
	Stop();
}

status_t GlPerformer::InitStatus(	const BMessenger& target, int32 code,
									float width)
{
	mTarget = target;
	mCode = code;
	mStatus.SetTarget(target, code);
	mStatus.SetProgressWidth(int32(width));
	return B_OK;
}

status_t GlPerformer::StartRoot(const GlRootRef* ref, bool debug)
{
	BAutolock		l(mAccess);
	if (mState == ROOT_RUNNING) return B_OK;
	FreePerformData();
	status_t		err = B_ERROR;
	if (ref) err = SetupFromProject(*ref);
	if (err != B_OK) return err;

	mThread = spawn_thread(	PerformThreadEntry,
							"ARP Perform Root",
							B_NORMAL_PRIORITY,
							this);
	if (mThread <= 0) return B_ERROR;
	mState = ROOT_RUNNING;
	BMessage		msg(GL_DISABLE_PLAY_CONTROLS_MSG);
	msg.AddInt32("c", mCode);
	mTarget.SendMessage(&msg);
	resume_thread(mThread);
	return B_OK;
}

void GlPerformer::Stop()
{
	BAutolock		l(mAccess);
	mStatus.SetCanceled();
	/* This is just in case -- in any normal condition, the
	 * thread will have taken ownership of the data.
	 */
	delete mData;
	mData = 0;
}

GlImage* GlPerformer::NewInputImage() const
{
	return mResult.NewFrozenImage();
}

status_t GlPerformer::SetResult(GlImage* img)
{
	return mResult.UpdateImage(img);
}

status_t GlPerformer::SetupFromProject(const GlRootRef& ref)
{
	mData = new _GlPerformerData();
	if (!mData) return B_NO_MEMORY;

	gl_generate_args		args;
	const GlRootNode*		n = ref.ReadLock();
	if (n) mData->a = n->Generate(args);
	ref.ReadUnlock(n);
//if (mData->a) mData->a->Print();
	if (mData->a == 0) {
		FreePerformData();
		PlayNothing();
		return B_ERROR;
	}
	/* Freeze the display, if necessary, before building the input data.
	 */
//	if (mContextId && mContextId != mid)
//		mResult.FreezeDisplay();
	GlImage*				img = NewInputImage();
	if (img) mData->list.AddImage(img);

	return B_OK;
}

_GlPerformerData* GlPerformer::GiveData()
{
	_GlPerformerData*	ans = 0;
	BAutolock			l(mAccess);
	ans = mData;
	mData = 0;
	return ans;
}

void GlPerformer::PerformFinished(_GlPerformerData* data, status_t performErr)
{
	BAutolock		l(mAccess);
	ArpASSERT(mState == ROOT_RUNNING);
	if (data) {
		/* Print any errors.
		 */
		BString16				err;
		for (uint32 k = 0; (err = data->list.ErrorAt(k)).Length() > 0; k++)
			printf("ERROR: %s\n", err.String());
		/* Print any text.
		 */
		BString16*				txt;
		for (uint32 k = 0; (txt = data->list.TextAt(k)) != 0; k++)
			printf("Text: '%s'\n", txt->String());
		fflush(stdout);
		/* Update the result image.
		 */
		if (performErr == B_OK) SetResult(data->list.DetachImage());
	}

	FreePerformData();
	mState = ROOT_WAITING;
	mThread = 0;
	BMessage		msg(GL_ENABLE_PLAY_CONTROLS_MSG);
	msg.AddInt32("c", mCode);
	mTarget.SendMessage(&msg);
}

void GlPerformer::FreePerformData()
{
	/* In any normal case, I don't own the data, it's been
	 * passed off to the thread.  This is just in case something
	 * didn't happen properly.
	 */
	delete mData;
	mData = 0;
}

#if 0
/* When a grid is performed, all its Image Out nodes need to store
 * the dimensions of any image coming out -- this is for page layout views.
 */
class _GlCacheDimensAction : public GlProcessingNodeAction
{
public:
	_GlCacheDimensAction() : mC(GL_ARP_CREATOR), mN(GL_IMAGE_OUT_NODE)	{ }
	virtual ~_GlCacheDimensAction()										{ }
	
	virtual void	Perform(GlNode* node, GlNodeDataList* list)
	{
		ArpASSERT(node);
//		printf("Perform on %s:%s\n", node->AddOn()->Creator().String(), node->AddOn()->Name().String());
		if (mC == node->AddOn()->Creator() && mN == node->AddOn()->Name()) {
			GlImage*			im = 0;
			if (list && (im = list->ImageAt(0)) != 0) {
				int32			x, y;
				im->GetOrigin(&x, &y);
				GlPointWrap		pt(BPoint(x, y));
				node->Annotations().SetParamValue(GL_ORIGIN_ANN, pt);
				pt.pt.x = im->Width();
				pt.pt.y = im->Height();
				node->Annotations().SetParamValue(GL_EXTENT_ANN, pt);
			} else {
				node->Annotations().EraseParamValue(GL_ORIGIN_ANN);
				node->Annotations().EraseParamValue(GL_EXTENT_ANN);
			}
		}
	}

private:
	BString16				mC, mN;
};
#endif

int32 GlPerformer::PerformThreadEntry(void* arg)
{
	GlPerformer*					obj = (GlPerformer*)arg;
	if (!obj) return B_ERROR;

	status_t						err = B_OK;
	_GlPerformerData*				data = obj->GiveData();
	if (data && data->a) {
		gl_process_args			args;
//		args.projectFolder = grid->Path();
//		args.projectName = grid->Label();
		args.flags = obj->mArgsFlags;
		args.status = &(obj->mStatus);
		args.status->Init();
//		_GlCacheDimensAction	action;

bigtime_t				start = system_time();
		err = data->a->PerformAll(data->list, &args);
if (!(args.flags&GL_PREVIEW_F)) printf("Elapsed: %f sec\n", double(system_time() - start) / 1000000);
	}
	obj->PerformFinished(data, err);
	delete data;

	return err;
}

// #pragma mark -

/*************************************************************************
 * GL-PREVIEW-PERFORMER
 *************************************************************************/
GlPreviewPerformer::GlPreviewPerformer(GlResultCache& result)
		 : inherited(result), mPreview(0), mW(0), mH(0)
{
	mArgsFlags = GL_PREVIEW_F;
}

void GlPreviewPerformer::SetPreview(GlMainPreview* preview)
{
	mPreview = preview;
	if (mPreview) {
		BRect		b(mPreview->Bounds());
		mW = int32(b.Width());
		mH = int32(b.Height());
	}
}

GlImage* GlPreviewPerformer::NewInputImage() const
{
	return mResult.NewPreviewImage();
}

status_t GlPreviewPerformer::SetResult(GlImage* img)
{
	if (img && mPreview) {
		int32			w = img->Width(), h = img->Height();
		if (w <= mW && h <= mH)
			mPreview->TakeBitmap(img->AsBitmap());
		else {
			/* Scale down but maintain aspect ratio.
			 */
			float		scaleW = float(mW) / float(w),
						scaleH = float(mH) / float(h);
			float		scale = (scaleW < scaleH) ? scaleW : scaleH;
			w = int32(w * scale);
			h = int32(h * scale);
			if (w > mW) w = mW; else if (w < 1) w = 1;
			if (h > mH) h = mH; else if (h < 1) h = 1;
			GlImage*	n = img->AsScaledImage(w, h, 0.0);
			if (n) mPreview->TakeBitmap(n->AsBitmap());
			delete n;
		}
	}
	delete img;
	return B_OK;
}

void GlPreviewPerformer::PlayNothing()
{
	if (!mPreview) return;
	mPreview->TakeBitmap(mResult.NewPreviewBitmap());
}

// #pragma mark -

/********************************************************
 * GL-MAIN-PREVIEW
 ********************************************************/
GlMainPreview::GlMainPreview(	BRect frame, GlProjectList& path,
								GlResultCache& result)
		: inherited(frame, "preview", B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		  mBitmap(0), mPath(path), mPerformer(result)
{
	mBgC = Prefs().GetColor(ARP_BG_C);
	mPerformer.SetPreview(this);
}

GlMainPreview::~GlMainPreview()
{
	BAutolock		l(mAccess);
	delete mBitmap;
}

void GlMainPreview::AttachedToWindow()
{
	inherited::AttachedToWindow();
//	mPerformer.InitStatus(BMessenger(this), PREVIEW_CODE, 0);
	mPerformer.InitStatus(BMessenger(this), 1, 0);

/* FIX: Turn this back on once the parent is actually getting
 * assigned the proper bg c
 */
//	if (Parent()) mBgC = Parent()->ViewColor();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void GlMainPreview::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void GlMainPreview::MessageReceived(BMessage* msg)
{
	if (msg->WasDropped() || msg->what == B_REFS_RECEIVED) {
		if (Window()) {
			msg->what = GL_PREVIEW_IMAGE_DROPPED;
			Window()->PostMessage(msg);
		}
		return;
	}

	switch (msg->what) {
		case GlNotifier::CHANGE_MSG:
			Process();
			break;
		case GL_ROOT_CHANGED_MSG:
			RootChanged();
			break;
		case GL_INVALIDATE_MSG:
			this->Invalidate();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void GlMainPreview::MouseDown(BPoint where)
{
}

void GlMainPreview::MouseMoved(	BPoint where, uint32 code,
								const BMessage* dragMessage)
{
}

void GlMainPreview::MouseUp(BPoint where)
{
}

void GlMainPreview::TakeBitmap(ArpBitmap* bm)
{
	{
		BAutolock		l(mAccess);
		delete mBitmap;
		mBitmap = bm;
	}

	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
}

status_t GlMainPreview::Process()
{
	GlRootRef		ref = mPath.Current();
	if (!ref.IsValid()) return B_ERROR;
	return mPerformer.StartRoot(&ref);
}

status_t GlMainPreview::RootChanged()
{
	Process();
	GlRootRef		ref = mPath.Current();
	ref.AddObserver(GlNotifier::CHAIN_CODE, BMessenger(this));
	return ref.AddObserver(GlNotifier::NODE_CODE, BMessenger(this));
}

void GlMainPreview::DrawOn(BRect clip, BView* v)
{
	v->SetHighColor(mBgC);
	v->FillRect(clip);

	BAutolock				l(mAccess);
	if (mBitmap) {
		int32				bw = int32(Bounds().Width()), bh = int32(Bounds().Height()),
							w = mBitmap->Width(), h = mBitmap->Height();
		BPoint				origin(0, 0);
		if (w < bw) origin.x = (bw - w) / 2.0f;
		if (h < bh) origin.y = (bh - h) / 2.0f;
		// Do I ALWAYS want this?
		drawing_mode		mode = v->DrawingMode();
		v->SetDrawingMode(B_OP_ALPHA);
		v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		v->DrawBitmap(mBitmap->mBitmap, origin);
		v->SetDrawingMode(mode);
	}
}

// #pragma mark -

/***************************************************************************
 * Miscellaneous functions
 ***************************************************************************/
BMenuItem* add_menu_item(	BMenu* toMenu,
							const BString16* label,
							BMessage* msg,
							char shortcut,
							uint32 modifiers)
{
	BMenuItem	*item = new BMenuItem(label, msg, shortcut, modifiers);
	if (!item) {
		delete msg;
		return NULL;
	}
	toMenu->AddItem(item);
	return item;
}

BMenuItem* add_menu_item(	BMenu* toMenu,
							const BString16* label,
							uint32 what,
							char shortcut,
							uint32 modifiers)
{
	BMessage	*msg = new BMessage(what);
	if (!msg) return NULL;
	return add_menu_item(toMenu, label, msg, shortcut, modifiers);
}

