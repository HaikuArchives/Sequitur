/* AmEditMotionWindow.cpp
 */
#include <vector.h>
#include <BeExp/BitmapTools.h>
#include <InterfaceKit.h>
#include <support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpMultiScrollBar.h"
#include "AmPublic/AmBackgrounds.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmTimeConverter.h"
#include "AmKernel/AmEditorAux.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmMotion.h"
class _AmMotionPlug;
class _AmPlugView;

static const uint32		VIEW_ENVELOPE_MSG		= 'iVEn';
static const uint32		VIEW_RHYTHM_MSG			= 'iVRh';
static const uint32		VIEW_PROGRESSION_MSG	= 'iVPr';

static const uint32		PENCIL_TOOL_CODE		= 1;
static const uint32		WAND_TOOL_CODE			= 2;
static const uint32		ERASER_TOOL_CODE		= 3;

static const uint		CHANGED_MSG		= 'a_gc';

static const float		_BASE_BEAT_LENGTH		= 32;

static const char*		MEASURE_CTRL			= "measure_ctrl";

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
static void write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa)
{
	uint8*		pixel = (uint8*)( ((uint8*)bm->Bits()) + (uint32)(x * pa.bpp() ) + (uint32)(y * bm->BytesPerRow() ) );
	pa.write(pixel, c);
}

class _AmMotionEditorData
{
public:
	_AmMotionEditorData()		{ }

	AmTimeConverter				mMtc;
	vector<_AmMotionHit>		mHits;
	AmMotionMode				mEditingMode;
	AmSignaturePhrase			mSignatures;
};

//static void			write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa);

/***************************************************************************
 * _AM-MEASURE-CONTROL
 * ARGH!  The motion editor needs a measure control, but I don't want to
 * port the entire measure editor into the AmKernel.  For now I've just
 * copied it in, but without too much more work it should be feasible to
 * turn this into a public class and subclass the Sequitur measure controls
 * from it.
 ***************************************************************************/
class _AmMeasureControl : public BView
{
public:
	/* The leftIndent is the number of pixels that will be chopped off the
	 * left edge and replaced with a 'scaled' view that always keeps the
	 * first measure visible.
	 */
	_AmMeasureControl(	BRect frame,
						const char* name,
						AmSignaturePhrase& signatures,
						AmTimeConverter& mtc,
						float leftIndent = 0,
						int32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	virtual ~_AmMeasureControl();

	/* Set the amount of space over that the actual, fixed measure control starts.
	 */
	void			SetLeftIndent(float leftIndent);
		
	virtual	void	Draw(BRect updateRect);
	virtual	void	AttachedToWindow();
	virtual	void	FrameResized(float new_width, float new_height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	ScrollTo(BPoint where);

protected:
	/* The conversion utility that translates measures to screen locations.
	 */
	AmTimeConverter&		mMtc;
	/* These values are the pixel widths of the scaled caps to the
	 * left and right of my fixed-width center.
	 */
	float					mLeftIndent;
	/* The amount to offset the drawing of this view by.
	 */
	float					mScrollX;
	rgb_color				mViewColor;
	
	/* Render each of the backgrounds.
	 */
	virtual void	DrawLeftBgOn(BRect lBounds, BView* view, AmTime songEndTime);
	virtual void	DrawCenterBgOn(BRect cBounds, BView* view, AmTime songEndTime);

	void			DrawOn(BRect updateRect, BView* view);

	/* Draw the left area.  This area might be drawn at a fixed
	 * size, if the measure 1 fits in the view.  However, if measure
	 * 1 would appear to the left of the view, this area will be drawn
	 * at a scaled aspect, with measure 1 at the left, scaled to
	 * meet whatever view appears at the left of the center view.
	 */
	virtual void	DrawLeftOn(BRect lBounds, BView* view);
	void			LockedDrawLeftOn(const AmPhrase& signatures, BRect lBounds, BView* view);
	virtual void	DrawCenterOn(BRect bounds, BView* view);

	/* Answer true if the current mScrollX value places the
	 * absolute value of measure 1 (according to my mMtc) on or to
	 * the right of my left bound (which is always zero).
	 */
	bool			IsLeftFixed() const;
	/* Answer the time at the left and right edges of the center, fixed view.
	 */
	AmTime			CenterLeftTime() const;
	
	/* Some gratuitous graphics.  This is a bit map that fades in
	 * left to right so that the left-hand indent can match up with
	 * the center view.
	 */
	BBitmap*		mLeftBg;

private:
	typedef BView			inherited;
	AmSignaturePhrase&		mSignatures;
	/* Used while dragging a maker, stores the last point you were at.
	 */
	BPoint					mDownPt;
	/* Cache the song's end time so I don't have to lock the song
	 * to get access to it.
	 */
	AmTime					mCachedEndTime;
	
	void		HandleSignatureMsg(BMessage* msg);
	void		ShowTimeSignatureMenu(BPoint pt) const;
	status_t	SignatureForPt(BPoint pt, AmSignature& sig) const;
	
	void		ConstructLeftBg(BRect bounds);
};

/*************************************************************************
 * _AM-PLUG-VIEW
 * This view allows us to plug in and out different editing mode contexts.
 * I did it this way 'cause I don't feel like hassling with swapping
 * views in and out.
 *************************************************************************/
class _AmPlugView : public BView
{
public:
	_AmPlugView(BRect frame, _AmMotionPlug* plug, AmActiveToolView* atv,
				AmDurationControl* gridCtrl, BScrollBar* hsb, BScrollBar* vsb,
				uint32 resizeMask);
	virtual ~_AmPlugView();
	
	void					SetPlug(_AmMotionPlug* plug);
	_AmMotionPlug*			Plug() const;
	
	virtual void			AttachedToWindow();
	virtual void			Draw(BRect clip);
	virtual	void			FrameResized(float new_width, float new_height);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(	BPoint where,
										uint32 code,
										const BMessage *a_message);

private:
	typedef BView			inherited;
	_AmMotionPlug*			mPlug;
	AmActiveToolView*		mActiveTools;
	AmDurationControl* 		mGridCtrl;
	BScrollBar*				mHsb;
	BScrollBar*				mVsb;
};

/*************************************************************************
 * _AM-MOTION-PLUG
 * This is the abstract interface for views that can be plugged in and out.
 *************************************************************************/
class _AmMotionPlug
{
public:
	void					SetActiveTools(AmActiveToolView* atv);
	void					SetGridControl(AmDurationControl* gridCtrl);

	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid) = 0;
	virtual	void			FrameResized(BView* view, float new_width, float new_height) { }
	virtual void			GetPreferredHeight(float* height);
	virtual	void			MouseDown(BView* view, BPoint where);
	virtual	void			MouseMoved(	BView* view, BPoint where,
										uint32 code,
										const BMessage *a_message);

	virtual void			Installed(BView* view, BScrollBar* hsb, BScrollBar* vsb);
	void					SetupScrollBars(BView* view);

	const AmTimeConverter&	mMtc;
	vector<_AmMotionHit>&	mHits;
	AmSignaturePhrase&		mSignatures;

protected:
	_AmMotionPlug(	const AmTimeConverter& mtc,
					vector<_AmMotionHit>& hits,
					AmSignaturePhrase& signatures);

	AmActiveToolView*		mActiveTools;
	const AmEditorTool*		mActiveTool;
	
	virtual void			CreateEvent(BView* view, BPoint where);
	virtual bool			ChangeEvent(BView* view, BPoint where);
	virtual void			DeleteEvent(BView* view, BPoint where);

	AmTime					GridTime() const;

	AmRange					RangeFor(float start, float end) const;
	float					HitPointXFromTime(AmTime time) const;
	float					HitPointXFromPixel(float x) const;
	virtual float			HitPointYFromPixel(float y) const = 0;
	float					PixelXFromHitPoint(float x) const;
	void					GetGriddedHitRange(float x, float* outStart, float* outEnd) const;
		//void					SetLeftAndRight(BRect& r, float start, float end) const;

private:
	AmDurationControl*		mGridCtrl;
	BScrollBar*				mHsb;
	BScrollBar*				mVsb;

	float					PreferredWidth() const;
};

/*************************************************************************
 * _AM-PROGRESSION-INFO-PLUG
 *************************************************************************/
class _AmProgressionInfoPlug : public _AmMotionPlug
{
public:
	_AmProgressionInfoPlug(	const AmTimeConverter& mtc,
							vector<_AmMotionHit>& hits,
							AmSignaturePhrase& signatures);
	
	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid);
	virtual	void			MouseDown(BView* view, BPoint where)		{ }
	virtual	void			MouseMoved(	BView* view, BPoint where,
										uint32 code,
										const BMessage *a_message)		{ }

protected:
	float					mNoteHeight;
	virtual float			HitPointYFromPixel(float y) const			{ return 0; }
};

/*************************************************************************
 * _AM-PROGRESSION-DATA-PLUG
 *************************************************************************/
class _AmProgressionDataPlug : public _AmMotionPlug
{
public:
	_AmProgressionDataPlug(	const AmTimeConverter& mtc,
							vector<_AmMotionHit>& hits,
							AmSignaturePhrase& signatures);
	
	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid);
	virtual void			GetPreferredHeight(float* height);

protected:
	float					mNoteHeight;
	virtual float			HitPointYFromPixel(float y) const;
};

/*************************************************************************
 * _AM-RHYTHM-INFO-PLUG
 *************************************************************************/
