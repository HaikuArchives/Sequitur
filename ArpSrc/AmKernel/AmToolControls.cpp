/* AmToolControls.cpp
*/
#define _BUILDING_AmKernel 1

#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmFilterI.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolControls.h"

static const BBitmap*	gLrOn = NULL;
static const BBitmap*	gLrOff = NULL;

static const char*		TARGET_PIPELINE_STR		= "target_pipeline";
static const char*		TARGET_FILTER_STR		= "target_filter";
static const char*		TARGET_NAME_STR			= "target_name";
static const char*		VISUAL_STR				= "visual";
static const char*		ON_IMAGE_STR			= "on_image";
static const char*		OFF_IMAGE_STR			= "off_image";
static const char*		CONTROL_STR				= "control";

static const uint32		TEXT_VIS_FLATTENED		= 'itxv';
static const uint32		BITMAP_VIS_FLATTENED	= 'ibmv';

static BRect invalid_rect()
{
	return BRect(-1, -1, -1, -1);
}

static bool is_valid(const BRect& r)
{
	return r.left != -1 && r.top != -1 && r.right != -1 && r.bottom != -1;
}

static BRect merge_rects(BRect r1, BRect r2)
{
	if (!is_valid(r1)) return r2;
	else if (!is_valid(r2)) return r1;
	else return r1 | r2;
}

/***************************************************************************
 * _AM-TC-TARGET
 ****************************************************************************/
class _AmTcTarget
{
public:
	int32			mPipeline;
	int32			mFilter;
	BString			mName;

	_AmTcTarget();
	_AmTcTarget(int32 pipeline, int32 filterId, const BString& name);
	_AmTcTarget(const BMessage& flattened, int32 k);
	_AmTcTarget(const _AmTcTarget& o);

	_AmTcTarget&	operator=(const _AmTcTarget &o);

	bool			IsValid() const;

	status_t		SendBool(AmPipelineMatrixI* matrix, bool value);
	status_t		SendInt32(AmPipelineMatrixI* matrix, int32 value);

	status_t		WriteTo(BMessage& flattened) const;

	void			Print(uint32 tabs = 0) const;

private:
	status_t		Send(AmPipelineMatrixI* matrix, BMessage* config);
};

/***************************************************************************
 * AM-TOOL-CONTROL
 ****************************************************************************/
AmToolControl::AmToolControl()
		: mType(BOOL_TYPE), mMaxAlpha(140), mMinAlpha(30), mAlpha(127),
		  mScaledPt(0.5, 0.5), mActive(true), mMouseHit(false),
		  mVisual(NULL),
		  mOnImage(NULL), mOffImage(NULL), mCurImage(NULL)
{
	if (!gLrOn) gLrOn = ImageManager().FindBitmap("Tool Control - LR On");
	if (!gLrOff) gLrOff = ImageManager().FindBitmap("Tool Control - LR Off");

	mCurImage = mOnImage = gLrOn;
	mOffImage = gLrOff;

	mVisual = new AmToolControlBitmapVis(gLrOn, gLrOff);
}

#if 0
//	AmToolControl(AmToolRef toolRef, BPoint scaledPt, uint8 alpha = 127);
AmToolControl::AmToolControl(AmToolRef toolRef, BPoint scaledPt, uint8 alpha)
		: mType(BOOL_TYPE), mToolRef(toolRef), mAlpha(alpha),
		  mScaledPt(scaledPt), mActive(true), mMouseHit(false),
		  mVisual(NULL),
		  mOnImage(NULL), mOffImage(NULL), mCurImage(NULL)
{
	if (!gLrOn) gLrOn = ImageManager().FindBitmap("Tool Control - LR On");
	if (!gLrOff) gLrOff = ImageManager().FindBitmap("Tool Control - LR Off");

	mCurImage = mOnImage = gLrOn;
	mOffImage = gLrOff;

	mVisual = new AmToolControlBitmapVis(gLrOn, gLrOff);
}
#endif

