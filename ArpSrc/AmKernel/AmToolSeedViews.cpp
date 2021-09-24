/* AmToolSeedsUI.cpp
 */
#include <cstdio>
#include <interface/StringView.h>
#include <interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpFloatControl.h"
#include "AmPublic/AmToolSeedI.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolSeeds.h"
#include "AmKernel/AmToolSeedViews.h"

static const uint32		WRITE_TOOL_MSG		= 'iwri';
static const uint32		SCROLL_MSG			= 'iscr';

/* These are all the names used in the configuration methods.  NOTE THAT THESE
 * ARE SHARED WITH AmToolSeeds.cpp, so if you change them here, you better
 * change them there.
 */
static const char*	FLAGS_STR			= "flags";

/***************************************************************************
 * AM-TOOL-SEED-VIEW
 ***************************************************************************/
AmToolSeedView::AmToolSeedView()
		: inherited(BRect(0, 0, 0, 0), "tool_seed_view", B_FOLLOW_ALL, B_WILL_DRAW),
		  mScrollBox(NULL)
{
}

AmToolSeedView::~AmToolSeedView()
{
}

void AmToolSeedView::SetToolRef(AmToolRef toolRef)
{
	mToolRef = toolRef;
}

status_t AmToolSeedView::SetInfo(	const BString& factoryKey,
									const BString& viewKey,
									const BString& seedKey)
{
	mFactoryKey = factoryKey;
	mViewKey = viewKey;
	mSeedKey = seedKey;
	BMessage			config;
	status_t			err = B_OK;
	BString				testKey;
	// READ TOOL BLOCK
	const AmTool*		tool = mToolRef.ReadLock();
	if (tool) err = tool->GetSeed(factoryKey, viewKey, testKey, &config);
	mToolRef.ReadUnlock(tool);
	// END READ TOOL BLOCK
	ArpASSERT(testKey == seedKey);
	if (err == B_OK) {
		SetConfiguration(&config);
		return B_OK;
	}
	return err;
}

void AmToolSeedView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (Parent() ) SetViewColor(Parent()->ViewColor() );
	if (mScrollBox) mScrollBox->SetTarget(this);
}

void AmToolSeedView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case SCROLL_MSG:
		case WRITE_TOOL_MSG:
			WriteToToolRef();
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

status_t AmToolSeedView::SetConfiguration(const BMessage* config)
{
	int32		i;
	if (config->FindInt32(FLAGS_STR, &i) == B_OK) {
		if (mScrollBox && i&AmToolSeedI::AM_SCROLL_FLAG) mScrollBox->SetValue(B_CONTROL_ON);
	}
	return B_OK;
}

status_t AmToolSeedView::GetConfiguration(BMessage* config) const
{
	config->AddInt32(FLAGS_STR, BuildFlags() );
	return B_OK;
}

status_t AmToolSeedView::WriteToToolRef()
{
	BMessage		config;
	status_t		err = GetConfiguration(&config);
	if (err != B_OK) return err;

	// WRITE TOOL BLOCK
	AmTool*		tool = mToolRef.WriteLock();
	if (tool) tool->SetSeed(mFactoryKey, mViewKey, mSeedKey, &config);
	mToolRef.WriteUnlock(tool);
	// END WRITE TOOL BLOCK
	
	if (Window() ) Window()->PostMessage(AM_TOOL_SEED_CHANGED_MSG);
	return B_OK;
}

uint32 AmToolSeedView::BuildFlags() const
{
	uint32		flags = 0;
	if (mScrollBox && mScrollBox->Value() == B_CONTROL_ON) flags |= AmToolSeedI::AM_SCROLL_FLAG;
	return flags;
}

