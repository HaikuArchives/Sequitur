#include <stdio.h>
#include <be/interface/View.h>
#include <be/support/Errors.h>
#include <ArpCore/ArpDebug.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpControlLayer.h>

class _ControlLayerEntry
{
public:
	_ControlLayerEntry(int32 id);
	~_ControlLayerEntry();

	int32				mId;
	BRect				mFrame;
	bool				mEnabled;
	const ArpBitmap*		mExposed;
	const ArpBitmap*		mOver;
	
	void		DrawOn(BView* view);
};

/***************************************************************************
 * ARP-CONTROL-LAYER
 ****************************************************************************/
ArpControlLayer::ArpControlLayer()
		: mFlags(0), mMouseDownId(-1)
{
}

ArpControlLayer::~ArpControlLayer()
{
	for (uint32 k = 0; k < mEntries.size(); k++)
		delete mEntries[k];
	mEntries.resize(0);
}

void ArpControlLayer::SetEnabled(bool on)
{
	if (on) mFlags |= ENABLED_F;
	else mFlags &= ~ENABLED_F;
}

bool ArpControlLayer::IsEnabled() const
{
	return mFlags&ENABLED_F;
}

status_t ArpControlLayer::AddControl(	int32 id, const ArpBitmap* exposed,
										const ArpBitmap* over)
{
	ArpASSERT(!HasEntry(id));
	ArpVALIDATE(id >= 0, return B_ERROR);
	_ControlLayerEntry*		e = new _ControlLayerEntry(id);
	if (!e) return B_NO_MEMORY;
	e->mExposed = exposed;
	e->mOver = over;
	if (exposed) e->mFrame = exposed->Bounds();
	else if (over) e->mFrame = over->Bounds();
	mEntries.push_back(e);
	return B_OK;
}

status_t ArpControlLayer::GetControl(int32 id, bool* outEnabled, BRect* outFrame) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		const _ControlLayerEntry*		e = mEntries[k];
		if (e && e->mId == id) {
			if (outEnabled) *outEnabled = e->mEnabled;
			if (outFrame) *outFrame = e->mFrame;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t ArpControlLayer::ControlAt(const BPoint& pt, int32* outCtrlId,
									BRect* outFrame) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->mEnabled
				&& mEntries[k]->mFrame.Contains(pt)) {
			if (outCtrlId) *outCtrlId = mEntries[k]->mId;
			if (outFrame) *outFrame = mEntries[k]->mFrame;
			return B_OK;
		}
	}
	return B_ERROR;
}

#if 0
	void				DrawOn(ArpPainter& painter, BRect clip);

void ArpControlLayer::DrawOn(ArpPainter& painter, BRect clip)
{
	if (!(mFlags&ENABLED_F)) return;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->mEnabled
				&& clip.Intersects(mEntries[k]->mFrame)) {
			mEntries[k]->DrawOn(painter);
		}
	}
}
#endif

void ArpControlLayer::DrawOn(BRect clip, BView* v)
{
	ArpASSERT(v);
	if (!(mFlags&ENABLED_F)) return;

	drawing_mode		mode = v->DrawingMode();
	v->SetDrawingMode(B_OP_ALPHA);
	v->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->mEnabled
				&& clip.Intersects(mEntries[k]->mFrame)) {
			mEntries[k]->DrawOn(v);
		}
	}
	v->SetDrawingMode(mode);
}

#if 0
status_t ArpControlLayer::SetTarget(const BMessenger& target)
{
	if (mTarget) delete mTarget;
	mTarget = new BMessenger(target);
	if (!mTarget) return B_NO_MEMORY;
	return B_OK;
}
#endif

status_t ArpControlLayer::SetControlProps(int32 id, BPoint origin, bool enabled)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->mId == id) {
//			mEntries[k]->mFrame.MoveTo(origin);
			mEntries[k]->mFrame.OffsetTo(origin);
			mEntries[k]->mEnabled = enabled;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t ArpControlLayer::MouseDown(BPoint where, int32* outId)
{
	mMouseDownId = -1;
	if (!IsEnabled()) return B_ERROR;
	status_t		err;
	int32			ctrlId;
	if ((err = ControlAt(where, &ctrlId)) != B_OK) return err;
	mMouseDownId = ctrlId;
	if (outId) *outId = ctrlId;
	return B_OK;
}


status_t ArpControlLayer::MouseUp(BPoint where, int32* outId)
{
	int32			t = mMouseDownId;
	mMouseDownId = -1;
	if (!IsEnabled()) return B_ERROR;
	status_t		err;
	int32			ctrlId;
	if ((err = ControlAt(where, &ctrlId)) != B_OK) return err;
	if (ctrlId != t) return B_ERROR;
	if (outId) *outId = ctrlId;
	return B_OK;
}

bool ArpControlLayer::HasEntry(int32 id) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->mId == id)
			return true;
	}
	return false;
}

/***************************************************************************
 * _CONTROL-LAYER-ENTRY
 ****************************************************************************/
_ControlLayerEntry::_ControlLayerEntry(int32 id)
		: mId(id), mEnabled(true), mExposed(0), mOver(0)
{
}

_ControlLayerEntry::~_ControlLayerEntry()
{
	mExposed = 0;
	mOver = 0;
}

#if 0
//	void		DrawOn(ArpPainter& painter);
void _ControlLayerEntry::DrawOn(ArpPainter& painter)
{
	if (mExposed) painter.DrawBitmap(*mExposed, mFrame.LeftTop());
}
#endif

void _ControlLayerEntry::DrawOn(BView* v)
{
	if (mExposed && mExposed->mBitmap)
		v->DrawBitmap(mExposed->mBitmap, mFrame.LeftTop());
}