class _AmRhythmInfoPlug : public _AmMotionPlug
{
public:
	_AmRhythmInfoPlug(	const AmTimeConverter& mtc,
						vector<_AmMotionHit>& hits,
						AmSignaturePhrase& signatures);
	
	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid);
	virtual	void			FrameResized(BView* view, float new_width, float new_height);
	virtual	void			MouseDown(BView* view, BPoint where)		{ }
	virtual	void			MouseMoved(	BView* view, BPoint where,
										uint32 code,
										const BMessage *a_message)		{ }

protected:
	virtual float			HitPointYFromPixel(float y) const			{ return 0; }

};

/*************************************************************************
 * _AM-RHYTHM-DATA-PLUG
 *************************************************************************/
class _AmRhythmDataPlug : public _AmMotionPlug
{
public:
	_AmRhythmDataPlug(	const AmTimeConverter& mtc,
						vector<_AmMotionHit>& hits,
						AmSignaturePhrase& signatures);
	
	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid);
	virtual	void			FrameResized(BView* view, float new_width, float new_height);
	virtual void			GetPreferredHeight(float* height);

	virtual void			Installed(BView* view, BScrollBar* hsb, BScrollBar* vsb);

protected:
	float					mMid;
	virtual float			HitPointYFromPixel(float y) const;
};

/*************************************************************************
 * _AM-ENVELOPE-INFO-PLUG
 *************************************************************************/
class _AmEnvelopeInfoPlug : public _AmMotionPlug
{
public:
	_AmEnvelopeInfoPlug(const AmTimeConverter& mtc,
						vector<_AmMotionHit>& hits,
						AmSignaturePhrase& signatures);
	
	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid);
	virtual	void			FrameResized(BView* view, float new_width, float new_height);
	virtual	void			MouseDown(BView* view, BPoint where)		{ }
	virtual	void			MouseMoved(	BView* view, BPoint where,
										uint32 code,
										const BMessage *a_message)		{ }

protected:
	virtual float			HitPointYFromPixel(float y) const			{ return 0; }

};

/*************************************************************************
 * _AM-ENVELOPE-DATA-PLUG
 *************************************************************************/
class _AmEnvelopeDataPlug : public _AmMotionPlug
{
public:
	_AmEnvelopeDataPlug(const AmTimeConverter& mtc,
						vector<_AmMotionHit>& hits,
						AmSignaturePhrase& signatures);
	
	virtual void			DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid);
	virtual	void			FrameResized(BView* view, float new_width, float new_height);
	virtual void			GetPreferredHeight(float* height);

	virtual void			Installed(BView* view, BScrollBar* hsb, BScrollBar* vsb);

protected:
	float					mMid;

	virtual void			CreateEvent(BView* view, BPoint where);
	virtual bool			ChangeEvent(BView* view, BPoint where);
	virtual void			DeleteEvent(BView* view, BPoint where);
	virtual float			HitPointYFromPixel(float y) const;
};

/*************************************************************************
 * _AM-MOTION-GRID-BACKGROUND
 *************************************************************************/
class _AmMotionGridBackground : public AmGridBackground
{
public:
	_AmMotionGridBackground(AmTime grid, const AmTimeConverter& mtc)
			: mGrid(grid), mMtc(mtc)	{ }
	
	void			DrawGrid(BView* view, BRect clip)		{ DrawOn(view, clip); }

protected:
	virtual AmTime					GridTime() const		{ return mGrid; }
	virtual const AmTimeConverter&	TimeConverter() const	{ return mMtc; }

private:
	AmTime					mGrid;
	const AmTimeConverter&	mMtc;
};

/*************************************************************************
 * AM-MOTION-EDITOR
 *************************************************************************/
const BBitmap*		gPencilIcon = NULL;
const BBitmap*		gWandIcon = NULL;
const BBitmap*		gEraserIcon = NULL;

AmMotionEditor::AmMotionEditor(BRect frame, const char* name, uint32 resizeMask)
		: inherited(frame, name, NULL, NULL, resizeMask, B_WILL_DRAW),
		  mData(NULL), mViewAsField(NULL),
		  mActiveTools(NULL), mToolBar(NULL), mPencilTool(NULL),
		  mWandTool(NULL), mEraserTool(NULL), mGridCtrl(NULL),
		  mInfoPlug(NULL), mDataPlug(NULL), mPrefW(0), mTarget(NULL)
{
	Init();
	if (mData) AddViews(Bounds() );
}

AmMotionEditor::AmMotionEditor(	const char* name,
								AmFilterConfigLayout* target,
								const BMessage& initSettings,
								const char* settingsKey,
								int32 settingsIndex)
		: inherited(name, NULL, NULL),
		  mData(NULL), mViewAsField(NULL),
		  mActiveTools(NULL), mToolBar(NULL), mPencilTool(NULL),
		  mWandTool(NULL), mEraserTool(NULL), mGridCtrl(NULL),
		  mInfoPlug(NULL), mDataPlug(NULL), mPrefW(0),
		  mTarget(target), mSettingsKey(settingsKey), mSettingsIndex(settingsIndex)
{
	Init();
	BRect		r = Bounds();
		//if (r.left == 0 && r.top == 0 && r.right == 1 && r.bottom == 1);
	//r.right = r.bottom = 20;
	if (mData) AddViews(r);
	Refresh(initSettings);
}

AmMotionEditor::~AmMotionEditor()
{
	/* Make sure the refs to my tools are gone before I delete my tools.
	 */
	if (mActiveTools) mActiveTools->ClearTools();
	if (mToolBar) mToolBar->ClearTools();
	delete mPencilTool;
	delete mWandTool;
	delete mEraserTool;

	delete mData;
}

void AmMotionEditor::SetMotion(const AmMotionI* motionI)
{
	if (!mData) return;
	mData->mHits.resize(0);
	mData->mSignatures.DeleteEvents();
	const AmMotion*		motion = dynamic_cast<const AmMotion*>(motionI);
	if (!motion) {
		if (mInfoPlug) mInfoPlug->SetPlug(NULL);
		if (mDataPlug) mDataPlug->SetPlug(NULL);
		mData->mSignatures.SetSignature(1, 4, 4);
		Invalidate();
		return;
	}
	BPoint		pt;
	float		end;
	for (uint32 k = 0; motion->GetHit(k, &pt, &end) == B_OK; k++) {
		mData->mHits.push_back( _AmMotionHit(pt, end) );
	}

	mData->mEditingMode = motion->EditingMode();
	
	mData->mSignatures = motion->Signatures();
	if (mData->mSignatures.IsEmpty() ) mData->mSignatures.SetSignature(1, 4, 4);

	ReplugViews();
}

void AmMotionEditor::ReplugViews()
{
	if (!mData) return;
	if (mData->mEditingMode == PROGRESSION_MODE) {
		if (mInfoPlug) mInfoPlug->SetPlug(new _AmProgressionInfoPlug(mData->mMtc, mData->mHits, mData->mSignatures) );
		if (mDataPlug) mDataPlug->SetPlug(new _AmProgressionDataPlug(mData->mMtc, mData->mHits, mData->mSignatures) );
	} else if (mData->mEditingMode == RHYTHM_MODE) {
		if (mInfoPlug) mInfoPlug->SetPlug(new _AmRhythmInfoPlug(mData->mMtc, mData->mHits, mData->mSignatures) );
		if (mDataPlug) mDataPlug->SetPlug(new _AmRhythmDataPlug(mData->mMtc, mData->mHits, mData->mSignatures) );
	} else if (mData->mEditingMode == ENVELOPE_MODE) {
		if (mInfoPlug) mInfoPlug->SetPlug(new _AmEnvelopeInfoPlug(mData->mMtc, mData->mHits, mData->mSignatures) );
		if (mDataPlug) mDataPlug->SetPlug(new _AmEnvelopeDataPlug(mData->mMtc, mData->mHits, mData->mSignatures) );
	}
}

status_t AmMotionEditor::AddHitsTo(AmMotionI* motionI) const
{
	if (!mData) return B_ERROR;
	AmMotion*		motion = dynamic_cast<AmMotion*>(motionI);
	ArpASSERT(motion);
	if (!motion) return B_ERROR;
	for (uint32 k = 0; k < mData->mHits.size(); k++) {
		motion->AddHit(mData->mHits[k].mPt, mData->mHits[k].mEnd);
	}
	motion->SetEditingMode(mData->mEditingMode);
	motion->SetSignatures(mData->mSignatures);
	return B_OK;
}

void AmMotionEditor::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (Parent() ) SetViewColor(Parent()->ViewColor() );
	if (mViewAsField && mViewAsField->Menu() ) mViewAsField->Menu()->SetTargetForItems(this);
}

void AmMotionEditor::AllAttached()
{
	inherited::AllAttached();
	if (mScrollV) {
		mScrollV->ResizeTo(mScrollVF.Width(), mScrollVF.Height() );
		mScrollV->MoveTo(mScrollVF.left, mScrollVF.top);
	}
	if (mScrollH) {
		mScrollH->ResizeTo(mScrollHF.Width(), mScrollHF.Height() );
		mScrollH->MoveTo(mScrollHF.left, mScrollHF.top);
	}
}

void AmMotionEditor::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	if (mScrollV) {
		mScrollVF = mScrollV->Frame();
		mScrollV->ResizeTo(10, 100);
	}
	if (mScrollH) {
		mScrollHF = mScrollH->Frame();
		mScrollH->ResizeTo(100, 10);
	}
}