BPoint AmToolSeedView::AddViews()
{
	float			spaceX = 5, spaceY = 5;
	float			cbH = Prefs().Size(BOX_CTRL_Y);
	const char*		label = "Scroll";
	float			labelW = StringWidth(label) + 50;
	BRect			f(spaceX, spaceY, spaceX + labelW, spaceY + cbH);
	/* The Scroll box.
	 */
	mScrollBox = new BCheckBox(f, "scroll_box", label, new BMessage(SCROLL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mScrollBox) {
		AddChild(mScrollBox);
		f = mScrollBox->Frame();
	}
	return f.RightBottom();
}

// #pragma mark -

/***************************************************************************
 * AM-BOX-SEED-VIEW
 ***************************************************************************/
AmBoxSeedView::AmBoxSeedView()
{
	AddViews();
}

// #pragma mark -

/***************************************************************************
 * AM-BEZIER-SEED-VIEW
 ***************************************************************************/
AmBezierSeedView::AmBezierSeedView()
		: mPt1StartX(0), mPt1StartY(0), mPt1EndX(0), mPt1EndY(0),
		  mPt2StartX(0), mPt2StartY(0), mPt2EndX(0), mPt2EndY(0),
		  mFrame(0), mCreateBox(0), mMoveCtrl(0), mTransformCtrl(0)
{
	AddViews();
}

AmBezierSeedView::~AmBezierSeedView()
{
}

void AmBezierSeedView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mCreateBox) mCreateBox->SetTarget(this);
	if (mPt1StartX) mPt1StartX->SetTarget(this);
	if (mPt1StartY) mPt1StartY->SetTarget(this);
	if (mPt1EndX) mPt1EndX->SetTarget(this);
	if (mPt1EndY) mPt1EndY->SetTarget(this);
	if (mPt2StartX) mPt2StartX->SetTarget(this);
	if (mPt2StartY) mPt2StartY->SetTarget(this);
	if (mPt2EndX) mPt2EndX->SetTarget(this);
	if (mPt2EndY) mPt2EndY->SetTarget(this);
	if (mFrame) mFrame->SetTarget(this);
	if (mMoveCtrl) mMoveCtrl->SetTarget(this);
	if (mTransformCtrl) mTransformCtrl->SetTarget(this);
}

status_t AmBezierSeedView::SetConfiguration(const BMessage* config)
{
	status_t	err = inherited::SetConfiguration(config);
	if (err != B_OK) return err;
	BPoint		pt;
	if (mPt1StartX && mPt1StartY && config->FindPoint(AM_BEZIER_SEED_PT1START, &pt) == B_OK) {
		mPt1StartX->SetValue(pt.x);
		mPt1StartY->SetValue(pt.y);
	}
	if (mPt1EndX && mPt1EndY && config->FindPoint(AM_BEZIER_SEED_PT1END, &pt) == B_OK) {
		mPt1EndX->SetValue(pt.x);
		mPt1EndY->SetValue(pt.y);
	}
	if (mPt2StartX && mPt2StartY && config->FindPoint(AM_BEZIER_SEED_PT2START, &pt) == B_OK) {
		mPt2StartX->SetValue(pt.x);
		mPt2StartY->SetValue(pt.y);
	}
	if (mPt2EndX && mPt2EndY && config->FindPoint(AM_BEZIER_SEED_PT2END, &pt) == B_OK) {
		mPt2EndX->SetValue(pt.x);
		mPt2EndY->SetValue(pt.y);
	}
	float		f;
	if (mFrame && config->FindFloat(AM_BEZIER_SEED_FRAME, &f) == B_OK)
		mFrame->SetValue(f);
	int32		i32;
	if (config->FindInt32(AM_BEZIER_SEED_MODE, &i32) == B_OK) {
		if (mCreateBox) {
			if (i32&AmBezierToolSeed::CREATE_MODE) mCreateBox->SetValue(B_CONTROL_ON);
			else mCreateBox->SetValue(B_CONTROL_OFF);
		}
		if (mMoveCtrl && mTransformCtrl) {
			if (i32&AmBezierToolSeed::MOVE_MODE) mMoveCtrl->SetValue(B_CONTROL_ON);
			else mTransformCtrl->SetValue(B_CONTROL_ON);
		}
	}
	return B_OK;
}

