#include <StorageKit.h>
#include <support/Autolock.h>
#include <ArpCore/ArpDebug.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ViewTools.h>
#include <GlPublic/GlChain.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlPlaneNode.h>
#include "GlPublic/GlGlobalsI.h"
#include "GlPublic/GlImage.h"
#include <GlPublic/GlRootNode.h>
#include <Glasslike/GlApp.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlMainNavView.h>
#include <Glasslike/GlMainWinLists.h>

static const int32				DOWN_CTRL_ID			= 0;
static const int32				CONFIG_CTRL_ID			= 1;
static const int32				PIX_CTRL_ID				= 2;

/***************************************************************************
 * GL-NAV-CONTROL-LAYER
 ***************************************************************************/
GlNavControlLayer::GlNavControlLayer()
		: id(0), mDownExp(0), mDownOvr(0), mConfigExp(0), mConfigOvr(0),
		  mPixExp(0), mPixOvr(0)
{
	float				x = float(Prefs().GetInt32(GL_NODE_CONTROL_X)),
						y = float(Prefs().GetInt32(GL_NODE_CONTROL_Y));
	mDownExp = new ArpBitmap(x, y);
	mDownOvr = new ArpBitmap(x, y);
	mConfigExp = new ArpBitmap(x, y);
	mConfigOvr = new ArpBitmap(x, y);
	mPixExp = GlGlobals().CloneBitmap(GL_PIXEL_TARGET_IMAGE_ID);
	mPixOvr = GlGlobals().CloneBitmap(GL_PIXEL_TARGET_IMAGE_ID);
	
	AddControl(DOWN_CTRL_ID, mDownExp, mDownOvr);
	AddControl(CONFIG_CTRL_ID, mConfigExp, mConfigOvr);
	AddControl(PIX_CTRL_ID, mPixExp, mPixOvr);
}

GlNavControlLayer::~GlNavControlLayer()
{
	delete mDownExp;
	delete mDownOvr;
	delete mConfigExp;
	delete mConfigOvr;
	delete mPixExp;
	delete mPixOvr;
}

void GlNavControlLayer::Arrange(GlMainImageEntry* node)
{
	ArpVALIDATE(node, return);
	float			l = node->frame.left, t = node->frame.top,
					r = node->frame.right, b = node->frame.bottom;
	id = node->id;
	
	if (node->chainSize > 0)
			SetControlProps(DOWN_CTRL_ID,	BPoint(l - 8, b - 4), true);
	else	SetControlProps(DOWN_CTRL_ID,	BPoint(0, 0), false);
	SetControlProps(CONFIG_CTRL_ID,			BPoint(l - 8, t - 6), true);

	if (node->flags&GL_PIXEL_TARGETS_F)
		SetControlProps(PIX_CTRL_ID,		BPoint(r - 5, t - 6), true);
	else
		SetControlProps(PIX_CTRL_ID,		BPoint(0, 0), false);

}

// #pragma mark -

/********************************************************
 * GL-NAV-CONTROL
 ********************************************************/
GlNavControl::GlNavControl()
		: code(GL_ON_NOTHING), index(0), id(0)
{
}

GlNavControl::GlNavControl(const GlNavControl& o)
		: code(o.code), index(o.index), id(o.id)
{
}

bool GlNavControl::operator==(const GlNavControl& o) const
{
	if (code != o.code) return false;
	if (code == GL_ON_NOTHING) return true;
	return index == o.index && id == o.id;
}

bool GlNavControl::operator!=(const GlNavControl& o) const
{
	if (code != o.code) return true;
	if (code == GL_ON_NOTHING) return false;
	return index != o.index || id != o.id;
}

GlNavControl& GlNavControl::operator=(const GlNavControl& o)
{
	code = o.code;
	index = o.index;
	id = o.id;
	return *this;
}

// #pragma mark -

/********************************************************
 * GL-MAIN-NAV-VIEW
 ********************************************************/