AmToolControl::AmToolControl(const BMessage& flattened)
		: mType(BOOL_TYPE), mMaxAlpha(140), mMinAlpha(30), mAlpha(127),
		  mScaledPt(0.5, 0.5), mActive(true), mMouseHit(false),
		  mVisual(NULL),
		  mOnImage(NULL), mOffImage(NULL), mCurImage(NULL)
{
	for (int32 k = 0; true; k++) {
		_AmTcTarget		target(flattened, k);
		if (target.IsValid() ) mTargets.push_back(target);
		else break;
	}
	BMessage		msg;
	if (flattened.FindMessage(VISUAL_STR, &msg) == B_OK)
		mVisual = AmToolControlVis::Instantiate(msg);
}

AmToolControl::AmToolControl(const AmToolControl& o)
		: mType(o.mType), mToolRef(o.mToolRef),
		  mMaxAlpha(o.mMaxAlpha), mMinAlpha(o.mMinAlpha), mAlpha(o.mAlpha),
		  mScaledPt(o.mScaledPt), mActive(o.mActive),
		  mMouseHit(o.mMouseHit), mVisual(NULL),
		  mOnImage(NULL), mOffImage(NULL), mCurImage(NULL)
{
	if (!gLrOn) gLrOn = ImageManager().FindBitmap("Tool Control - LR On");
	if (!gLrOff) gLrOff = ImageManager().FindBitmap("Tool Control - LR Off");

	if (o.mVisual) mVisual = o.mVisual->Copy();

	mCurImage = mOnImage = gLrOn;
	mOffImage = gLrOff;

	for (uint32 k = 0; k < o.mTargets.size(); k++) {
		mTargets.push_back( o.mTargets[k] );
	}
}

AmToolControl::~AmToolControl()
{
	delete mVisual;
}

AmToolControl& AmToolControl::operator=(const AmToolControl &o)
{
	mAlpha = o.mAlpha;
	mToolRef = o.mToolRef;
	mScaledPt = o.mScaledPt;
	mActive = o.mActive;
	mMouseHit = o.mMouseHit;
	if (o.mVisual) mVisual = o.mVisual->Copy();
	else mVisual = NULL;
	return *this;
}

BRect AmToolControl::Frame() const
{
	if (mVisual) return mVisual->Frame();
	else return invalid_rect();
}

void AmToolControl::SetTool(const AmTool* tool)
{
	mToolRef.SetTo(tool);
}

void AmToolControl::SetVisual(AmToolControlVis* visual)
{
	ArpASSERT(visual);
	delete mVisual;
	if (visual) mVisual = visual->Copy();
	else mVisual = NULL;
}

status_t AmToolControl::GetTarget(	uint32 index, int32* outPipeline,
									int32* outFilter, BString* outName) const
{
	if (index >= mTargets.size() ) return B_BAD_INDEX;
	*outPipeline = mTargets[index].mPipeline;
	*outFilter = mTargets[index].mFilter;
	*outName = mTargets[index].mName;
	return B_OK;
}

void AmToolControl::AddTarget(	int32 pipeline, int32 filter,
								const BString& name)
{
	mTargets.push_back( _AmTcTarget(pipeline, filter, name) );
}

status_t AmToolControl::RemoveTarget(uint32 index)
{
	if (index >= mTargets.size() ) return B_BAD_INDEX;
	mTargets.erase(mTargets.begin() + index);
	return B_OK;
}

void AmToolControl::SetFrame(BView* view, BRect containingFrame, BPoint where)
{
	if (mVisual) mVisual->SetFrame(view, containingFrame, mScaledPt);
	mAlpha = AlphaForPoint(where, view->Bounds() );
}

void AmToolControl::DrawOn(BView* view, BRect clip)
{
	if (mVisual) mVisual->DrawOn(view, clip, mAlpha);
}