status_t AmBezierSeedView::GetConfiguration(BMessage* config) const
{
	status_t		err = inherited::GetConfiguration(config);
	if (err != B_OK) return err;
	if (mPt1StartX && mPt1StartY) config->AddPoint(AM_BEZIER_SEED_PT1START, BPoint(mPt1StartX->Value(), mPt1StartY->Value()));
	if (mPt1EndX && mPt1EndY) config->AddPoint(AM_BEZIER_SEED_PT1END, BPoint(mPt1EndX->Value(), mPt1EndY->Value()));
	if (mPt2StartX && mPt2StartY) config->AddPoint(AM_BEZIER_SEED_PT2START, BPoint(mPt2StartX->Value(), mPt2StartY->Value()));
	if (mPt2EndX && mPt2EndY) config->AddPoint(AM_BEZIER_SEED_PT2END, BPoint(mPt2EndX->Value(), mPt2EndY->Value()));
	if (mFrame) config->AddFloat(AM_BEZIER_SEED_FRAME, mFrame->Value());
	int32		mode = 0;
	if (mCreateBox && mCreateBox->Value() == B_CONTROL_ON) mode |= AmBezierToolSeed::CREATE_MODE;
	if (mMoveCtrl && mMoveCtrl->Value() == B_CONTROL_ON) mode |= AmBezierToolSeed::MOVE_MODE;
	else mode |= AmBezierToolSeed::TRANFORM_MODE;
	config->AddInt32(AM_BEZIER_SEED_MODE, mode);
	
	return B_OK;
}

static void shift_int_frames(BRect& f1, BRect& f2, float sy, float ih)
{
	f1.top = f1.bottom + sy;
	f1.bottom = f1.top + ih;
	f2.top = f2.bottom + sy;
	f2.bottom = f2.top + ih;
}

