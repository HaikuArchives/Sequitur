#include <ArpCore/StlVector.h>
#include <StorageKit.h>
#include <support/Autolock.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpPopUpMenu.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ViewTools.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlRootNode.h>
#include <Glasslike/GlApp.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlMainNodesView.h>
#include <Glasslike/GlMainWinLists.h>

/********************************************************
 * _GL-MAIN-NODES-BAR
 ********************************************************/
class _GlMainNodesBar : public GlMainImages
{
public:
	BString16		label;
	
	_GlMainNodesBar(const GlToolBarRef& ref, uint32 io);

	bool			NeedsLayout() const;
	status_t		GetAt(BPoint pt, int32* nodeIndex);

	BPoint			Layout(	const BRect& b, const BView* v,
							BPoint origin, const BPoint& pad,
							float rowH);
	void			DrawOn(const BRect& clip, BView* v);
	status_t		Cache(const GlToolBarRef& ref, uint32 io);

private:
	bool			mNeedsLayout;
	BPoint			mOrigin;
	float			mRowH;
	
	status_t		Cache(const GlToolBar* tb, uint32 io);
};

/********************************************************
 * _GL-MAIN-NODES-CACHE
 * This object creates images to represent each tool bar.
 ********************************************************/
class _GlMainNodesCache
{
public:
	uint32						io;
	gl_chain_id					id;
	vector<_GlMainNodesBar*>	bars;

	_GlMainNodesCache(const GlChain* chain);
	~_GlMainNodesCache();

	bool			NeedsLayout() const;
	status_t		GetAt(	BPoint pt, int32* barIndex,
							int32* nodeIndex);

	status_t		Layout(const BRect& b, const BView* v);
	void			DrawOn(const BRect& clip, BView* v);
	status_t		Cache();

private:
	void			FreeData();
};

/********************************************************
 * _GL-MAIN-NODES-CACHE-MANAGER
 * This object creates images to represent each tool bar.
 * It is essentially a series of pages, one page for
 * each node io, so tool bars are not constantly being
 * constructed when flipping between different ios of
 * nodes.
 ********************************************************/
class _GlMainNodesCacheManager
{
public:
	_GlMainNodesCache*		cache;

	_GlMainNodesCacheManager();
	~_GlMainNodesCacheManager();

	bool		SameContext(const GlChain* chain);
	status_t	InstallContext(	const GlChain* chain, const BRect& b,
								const BView* v);

private:
	vector<_GlMainNodesCache*>	mCaches;
};

/********************************************************
 * GL-MAIN-NODES-VIEW
 ********************************************************/