void AmMotionEditor::GetPreferredSize(float* width, float* height)
{
	*width = mPrefW;
	*height = 200;
}

void AmMotionEditor::MessageReceived(BMessage* msg)
{
	/* If I don't have a target, then this message should continue on
	 * up to my parent.
	 */
	if (msg->what == CHANGED_MSG && mTarget) {
		if (mSettingsKey.Length() <= 0) return;
		BMessage	init;
		AmMotionI*	m = AmMotionI::NewMotion(init);
		if (m && AddHitsTo(m) == B_OK) {
			BMessage		rMsg, containerMsg;
			if (m->WriteTo(rMsg) == B_OK && containerMsg.AddMessage(mSettingsKey.String(), &rMsg) == B_OK) {
				mTarget->Implementation().SendConfiguration(&containerMsg);
				mTarget->Settings().Update(containerMsg);
			}
			delete m;
		}
		return;
	}
	
	switch (msg->what) {
		case VIEW_ENVELOPE_MSG:
			if (mData && mData->mEditingMode != ENVELOPE_MODE) {
				mData->mEditingMode = ENVELOPE_MODE;
				ReplugViews();
			}
			break;
		case VIEW_PROGRESSION_MSG:
			if (mData && mData->mEditingMode != PROGRESSION_MODE) {
				mData->mEditingMode = PROGRESSION_MODE;
				ReplugViews();
			}
			break;
		case VIEW_RHYTHM_MSG:
			if (mData && mData->mEditingMode != RHYTHM_MODE) {
				mData->mEditingMode = RHYTHM_MODE;
				ReplugViews();
			}
			break;

		case DUR_QUANTIZE_FINISHED_MSG:
			if (mDataPlug) mDataPlug->Invalidate();
			break;
		case DUR_EIGHTHS_FINISHED_MSG:
			if (mDataPlug) mDataPlug->Invalidate();
			break;
		case B_OBSERVER_NOTICE_CHANGE: {
			int32		what;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
			if (what == 'zoom') {
				float	valueX;
				if (mData && msg->FindFloat("x_value", &valueX) == B_OK) {
					mData->mMtc.SetBeatLength(valueX * _BASE_BEAT_LENGTH);
					mDataPlug->Invalidate();
					BView*		v = FindView(MEASURE_CTRL);
					if (v) v->Invalidate();
				}
			}
		} break;

		default:
			inherited::MessageReceived(msg);
	}
}

void AmMotionEditor::Refresh(const BMessage& settings)
{
	BMessage	msg;
	if (settings.FindMessage(mSettingsKey.String(), mSettingsIndex, &msg) == B_OK) {
		AmMotionI*	motion = AmMotionI::NewMotion(msg);
		if (motion) SetMotion(motion);
		delete motion;
	}
}

void AmMotionEditor::ComputeDimens(ArpDimens& cur_dimens)
{
#if 0
	float		vw = 0, vh = 0;
	GetPreferredSize(&vw, &vh);
	
	const float divw = BasicFont()->StringWidth(Label())
					 + BasicFont()->StringWidth("  ");
	vw = divw+12+StringWidth("WWWWWW");
	
	cur_dimens.X().SetTo(divw, vw-divw-12, vw-divw-12, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, vh, vh, vh, 0);
	
	//printf("Left: %f, Width: %f, Divider: %f\n",
	//		Frame().left, Frame().Width(), Divider());
	ArpUniDimens& dx = cur_dimens.X();
	float minb = dx.MinBody();
	float prefb = mPrefBW;
	float maxb = dx.MaxBody();
	if (maxb < prefb) maxb = prefb;
#endif

	float		vw = 0, vh = 0;
	GetPreferredSize(&vw, &vh);

//	cur_dimens.X().SetTo(divw, vw-divw-12, vw-divw-12, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, 80, vh, 1000, 0);

	ArpUniDimens& 	dx = cur_dimens.X();
	dx.SetBody(30, vw, 1000);
	dx.AddBody(12);
}

void AmMotionEditor::Layout(void)
{
	ArpBaseLayout::Layout();
}