BRect AmToolControl::MouseMoved(BView* view, BPoint where)
{
	if (!mVisual) return invalid_rect();
	BRect		f = mVisual->Frame();
	if (f.Contains(where)) {
		if (mMouseHit) return invalid_rect();
		mMouseHit = true;
		return DoHit();
	} else if (mMouseHit) {
		mMouseHit = false;
	}

	if (mMouseHit && f.Contains(where)) return invalid_rect();
	uint8	alpha = AlphaForPoint(where, view->Bounds() );
	if (alpha == mAlpha) return invalid_rect();
	mAlpha = alpha;
	return f;	
#if 0
	bool		contains = false;
	if (mMouseHit && (contains = f.Contains(where)) == true)
		return invalid_rect();
		
	float	xd = (where.x - f.left);
	float	yd = (where.y - f.top);
	float	div = view->Bounds().Width() + view->Bounds().Height();
	float	dist = ((xd * xd) + (yd * yd)) / (div / 4);

	int32		max = 140, min = 30;

	int32	alpha = (int32)(max - dist);
	if (alpha > max) alpha = max;
	if (alpha < min) alpha = min;
	if (alpha == mAlpha) return invalid_rect();
	mAlpha = alpha;
#endif
	return f;
}

status_t AmToolControl::WriteTo(BMessage& flattened) const
{
	status_t	err;
	if (mVisual) {
		BMessage	msg;
		if ((err = mVisual->WriteTo(msg)) != B_OK) return err;
		if ((err = flattened.AddMessage(VISUAL_STR, &msg)) != B_OK) return err;
	}
	for (uint32 k = 0; k < mTargets.size(); k++) {
		if ((err = mTargets[k].WriteTo(flattened)) != B_OK) return err;
	}
	return B_OK;
}

uint32 AmToolControl::Type() const
{
	return mType;
}

void AmToolControl::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	BRect	f = Frame();
	printf("AmToolControl l: %f t: %f r: %f b: %f (%p), targets:\n", f.left, f.top, f.right, f.bottom, this);
	for (uint32 k = 0; k < mTargets.size(); k++) {
		mTargets[k].Print(tabs + 1);
	}
}

BRect AmToolControl::DoHit()
{
	if (mActive) mActive = false; else mActive = true;
	if (mVisual) mVisual->SetActive(mActive);
	SendValueToTargets();
	return Frame();
}

uint8 AmToolControl::AlphaForPoint(BPoint where, BRect bounds) const
{
	BRect		f = Frame();
	if (f.Contains(where)) return mMaxAlpha;
		
	float	xd = (where.x - f.left);
	float	yd = (where.y - f.top);
	float	div = bounds.Width() + bounds.Height();
	float	dist = ((xd * xd) + (yd * yd)) / (div / 4);

	int32	alpha = (int32)(mMaxAlpha - dist);
	if (alpha > mMaxAlpha) alpha = mMaxAlpha;
	if (alpha < mMinAlpha) alpha = mMinAlpha;
	return alpha;
}

status_t AmToolControl::SendValueToTargets()
{
	ArpASSERT(mType != NO_TYPE);
	if (mType == NO_TYPE) return B_ERROR;
	// WRITE TOOL BLOCK
	AmTool*		tool = mToolRef.WriteLock();
	if (tool) {
		for (uint32 k = 0; k < mTargets.size(); k++) {
			if (mType == BOOL_TYPE) {
				mTargets[k].SendBool(tool, mActive);
			}
		}
	}
	mToolRef.WriteUnlock(tool);
	// END WRITE TOOL BLOCK
	return B_OK;
}

/***************************************************************************
 * AM-TOOL-CONTROL-LIST
 ****************************************************************************/
AmToolControlList::AmToolControlList()
{
}

AmToolControlList::AmToolControlList(const BMessage& flattened)
{
	BMessage		controlMsg;
	for (int32 k = 0; flattened.FindMessage(CONTROL_STR, k, &controlMsg) == B_OK; k++) {
		AmToolControl*	ctrl = new AmToolControl(controlMsg);
		if (ctrl) mControls.push_back(ctrl);
		controlMsg.MakeEmpty();
	}
}

AmToolControlList::AmToolControlList(const AmToolControlList& o)
{
	for (uint32 k = 0; k < o.mControls.size(); k++) {
		AmToolControl*	ctrl = new AmToolControl( *(o.mControls[k]) );
		if (ctrl) mControls.push_back(ctrl);
	}
}

AmToolControlList::~AmToolControlList()
{
	for (uint32 k = 0; k < mControls.size(); k++)
		delete mControls[k];
	mControls.resize(0);
}

void AmToolControlList::SetTool(const AmTool* tool)
{
	for (uint32 k = 0; k < mControls.size(); k++)
		mControls[k]->SetTool(tool);
}