BPoint AmBezierSeedView::AddViews()
{
	float		fh = arp_get_font_height(this);
	float		spaceX = 5, spaceY = 5;
	BPoint		end = inherited::AddViews();
	const char*	lab1 = "Point 1 start x:";
	const char*	lab2 = "Point 1 end x:";
	const char*	lab3 = "Point 2 start x:";
	const char*	lab4 = "Point 2 end x:";
	const char*	lab5 = "y:";
	float		div1 = StringWidth(lab3) + spaceX, div2 = StringWidth(lab5) + spaceX;
	float		floatCtrlW = StringWidth("0000.00") + 10;
	float		ih = Prefs().Size(INT_CTRL_Y);
	BRect		col1(spaceX, end.y + spaceY, spaceX + div1 + floatCtrlW, end.y + spaceY + ih);
	BRect		col2(col1);
	col2.left = col1.right + spaceX;
	col2.right = col2.left + div2 + floatCtrlW;
	float		steps = 0.01;
	
	/* Point 1 start
	 */
	mPt1StartX = new ArpFloatControl(col1, "1sx", lab1, new BMessage(WRITE_TOOL_MSG));
	if (mPt1StartX) {
		mPt1StartX->SetDivider(div1);
		mPt1StartX->SetLimits(-10, 10);
		mPt1StartX->SetSteps(steps);
		AddChild(mPt1StartX);
	}
	mPt1StartY = new ArpFloatControl(col2, "1sy", lab5, new BMessage(WRITE_TOOL_MSG));
	if (mPt1StartY) {
		mPt1StartY->SetDivider(div2);
		mPt1StartY->SetLimits(-10, 10);
		mPt1StartY->SetSteps(steps);
		AddChild(mPt1StartY);
	}
	/* Point 1 end
	 */
	shift_int_frames(col1, col2, spaceY, ih);
	mPt1EndX = new ArpFloatControl(col1, "1ex", lab2, new BMessage(WRITE_TOOL_MSG));
	if (mPt1EndX) {
		mPt1EndX->SetDivider(div1);
		mPt1EndX->SetLimits(-10, 10);
		mPt1EndX->SetSteps(steps);
		AddChild(mPt1EndX);
	}
	mPt1EndY = new ArpFloatControl(col2, "1ey", lab5, new BMessage(WRITE_TOOL_MSG));
	if (mPt1EndY) {
		mPt1EndY->SetDivider(div2);
		mPt1EndY->SetLimits(-10, 10);
		mPt1EndY->SetSteps(steps);
		AddChild(mPt1EndY);
	}
	/* Point 2 start
	 */
	shift_int_frames(col1, col2, spaceY, ih);
	mPt2StartX = new ArpFloatControl(col1, "2sx", lab3, new BMessage(WRITE_TOOL_MSG));
	if (mPt2StartX) {
		mPt2StartX->SetDivider(div1);
		mPt2StartX->SetLimits(-10, 10);
		mPt2StartX->SetSteps(steps);
		mPt2StartX->SetValue(1);
		AddChild(mPt2StartX);
	}
	mPt2StartY = new ArpFloatControl(col2, "2sy", lab5, new BMessage(WRITE_TOOL_MSG));
	if (mPt2StartY) {
		mPt2StartY->SetDivider(div2);
		mPt2StartY->SetLimits(-10, 10);
		mPt2StartY->SetSteps(steps);
		mPt2StartY->SetValue(1);
		AddChild(mPt2StartY);
	}
	/* Point 2 end
	 */
	shift_int_frames(col1, col2, spaceY, ih);
	mPt2EndX = new ArpFloatControl(col1, "2ex", lab4, new BMessage(WRITE_TOOL_MSG));
	if (mPt2EndX) {
		mPt2EndX->SetDivider(div1);
		mPt2EndX->SetLimits(-10, 10);
		mPt2EndX->SetSteps(steps);
		mPt2EndX->SetValue(1);
		AddChild(mPt2EndX);
	}
	mPt2EndY = new ArpFloatControl(col2, "2ey", lab5, new BMessage(WRITE_TOOL_MSG));
	if (mPt2EndY) {
		mPt2EndY->SetDivider(div2);
		mPt2EndY->SetLimits(-10, 10);
		mPt2EndY->SetSteps(steps);
		mPt2EndY->SetValue(1);
		AddChild(mPt2EndY);
	}
	/* Frame
	 */
	BRect		ff(col1);
	ff.top = ff.bottom + spaceY;
	ff.bottom = ff.top + ih;
	mFrame = new ArpFloatControl(ff, "fr", "Frame:", new BMessage(WRITE_TOOL_MSG));
	if (mFrame) {
		mFrame->SetDivider(div1);
		mFrame->SetSteps(steps);
		AddChild(mFrame);
	}
	/* Create box.
	 */
	BRect		cf(ff.left, ff.bottom + spaceY, ff.left, ff.bottom + spaceY);
	const char*	createLabel = "Create events";
	cf.right = cf.left + StringWidth(createLabel) + 50;
	cf.bottom = cf.top + fh + 10;
	mCreateBox = new BCheckBox(cf, "create_box", createLabel, new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mCreateBox) {
		mCreateBox->SetValue(B_CONTROL_OFF);
		AddChild(mCreateBox);
		cf.top = mCreateBox->Frame().bottom;
	}

	/* Add the new event fields.
	 */
	const char*	changeEventsLabel = "Change events by:";
	BRect		f(cf);
	f.right = StringWidth(changeEventsLabel) + 30;
	f.top += spaceY;
	f.bottom = f.top + fh;
	BStringView*	sv = new BStringView(f, "change_view", changeEventsLabel, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (sv) {
		AddChild(sv);
		f.top = sv->Frame().bottom;
	}
	f.top += spaceY;
	mTransformCtrl = new BRadioButton(f, "trans_ctrl", "Transforming them", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mTransformCtrl) {
		mTransformCtrl->SetValue(B_CONTROL_ON);
		AddChild(mTransformCtrl);
		f.top = mTransformCtrl->Frame().bottom;
	}	
	f.top += spaceY;
	mMoveCtrl = new BRadioButton(f, "move_ctrl", "Moving them", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mMoveCtrl) {
		AddChild(mMoveCtrl);
		f.top = mMoveCtrl->Frame().bottom;
	}

	return BPoint(col2.right, f.top);
}

// #pragma mark -

/***************************************************************************
 * AM-MOVE-SEED-VIEW
 ***************************************************************************/
AmMoveSeedView::AmMoveSeedView()
{
	AddViews();
}

// #pragma mark -

/***************************************************************************
 * AM-TOUCH-SEED-VIEW
 ***************************************************************************/
AmTouchSeedView::AmTouchSeedView()
{
	AddViews();
}

// #pragma mark -

/***************************************************************************
 * AM-TRANSFORM-SEED-VIEW
 ***************************************************************************/
AmTransformSeedView::AmTransformSeedView()
		: mOneByOneCtrl(NULL), mEnMasseCtrl(NULL)
{
	AddViews();
}

AmTransformSeedView::~AmTransformSeedView()
{
}

void AmTransformSeedView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mOneByOneCtrl) mOneByOneCtrl->SetTarget(this);
	if (mEnMasseCtrl) mEnMasseCtrl->SetTarget(this);
}