static BMenuField* new_view_as_field(BRect f)
{
	BMenu*		menu = new BMenu("View As");
	if (!menu) return NULL;
	BMenuItem*	item = new BMenuItem("Rhythm", new BMessage(VIEW_RHYTHM_MSG), 0, 0);
	if (item) menu->AddItem(item);
	item = new BMenuItem("Progression", new BMessage(VIEW_PROGRESSION_MSG), 0, 0);
	if (item) menu->AddItem(item);
	item = new BMenuItem("Envelope", new BMessage(VIEW_ENVELOPE_MSG), 0, 0);
	if (item) menu->AddItem(item);
	
	menu->SetLabelFromMarked(false);
//	menu->SetRadioMode(true);
	BMenuField*	field = new BMenuField(f, "view_as_field", NULL, menu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

void AmMotionEditor::AddViews(BRect bounds)
{
	if (!mData) return;
	/* Add the View As field.
	 */
	float			w = StringWidth("View As") + 28;
	BRect			fieldR(0, 0, w, 10);
	mViewAsField = new_view_as_field(fieldR);
	if (mViewAsField) AddChild(mViewAsField);
	mPrefW = w;
	/* Add the tool row.
	 */
	float			left = bounds.left;
	float			top = bounds.top;
	mActiveTools = new AmActiveToolView(BPoint(w + 5, 0) );
	if (mActiveTools) {
		mActiveTools->ResizeToPreferred();
		top = mActiveTools->Frame().bottom;
		left = mActiveTools->Frame().right + 5;
		AddChild(mActiveTools);
	}
	mToolBar = new AmEditorToolBarView(BPoint(left, 4), "toolbar");
	if (mToolBar) {
		if (gPencilIcon) mPencilTool = new AmEditorTool(gPencilIcon, "Pencil", PENCIL_TOOL_CODE);
		if (mPencilTool) mToolBar->AddTool(mPencilTool);
		if (gWandIcon) mWandTool = new AmEditorTool(gWandIcon, "Wand", WAND_TOOL_CODE);
		if (mWandTool) mToolBar->AddTool(mWandTool);
		if (gEraserIcon) mEraserTool = new AmEditorTool(gEraserIcon, "Eraser", ERASER_TOOL_CODE);
		if (mEraserTool) mToolBar->AddTool(mEraserTool);

		mToolBar->SetActiveToolView(mActiveTools);
		mToolBar->ResizeToPreferred();
		BRect		f = mToolBar->Frame();
		if (f.bottom > top) top = f.bottom;
		left = f.right + 5;
		AddChild(mToolBar);
	}
	if (mActiveTools) {
		mActiveTools->SetActiveTool(B_PRIMARY_MOUSE_BUTTON, mPencilTool);
		mActiveTools->SetActiveTool(B_SECONDARY_MOUSE_BUTTON, mEraserTool);
		mActiveTools->SetActiveTool(B_TERTIARY_MOUSE_BUTTON, mWandTool);
	}
	mGridCtrl = new AmDurationControl(BPoint(left, 4), "grid_ctrl", "Grid:", 0);
	if (mGridCtrl) {
		AddChild(mGridCtrl);
		/* For some reason, the right edge is too far, so I smoosh it down.
		 */
		mPrefW = mGridCtrl->Frame().right - 40;
	}
	top += 4;
	/* Add the measure and editing views.
	 */
	float		measureH = 25;
	float		infoW = 40;
	BRect		measureF(bounds.left, top, bounds.right, top + measureH);
	mScrollVF	= BRect(bounds.right - Prefs().Size(V_SCROLLBAR_X) - 1, measureF.bottom + 1,
						bounds.right, bounds.bottom - Prefs().Size(H_SCROLLBAR_Y) - 2);
	mScrollHF	= BRect(bounds.left, bounds.bottom - Prefs().Size(H_SCROLLBAR_Y) - 1,
						bounds.right - Prefs().Size(V_SCROLLBAR_X) - 2, bounds.bottom);
	BRect		infoF(bounds.left, measureF.bottom + 1, bounds.left + infoW, mScrollHF.top - 1);
	BRect		dataF(infoF.right + 1, infoF.top, mScrollVF.left - 1, infoF.bottom);

	_AmMeasureControl*		m = new _AmMeasureControl(	measureF, MEASURE_CTRL, mData->mSignatures,
														mData->mMtc, infoW + 1);
	if (m) AddChild(m);
#if 1
	ArpMultiScrollBar*	vsb = new ArpMultiScrollBar(BRect(0, 0, 10, 100), "vsb", NULL, 0, 1, B_VERTICAL);
	mScrollV = vsb;
	if (vsb) {
		AddChild(vsb);
	}
	ArpMultiScrollBar*	hsb = new ArpMultiScrollBar(BRect(0, 0, 100, 10), "hsb", NULL, 0, 1, B_HORIZONTAL);
	mScrollH = hsb;
	if (hsb) {
		if (m) hsb->AddTarget(m);
		AddChild(hsb);
	}
#endif
#if 0
	ArpMultiScrollBar*	vsb = new ArpMultiScrollBar(mScrollVF, "vsb", NULL, 0, 0, B_VERTICAL);
	if (vsb) AddChild(vsb);
	ArpMultiScrollBar*	hsb = new ArpMultiScrollBar(mScrollHF, "hsb", NULL, 0, 0, B_HORIZONTAL);
	if (hsb) {
		if (m) hsb->AddTarget(m);
		AddChild(hsb);
	}
#endif
	BRect				frame(mScrollHF.right + 1, mScrollHF.top, mScrollHF.right + 1 +  ARP_ZOOM_CONTROL_WIDTH, mScrollHF.top + ARP_ZOOM_CONTROL_HEIGHT);
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

		zoom->SetUpdatedMessage( new BMessage('zoom') );
		zoom->StartWatching(this, 'zoom');
		zoom->SetZoomX(mData->mMtc.BeatLength() / _BASE_BEAT_LENGTH);
//		zoom->SetZoomX(1);
		AddChild(zoom);
	}


	mInfoPlug = new _AmPlugView(infoF, new _AmRhythmInfoPlug(mData->mMtc, mData->mHits, mData->mSignatures),
								NULL, mGridCtrl, NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	if (mInfoPlug) {
		if (vsb) vsb->AddTarget(mInfoPlug);
		AddChild(mInfoPlug);
	}
	mDataPlug = new _AmPlugView(dataF, new _AmRhythmDataPlug(mData->mMtc, mData->mHits, mData->mSignatures),
								mActiveTools, mGridCtrl, hsb, vsb, B_FOLLOW_ALL);
	if (mDataPlug) {
		if (hsb) hsb->AddTarget(mDataPlug);
		if (vsb) vsb->AddTarget(mDataPlug);
		AddChild(mDataPlug);
	}
}

void AmMotionEditor::Init()
{
	if (!gPencilIcon) gPencilIcon = ImageManager().FindBitmap("Pencil Tool");
	if (!gWandIcon) gWandIcon = ImageManager().FindBitmap("Wand Tool");
	if (!gEraserIcon) gEraserIcon = ImageManager().FindBitmap("Eraser Tool");

	mData = new _AmMotionEditorData();
	if (mData) {
		mData->mEditingMode = RHYTHM_MODE;
		mData->mSignatures.SetSignature(1, 4, 4);
		mData->mMtc.SetBeatLength(_BASE_BEAT_LENGTH);
	}
}

/*************************************************************************
 * _AM-PLUG-VIEW
 *************************************************************************/
_AmPlugView::_AmPlugView(	BRect frame, _AmMotionPlug* plug, AmActiveToolView* atv,
							AmDurationControl* gridCtrl, BScrollBar* hsb,
							BScrollBar* vsb, uint32 resizeMask)
		: inherited(frame, "plug_view", resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
		  mPlug(NULL), mActiveTools(atv), mGridCtrl(gridCtrl), mHsb(hsb), mVsb(vsb)
{
	SetPlug(plug);
}

_AmPlugView::~_AmPlugView()
{
	delete mPlug;
}

void _AmPlugView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void _AmPlugView::SetPlug(_AmMotionPlug* plug)
{
	delete mPlug;
	mPlug = plug;
	if (mPlug) {
		mPlug->SetActiveTools(mActiveTools);
		mPlug->SetGridControl(mGridCtrl);
		mPlug->Installed(this, mHsb, mVsb);
	}
	Invalidate();
}

_AmMotionPlug* _AmPlugView::Plug() const
{
	return mPlug;
}

void _AmPlugView::Draw(BRect clip)
{
	if (!mPlug) {
		inherited::Draw(clip);
		return;
	}
	BView*			into = this;
	
	ArpBitmapCache*	cache = dynamic_cast<ArpBitmapCache*>(Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	AmTime			grid = PPQN;
	if (mGridCtrl) grid = mGridCtrl->RawTicks();
	
	if (mPlug) mPlug->DrawOn(into, clip, Bounds(), grid);
	if (cache) cache->FinishDrawing(into);
}

void _AmPlugView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	if (mPlug) {
		mPlug->FrameResized(this, new_width, new_height);
		mPlug->SetupScrollBars(this);
	}
}

void _AmPlugView::MouseDown(BPoint where)
{
	inherited::MouseDown(where);
	if (mPlug) mPlug->MouseDown(this, where);
}

void _AmPlugView::MouseMoved(	BPoint where,
								uint32 code,
								const BMessage *a_message)
{
	inherited::MouseMoved(where, code, a_message);
	if (mPlug) mPlug->MouseMoved(this, where, code, a_message);
}

/*************************************************************************
 * _AM-MOTION-PLUG
 *************************************************************************/
_AmMotionPlug::_AmMotionPlug(	const AmTimeConverter& mtc,
								vector<_AmMotionHit>& hits,
								AmSignaturePhrase& signatures)
		: mMtc(mtc), mHits(hits), mSignatures(signatures),
		  mActiveTools(NULL), mActiveTool(NULL), mGridCtrl(NULL),
		  mHsb(NULL), mVsb(NULL)
{
}

void _AmMotionPlug::SetActiveTools(AmActiveToolView* atv)
{
	mActiveTools = atv;
}

void _AmMotionPlug::SetGridControl(AmDurationControl* gridCtrl)
{
	mGridCtrl = gridCtrl;
}

void _AmMotionPlug::GetPreferredHeight(float* height)
{
	ArpASSERT(height);
	*height = 0;
}

void _AmMotionPlug::MouseDown(BView* view, BPoint where)
{
	ArpASSERT(view);
	mActiveTool = NULL;
	if (mActiveTools) {
		int32		button = 0;
		view->Window()->CurrentMessage()->FindInt32("buttons", &button);
		mActiveTool = mActiveTools->ActiveTool(button);
	}
	if (!mActiveTool) return;
	if (mActiveTool->Code() == PENCIL_TOOL_CODE) CreateEvent(view, where);
	else if (mActiveTool->Code() == WAND_TOOL_CODE) ChangeEvent(view, where);
	else if (mActiveTool->Code() == ERASER_TOOL_CODE) DeleteEvent(view, where);
}

void _AmMotionPlug::MouseMoved(	BView* view, BPoint where,
								uint32 code,
								const BMessage *a_message)
{
	ArpASSERT(view);
	int32		button = 0;
	view->Window()->CurrentMessage()->FindInt32("buttons", &button);
	if (button == 0) {
		mActiveTool = NULL;
		return;
	}

	if (mActiveTool) {
		if (mActiveTool->Code() == PENCIL_TOOL_CODE) CreateEvent(view, where);
		else if (mActiveTool->Code() == WAND_TOOL_CODE) ChangeEvent(view, where);
		else if (mActiveTool->Code() == ERASER_TOOL_CODE) DeleteEvent(view, where);
	}
}

void _AmMotionPlug::Installed(BView* view, BScrollBar* hsb, BScrollBar* vsb)
{
	mHsb = hsb;
	mVsb = vsb;
	SetupScrollBars(view);
}

void _AmMotionPlug::SetupScrollBars(BView* view)
{
	BRect		b = view->Bounds();
	if (mHsb) {
		float				preferredWidth = PreferredWidth(), proportion;
		proportion = b.Width() / preferredWidth;
		if (proportion < 0) proportion = 0;
		if (proportion > 1) proportion = 1;
		mHsb->SetProportion(proportion);
		float			width = preferredWidth - b.Width();
		if (width < 0) width = 0;
		mHsb->SetRange(0, width);
	}
	if (mVsb) {
		float				preferredHeight, proportion;
		GetPreferredHeight(&preferredHeight);
		proportion = b.Height() / preferredHeight;
		if (proportion < 0) proportion = 0;
		if (proportion > 1) proportion = 1;
		mVsb->SetProportion(proportion);
		float			height = preferredHeight - b.Height();
		if (height < 0) height = 0;
		mVsb->SetRange(0, height);
	}
}

void _AmMotionPlug::CreateEvent(BView* view, BPoint where)
{
	ArpASSERT(view);
	/* If there's already a hit where the user clicked,
	 * then just do a change operation on that.
	 */
	if (ChangeEvent(view, where) ) return;

	float			start, end;
	GetGriddedHitRange(where.x, &start, &end);
	if (start < 0 || end < 0) return;
	BRect			hitR(view->Bounds() );
	/* The ChangeEvent() has different rules for finding hits to
	 * change than I've got for finding a place to add a hit.  If
	 * the hit I would be adding would conflict with an existing one,
	 * then just do nothing.
	 */
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (start <= mHits[k].mEnd && end >= mHits[k].mPt.x)
			return;
	}
	_AmMotionHit		hit(BPoint(start, HitPointYFromPixel(where.y)), end);
	hitR.left = PixelXFromHitPoint(start);
	hitR.right = PixelXFromHitPoint(end);
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (end < mHits[k].mPt.x) {
			mHits.insert(mHits.begin() + k, hit);
			view->Invalidate(hitR);
			view->Window()->PostMessage(CHANGED_MSG, view);	
			return;
		}
	}
	mHits.push_back(hit);
	view->Invalidate(hitR);
	view->Window()->PostMessage(CHANGED_MSG, view);	
	SetupScrollBars(view);
}

bool _AmMotionPlug::ChangeEvent(BView* view, BPoint where)
{
	ArpASSERT(view);
	float			hit = HitPointXFromPixel(where.x);
	if (hit < 0) return false;
	bool			foundHit = false;
	BRect			invalid = arp_invalid_rect();
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (hit >= mHits[k].mPt.x && hit <= mHits[k].mEnd) {
			BRect	hitR(view->Bounds() );
			hitR.left = PixelXFromHitPoint(mHits[k].mPt.x);
			hitR.right = PixelXFromHitPoint(mHits[k].mEnd);
			invalid = arp_merge_rects(invalid, hitR);
			mHits[k].mPt.y = HitPointYFromPixel(where.y);
			view->Window()->PostMessage(CHANGED_MSG, view);
			foundHit = true;
		}
		if (hit < mHits[k].mPt.x) break;
	}
	if (arp_is_valid_rect(invalid) ) view->Invalidate(invalid);
	return foundHit;
}