status_t AmToolControlList::AddControl(AmToolControl* ctrl)
{
	if (!ctrl) return B_NO_MEMORY;
	mControls.push_back(ctrl);
	return B_OK;
}

AmToolControl* AmToolControlList::RemoveControl(uint32 index)
{
	if (index >= mControls.size() ) return NULL;
	AmToolControl*	ctrl = mControls[index];
	mControls.erase(mControls.begin() + index);
	return ctrl;
}

status_t AmToolControlList::GetControl(uint32 index, AmToolControl** control) const
{
	if (index >= mControls.size() ) return B_BAD_INDEX;
	*control = mControls[index];
	return B_OK;
}

status_t AmToolControlList::GetControl(void* toolControlId, AmToolControl** control) const
{
	for (uint32 k = 0; k < mControls.size(); k++) {
		if (mControls[k] == toolControlId) {
			*control = mControls[k];
			return B_OK;
		}
	}
	return B_ERROR;
}

void AmToolControlList::DrawOn(BView* view, BRect clip)
{
	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	for (uint32 k = 0; k < mControls.size(); k++)
		mControls[k]->DrawOn(view, clip);	

	view->SetDrawingMode(mode);
}

void AmToolControlList::MouseMoved(BView* view, BPoint where)
{
	BRect		r = invalid_rect();
	for (uint32 k = 0; k < mControls.size(); k++) {
		r = merge_rects(r, mControls[k]->MouseMoved(view, where) );
	}
	if (is_valid(r) ) view->Invalidate(r);
}

void AmToolControlList::Prepare(BView* view, BRect containingFrame, BPoint where)
{
	for (uint32 k = 0; k < mControls.size(); k++)
		mControls[k]->SetFrame(view, containingFrame, where);
}

void AmToolControlList::MouseCleanup(BView* view)
{
	if (mControls.size() < 1) return;
	BRect		r(mControls[0]->Frame() );
	for (uint32 k = 1; k < mControls.size(); k++) r = r | mControls[k]->Frame();
	view->Invalidate(r);
}

AmToolControlList* AmToolControlList::Copy() const
{
	return new AmToolControlList(*this);
}

status_t AmToolControlList::WriteTo(BMessage& flattened) const
{
	for (uint32 k = 0; k < mControls.size(); k++) {
		BMessage	controlMsg;
		status_t	err = mControls[k]->WriteTo(controlMsg);
		if (err != B_OK) return err;
		if ((err = flattened.AddMessage(CONTROL_STR, &controlMsg)) != B_OK) return err;
	}
	return B_OK;
}

void AmToolControlList::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("AmToolControlList controls:\n");
	for (uint32 k = 0; k < mControls.size(); k++) {
		mControls[k]->Print(tabs + 1);
	}
}

/***************************************************************************
 * AM-TOOL-CONTROL-VIS
 ****************************************************************************/
AmToolControlVis* AmToolControlVis::Instantiate(BMessage& flattened)
{
	if (flattened.what == TEXT_VIS_FLATTENED)
		return NULL;
	else if (flattened.what == BITMAP_VIS_FLATTENED)
		return new AmToolControlBitmapVis(flattened);
	else return NULL;
}

BRect AmToolControlVis::Frame() const
{
	return mFrame;
}

void AmToolControlVis::SetActive(bool active)
{
	mActive = active;
}

AmToolControlVis::AmToolControlVis()
{
	mFrame = invalid_rect();
	mActive = true;
}

AmToolControlVis::AmToolControlVis(const AmToolControlVis& o)
		: mFrame(o.mFrame), mActive(o.mActive)
{
}

/***************************************************************************
 * AM-TOOL-CONTROL-TEXT-VIS
 ****************************************************************************/
AmToolControlTextVis::AmToolControlTextVis(	const char* onText,
											const char* offText)
		: mOnText(onText), mOffText(offText)
{
}

AmToolControlTextVis::AmToolControlTextVis(const AmToolControlTextVis& o)
		: inherited(o), mOnText(o.mOnText), mOffText(o.mOffText)
{
}

void AmToolControlTextVis::SetFrame(BView* view, BRect containingFrame, BPoint position)
{
}