GlMainNavView::GlMainNavView(BRect frame, GlProjectList& path)
		: inherited(frame, "chain", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mPath(path), mParentId(0), mContextId(0),
		  mNodes(new GlMainImages()), mLabels(new GlMainLabels()),
		  mMouseDownButtons(0)
{
}

GlMainNavView::~GlMainNavView()
{
	delete mNodes;
	delete mLabels;
}

void GlMainNavView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void GlMainNavView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes != 1) {
		inherited::KeyDown(bytes, numBytes);
		return;
	}
	switch ( bytes[0] ) {
		case B_DELETE: {
			if (mOverCache.code == mOverCache.GL_ON_NODE
					|| mOverCache.code == mOverCache.GL_ON_CONTROL) {
				NodeDeleted(mOverCache.id);
			}
		} break;
		inherited::KeyDown(bytes, numBytes);
		break;
	}
}

void GlMainNavView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case GlNotifier::CHANGE_MSG:
			Recache();
			break;
		case GL_NODE_ADDON_DRAG_MSG: {
			gl_id		id;
			BPoint		pt;
			if (msg->FindPointer(GL_NODE_ADDON_ID_STR, &id) == B_OK
					&& msg->FindPoint("_drop_point_", &pt) == B_OK)
				AddOnDropped(id, ConvertFromScreen(pt));
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

void GlMainNavView::MouseDown(BPoint where)
{
	ArpVALIDATE(mNodes && mLabels, return);
	mMouseDownButtons = arp_get_mouse_buttons(*this);
	CacheControl(where, mDownCache);

/* FIX: Can't remember what this was for -- I think tracking the
 * mouse beyond the bounds of the view?
 */
	SetMouseEventMask(B_POINTER_EVENTS,
					  B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
}

void GlMainNavView::MouseMoved(	BPoint where, uint32 code,
								const BMessage* dragMessage)
{
	bool						redraw = false;
	GlMainImageEntry*			nc = 0;
	GlNavControl				ctrl;
	CacheControl(where, ctrl);

	if (dragMessage) {
		if (mControls.IsEnabled()) {
			mControls.SetEnabled(false);
			redraw = true;
		}
		if (dragMessage->what == GL_NODE_ADDON_DRAG_MSG) {
			gl_id			id;
			if (dragMessage->FindPointer(GL_NODE_ADDON_ID_STR, &id) == B_OK) {
			}
		}
	} else if (ctrl.code == ctrl.GL_ON_NODE || ctrl.code == ctrl.GL_ON_CONTROL) {
		if (ctrl != mOverCache) {
			nc = mNodes->EntryAt(ctrl.id);
			if (nc) {
				mControls.Arrange(nc);
				mControls.SetEnabled(true);
				redraw = true;
			}
		}
	} else if (mControls.IsEnabled()) {
		mControls.SetEnabled(false);
		redraw = true;
	}

	if (ctrl != mOverCache) {
		BMessage		msg(GL_SET_STATUS_MSG);
		if (ctrl.code == ctrl.GL_ON_NODE) {
			if (!nc) nc = mNodes->EntryAt(ctrl.id);
			if (nc && nc->label.Length() > 0)
				msg.AddString16(GL_TEXT_STR, nc->label);
		}
		if (Window()) Window()->PostMessage(&msg);
	}
	
	mOverCache = ctrl;
	if (redraw) this->Invalidate();
}

void GlMainNavView::MouseUp(BPoint where)
{
	ArpVALIDATE(mNodes && mLabels, return);

	if (mMouseDownButtons&B_SECONDARY_MOUSE_BUTTON) {
		if (mPath.PathSize() > 1 && mPath.Pop() == B_OK) {
			if (Window()) Window()->PostMessage(GL_PATH_CHANGED_MSG);
		}
	} else if (mDownCache.code == mDownCache.GL_ON_CONTROL) {
		GlNavControl		cache;
		CacheControl(where, cache);
		if (mDownCache == cache) {
			if (mDownCache.index == DOWN_CTRL_ID) {
				PathDown(mContextId, mDownCache.id);
				Window()->PostMessage(GL_PATH_CHANGED_MSG);
			} else if (mDownCache.index == CONFIG_CTRL_ID) {
				int32				index;
				GlMainImageEntry*	cache = mNodes->EntryAt(mDownCache.id, &index);
				if (cache) NodePressed(index, GL_CONFIG_VIEW);
			} else if (mDownCache.index == PIX_CTRL_ID) {
				int32				index;
				GlMainImageEntry*	cache = mNodes->EntryAt(mDownCache.id, &index);
				if (cache) NodePressed(index, GL_PIXEL_TARGET_VIEW);
			}
		}
	} else if (mDownCache.code == mDownCache.GL_ON_NODE) {
		int32				index;
		GlMainImageEntry*	cache = mNodes->EntryAt(where, &index);
		gl_id				id = 0;
		if (cache) id = cache->id;
		if (mDownCache.id == id) NodePressed(index, GL_INSPECTOR_VIEW);
	} else if (mDownCache.code == mDownCache.GL_ON_CHAIN) {
		int32				index;
		GlMainLabelEntry*	cache = mLabels->EntryAt(where, &index);
		gl_chain_id			cid = 0;
		if (cache) cid = cache->id;
		if (mDownCache.id == cid) {
			if (mParentId) ChainPressed(index);
		}
	}
	mMouseDownButtons = 0;
	mDownCache.code = mDownCache.GL_ON_NOTHING;
}

void GlMainNavView::SetContext(const GlChain* chain)
{
#if 0
printf("GlMainNavView::SetContext()\n");
if (chain) chain->Print();
#endif
	GlRootRef			ref = mPath.Current();
	ref.RemoveObserver(GlNotifier::CHAIN_CODE, BMessenger(this));
	ref.AddObserver(GlNotifier::CHAIN_CODE, BMessenger(this));

	mParentId = 0;
	gl_chain_id		old = mContextId;
	if (chain) {
		mContextId = chain->Id();
		if (chain->Parent()) mParentId = chain->Parent()->Id();
	} else mContextId = 0;

	if (old != mContextId && Window())
		Window()->PostMessage(GL_RECACHE_MSG, this);
}

void GlMainNavView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

status_t GlMainNavView::NodePressed(int32 index, int32 viewType)
{
	ArpVALIDATE(mNodes, return B_NO_MEMORY);
	ArpVALIDATE(mContextId != 0, return B_ERROR);
	ArpVALIDATE(index >= 0 && index < int32(mNodes->e.size()), return B_ERROR);

	gl_node_id		id = mNodes->e[index]->id;
	if (!id) return  B_ERROR;

	BMessage		msg(GL_INSPECT_NODE_MSG);
	msg.AddPointer(GL_CHAIN_ID_STR, mContextId);
	msg.AddPointer(GL_NODE_ID_STR, id);
	msg.AddInt32(GL_NODE_VIEW_STR, viewType);

	if (Window()) Window()->PostMessage(&msg);
	return B_OK;
}

status_t GlMainNavView::NodeDeleted(gl_node_id nid)
{
	ArpVALIDATE(mContextId, return B_ERROR);
	ArpVALIDATE(mNodes, return B_NO_MEMORY);

	GlRootNode*			root = mPath.Current().WriteLock();
	status_t			err = B_ERROR;
	if (root) {
		GlChain*		chain = root->FindChain(mContextId);
		if (chain) err = chain->RemoveNode(nid);
	}
	mPath.Current().WriteUnlock(root);

	return err;
}

status_t GlMainNavView::ChainPressed(int32 index)
{
	ArpVALIDATE(mParentId, return B_ERROR);
	ArpVALIDATE(mLabels, return B_NO_MEMORY);
	ArpVALIDATE(index >= 0 && index < int32(mLabels->e.size()), return B_ERROR);

	gl_chain_id		cid = mLabels->e[index]->id;
	if (!cid) return  B_ERROR;

	status_t		err = mPath.SetChain(mParentId, cid);
	if (err != B_OK) return err;

	if (Window()) Window()->PostMessage(GL_PATH_CHANGED_MSG, this);
	return B_OK;
}

status_t GlMainNavView::AddOnDropped(gl_node_add_on_id id, BPoint pt)
{
	int32				index, action;
	status_t			err = GetDropAction(pt, &index, &action);
	if (err != B_OK) return err;

	const GlNodeAddOn*	addon = GlGlobals().GetAddOn(id);
	if (!addon) return B_ERROR;
	GlNode*				n = addon->NewInstance(0);
	if (!n) return B_ERROR;
	n->IncRefs();
	
	GlRootNode*			root = mPath.Current().WriteLock();
	err = B_ERROR;
	if (root) {
		GlChain*		chain = root->FindChain(mContextId);
		if (chain) {
			if (action == BEFORE_ACTION) err = chain->InsertNode(n, index);
			else if (action == AFTER_ACTION) err = chain->InsertNode(n, index + 1);
			else if (action == REPLACE_ACTION) err = chain->ReplaceNode(n, index);
		}
	}
	mPath.Current().WriteUnlock(root);
	n->DecRefs();

	return err;
}

status_t GlMainNavView::PathDown(gl_chain_id cid, gl_node_id nid)
{
	const GlRootNode*			root = mPath.Current().ReadLock();
	status_t					err = B_ERROR;
	if (root) {
		const GlNode*			node = root->FindNode(cid, nid);
		if (node) {
			gl_chain_id			cid = 0;
			uint32				chainSize = node->ChainSize();
			if (chainSize >= 1) {
				const GlChain*	chain = node->ChainAt(0);
				if (chain) cid = chain->Id();
			}
			err = mPath.AddNode(node->Id(), cid, chainSize);
		}
	}
	mPath.Current().ReadUnlock(root);
	return err;
}

void GlMainNavView::DrawOn(BRect clip, BView* v)
{
	ArpVALIDATE(mNodes, return);
	v->SetLowColor(Prefs().GetColor(ARP_BG_C));
	v->SetHighColor(Prefs().GetColor(ARP_BG_C));
	v->FillRect(clip);

	drawing_mode		mode = v->DrawingMode();
	v->SetDrawingMode(B_OP_ALPHA);
	v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	if (mLabels) mLabels->DrawOn(clip, v, mContextId);

	for (uint32 k = 0; k < mNodes->e.size(); k++) {
		if (mNodes->e[k] && mNodes->e[k]->image) {
			v->DrawBitmap(mNodes->e[k]->image->mBitmap, mNodes->e[k]->frame.LeftTop());
		}
	}
	v->SetDrawingMode(mode);

	mControls.DrawOn(clip, v);
}

status_t GlMainNavView::Recache()
{
	ArpVALIDATE(mNodes, return B_NO_MEMORY);
	mNodes->DeleteCache();
	ArpVALIDATE(mContextId, return B_ERROR);
	BRect							b = Bounds();
	float							padX = float(Prefs().GetInt32(ARP_PADX)),
									padY = float(Prefs().GetInt32(ARP_PADY)),
									nodeY = float(Prefs().GetInt32(GL_NODE_IMAGE_Y));
	BPoint							origin(padX, b.bottom - padY - nodeY);

	const GlRootNode*				root = mPath.Current().ReadLock();
	if (root) {
//root->Print();
		const GlChain*				chain = root->FindChain(mContextId);
		if (chain) {
			if (mLabels) RecacheLabels(chain->Parent(), BPoint(padX, padY));
			const GlNode*			node;
			for (uint32 k = 0; (node = chain->NodeAt(k)) != 0; k++) {
				const GlNodeAddOn*	addon = node->AddOn();
				const ArpBitmap*	image = node->Image();
				if (addon && image) {
					GlMainImageEntry*	e = new GlMainImageEntry(node->Id(), image, node->ChainSize(), node->Flags());
					if (e) {
						e->label = addon->Label();
						mNodes->e.push_back(e);
						int32			w = e->image->Width(), h = e->image->Height();
						e->frame = BRect(origin.x, origin.y, origin.x + w, origin.y + h);
						origin.x += w + padX;
					}
				}
			}
		}
	}
	mPath.Current().ReadUnlock(root);

	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
	return B_OK;
}

status_t GlMainNavView::RecacheLabels(const GlNode* node, BPoint origin)
{
	ArpVALIDATE(mLabels, return B_NO_MEMORY);
	mLabels->Free();
	if (!node) return B_OK;
	if (node->ChainSize() < 1) return B_OK;
	
	float						fH = arp_get_font_height(this);
	BPoint						pt(origin.x, origin.y + fH);
	
	const GlChain*				chain;
	for (uint32 k = 0; (chain = node->ChainAt(k)) != 0; k++) {
		BString16				str = chain->Label(true);
		if (str.Length() > 0) {
			GlMainLabelEntry*	le = new GlMainLabelEntry(str, chain->Id());
			if (le) {
				float			w = StringWidth(&str);
				le->frame.Set(pt.x, origin.y, pt.x + w, pt.y);
				mLabels->e.push_back(le);
				pt.x += w + 10;
			}
		}
	}
	return B_OK;
}

status_t GlMainNavView::CacheControl(BPoint& pt, GlNavControl& cache)
{
	cache.code = cache.GL_ON_NOTHING;

	int32						i32;
	if (mControls.IsEnabled() && mControls.ControlAt(pt, &i32) == B_OK) {
		cache.index = i32;
		cache.code = cache.GL_ON_CONTROL;
		cache.id = mControls.id;
		return B_OK;
	}
	if (!mNodes) return B_NO_MEMORY;
	const GlMainImageEntry*		nc = mNodes->EntryAt(pt, &i32);
	if (nc) {
		cache.index = i32;
		cache.id = nc->id;
		if (cache.id) cache.code = cache.GL_ON_NODE;
		return B_OK;
	}
	if (!mLabels) return B_NO_MEMORY;
	const GlMainLabelEntry*	lc = mLabels->EntryAt(pt, &i32);
	if (lc) {
		cache.index = i32;
		cache.id = lc->id;
		if (cache.id) cache.code = cache.GL_ON_CHAIN;
		return B_OK;
	}
	return B_OK;
}

status_t GlMainNavView::GetDropAction(const BPoint& pt, int32* index, int32* action) const
{
	ArpASSERT(index && action);
	BRect					b = Bounds();
	float					padY = float(Prefs().GetInt32(ARP_PADY)),
							nodeY = float(Prefs().GetInt32(GL_NODE_IMAGE_Y));
	BPoint					origin(0, b.bottom - padY - nodeY);
	if (pt.y < origin.y) return B_ERROR;
	*index = 0;
	*action = AFTER_ACTION;
	ArpVALIDATE(mNodes, return B_OK);
	if (mNodes->e.size() < 1) return  B_OK;
	float					firstFudge = 20 * 0.4;
	
	for (uint32 k = 0; k < mNodes->e.size(); k++) {
		if (pt.x < mNodes->e[k]->frame.left) {
			*index = k;
			*action = BEFORE_ACTION;
			return B_OK;
		} else if (pt.x >= mNodes->e[k]->frame.left
				&& pt.x <= mNodes->e[k]->frame.right) {
			*index = k;
			/* Since the first node is crammed up against the edge,
			 * it gets a little larger insert area.
			 */
			if (k == 0 && pt.x <= mNodes->e[k]->frame.left + firstFudge)
				*action = BEFORE_ACTION;
			else *action = REPLACE_ACTION;
			return B_OK;
		}
	}
	*index = int32(mNodes->e.size() - 1);
	return B_OK;
}