void _AmMotionPlug::DeleteEvent(BView* view, BPoint where)
{
	ArpASSERT(view);
	float			loc = HitPointXFromPixel(where.x);
	if (loc < 0) return;
	BRect			invalid = arp_invalid_rect();
	BRect			b(view->Bounds() );
	bool			setScrollBars = false;
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (loc >= mHits[k].mPt.x) {
			if (mHits[k].HasEnd() && loc <= mHits[k].mEnd) {
				if (k == mHits.size() -1) setScrollBars = true;
				BRect		hitR(b);
				hitR.left = PixelXFromHitPoint(mHits[k].mPt.x);
				hitR.right = PixelXFromHitPoint(mHits[k].mEnd);
				invalid = arp_merge_rects(invalid, hitR);
				mHits.erase(mHits.begin() + k);
				view->Window()->PostMessage(CHANGED_MSG, view);	
				k = 0;
			}
		}
	}
	if (arp_is_valid_rect(invalid) ) view->Invalidate(invalid);
	if (setScrollBars) SetupScrollBars(view);
}

AmTime _AmMotionPlug::GridTime() const
{
	if (!mGridCtrl) return PPQN;
	return mGridCtrl->RawTicks();
}

AmRange _AmMotionPlug::RangeFor(float start, float end) const
{
	AmSignature		sig;
	AmRange			r;
	float			f = floor(start);
	if (mSignatures.GetSignatureForMeasure(int32(f) + 1, sig) == B_OK) {
		r.start = AmTime(sig.StartTime() + (sig.Duration() * (start - f)));
	}
	f = floor(end);
	if (mSignatures.GetSignatureForMeasure(int32(f) + 1, sig) == B_OK) {
		r.end = AmTime(sig.StartTime() + (sig.Duration() * (end - f)));
	}
	return r;
}

float _AmMotionPlug::HitPointXFromTime(AmTime time) const
{
	if (time < 0) return -1;
	AmSignature		sig;
	for (int32 k = 1; mSignatures.GetSignatureForMeasure(k, sig) == B_OK; k++) {
		AmTime	start = sig.StartTime(), end = sig.EndTime();
		if (time >= start && time <= end) {
			return k - 1 + (float(time - start) / double(sig.Duration()));
		}
	}
	return -1;
}

float _AmMotionPlug::HitPointXFromPixel(float x) const
{
	if (x < 0) return -1;
	AmSignature		sig;
	for (int32 k = 1; mSignatures.GetSignatureForMeasure(k, sig) == B_OK; k++) {
		float		start = mMtc.TickToPixel(sig.StartTime() );
		float		end = mMtc.TickToPixel(sig.EndTime() );
		if (x >= start && x <= end) {
			return k - 1 + ((x - start) / (end - start));
		}
		/* This would be an error condition.  Not sure how it would happen,
		 * but it would cause an endless iteration, so let's avoid it.
		 */
		if (start >= x) return -1;
	}
	return -1;
}

void _AmMotionPlug::GetGriddedHitRange(float x, float* outStart, float* outEnd) const
{
	ArpASSERT(outStart && outEnd);
	AmTime		tick = mMtc.PixelToTick(x);
	tick = tick - (tick % GridTime() );
	*outStart = HitPointXFromTime(tick);
	*outEnd = HitPointXFromTime(tick + GridTime() - 1) - 0.00001;
}

float _AmMotionPlug::PixelXFromHitPoint(float x) const
{
	AmSignature		sig;
	float			f = floor(x);
	if (mSignatures.GetSignatureForMeasure(int32(f) + 1, sig) == B_OK) {
		AmTime		t = AmTime(sig.StartTime() + (sig.Duration() * (x - f)));
		return mMtc.TickToPixel(t);
	}
	return -1;
}

float _AmMotionPlug::PreferredWidth() const
{
	if (mHits.size() < 1) return 0;
	float			hit = mHits[mHits.size()-1].mPt.x;
	if (mHits[mHits.size()-1].HasEnd() ) hit = mHits[mHits.size()-1].mEnd;
	AmSignature		sig;
	int32			measure = int32(floor(hit)) + 2;
	if (mSignatures.GetSignatureForMeasure(measure, sig) != B_OK) return 0;
	return mMtc.TickToPixel(sig.EndTime() );
}

/*************************************************************************
 * _AM-PROGRESSION-INFO-PLUG
 *************************************************************************/
static const int32		TOP = 0;
static const int32		CENTER = 128;
static const int32		BOTTOM = 256;

_AmProgressionInfoPlug::_AmProgressionInfoPlug(	const AmTimeConverter& mtc,
												vector<_AmMotionHit>& hits,
												AmSignaturePhrase& signatures)
		: _AmMotionPlug(mtc, hits, signatures), mNoteHeight(5)
{
}

void _AmProgressionInfoPlug::DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid)
{
	ArpASSERT(view);
	view->SetHighColor( Prefs().Color(AM_INFO_BG_C) );
	view->FillRect(clip);

	float			mid = mNoteHeight * CENTER;
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->SetLowColor( Prefs().Color(AM_INFO_BG_C) );
	float			fh = arp_get_font_height(view);

	const char*		label = "128";
	float			w = view->StringWidth(label) + 5;
	BRect			f(bounds.right - w, 0, bounds.right, fh);
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );

	label = "0";
	w = view->StringWidth(label) + 5;
	f.Set(bounds.right - w, mid - (fh / 2), bounds.right, mid + (fh / 2) );
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );

	label = "-128";
	w = view->StringWidth(label) + 5;
	float		bottom = (BOTTOM * mNoteHeight) + mNoteHeight - 1;
	f.Set(bounds.right - w, bottom - fh, bounds.right, bottom);
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );
}

/*************************************************************************
 * _AM-PROGRESSION-DATA-PLUG
 *************************************************************************/
_AmProgressionDataPlug::_AmProgressionDataPlug(	const AmTimeConverter& mtc,
												vector<_AmMotionHit>& hits,
												AmSignaturePhrase& signatures)
		: _AmMotionPlug(mtc, hits, signatures), mNoteHeight(5)
{
}

void _AmProgressionDataPlug::DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid)
{
	ArpASSERT(view);
	view->SetHighColor( Prefs().Color(AM_DATA_BG_C) );
	view->FillRect(clip);

	_AmMotionGridBackground		gridBg(grid, mMtc);
	gridBg.DrawGrid(view, clip);

	BRect		r(clip);
	r.top = 0;
	r.bottom = r.top + mNoteHeight - 1;
	for (int32 k = 0; k <= BOTTOM; k += 2) {
		if (r.Intersects(clip) ) {
			view->SetHighColor( Prefs().Color(AM_MEASURE_BEAT_C) );
			view->FillRect(r);
		}
		r.top = r.bottom + 1;
		r.bottom = r.top + mNoteHeight - 1;
		r.top = r.bottom + 1;
		r.bottom = r.top + mNoteHeight - 1;
	}

	AmSignatureMeasureBackground	bg(mSignatures, mMtc);
	bg.SetFlag(bg.DRAW_MEASURE_FLAG, true);
	bg.SetFlag(bg.DRAW_BEATS_FLAG, true);
	bg.DrawAllOn(view, clip);

	for (uint32 k = 0; k < mHits.size(); k++) {
		BRect		r;
		AmRange		range = RangeFor(mHits[k].mPt.x, mHits[k].mEnd);
		r.left = mMtc.TickToPixel(range.start);
		r.right = mMtc.TickToPixel(range.end);
		int32		note = CENTER;
		if (mHits[k].mPt.y > 0) note = int32(fabs( (mHits[k].mPt.y * CENTER) - CENTER));
		else if (mHits[k].mPt.y < 0) note = CENTER + int32(fabs(mHits[k].mPt.y * CENTER));
		if (note < TOP) note = TOP;
		else if (note > BOTTOM) note = BOTTOM;
		r.top = note * mNoteHeight;
		r.bottom = r.top + mNoteHeight - 1;
		if (r.Intersects(clip) ) {
			view->SetHighColor( AmPrefs().PrimaryColor() );
			view->FillRect(r);
			view->SetHighColor(0, 0, 0);
			view->StrokeRect(r);
		}
	}
}

void _AmProgressionDataPlug::GetPreferredHeight(float* height)
{
	ArpASSERT(height);
	*height = (BOTTOM * mNoteHeight) + mNoteHeight - 1;
}

float _AmProgressionDataPlug::HitPointYFromPixel(float y) const
{
	int32		shift = int32(y / mNoteHeight);
	float		step = float(1) / float(128);
	if (shift < CENTER) {
		float	hit = 1 - (float(shift) * step);
		if (hit > 1) hit = 1;
		else if (hit < 0) hit = 0;
		return hit;
	} else if (shift > CENTER) {
		float	hit = 0 - (float(shift - CENTER) * step);
		if (hit > 0) hit = 0;
		else if (hit < -1) hit = -1;
		return hit;
	} else return 0;
}

/*************************************************************************
 * _AM-RHYTHM-INFO-PLUG
 *************************************************************************/
_AmRhythmInfoPlug::_AmRhythmInfoPlug(	const AmTimeConverter& mtc,
										vector<_AmMotionHit>& hits,
										AmSignaturePhrase& signatures)
		: _AmMotionPlug(mtc, hits, signatures)
{
}