void AmToolControlTextVis::DrawOn(BView* view, BRect clip, uint8 alpha)
{
	if (!is_valid(mFrame) ) return;

	view->SetHighColor(0, 0, 0, alpha);
	if (mActive) view->DrawString(mOnText.String(), mFrame.LeftBottom() );
	else view->DrawString(mOffText.String(), mFrame.LeftBottom() );
}

AmToolControlVis* AmToolControlTextVis::Copy() const
{
	return new AmToolControlTextVis(*this);
}

status_t AmToolControlTextVis::WriteTo(BMessage& flattened) const
{
	flattened.what = TEXT_VIS_FLATTENED;
#if 0
	status_t	err;
	if (mOnImage) {
		BMessage	msg;
		if ((err = mOnImage->Archive(&msg)) != B_OK) return err;
		if ((err = flattened.AddMessage(ON_IMAGE_STR, &msg)) != B_OK) return err;
	}
	if (mOffImage) {
		BMessage	msg;
		if ((err = mOffImage->Archive(&msg)) != B_OK) return err;
		if ((err = flattened.AddMessage(OFF_IMAGE_STR, &msg)) != B_OK) return err;
	}
#endif
	return B_OK;
}

/***************************************************************************
 * AM-TOOL-CONTROL-BITMAP-VIS
 ****************************************************************************/
AmToolControlBitmapVis::AmToolControlBitmapVis(	const BBitmap* activeBm,
												const BBitmap* inactiveBm)
		: mActiveBm(NULL), mInactiveBm(NULL)
{
	if (activeBm) mActiveBm = new BBitmap(activeBm);
	if (inactiveBm) mInactiveBm = new BBitmap(inactiveBm);

	if (mActiveBm) mBounds = mActiveBm->Bounds();
	else mBounds.Set(0, 0, 30, 12);
}

AmToolControlBitmapVis::AmToolControlBitmapVis(const BMessage& flattened)
		: mActiveBm(NULL), mInactiveBm(NULL)
{
	BMessage	msg;
	if (flattened.FindMessage(ON_IMAGE_STR, &msg) == B_OK) {
		mActiveBm = dynamic_cast<BBitmap*>( BBitmap::Instantiate(&msg) );
	}
	msg.MakeEmpty();
	if (flattened.FindMessage(OFF_IMAGE_STR, &msg) == B_OK)
		mInactiveBm = dynamic_cast<BBitmap*>( BBitmap::Instantiate(&msg) );

	if (mActiveBm) mBounds = mActiveBm->Bounds();
	else mBounds.Set(0, 0, 30, 12);
}

AmToolControlBitmapVis::AmToolControlBitmapVis(const AmToolControlBitmapVis& o)
		: inherited(o), mActiveBm(NULL), mInactiveBm(NULL)
{
	if (o.mActiveBm) mActiveBm = new BBitmap(o.mActiveBm);
	if (o.mInactiveBm) mInactiveBm = new BBitmap(o.mInactiveBm);

	if (mActiveBm) mBounds = mActiveBm->Bounds();
	else mBounds.Set(0, 0, 30, 12);
}

AmToolControlBitmapVis::~AmToolControlBitmapVis()
{
	delete mActiveBm;
	delete mInactiveBm;
}

void AmToolControlBitmapVis::SetFrame(BView* view, BRect containingFrame, BPoint position)
{
	mFrame.left = containingFrame.left + (containingFrame.Width() * position.x) - mBounds.Width();
	mFrame.top = containingFrame.top + (containingFrame.Height() * position.y) - mBounds.Height();
	mFrame.right = mFrame.left + mBounds.Width();
	mFrame.bottom = mFrame.top + mBounds.Height();
}

void AmToolControlBitmapVis::DrawOn(BView* view, BRect clip, uint8 alpha)
{
	if (!is_valid(mFrame) ) return;

	BBitmap*		im = (mActive) ? mActiveBm : mInactiveBm;
	if (im) {
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		view->SetHighColor(0, 0, 0, alpha);
		view->DrawBitmapAsync(im, mFrame.LeftTop() );
	} else {
		view->SetHighColor(255, 0, 150, alpha);
		view->FillRect(mFrame);
	}
}