GlMainNodesView::GlMainNodesView(BRect frame, GlProjectList& path)
		: inherited(frame, "nodes", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mPath(path), mManager(new _GlMainNodesCacheManager()),
		  mDownBarIndex(-1), mDownNodeIndex(-1),
		  mMouseOverBarIndex(-1), mMouseOverNodeIndex(-1)
{
}

GlMainNodesView::~GlMainNodesView()
{
	delete mManager;
}

void GlMainNodesView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case GL_INVALIDATE_MSG:
			this->Invalidate();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void GlMainNodesView::MouseDown(BPoint where)
{
	ArpVALIDATE(mManager, return);
	mDownBarIndex = mDownNodeIndex = -1;
	mDownPt.Set(-1, -1);
	mDownButtons = 0;
		
	if (mManager->cache) {
		mManager->cache->GetAt(where, &mDownBarIndex, &mDownNodeIndex);
		mDownPt = where;
	}

	mDownButtons = arp_get_mouse_buttons(*this);

//	GlMainImageEntry*	cache = mNodes->EntryAt(where);
//	if (cache) mMouseDownId = cache->id;
}

void GlMainNodesView::MouseMoved(	BPoint where, uint32 code,
									const BMessage* dragMessage)
{
	ArpVALIDATE(mManager, return);
	int32				barIndex = -1, nodeIndex = -1;
	if (mManager->cache) mManager->cache->GetAt(where, &barIndex, &nodeIndex);

	if (barIndex != mMouseOverBarIndex || nodeIndex != mMouseOverNodeIndex) {
		BMessage		msg(GL_SET_STATUS_MSG);
		if (barIndex >= 0 && nodeIndex >= 0)
			msg.AddString16(GL_TEXT_STR, mManager->cache->bars[barIndex]->e[nodeIndex]->label);
		if (Window()) Window()->PostMessage(&msg);
		mMouseOverBarIndex = barIndex;
		mMouseOverNodeIndex = nodeIndex;
	}

	/* Start a drag if we aren't already dragging, there's a node
	 * under the mouse, and the mouse has moved enough.
	 */
	if (!dragMessage && mDownButtons && nodeIndex >= 0) {
		int32			buttons = arp_get_mouse_buttons(*this);
		float			delta = fabsf(mDownPt.x - where.x) + fabsf(mDownPt.y - where.y);
		if (buttons && delta > 3) {
			StartDrag(barIndex, nodeIndex);
			mDownButtons = 0;
		}
	}
}

void GlMainNodesView::MouseUp(BPoint where)
{
	ArpVALIDATE(mManager, return);
	if (mManager->cache && mDownNodeIndex >= 0) {
		int32				barIndex, nodeIndex;
		if (mManager->cache->GetAt(where, &barIndex, &nodeIndex) == B_OK
				&& mDownBarIndex == barIndex
				&& mDownNodeIndex == nodeIndex) {
			NodePressed(barIndex, nodeIndex);
		}
	}
	mDownBarIndex = mDownNodeIndex = -1;
	mDownPt.Set(-1, -1);
	mDownButtons = 0;
}

void GlMainNodesView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void GlMainNodesView::SetContext(const GlChain* chain)
{
#if 0
printf("GlMainNodesView::SetContext()\n");
if (chain) chain->Print();
#endif
	ArpVALIDATE(mManager, return);
	if (mManager->SameContext(chain)) {
		if (chain && mManager->cache) mManager->cache->id = chain->Id();
		return;
	}
	status_t			err = mManager->InstallContext(chain, Bounds(), this);
	if (err != B_OK) return;
		
	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
}

status_t GlMainNodesView::NodePressed(int32 barIndex, int32 nodeIndex)
{
	ArpVALIDATE(mManager, return B_NO_MEMORY);
	ArpVALIDATE(mManager->cache, return B_ERROR);
	_GlMainNodesCache*		c = mManager->cache;
	if (c->id == 0) return B_ERROR;
	ArpVALIDATE(barIndex >= 0 && barIndex < int32(c->bars.size()), return B_ERROR);
	_GlMainNodesBar*		b = c->bars[barIndex];
	if (b == 0) return B_ERROR;
	ArpVALIDATE(nodeIndex >= 0 && nodeIndex < int32(b->e.size()), return B_ERROR);
	if (b->e[nodeIndex]->id == 0) return B_ERROR;
	const GlNodeAddOn*	addon = GlGlobals().GetAddOn(b->e[nodeIndex]->id);
	if (!addon) return B_ERROR;

	GlRootNode*			root = mPath.Current().WriteLock();
	status_t			err = B_ERROR;
	if (root) {
		GlChain*		chain = root->FindChain(c->id);
		if (chain) {
			GlNode*		node = addon->NewInstance(0);
			if (node) err = chain->AddNode(node);
		}
	}
	mPath.Current().WriteUnlock(root);

	return err;
}


status_t GlMainNodesView::StartDrag(int32 barIndex, int32 nodeIndex)
{
	GlMainImageEntry*	e = EntryAt(barIndex, nodeIndex);
	if (!e) return B_ERROR;

	BBitmap*			img = 0;
	if (e->image && e->image->mBitmap) img = new BBitmap(e->image->mBitmap);
	ArpASSERT(e->id);
	
//	BMessage			archive;
//	status_t			err = filter->Archive(&archive, 0);
	
	status_t			err = B_OK;
	BMessage			msg(GL_NODE_ADDON_DRAG_MSG);
//	if (err == B_OK) err = msg.AddMessage(SZ_FILTER_ARCHIVE, &archive);
	if (err == B_OK) err = msg.AddPointer(GL_NODE_ADDON_ID_STR, e->id);
//	if (err == B_OK) err = msg.AddInt32("buttons", buttons);

	if (err != B_OK) {
		delete img;
		return err;
	}
	if (!img) {
		int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
						h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
		DragMessage(&msg, BRect(0, 0, float(w), float(h)) );
	} else {
		BPoint			pt(img->Bounds().Width() / 2, img->Bounds().Height() / 2);
		DragMessage(&msg, img, B_OP_BLEND, pt);
	}
	return B_OK;
}

void GlMainNodesView::DrawOn(BRect clip, BView* v)
{
	ArpVALIDATE(mManager, return);
	v->SetLowColor(ViewColor());
	v->SetHighColor(ViewColor());
	v->FillRect(clip);

	drawing_mode		mode = v->DrawingMode();
	v->SetDrawingMode(B_OP_ALPHA);
	v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	if (mManager->cache) mManager->cache->DrawOn(clip, v);

	v->SetDrawingMode(mode);
}

GlMainImageEntry* GlMainNodesView::EntryAt(int32 barIndex, int32 nodeIndex)
{
	ArpVALIDATE(mManager, return 0);
	ArpVALIDATE(mManager->cache, return 0);
	_GlMainNodesCache*		c = mManager->cache;
	if (c->id == 0) return 0;
	ArpVALIDATE(barIndex >= 0 && barIndex < int32(c->bars.size()), return 0);
	_GlMainNodesBar*		b = c->bars[barIndex];
	if (!b) return 0;
	ArpVALIDATE(nodeIndex >= 0 && nodeIndex < int32(b->e.size()), return 0);
	return b->e[nodeIndex];
}

// #pragma mark -

/********************************************************
 * _GL-MAIN-NODES-BAR
 ********************************************************/
_GlMainNodesBar::_GlMainNodesBar(const GlToolBarRef& ref, uint32 io)
		: mNeedsLayout(false)
{
	Cache(ref, io);
}

bool _GlMainNodesBar::NeedsLayout() const
{
	return mNeedsLayout;
}

BPoint _GlMainNodesBar::Layout(	const BRect& b, const BView* v,
								BPoint origin, const BPoint& pad,
								float rowH)
{
	mOrigin = origin;
	mRowH = rowH;
//	float			lW = arp_get_string_width(v, label);
	float			lW = v->StringWidth(&label);
	origin.x += lW + 8;

	/* I need to fit at least 2 nodes and my label, or else I drop
	 * to the next row.
	 */
//	uint32			c = 0;
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k] && e[k]->image) {
			int32		w = e[k]->image->Width(), h = e[k]->image->Height();
			e[k]->frame = BRect(origin.x, origin.y + 2, origin.x + w, origin.y + 2 + h);
			origin.x += w + pad.x;
		}
	}
	return origin;
}