void _AmRhythmInfoPlug::DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid)
{
	ArpASSERT(view);
	view->SetHighColor( Prefs().Color(AM_INFO_BG_C) );
	view->FillRect(clip);

	float			mid = bounds.Height() / 2;
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->SetLowColor( Prefs().Color(AM_INFO_BG_C) );
	float			fh = arp_get_font_height(view);

	const char*		label = "100\%";
	float			w = view->StringWidth(label) + 5;
	BRect			f(bounds.right - w, 0, bounds.right, fh);
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );

	label = "0";
	w = view->StringWidth(label) + 5;
	f.Set(bounds.right - w, mid - (fh / 2), bounds.right, mid + (fh / 2) );
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );

	label = "-100\%";
	w = view->StringWidth(label) + 5;
	f.Set(bounds.right - w, bounds.bottom - fh, bounds.right, bounds.bottom);
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );
}

void _AmRhythmInfoPlug::FrameResized(BView* view, float new_width, float new_height)
{
	ArpASSERT(view);
	view->Invalidate();
}

/*************************************************************************
 * _AM-RHYTHM-DATA-PLUG
 *************************************************************************/
_AmRhythmDataPlug::_AmRhythmDataPlug(	const AmTimeConverter& mtc,
										vector<_AmMotionHit>& hits,
										AmSignaturePhrase& signatures)
		: _AmMotionPlug(mtc, hits, signatures)
{
}

void _AmRhythmDataPlug::DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid)
{
	ArpASSERT(view);
	view->SetHighColor( Prefs().Color(AM_DATA_BG_C) );
	view->FillRect(clip);

	_AmMotionGridBackground		gridBg(grid, mMtc);
	gridBg.DrawGrid(view, clip);

	AmSignatureMeasureBackground	bg(mSignatures, mMtc);
	bg.SetFlag(bg.DRAW_MEASURE_FLAG, true);
	bg.SetFlag(bg.DRAW_BEATS_FLAG, true);
	bg.DrawAllOn(view, clip);
	
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->StrokeLine( BPoint(clip.left, mMid), BPoint(clip.right, mMid) );

	for (uint32 k = 0; k < mHits.size(); k++) {
		BRect		r;
		AmRange		range = RangeFor(mHits[k].mPt.x, mHits[k].mEnd);
		r.left = mMtc.TickToPixel(range.start);
		r.right = mMtc.TickToPixel(range.end);
		float	strength = mHits[k].mPt.y;
		if (strength > 0) {
			r.top = mMid - (mMid * strength);
			r.bottom = mMid;
		} else if (strength < 0) {
			r.top = mMid;
			r.bottom = mMid + (mMid * fabs(strength));
		} else {
			r.top = r.bottom = mMid;
		}
		view->SetHighColor( AmPrefs().PrimaryColor() );
		view->FillRect(r);
		view->SetHighColor(0, 0, 0);
		view->StrokeRect(r);
	}
}

void _AmRhythmDataPlug::FrameResized(BView* view, float new_width, float new_height)
{
	ArpASSERT(view);
	mMid = view->Bounds().Height() / 2;
	view->Invalidate();
}

void _AmRhythmDataPlug::GetPreferredHeight(float* height)
{
	ArpASSERT(height);
	*height = mMid * 2;
}

void _AmRhythmDataPlug::Installed(BView* view, BScrollBar* hsb, BScrollBar* vsb)
{
	_AmMotionPlug::Installed(view, hsb, vsb);
	mMid = view->Bounds().Height() / 2;
}

float _AmRhythmDataPlug::HitPointYFromPixel(float y) const
{
	if (y < mMid) {
		float	ans = (mMid - y) / mMid;
		if (ans > 1) ans = 1;
		return ans;
	} else if (y > mMid) {
		float	ans = 1 - (y / mMid);
		if (ans < -1) ans = -1;
		return ans;
	} else return 0;
}

/*************************************************************************
 * _AM-ENVELOPE-INFO-PLUG
 *************************************************************************/
_AmEnvelopeInfoPlug::_AmEnvelopeInfoPlug(	const AmTimeConverter& mtc,
											vector<_AmMotionHit>& hits,
											AmSignaturePhrase& signatures)
		: _AmMotionPlug(mtc, hits, signatures)
{
}

void _AmEnvelopeInfoPlug::DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid)
{
	ArpASSERT(view);
	view->SetHighColor( Prefs().Color(AM_INFO_BG_C) );
	view->FillRect(clip);

	float			mid = bounds.Height() / 2;
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->SetLowColor( Prefs().Color(AM_INFO_BG_C) );
	float			fh = arp_get_font_height(view);

	const char*		label = "100\%";
	float			w = view->StringWidth(label) + 5;
	BRect			f(bounds.right - w, 0, bounds.right, fh);
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );

	label = "0";
	w = view->StringWidth(label) + 5;
	f.Set(bounds.right - w, mid - (fh / 2), bounds.right, mid + (fh / 2) );
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );

	label = "-100\%";
	w = view->StringWidth(label) + 5;
	f.Set(bounds.right - w, bounds.bottom - fh, bounds.right, bounds.bottom);
	if (f.Intersects(clip) ) view->DrawString(label, BPoint(f.left, f.bottom) );
}

void _AmEnvelopeInfoPlug::FrameResized(BView* view, float new_width, float new_height)
{
	ArpASSERT(view);
	view->Invalidate();
}

/*************************************************************************
 * _AM-ENVELOPE-DATA-PLUG
 *************************************************************************/
static const float		HANDLE_HALF_W		= 3;

_AmEnvelopeDataPlug::_AmEnvelopeDataPlug(	const AmTimeConverter& mtc,
											vector<_AmMotionHit>& hits,
											AmSignaturePhrase& signatures)
		: _AmMotionPlug(mtc, hits, signatures)
{
}

void _AmEnvelopeDataPlug::DrawOn(BView* view, BRect clip, BRect bounds, AmTime grid)
{
	ArpASSERT(view);
	view->SetHighColor( Prefs().Color(AM_DATA_BG_C) );
	view->FillRect(clip);

	_AmMotionGridBackground		gridBg(grid, mMtc);
	gridBg.DrawGrid(view, clip);

	AmSignatureMeasureBackground	bg(mSignatures, mMtc);
	bg.SetFlag(bg.DRAW_MEASURE_FLAG, true);
	bg.SetFlag(bg.DRAW_BEATS_FLAG, true);
	bg.DrawAllOn(view, clip);
	
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->StrokeLine( BPoint(clip.left, mMid), BPoint(clip.right, mMid) );

	int32			numPts = mHits.size() + 2;
	BPoint			pts[numPts];
	pts[0].Set(0, mMid);
	for (uint32 k = 0; k < mHits.size(); k++) {
		AmRange		range = RangeFor(mHits[k].mPt.x, mHits[k].mPt.x);
		float		yHit = mHits[k].mPt.y;
		pts[k + 1].x = mMtc.TickToPixel(range.start);
		if (yHit > 0) pts[k + 1].y = mMid - (mMid * yHit);
		else if (yHit < 0) pts[k + 1].y = mMid + (mMid * fabs(yHit));
		else pts[k + 1].y = mMid;
	}
	pts[numPts - 1].Set(pts[mHits.size()].x, mMid);
	
	view->SetHighColor( AmPrefs().PrimaryColor() );
	view->FillPolygon(pts, numPts);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->StrokePolygon(pts, numPts);

	for (int32 k = 1; k < numPts - 1; k++) {
		view->SetHighColor( AmPrefs().PrimaryColor() );
		BRect		r(	pts[k].x - HANDLE_HALF_W, pts[k].y - HANDLE_HALF_W,
						pts[k].x + HANDLE_HALF_W, pts[k].y + HANDLE_HALF_W);
		view->FillRect(BRect(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1));

		view->SetHighColor( tint_color(AmPrefs().PrimaryColor(), B_LIGHTEN_2_TINT) );
		view->StrokeLine( BPoint(r.right - 1, r.top + 1), BPoint(r.left + 1, r.top + 1) );
		view->StrokeLine( BPoint(r.left + 1, r.top + 2), BPoint(r.left + 1, r.bottom - 1) );
		view->SetHighColor( tint_color(AmPrefs().PrimaryColor(), B_DARKEN_2_TINT) );
		view->StrokeLine( BPoint(r.right - 1, r.top + 2), BPoint(r.right - 1, r.bottom - 1) );
		view->StrokeLine( BPoint(r.right - 2, r.bottom - 1), BPoint(r.left + 2, r.bottom - 1) );

		view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
		view->StrokeRect(r);		
	}
}

void _AmEnvelopeDataPlug::FrameResized(BView* view, float new_width, float new_height)
{
	ArpASSERT(view);
	mMid = view->Bounds().Height() / 2;
	view->Invalidate();
}

void _AmEnvelopeDataPlug::GetPreferredHeight(float* height)
{
	ArpASSERT(height);
	*height = mMid * 2;
}

void _AmEnvelopeDataPlug::Installed(BView* view, BScrollBar* hsb, BScrollBar* vsb)
{
	_AmMotionPlug::Installed(view, hsb, vsb);
	mMid = view->Bounds().Height() / 2;
}