status_t AmTransformSeedView::SetConfiguration(const BMessage* config)
{
	status_t	err = inherited::SetConfiguration(config);
	if (err != B_OK) return err;
	int32		i;
	if (mOneByOneCtrl && mEnMasseCtrl
			&& config->FindInt32(FLAGS_STR, &i) == B_OK) {
		if (i&AmTransformToolSeed::ONE_BY_ONE_FLAG) mOneByOneCtrl->SetValue(B_CONTROL_ON);
		else mEnMasseCtrl->SetValue(B_CONTROL_ON);
	}
	return B_OK;
}

uint32 AmTransformSeedView::BuildFlags() const
{
	uint32		flags = inherited::BuildFlags();
	if (mOneByOneCtrl && mOneByOneCtrl->Value() == B_CONTROL_ON) flags |= AmTransformToolSeed::ONE_BY_ONE_FLAG;
	else if (mEnMasseCtrl && mEnMasseCtrl->Value() == B_CONTROL_ON) flags |= AmTransformToolSeed::EN_MASSE_FLAG;
	return flags;
}

BPoint AmTransformSeedView::AddViews()
{
	float		fh = arp_get_font_height(this);
	float		spaceX = 5, spaceY = 5;
	BPoint		end = inherited::AddViews();
	BRect		f(spaceX, end.y + spaceY, spaceX, end.y + spaceY);

	/* Add the target fields.
	 */
	const char*	transLabel = "Transform:";
	f.right = StringWidth(transLabel) + 30;
	f.bottom = f.top + fh;
	BStringView*	sv = new BStringView(f, "trans_label", transLabel, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (sv) {
		AddChild(sv);
		f.top = sv->Frame().bottom;
	}
	f.top += spaceY;
	mOneByOneCtrl = new BRadioButton(f, "one_by_one_ctrl", "One by one", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mOneByOneCtrl) {
		AddChild(mOneByOneCtrl);
		f.top = mOneByOneCtrl->Frame().bottom;
	}
	f.top += spaceY;
	mEnMasseCtrl = new BRadioButton(f, "en_masse_ctrl", "En masse", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mEnMasseCtrl) {
		mEnMasseCtrl->SetValue(B_CONTROL_ON);
		AddChild(mEnMasseCtrl);
		f.top = mOneByOneCtrl->Frame().bottom;
	}
	
	return f.RightBottom();
}

// #pragma mark -

/***************************************************************************
 * AM-CREATE-SEED-VIEW
 ***************************************************************************/
AmCreateSeedView::AmCreateSeedView()
		: mMoveCtrl(NULL), mAddNCtrl(NULL), mTransformCtrl(NULL),
		  mRefilterCtrl(NULL)
{
	AddViews();
}

AmCreateSeedView::~AmCreateSeedView()
{
}

void AmCreateSeedView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mMoveCtrl) mMoveCtrl->SetTarget(this);
	if (mAddNCtrl) mAddNCtrl->SetTarget(this);
	if (mTransformCtrl) mTransformCtrl->SetTarget(this);
	if (mRefilterCtrl) mRefilterCtrl->SetTarget(this);
}

status_t AmCreateSeedView::SetConfiguration(const BMessage* config)
{
	status_t	err = inherited::SetConfiguration(config);
	if (err != B_OK) return err;
	int32		i;
	if (mMoveCtrl && mAddNCtrl && mTransformCtrl
			&& config->FindInt32(FLAGS_STR, &i) == B_OK) {
		if (i&AmCreateToolSeed::ADDN_FLAG) mAddNCtrl->SetValue(B_CONTROL_ON);
		else if (i&AmCreateToolSeed::TRANSFORM_FLAG) mTransformCtrl->SetValue(B_CONTROL_ON);
		else if (i&AmCreateToolSeed::REFILTER_FLAG) mRefilterCtrl->SetValue(B_CONTROL_ON);
		else mMoveCtrl->SetValue(B_CONTROL_ON);
	}
	return B_OK;
}