AmToolControlVis* AmToolControlBitmapVis::Copy() const
{
	return new AmToolControlBitmapVis(*this);
}

status_t AmToolControlBitmapVis::WriteTo(BMessage& flattened) const
{
	flattened.what = BITMAP_VIS_FLATTENED;
	status_t	err;
	if (mActiveBm) {
		BMessage	msg;
		if ((err = mActiveBm->Archive(&msg)) != B_OK) return err;
		if ((err = flattened.AddMessage(ON_IMAGE_STR, &msg)) != B_OK) return err;
	}
	if (mInactiveBm) {
		BMessage	msg;
		if ((err = mInactiveBm->Archive(&msg)) != B_OK) return err;
		if ((err = flattened.AddMessage(OFF_IMAGE_STR, &msg)) != B_OK) return err;
	}
	return B_OK;
}

/***************************************************************************
 * _AM-TC-TARGET
 ****************************************************************************/
_AmTcTarget::_AmTcTarget()
		: mPipeline(-1), mFilter(-1)
{
}

_AmTcTarget::_AmTcTarget(int32 pipeline, int32 filter, const BString& name)
		: mPipeline(pipeline), mFilter(filter), mName(name)
{
}

_AmTcTarget::_AmTcTarget(const BMessage& flattened, int32 k)
		: mPipeline(-1), mFilter(-1)
{
	int32			i;
	const char*		n;
	if (flattened.FindInt32(TARGET_PIPELINE_STR, k, &i) == B_OK) mPipeline = i;
	if (flattened.FindInt32(TARGET_FILTER_STR, k, &i) == B_OK) mFilter = i;
	if (flattened.FindString(TARGET_NAME_STR, k, &n) == B_OK) mName = n;
}

_AmTcTarget::_AmTcTarget(const _AmTcTarget& o)
		: mPipeline(o.mPipeline), mFilter(o.mFilter), mName(o.mName)
{
}

_AmTcTarget& _AmTcTarget::operator=(const _AmTcTarget &o)
{
	mPipeline = o.mPipeline;
	mFilter = o.mFilter;
	mName = o.mName;
	return *this;
}

bool _AmTcTarget::IsValid() const
{
	return mPipeline >= 0 && mFilter >= 0 && mName.Length() > 0;
}

status_t _AmTcTarget::SendBool(AmPipelineMatrixI* matrix, bool value)
{
	ArpASSERT(matrix);
	if (mName.Length() < 1) return B_ERROR;
	BMessage		config;
	status_t		err = config.AddBool(mName.String(), value);
	if (err != B_OK) return err;
	return Send(matrix, &config);
}

status_t _AmTcTarget::SendInt32(AmPipelineMatrixI* matrix, int32 value)
{
	ArpASSERT(matrix);
	if (mName.Length() < 1) return B_ERROR;
	BMessage		config;
	status_t		err = config.AddInt32(mName.String(), value);
	if (err != B_OK) return err;
	return Send(matrix, &config);
}

status_t _AmTcTarget::WriteTo(BMessage& flattened) const
{
	status_t	err;
	if ((err = flattened.AddInt32(TARGET_PIPELINE_STR, mPipeline)) != B_OK) return err;
	if ((err = flattened.AddInt32(TARGET_FILTER_STR, mFilter)) != B_OK) return err;
	if ((err = flattened.AddString(TARGET_NAME_STR, mName)) != B_OK) return err;
	return B_OK;
}

void _AmTcTarget::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("pipeline: %ld filter: %ld name: %s\n", mPipeline, mFilter, mName.String() );
}

status_t _AmTcTarget::Send(AmPipelineMatrixI* matrix, BMessage* config)
{
	if (mPipeline < 0 || mFilter < 0) return B_ERROR;
	pipeline_id			pid = matrix->PipelineId(mPipeline);
	if (!pid) return B_ERROR;
	AmFilterHolderI*	holder = matrix->Filter(pid, THROUGH_PIPELINE);
	int32				index = 0;
	while (holder) {
		if (index == mFilter) {
			if (!holder->Filter() ) return B_ERROR;
			holder->Filter()->PutConfiguration(config);
			return B_OK;
		}
		holder = holder->NextInLine();
		index++;
	}
	return B_ERROR;
}