void _AmEnvelopeDataPlug::CreateEvent(BView* view, BPoint where)
{
	ArpASSERT(view);
	/* If there's already a hit where the user clicked,
	 * then just do a change operation on that.
	 */
	if (ChangeEvent(view, where) ) return;

	float			start, end;
	GetGriddedHitRange(where.x, &start, &end);
	if (start < 0 || end < 0) return;
	float			width = end - start;
	BRect			hitR(view->Bounds() );
	hitR.left = 0;
	/* The ChangeEvent() has different rules for finding hits to
	 * change than I've got for finding a place to add a hit.  If
	 * the hit I would be adding would conflict with an existing one,
	 * then just do nothing.
	 */
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (start <= (mHits[k].mPt.x + width) && end >= mHits[k].mPt.x)
			return;
	}
	_AmMotionHit		hit(BPoint(start, HitPointYFromPixel(where.y)), end);
//	hitR.right = PixelXFromHitPoint(end);
	for (uint32 k = 0; k < mHits.size(); k++) {
		if (end < mHits[k].mPt.x) {
			mHits.insert(mHits.begin() + k, hit);
			view->Invalidate(hitR);
			view->Window()->PostMessage(CHANGED_MSG, view);	
			return;
		}
	}
	mHits.push_back(hit);
	view->Invalidate(hitR);
	view->Window()->PostMessage(CHANGED_MSG, view);	
	SetupScrollBars(view);
}

bool _AmEnvelopeDataPlug::ChangeEvent(BView* view, BPoint where)
{
	ArpASSERT(view);
	bool			foundHit = false;
	BRect			invalid = arp_invalid_rect();
	/* This is currently drawing from the left edge to the right of any changes
	 * -- it could be a little more efficient, but the loop wasn't capturing
	 * the right left edge info and it's not worth my time to clean up.
	 */
	float			prevPixel = 0;
	for (uint32 k = 0; k < mHits.size(); k++) {
		float		hitPixel = PixelXFromHitPoint(mHits[k].mPt.x);
		if (where.x >= (hitPixel - HANDLE_HALF_W) && where.x <= (hitPixel + HANDLE_HALF_W)) {
			BRect	hitR(view->Bounds() );
			if (k < mHits.size() - 1) hitR.right = PixelXFromHitPoint(mHits[k+1].mPt.x) + HANDLE_HALF_W;
			if (k > 0) hitR.left = PixelXFromHitPoint(mHits[k-1].mPt.x) - HANDLE_HALF_W;
			invalid = arp_merge_rects(invalid, hitR);
			mHits[k].mPt.y = HitPointYFromPixel(where.y);
			view->Window()->PostMessage(CHANGED_MSG, view);	
			foundHit = true;
		}
		if (where.x < hitPixel - (HANDLE_HALF_W + 1)) break;
	}
	if (arp_is_valid_rect(invalid) ) view->Invalidate(BRect(prevPixel, invalid.top, invalid.right, invalid.bottom) );
	return foundHit;
}

void _AmEnvelopeDataPlug::DeleteEvent(BView* view, BPoint where)
{
	ArpASSERT(view);
	BRect			invalid = arp_invalid_rect();
	bool			setScrollBars = false;
	for (uint32 k = 0; k < mHits.size(); k++) {
		float		hitPixel = PixelXFromHitPoint(mHits[k].mPt.x);
		if (where.x >= (hitPixel - HANDLE_HALF_W) && where.x <= (hitPixel + HANDLE_HALF_W)) {
			if (k == mHits.size() -1) setScrollBars = true;
			BRect	hitR(view->Bounds() );
			if (k < mHits.size() - 1) hitR.right = PixelXFromHitPoint(mHits[k+1].mPt.x) + HANDLE_HALF_W;
			if (k > 0) hitR.left = PixelXFromHitPoint(mHits[k-1].mPt.x) - HANDLE_HALF_W;
			invalid = arp_merge_rects(invalid, hitR);
			mHits.erase(mHits.begin() + k);
			view->Window()->PostMessage(CHANGED_MSG, view);	
			k = 0;
		}
	}
	if (arp_is_valid_rect(invalid) ) view->Invalidate(invalid);
	if (setScrollBars) SetupScrollBars(view);
}

float _AmEnvelopeDataPlug::HitPointYFromPixel(float y) const
{
	if (y < mMid) {
		float	ans = (mMid - y) / mMid;
		if (ans > 1) ans = 1;
		return ans;
	} else if (y > mMid) {
		float	ans = 1 - (y / mMid);
		if (ans < -1) ans = -1;
		return ans;
	} else return 0;
}

/*************************************************************************
 * _AM-MEASURE-CONTROL
 *************************************************************************/
static const uint32		CHANGE_SIGNATURE_MSG	= 'iChS';

_AmMeasureControl::_AmMeasureControl(	BRect frame,
										const char* name,
										AmSignaturePhrase& signatures,
										AmTimeConverter& mtc,
										float leftIndent,
										int32 resizeMask)
		: inherited(frame, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMtc(mtc),
		  mLeftIndent(leftIndent), mScrollX(0),
		  mLeftBg(0), mSignatures(signatures),
		  mDownPt(0, 0), mCachedEndTime(0)
{
}


_AmMeasureControl::~_AmMeasureControl()
{
	delete mLeftBg;
}

void _AmMeasureControl::SetLeftIndent(float leftIndent)
{
	mLeftIndent = leftIndent;
	if (mLeftIndent > 0) {
		BRect		b = Bounds();
		delete mLeftBg;
		ConstructLeftBg(BRect(0, 0, mLeftIndent - 1, b.Height() - 1));
	}
	Invalidate();
}

void _AmMeasureControl::Draw(BRect updateRect)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>(Window() );
	if (cache) into = cache->StartDrawing(this, updateRect);
	
	DrawOn(updateRect, into);
	if (cache) cache->FinishDrawing(into);
}

void _AmMeasureControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	mViewColor = Prefs().Color(AM_MEASURE_TOP_BG_C);

	BRect		b = Bounds();
	if (mLeftIndent > 0) {
		delete mLeftBg;
		ConstructLeftBg(BRect(0, 0, mLeftIndent - 1, b.Height() - 1));
	}

	SetFontSize(10);
}

void _AmMeasureControl::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	Invalidate();
}

void _AmMeasureControl::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case CHANGE_SIGNATURE_MSG:
			HandleSignatureMsg(msg);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void _AmMeasureControl::MouseDown(BPoint pt)
{
	MakeFocus(true);
	BPoint		where;
	ulong		buttons;
	GetMouse(&where, &buttons, false);
	if (buttons&B_SECONDARY_MOUSE_BUTTON) {
		ShowTimeSignatureMenu(pt);
		return;
	}
}

void _AmMeasureControl::ScrollTo(BPoint where)
{
	mScrollX = where.x;
	Invalidate();
}

static float view_font_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

void _AmMeasureControl::DrawOn(BRect updateRect, BView* view)
{
	BRect				b = Bounds();
	BRect				lBounds(b.left, b.top, mLeftIndent - 1, b.bottom);
	BRect				cBounds(b.left + mLeftIndent, b.top, b.right, b.bottom);

	AmTime				endTime = 0;
	if (mLeftIndent > 0) DrawLeftBgOn(lBounds, view, endTime);
	DrawCenterBgOn(cBounds, view, endTime);

	DrawCenterOn(b, view);
	if (mLeftIndent > 0) DrawLeftOn( lBounds, view );

	// Draw some edges
	view->SetHighColor( tint_color( mViewColor, B_LIGHTEN_2_TINT) );
	view->StrokeLine( BPoint(b.left, b.top), BPoint(b.right, b.top) );
	view->SetHighColor( 0, 0, 0 );
	view->StrokeLine( BPoint(b.left, b.top), BPoint(b.right, b.top) );
	view->StrokeLine( BPoint(b.left, b.bottom), BPoint(b.right, b.bottom) );
}

void _AmMeasureControl::DrawLeftBgOn(BRect lBounds, BView* view, AmTime songEndTime)
{
	// If the left isn't fixed then the DrawLeftOn() method will
	// handle drawing the background, since it wants to obliterate
	// any drawing the center might have done in the left view.
	if ( !IsLeftFixed() ) return;
	if ( mLeftBg ) view->DrawBitmapAsync( mLeftBg, BPoint(0, 1) );
}

void _AmMeasureControl::DrawCenterBgOn(BRect cBounds, BView* view, AmTime songEndTime)
{
	BPoint		left(cBounds.left, cBounds.top + 1);
	BPoint		right(cBounds.right, cBounds.top + 1);
	rgb_color	c = Prefs().Color( AM_MEASURE_TOP_BG_C );
	rgb_color	bc = Prefs().Color( AM_MEASURE_BOTTOM_BG_C );
	float		r_delta = 0, g_delta = 0, b_delta = 0;
	float		height = cBounds.bottom - left.y;
	if( c.red != bc.red )		r_delta = ((float)bc.red - (float)c.red) / height;
	if( c.green != bc.green )	g_delta = ((float)bc.green - (float)c.green) / height;
	if( c.blue != bc.blue )		b_delta = ((float)bc.blue - (float)c.blue) / height;

	float		i = 1;
	rgb_color	o = c;
	while (left.y < cBounds.bottom) {
		view->SetHighColor( c );
		view->StrokeLine( left, right );
		c.red = (uint8)(o.red + (i * r_delta));
		c.green = (uint8)(o.green + (i * g_delta));
		c.blue = (uint8)(o.blue + (i * b_delta));
		left.y++;
		right.y++;
		i++;
	}
}