status_t _GlMainNodesBar::GetAt(BPoint pt, int32* nodeIndex)
{
	if (EntryAt(pt, nodeIndex) == 0) return B_ERROR;
	return B_OK;
}

void _GlMainNodesBar::DrawOn(const BRect& clip, BView* v)
{
	if (label.Length() > 0) {
		v->SetHighColor(0, 0, 0);
		v->DrawString(label.String(), BPoint(mOrigin.x + 2, mOrigin.y + mRowH - 2));
	}
	
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k] && e[k]->image)
			v->DrawBitmap(e[k]->image->mBitmap, e[k]->frame.LeftTop());
	}
}

status_t _GlMainNodesBar::Cache(const GlToolBarRef& ref, uint32 io)
{
	const GlToolBar*		tb = ref.ReadLock();
	if (!tb) return B_ERROR;
	status_t				err = Cache(tb, io);
	ref.ReadUnlock(tb);
	return err;
}

status_t _GlMainNodesBar::Cache(const GlToolBar* tb, uint32 barIo)
{
	ArpASSERT(tb);
	mNeedsLayout = true;
	label = tb->Label();

	DeleteCache();
	BString16					creator;
	int32						key;
	for (uint32 k = 0; tb->GetTool(k, creator, &key) == B_OK; k++) {
		const GlNodeAddOn*		addon = GlGlobals().GetAddOn(creator, key);
		if (addon) {
			gl_node_add_on_id	id = addon->Id();
			uint32				io = addon->Io();
			const ArpBitmap*	img = addon->Image();
			BString16			label = addon->Label();
			GlMainImageEntry*	entry;
			if (io&barIo && img && (entry = new GlMainImageEntry(id, img, 0, 0, &label)) != 0)
				e.push_back(entry);
		}
	}
	return B_OK;
}