uint32 AmCreateSeedView::BuildFlags() const
{
	uint32		flags = inherited::BuildFlags();
	if (mAddNCtrl && mAddNCtrl->Value() == B_CONTROL_ON) flags |= AmCreateToolSeed::ADDN_FLAG;
	else if (mTransformCtrl && mTransformCtrl->Value() == B_CONTROL_ON) flags |= AmCreateToolSeed::TRANSFORM_FLAG;
	else if (mRefilterCtrl && mRefilterCtrl->Value() == B_CONTROL_ON) flags |= AmCreateToolSeed::REFILTER_FLAG;
	else flags |= AmCreateToolSeed::MOVE_FLAG;
	return flags;
}

BPoint AmCreateSeedView::AddViews()
{
	float		fh = arp_get_font_height(this);
	float		spaceX = 5, spaceY = 5;
	BPoint		end = inherited::AddViews();
	BRect		f(spaceX, end.y + spaceY, spaceX, end.y + spaceY);

	/* Add the new event fields.
	 */
	const char*	newEventLabel = "After adding event:";
	f.right = StringWidth(newEventLabel) + 30;
	f.bottom = f.top + fh;
	BStringView*	sv = new BStringView(f, "action_view", newEventLabel, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (sv) {
		AddChild(sv);
		f.top = sv->Frame().bottom;
	}
	f.top += spaceY;
	mMoveCtrl = new BRadioButton(f, "move_ctrl", "Move selection", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mMoveCtrl) {
		mMoveCtrl->SetValue(B_CONTROL_ON);
		AddChild(mMoveCtrl);
		f.top = mMoveCtrl->Frame().bottom;
	}
	f.top += spaceY;
	mAddNCtrl = new BRadioButton(f, "addn_ctrl", "Continue adding", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mAddNCtrl) {
		AddChild(mAddNCtrl);
		f.top = mAddNCtrl->Frame().bottom;
	}
	f.top += spaceY;
	mTransformCtrl = new BRadioButton(f, "trans_ctrl", "Transform selection", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mTransformCtrl) {
		AddChild(mTransformCtrl);
		f.top = mTransformCtrl->Frame().bottom;
	}	
	f.top += spaceY;
	mRefilterCtrl = new BRadioButton(f, "refilt_ctrl", "Refilter selection", new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mRefilterCtrl) {
		AddChild(mRefilterCtrl);
		f.top = mRefilterCtrl->Frame().bottom;
	}	
	return f.RightBottom();
}

// #pragma mark -

/***************************************************************************
 * AM-REFILTER-SEED-VIEW
 ***************************************************************************/
AmRefilterSeedView::AmRefilterSeedView()
		: mRestoreBox(NULL)
{
	AddViews();
}

AmRefilterSeedView::~AmRefilterSeedView()
{
}

void AmRefilterSeedView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mRestoreBox) mRestoreBox->SetTarget(this);
}

status_t AmRefilterSeedView::SetConfiguration(const BMessage* config)
{
	status_t	err = inherited::SetConfiguration(config);
	if (err != B_OK) return err;
	int32		i;
	if (mRestoreBox && config->FindInt32(FLAGS_STR, &i) == B_OK) {
		if (i&AmRefilterToolSeed::RESTORE_FLAG) mRestoreBox->SetValue(B_CONTROL_ON);
		else mRestoreBox->SetValue(B_CONTROL_OFF);
	}
	return B_OK;
}

uint32 AmRefilterSeedView::BuildFlags() const
{
	uint32		flags = inherited::BuildFlags();
	if (mRestoreBox && mRestoreBox->Value() == B_CONTROL_ON) flags |= AmRefilterToolSeed::RESTORE_FLAG;
	return flags;
}

BPoint AmRefilterSeedView::AddViews()
{
	float		fh = arp_get_font_height(this);
	float		spaceX = 5, spaceY = 5;
	BPoint		end = inherited::AddViews();

	/* Add the Restore box.
	 */
	BRect		f(spaceX, end.y + spaceY, spaceX, end.y + spaceY);
	const char*	restoreLabel = "Restore after filtering";
	f.right = f.left + StringWidth(restoreLabel) + 50;
	f.bottom = f.top + fh + 10;
	mRestoreBox = new BCheckBox(f, "restore_box", restoreLabel, new BMessage(WRITE_TOOL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mRestoreBox) {
		mRestoreBox->SetValue(B_CONTROL_ON);
		AddChild(mRestoreBox);
	}
	
	return f.RightBottom();
}