void _AmMeasureControl::DrawLeftOn(BRect lBounds, BView* view)
{
	// If it's fixed than I just let the center view take care of drawing it.
	if (IsLeftFixed() ) return;

	if (mLeftBg) view->DrawBitmapAsync( mLeftBg, BPoint(0, 1) );
	LockedDrawLeftOn(mSignatures, lBounds, view);
}

void _AmMeasureControl::LockedDrawLeftOn(const AmPhrase& signatures, BRect lBounds, BView* view)
{
	AmTime				rightTick = mMtc.PixelToTick(mScrollX);
	AmTimeConverter		mtc( mLeftIndent / ((float)rightTick / (float)PPQN) );
	AmNode*				node = signatures.HeadNode();
	if (node == 0) return;
	AmSignature*		sig = dynamic_cast<AmSignature*>( node->Event() );
	if (sig == 0) return;
	AmSignature			currentSig(*sig);
	AmTime				sigLength = currentSig.Duration();
	AmSignature*		nextSig = 0;
	AmNode*				nextNode = node->next;
	if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	char				buf[16];
	float				lastRight = -1;

	/* Draw the center dividing line.
	 */
	float				fh = view_font_height(view);
	lBounds.top = lBounds.bottom - fh;
	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->StrokeLine( BPoint(lBounds.left, lBounds.top-1), BPoint(lBounds.right, lBounds.top-1) );
	view->SetHighColor( Prefs().Color( AM_MEASURE_HIGHLIGHT_C ) );
	view->StrokeLine( BPoint(lBounds.left, lBounds.top), BPoint(lBounds.right, lBounds.top) );


	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	while (currentSig.EndTime() < rightTick) {

		float		pixel = mtc.TickToPixel( currentSig.StartTime() );
		if( pixel > lastRight ) {
			view->StrokeLine( BPoint(pixel, lBounds.top), BPoint(pixel, lBounds.bottom) );
			sprintf( buf, "%ld", currentSig.Measure() );
			float	width = view->StringWidth( buf );
			if( pixel + 2 + width > mLeftIndent ) {
				lastRight = pixel + 2;
			} else {
				view->DrawString( buf, BPoint( pixel + 2, lBounds.bottom - 2 ) );
				lastRight = pixel + 2 + width + 2;
			}
		}
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			int32	measure = currentSig.Measure();
			currentSig.Set( *nextSig );
			currentSig.SetMeasure( measure );
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = (AmNode*)nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
}

void _AmMeasureControl::DrawCenterOn(BRect bounds, BView* view)
{
	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->SetHighColor( Prefs().Color(AM_MEASURE_FG_C) );
	view->SetLowColor( Prefs().Color(AM_MEASURE_BOTTOM_BG_C) );

	AmSignatureMeasureBackground		bg(mSignatures, mMtc);
	bg.SetScrollX(mScrollX);
	bg.SetLeftIndent(mLeftIndent);
	bg.SetFlag(bg.DRAW_SIGNATURES_FLAG, true);
	bg.SetFlag(bg.DRAW_MEASURE_FLAG, true);
	/* This looks like a hack I did for drawing the measure in
	 * the track window.  However, as of now, I have no plans to
	 * allow tracks to have their own signatures, so this is
	 * currently a waste of time.  Probably, if I do want to reinstate
	 * this, I should do it a better way, like another subclass.  This
	 * is just gross.
	 */
	bg.DrawAllOn(view, bounds);
}

void _AmMeasureControl::HandleSignatureMsg(BMessage* msg)
{
	int32	measure;
	if (msg->FindInt32("measure", &measure) != B_OK) return;
	int32	beats = -1, beatvalue = -1;
	if (msg->FindInt32("beats", &beats) != B_OK) beats = -1;
	if (msg->FindInt32("beat value", &beatvalue) != B_OK) beatvalue = -1;

	if (beats < 1 || beatvalue < 1) {
		if ( Window() ) Window()->PostMessage(msg);
	} else {
#if 0
		// WRITE SONG OR TRACK BLOCK
		AmSong*		song = WriteLock();
		if (song) {
#if 0
			if ( mTrackRef.IsValid() ) {
				AmTrack*	track = song ? song->Track(mTrackRef) : 0;
				if (track) track->SetSignature(measure, beats, beatvalue);
			} else {
#endif
				song->SetSignature(measure, beats, beatvalue);
#if 0
			}
#endif
		}
		WriteUnlock(song);
		// END WRITE SONG OR TRACK BLOCK
#endif
	}
}

void _AmMeasureControl::ShowTimeSignatureMenu(BPoint pt) const
{
#if 0
	AmSignature		sig;
	if (SignatureForPt(pt, sig) != B_OK) return;
	BPopUpMenu*		menu = new BPopUpMenu("menu");
	if (!menu) return;
	menu->SetFontSize(10);
	menu->SetAsyncAutoDestruct(true);

	BMessage	signatureChoices;
	bool		needSeparator = false;
	if (seq_get_message_preference(SIGNATURE_CHOICES_PREF, &signatureChoices) == B_OK) {
		int32	beats;
		for(int32 k = 0; signatureChoices.FindInt32("beats", k, &beats) == B_OK; k++) {
			int32	beatvalue;
			if (signatureChoices.FindInt32("beat value", k, &beatvalue) == B_OK) {
				BString		label;
				label << beats << " / " << beatvalue;
				BMessage*	msg = new BMessage(CHANGE_SIGNATURE_MSG);
				BMenuItem*	item;
				if (msg && (item = new BMenuItem(label.String(), msg)) ) {
					needSeparator = true;
					msg->AddInt32("measure", sig.Measure() );
					msg->AddInt32("beats", beats);
					msg->AddInt32("beat value", beatvalue);
					menu->AddItem(item);
					item->SetTarget(this);
				}
			}
		}
	}
	if (needSeparator) menu->AddSeparatorItem();
	BMessage*		msg = new BMessage(CHANGE_SIGNATURE_MSG);
	BMenuItem*		item;
	if ( msg && (item = new BMenuItem("Other...", msg)) ) {
		msg->AddInt32("measure", sig.Measure() );
		msg->AddInt32("beats", sig.Beats() );
		msg->AddInt32("beat value", sig.BeatValue() );
		menu->AddItem(item);
		item->SetTarget( Window() );
	}
	BRect	frame(pt, pt);
	menu->Go( ConvertToScreen(pt), true, false, ConvertToScreen(frame), true);
#endif
}

status_t _AmMeasureControl::SignatureForPt(BPoint pt, AmSignature& sig) const
{
	status_t	err = B_ERROR;
	AmTime		time = mMtc.PixelToTick(pt.x + mScrollX - mLeftIndent);
	/* If the time being requested is less than the first measure, constrain
	 * it to the first measure.
	 */
	if (time < 0) time = 0;
	err = mSignatures.GetSignature(time, sig);
	return err;
}

bool _AmMeasureControl::IsLeftFixed() const
{
	if (mLeftIndent == 0) return true;
	return (mScrollX - mLeftIndent < 0);
}

AmTime _AmMeasureControl::CenterLeftTime() const
{
	return mMtc.PixelToTick(mScrollX);
}

void _AmMeasureControl::ConstructLeftBg(BRect bounds)
{
	if ( !Window() ) return;
	BScreen			screen( Window() );

	mLeftBg = new BBitmap(bounds, screen.ColorSpace() );
	if (!mLeftBg) return;
		pixel_access	pa(mLeftBg->ColorSpace() );
	
	BRect			b = mLeftBg->Bounds();
	rgb_color		c = Prefs().Color( AM_MEASURE_TOP_BG_C );
	float			r_row_delta = 0, g_row_delta = 0, b_row_delta = 0;
	{
		rgb_color	bc = Prefs().Color(AM_MEASURE_BOTTOM_BG_C);
		float		height = b.bottom;
		if (c.red != bc.red)		r_row_delta = ((float)bc.red - (float)c.red) / height;
		if (c.green != bc.green)	g_row_delta = ((float)bc.green - (float)c.green) / height;
		if (c.blue != bc.blue)		b_row_delta = ((float)bc.blue - (float)c.blue) / height;
	}
	rgb_color		rowC = c;
	rgb_color		leftC = Prefs().Color(AM_MEASURE_LEFT_BG_C);
	float			i = 1;
	for (float k=0; k<b.bottom; k++) {
		float		r_col_delta = 0, g_col_delta = 0, b_col_delta = 0;
		float		width = b.right;
		if (rowC.red != leftC.red)		r_col_delta = ((float)leftC.red - (float)rowC.red) / width;
		if (rowC.green != leftC.green)	g_col_delta = ((float)leftC.green - (float)rowC.green) / width;
		if (rowC.blue != leftC.blue)	b_col_delta = ((float)leftC.blue - (float)rowC.blue) / width;
		for (float j=b.right; j>=0; j--) {
			rgb_color	c;
			c.red = (uint8)(rowC.red + ( fabs(b.right - j) * r_col_delta));
			c.green = (uint8)(rowC.green + ( fabs(b.right - j) * g_col_delta));
			c.blue = (uint8)(rowC.blue + ( fabs(b.right - j) * b_col_delta));
						write_pixel(mLeftBg, j, k, c, pa);
		}
		rowC.red = (uint8)(c.red + (i * r_row_delta));
		rowC.green = (uint8)(c.green + (i * g_row_delta));
		rowC.blue = (uint8)(c.blue + (i * b_row_delta));
		i++;
	}
}