// #pragma mark -

/********************************************************
 * _GL-MAIN-NODES-CACHE
 ********************************************************/
_GlMainNodesCache::_GlMainNodesCache(const GlChain* chain)
		: io(0), id(0)
{
	ArpVALIDATE(chain, return);
	io = chain->Io();
	id = chain->Id();
}

_GlMainNodesCache::~_GlMainNodesCache()
{
	FreeData();
}

bool _GlMainNodesCache::NeedsLayout() const
{
	for (uint32 k = 0; k < bars.size(); k++) {
		if (bars[k]->NeedsLayout()) return true;
	}
	return false;
}

status_t _GlMainNodesCache::GetAt(	BPoint pt, int32* barIndex,
									int32* nodeIndex)
{
	for (uint32 k = 0; k < bars.size(); k++) {
		if (bars[k]->GetAt(pt, nodeIndex) == B_OK) {
			if (barIndex) *barIndex = k;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t _GlMainNodesCache::Layout(const BRect& b, const BView* v)
{
	float					padX = float(Prefs().GetInt32(ARP_PADX)),
							padY = float(Prefs().GetInt32(ARP_PADY));
	BPoint					pad(padX, padY), origin(padX, padY);

	float					rowH = arp_get_font_height(v) + 4;
	int32					h = Prefs().GetInt32(GL_NODE_IMAGE_Y) + 4;
	if (h > rowH) rowH = float(h);
//debugger("Sdsds");
	for (uint32 k = 0; k < bars.size(); k++) {
		origin = bars[k]->Layout(b, v, origin, pad, rowH);
	}
	return B_OK;
}

void _GlMainNodesCache::DrawOn(const BRect& clip, BView* v)
{
	for (uint32 k = 0; k < bars.size(); k++) {
		if (bars[k]) bars[k]->DrawOn(clip, v);
	}
}

status_t _GlMainNodesCache::Cache()
{
	ArpVALIDATE(io > 0, return B_ERROR);
	ArpVALIDATE(gl_app, return B_ERROR);
	FreeData();
	GlToolBarRoster&		roster = gl_app->ToolBars();

	/* Step through the roster's tool bars and my cached ones,
	 * and impose the roster one on my cache, generating a new
	 * bitmap if necessary.
	 */
	uint32					i = 0;
	GlToolBarRef			ref;
	for (uint32 k = 0; (ref = roster.GetToolBar(io, k)).IsValid(); k++) {
		if (i < bars.size()) bars[i]->Cache(ref, io);
		else {
			_GlMainNodesBar*	bar = new _GlMainNodesBar(ref, io);
			if (bar) bars.push_back(bar);
		}
		i++;
	}

	/* FIX: Remove excess bars, if any.
	 */

	return B_OK;
}

void _GlMainNodesCache::FreeData()
{
	for (uint32 k = 0; k < bars.size(); k++) delete bars[k];
	bars.resize(0);
}

// #pragma mark -

/********************************************************
 * _GL-MAIN-NODES-CACHE-MANAGER
 ********************************************************/
_GlMainNodesCacheManager::_GlMainNodesCacheManager()
		: cache(0)
{
}

_GlMainNodesCacheManager::~_GlMainNodesCacheManager()
{
	for (uint32 k = 0; k < mCaches.size(); k++) delete mCaches[k];
	mCaches.resize(0);
}

bool _GlMainNodesCacheManager::SameContext(const GlChain* chain)
{
	if (!chain && !cache) return true;
	else if (!chain && cache) return false;
	else if (chain && !cache) return false;
	else return (chain->Io() == cache->io);
}

status_t _GlMainNodesCacheManager::InstallContext(	const GlChain* chain,
													const BRect& b,
													const BView* v)
{
	if (!chain) {
		cache = 0;
		return B_OK;
	}
	uint32				io = chain->Io();
	for (uint32 k = 0; k < mCaches.size(); k++) {
		if (io == mCaches[k]->io) {
			cache = mCaches[k];
			return B_OK;
		}
	}
	
	_GlMainNodesCache*	c = new _GlMainNodesCache(chain);
	if (!c) return B_NO_MEMORY;
	mCaches.push_back(c);
	cache = c;

	cache->Cache();
	if (cache->NeedsLayout()) cache->Layout(b, v);

	return B_OK;
}
