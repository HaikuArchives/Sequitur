/* AmControls.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <interface/StringView.h>
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViews/ArpRangeControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

/*************************************************************************
 * AM-KEY-CONTROL
 *************************************************************************/
AmKeyControl::AmKeyControl(	BRect frame,
							const char* name,
							const char* label,
							BMessage* message,
							uint32 rmask,
							uint32 flags)
		: inherited(frame, name, label, message, rmask, flags)
{
	SetLimits(0, 127);
	SetStringMap( new AmKeyMap() );
}

AmKeyControl::AmKeyControl(	const char* name,
							const char* label,
							BMessage* message)
		: inherited(name, label, message, 0, 127)
{
	SetStringMap( new AmKeyMap() );
}

AmKeyControl::~AmKeyControl()
{
}

/*************************************************************************
 * AM-KEY-MAP
 *************************************************************************/
static char* iKeyMap[] = {	"C-2",	"C#-2",	"D-2",	"D#-2",	"E-2",	"F-2",	"F#-2",
							"G-2",	"G#-2",	"A-2",	"A#-2",	"B-2",	"C-1",	"C#-1",
							"D-1",	"D#-1",	"E-1",	"F-1",	"F#-1",	"G-1",	"G#-1",
							"A-1",	"A#-1",	"B-1",	"C0",	"C#0",	"D0",	"D#0",
							"E0",	"F0",	"F#0",	"G0",	"G#0",	"A0",	"A#0",
							"B0",	"C1",	"C#1",	"D1",	"D#1",	"E1",	"F1",
							"F#1",	"G1",	"G#1",	"A1",	"A#1",	"B1",	"C2",
							"C#2",	"D2",	"D#2",	"E2",	"F2",	"F#2",	"G2",
							"G#2",	"A2",	"A#2",	"B2",	"C3",	"C#3",	"D3",
							"D#3",	"E3",	"F3",	"F#3",	"G3",	"G#2",	"A3",
							"A#3",	"B3",	"C4",	"C#4",	"D4",	"D#4",	"E4",
							"F4",	"F#4",	"G4",	"G#4",	"A4",	"A#4",	"B4",
							"C5",	"C#5",	"D5",	"D#5",	"E5",	"F5",	"F#5",
							"G5",	"G#5",	"A5",	"A#5",	"B5",	"C6",	"C#6",
							"D6",	"D#6",	"E6",	"F6",	"F#6",	"G6",	"G#6",
							"A6",	"A#6",	"B6",	"C7",	"C#7",	"D7",	"D#7",
							"E7",	"F7",	"F#7",	"G7",	"G#7",	"A7",	"A#7",
							"B7",	"C8",	"C#8",	"D8",	"D#8",	"E8",	"F8",
							"F#8",	"G8" };

AmKeyMap::AmKeyMap()
{
}

status_t AmKeyMap::IdForName(const char *name, int32 *answer) const
{	*answer = -1;

	for (int32 k = 0; k < 128; k++) {
		if (strcmp(iKeyMap[k], name) == 0) {
			*answer = k;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t AmKeyMap::NameForId(int32 id, char **answer) const
{	if ( (id < 0) || (id > 127) ) return B_ERROR;
	*answer = iKeyMap[id];
	return B_OK;
}

/*************************************************************************
 * AM-DURATION-CONTROL
 *************************************************************************/
/* These are the hardcoded duration constants, all based on 4/4 time.
 */
static const float	WHOLE						= PPQN * 4;
static const float	WHOLE_HALF					= PPQN * 3;
static const float	HALF						= PPQN * 2;
static const float	HALF_QUARTER				= PPQN * 1.5;
static const float	QUARTER						= PPQN;
static const float	QUARTER_EIGHTH				= PPQN * 0.75;
static const float	EIGHTH						= PPQN * 0.5;
static const float	EIGTH_SIXTEENTH				= PPQN * 0.375;
static const float	SIXTEENTH					= PPQN * 0.25;
static const float	SIXTEENTH_THIRTYSECOND		= PPQN * 0.1875;
static const float	THIRTYSECOND				= PPQN * 0.125;
static const float	THIRTYSECOND_SIXTYFOURTH	= PPQN * 0.09375;
static const float	SIXTYFOURTH					= PPQN * 0.0625;

static const float	DUR_CTRL_SPACE_X			= 5;

AmDurationControl::AmDurationControl(	BPoint origin,
										const char* name,
										const char* label,
										uint32 initFlags,
										const char* quantizeKey,
										const char* eighthsKey,
										const char* multiplierKey,
										uint32 rmask,
										uint32 flags)
		: inherited(BRect(origin, origin), name, label, NULL, rmask, flags),
		  mQuantCtrl(NULL), mEighthsCtrl(NULL), mMultiplierCtrl(NULL), mTextCtrl(NULL),
		  mQuantKey(quantizeKey), mEighthsKey(eighthsKey), mMultiplierKey(multiplierKey),
		  mPrefW(0), mPrefH(0), mTarget(NULL),
		  mMultiplier(1), mValue(1), mDivider(2)
{
	AddViews(label, NULL, initFlags);
	ResizeTo(mPrefW, mPrefH);
	SetBodyFill(ArpWest);
}

AmDurationControl::AmDurationControl(	const char* name,
										const char* label,
										AmFilterConfigLayout* target,
										const BMessage& initSettings,
										uint32 initFlags,
										const char* quantizeKey,
										const char* eighthsKey,
										const char* multiplierKey)
		: inherited(name, label, NULL),
		  mQuantCtrl(NULL), mEighthsCtrl(NULL), mMultiplierCtrl(NULL), mTextCtrl(NULL),
		  mQuantKey(quantizeKey), mEighthsKey(eighthsKey), mMultiplierKey(multiplierKey),
		  mPrefW(0), mPrefH(0), mPrefBW(0), mDividingLine(0), mTarget(target),
		  mMultiplier(1), mValue(1), mDivider(2)
{
	AddViews(label, &initSettings, initFlags);
	ResizeTo(mPrefW, mPrefH);
	SetBodyFill(ArpWest);
}

AmDurationControl::~AmDurationControl()
{
}

void AmDurationControl::SplitTicks(AmTime ticks, int32* multiplier, AmTime* quant, int32* eighths)
{
	int32	mult = 1;
	AmTime	value = 0;
	int32	divider = 2;

	static int32 dividers[] = { 2, 3, 5, 7 };
	static AmTime values[] = { PPQN*4, PPQN*2, PPQN, PPQN/2, PPQN/4, PPQN/8, PPQN/16 };
	
	for (int32 i=0; i<(int32)(sizeof(dividers)/sizeof(*dividers)); i++) {
		for (int32 j=0; j<(int32)(sizeof(values)/sizeof(*values)); j++) {
			AmTime test = (values[j]*2)/dividers[i];
			if ((ticks%test) == 0) {
				value = values[j];
				divider = dividers[i];
				goto found;
			}
		}
	}
	
	value = ticks;
	divider = 2;
	
found:
	mult = ticks/((value*2)/divider);

	*multiplier = mult;
	*quant = value;
	*eighths = divider;
}

AmTime AmDurationControl::RawTicks() const
{
	int32		multiplier;
	AmTime		value;
	int32		divider;
	GetSplitTime(&multiplier, &value, &divider);
	return (((value*2)/divider)*multiplier);
}

void AmDurationControl::SetValue(AmTime value)
{
	UpdateRawTime(value);
	UpdateSplitTime(value);
}

void AmDurationControl::SetValue(int32 multiplier, AmTime value, int32 divider)
{
	UpdateSplitTime(multiplier, value, divider);
	UpdateRawTime(multiplier, value, divider);
}

void AmDurationControl::SetDivider(float dividing_line)
{
	if (dividing_line != mDividingLine) {
		mDividingLine = dividing_line;

		float		left = mDividingLine;
		if (mMultiplierCtrl) {
			mMultiplierCtrl->MoveTo(left, mMultiplierCtrl->Frame().top);
			left = mMultiplierCtrl->Frame().right + DUR_CTRL_SPACE_X;
		}
		if (mQuantCtrl) {
			mQuantCtrl->MoveTo(left, mQuantCtrl->Frame().top);
			left = mQuantCtrl->Frame().right + DUR_CTRL_SPACE_X;
		}
		if (mEighthsCtrl) {
			mEighthsCtrl->MoveTo(left, mEighthsCtrl->Frame().top);
			left = mEighthsCtrl->Frame().right + DUR_CTRL_SPACE_X;
		}
		if (mTextCtrl) {
			/* I want the right of this control to stay the same.
			 */
			mTextCtrl->MoveTo(left, mTextCtrl->Frame().top);
			BRect		tf = mTextCtrl->Frame();
			mTextCtrl->ResizeTo( Bounds().right - tf.left, tf.Height() );
		}
		Invalidate();
	}
}

void AmDurationControl::Refresh(const BMessage& settings)
{
	int32	i;
	AmTime	t;
	if (mMultiplierCtrl && mMultiplierKey.Length() > 0
			&& settings.FindInt32(mMultiplierKey.String(), &i) == B_OK)
		mMultiplierCtrl->SetValue(i);
	if (mQuantCtrl && mQuantKey.Length() > 0
			&& find_time(settings, mQuantKey.String(), &t) == B_OK)
		mQuantCtrl->SetZoomY( (float)t );
	if (mEighthsCtrl && mEighthsKey.Length() > 0
			&& settings.FindInt32(mEighthsKey.String(), &i) == B_OK)
		mEighthsCtrl->SetZoomY( (float)i);

	if (mMultiplierCtrl && mQuantCtrl && mEighthsCtrl) {
		UpdateRawTime(	mMultiplierCtrl->Value(),
						(AmTime)mQuantCtrl->ZoomY(),
						(int32)mEighthsCtrl->ZoomY() );
	}
}

ArpRangeControl* AmDurationControl::QuantizeControl() const
{
	return mQuantCtrl;
}

ArpRangeControl* AmDurationControl::EighthsControl() const
{
	return mEighthsCtrl;
}

void AmDurationControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mMultiplierCtrl) mMultiplierCtrl->SetTarget(this);
	if (mTextCtrl) mTextCtrl->SetTarget(this);
}

void AmDurationControl::GetPreferredSize(float *width, float *height)
{
	*width = mPrefW;
	*height = mPrefH;
}

static void update_target(const char* key, AmFilterConfigLayout* target, AmTime time)
{
	if (!key) return;
	BMessage	update;
	add_time(update, key, time);
	target->Implementation().SendConfiguration(&update);
	target->Settings().Update(update);
}

void AmDurationControl::MessageReceived(BMessage* msg)
{
	if (!mTarget) {
		inherited::MessageReceived(msg);
		return;
	}
	switch (msg->what) {
		case DUR_QUANTIZE_FINISHED_MSG: {
			float		yValue;
			if (msg->FindFloat( "y_value", &yValue ) == B_OK) {
				RefreshSplitToRaw();
				SendValueUpdate();
			}
		} break;
		case DUR_EIGHTHS_FINISHED_MSG: {
			float		yValue;
			if (msg->FindFloat( "y_value", &yValue ) == B_OK) {
				RefreshSplitToRaw();
				SendDividerUpdate();
			}
		} break;
		case DUR_MULT_FINISHED_MSG: {
			if (mMultiplierCtrl) {
				RefreshSplitToRaw();
				SendMultiplierUpdate();
			}
		} break;
		case DUR_TEXT_FINISHED_MSG: {
			if (mTextCtrl) {
				RefreshRawToSplit();
				SendMultiplierUpdate();
				SendValueUpdate();
				SendDividerUpdate();
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmDurationControl::ComputeDimens(ArpDimens& cur_dimens)
{
	float vw=0, vh=0;
	GetPreferredSize(&vw,&vh);
	
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

	dx.SetBody(minb, prefb, maxb);
	dx.AddBody(12);
}

void AmDurationControl::LayoutView()
{
	inherited::LayoutView();
	SetDivider(BodyFrame().left - LayoutFrame().left);
}


class _AmMultiplierFormat : public ArpIntFormatterI
{
public:
	_AmMultiplierFormat()	{ ; }
	virtual void FormatInt(int32 number, BString& out) const
	{
		out << number << " x";
	}
};

void AmDurationControl::AddViews(const char* label, const BMessage* initSettings, uint32 initFlags)
{
	BStringView*		strView = NULL;
	BRect				frame(0, 0, 0, 0);
	mDividingLine = 0;
	if (label) {
		frame.Set( 0, 0, StringWidth(label), Prefs().Size(FULL_FONT_Y) );
		strView = new BStringView(frame, "label", label);
		if (strView) AddChild(strView);
		frame.left = frame.right + DUR_CTRL_SPACE_X;
		mDividingLine = frame.left;
	}

	int32	initMult = 1;
	AmTime	initTime = PPQN;
	int32	initDivider = 2;
	int32	i;
	AmTime	t;
	if (initSettings && mMultiplierKey.Length() > 0)
		if (initSettings->FindInt32(mMultiplierKey.String(), &i) == B_OK) initMult = i;
	if (initSettings && mQuantKey.Length() > 0)
		if (find_time(*initSettings, mQuantKey.String(), &t) == B_OK) initTime = t;
	if (initSettings && mEighthsKey.Length() > 0)
		if(initSettings->FindInt32(mEighthsKey.String(), &i) == B_OK) initDivider = i;
	
	if (initFlags&AM_SHOW_DURATION_MULTIPLIER) {
		frame.right = frame.left + StringWidth("99 x") + 5;
		frame.top = 0;
		frame.bottom = frame.top + Prefs().Size(INT_CTRL_Y);
		mMultiplierCtrl = new ArpIntControl( frame, "mult_ctrl", NULL, new BMessage(DUR_MULT_FINISHED_MSG) );
		if (mMultiplierCtrl) {
			mMultiplierCtrl->SetLimits(1, 99);
			mMultiplierCtrl->SetValue(initMult);
			mMultiplierCtrl->SetDivider(0);
			mMultiplierCtrl->SetFormatter( new _AmMultiplierFormat() );
			AddChild(mMultiplierCtrl);
			frame.left = frame.right + DUR_CTRL_SPACE_X;
		}
	}

	ArpImageManagerI&	im = ImageManager();
	const BBitmap*		sizeMap = im.FindBitmap("Duration - whole");
	frame.right = frame.left + 20;
	frame.top = 0;
	frame.bottom = 20;
	if (sizeMap) {
		BRect	b = sizeMap->Bounds();
		frame.bottom = b.Height();
	}
	frame.bottom += 4;
	
	
	frame.right = frame.left + 20;
	if (sizeMap) {
		BRect	b = sizeMap->Bounds();
		frame.right = frame.left + b.Width();
	}
	// To account for the borders on the control
	frame.right += 4;
	
	ArpRangeControl*	ctrl = new ArpRangeControl(frame, "duration_range", B_FOLLOW_LEFT | B_FOLLOW_TOP, 1, (float)PPQN, ARP_DISPLAY_TEXT);
	if (ctrl) {
		ctrl->AddVerticalIcon(			WHOLE,			WHOLE,				im.FindBitmap("Duration - whole") );
//		ctrl->AddVerticalIntermediate(	WHOLE - 1,		HALF + 1,			1 );
		ctrl->AddVerticalIcon(			HALF,			HALF, 				im.FindBitmap("Duration - half") );
//		ctrl->AddVerticalIntermediate(	HALF - 1,		QUARTER + 1, 		1 );
		ctrl->AddVerticalIcon(			QUARTER,		QUARTER,			im.FindBitmap("Duration - quarter") );
//		ctrl->AddVerticalIntermediate(	QUARTER - 1,	EIGHTH + 1,			1 );
		ctrl->AddVerticalIcon(			EIGHTH,			EIGHTH, 			im.FindBitmap("Duration - eighth") );
//		ctrl->AddVerticalIntermediate(	EIGHTH - 1,		SIXTEENTH + 1,		1 );
		ctrl->AddVerticalIcon(			SIXTEENTH,		SIXTEENTH,			im.FindBitmap("Duration - sixteenth") );
//		ctrl->AddVerticalIntermediate(	SIXTEENTH - 1,	THIRTYSECOND + 1,	1 );
		ctrl->AddVerticalIcon(			THIRTYSECOND,	THIRTYSECOND,		im.FindBitmap("Duration - thirtysecond") );
//		ctrl->AddVerticalIntermediate(	THIRTYSECOND - 1, SIXTYFOURTH + 1,	1 );
		ctrl->AddVerticalIcon(			SIXTYFOURTH,	SIXTYFOURTH,		im.FindBitmap("Duration - sixtyfourth") );
		ctrl->SetFinishedMessage( new BMessage(DUR_QUANTIZE_FINISHED_MSG) );
		ctrl->SetTextScale(1, 1);
		ctrl->SetTextContext("", " ticks");
		ctrl->SetRangeColor( tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT) );
		ctrl->SetZoomY((float)initTime);
		AddChild(ctrl);
		mQuantCtrl = ctrl;
	}
	/* Add the modifier control.
	 */
	frame.OffsetBy(frame.Width() + DUR_CTRL_SPACE_X, 0);
	ctrl = new ArpRangeControl( frame, "eighths_range", B_FOLLOW_LEFT | B_FOLLOW_TOP, 1, 2, 0 );
	if (ctrl) {
		ctrl->AddVerticalIcon(	2,				2,					 im.FindBitmap("Duration - 2/8") );
		ctrl->AddVerticalIcon(	3,				3,					 im.FindBitmap("Duration - 3/8") );
		ctrl->AddVerticalIcon(	5,				5,					 im.FindBitmap("Duration - 5/8") );
		ctrl->AddVerticalIcon(	7,				7,					 im.FindBitmap("Duration - 7/8") );
		ctrl->SetFinishedMessage( new BMessage(DUR_EIGHTHS_FINISHED_MSG) );
		ctrl->SetTextScale(1, 1);
		ctrl->SetRangeColor( tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT) );
		ctrl->SetZoomY( (float)initDivider );
		AddChild(ctrl);
		mEighthsCtrl = ctrl;
	}
	
	if (initFlags&AM_SHOW_DURATION_TEXT) {
		frame.left = frame.right + DUR_CTRL_SPACE_X;
		frame.right = frame.left + StringWidth("= WWW");
		mTextCtrl = new BTextControl( frame, "text_ctrl", "=", "1", new BMessage(DUR_TEXT_FINISHED_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
		if (mTextCtrl) {
			mTextCtrl->SetDivider(mTextCtrl->StringWidth("= "));
			AddChild(mTextCtrl);
			UpdateRawTime(initMult, initTime, initDivider);
		}
	}
	mPrefW = frame.right;
	mPrefH = frame.bottom + 1;
	mPrefBW = frame.right - mDividingLine;

	if (strView) {
		BRect	f = strView->Frame();
		if (f.bottom < mPrefH) strView->MoveTo(f.left, ( mPrefH - f.Height() ) / 2);
	}
	if (mTextCtrl) {
		BRect	f = mTextCtrl->Frame();
		if (f.bottom < mPrefH) mTextCtrl->MoveTo(f.left, ( mPrefH - f.Height() ) / 2);
	}
	if (mMultiplierCtrl) {
		BRect	f = mMultiplierCtrl->Frame();
		if (f.bottom < mPrefH) mMultiplierCtrl->MoveTo(f.left, ( mPrefH - f.Height() ) / 2);
	}
}

void AmDurationControl::GetSplitTime(int32* m, AmTime* v, int32* d) const
{
	*m = 1;
	*v = 1;
	*d = 2;
	if (mMultiplierCtrl)	*m = mMultiplierCtrl->Value();
	if (mQuantCtrl)			*v = (AmTime)mQuantCtrl->ZoomY();
	if (mEighthsCtrl)		*d = (int32)mEighthsCtrl->ZoomY();
	
	if (*m <= 0)			*m = 1;
	if (*v <= 0)			*v = 1;
	if (*d != 2 && *d != 3 && *d != 5 && *d != 7)
		*d = 2;
}

void AmDurationControl::UpdateSplitTime(AmTime time)
{
	int32 multiplier = 1;
	AmTime value = 0;
	int32 divider = 2;

	static int32 dividers[] = { 2, 3, 5, 7 };
	static AmTime values[] = { PPQN*4, PPQN*2, PPQN, PPQN/2, PPQN/4, PPQN/8, PPQN/16 };
	
	for (int32 i=0; i<(int32)(sizeof(dividers)/sizeof(*dividers)); i++) {
		for (int32 j=0; j<(int32)(sizeof(values)/sizeof(*values)); j++) {
			AmTime test = (values[j]*2)/dividers[i];
			if ((time%test) == 0) {
				value = values[j];
				divider = dividers[i];
				goto found;
			}
		}
	}
	
	value = time;
	divider = 2;
	
found:
	multiplier = time/((value*2)/divider);
	
	#if 0
	// First see if this time can be quantized into a standard note.
	if ((time%(PPQN*4)) == 0)
		value = PPQN*4;         // at least a whole note
	else if ((time%(PPQN*2)) == 0)
		value = PPQN*2;         // at least a half note
	else if ((time%PPQN) == 0)
		value = PPQN;           // at least a quarter note
	else if ((time%(PPQN/2)) == 0)
		value = PPQN/2;         // at least an eight note
	else if ((time%(PPQN/4)) == 0)
		value = PPQN/4;         // at least a sixteenth note
	else if ((time%(PPQN/8)) == 0)
		value = PPQN/8;         // at least a thirtysecond note
	else if ((time%(PPQN/16)) == 0)
		value = PPQN/16;        // at least a sixtyfourth note
	else
		value = time;           // not a regular note value

	// Find the remaining number of ticks.
	time /= value;
	
	// Now see if there is a standard divisor in the time.
	if ((time%3) == 0)
		divisor = 3;            // modifier is (2/3)
	else if ((time%5) == 0)
		divisor = 5;            // modifier is (2/5)
	else if ((time%7) == 0)
		divisor = 7;            // modifier is (2/7)
	else
		divisor = 2;            // modifier is (2/2)

	// Apply the found divisor to the current time.  The multiplier
	// is whatever remains.
	multiplier = (time*2)/divisor;
	#endif
	
	UpdateSplitTime(multiplier, value, divider);
}

void AmDurationControl::UpdateSplitTime(int32 m, AmTime v, int32 d)
{
	mMultiplier = m;
	mValue = v;
	mDivider = d;
	
	if (mMultiplierCtrl) {
		mMultiplierCtrl->SetValue(m);
	}
	if (mQuantCtrl) {
		mQuantCtrl->SetZoomY((float)v);
	}
	if (mEighthsCtrl) {
		mEighthsCtrl->SetZoomY((float)d);
	}
}

void AmDurationControl::UpdateRawTime(int32 m, AmTime v, int32 d)
{
	mMultiplier = m;
	mValue = v;
	mDivider = d;
	UpdateRawTime(((v*2)/d)*m);
}

void AmDurationControl::UpdateRawTime(AmTime time)
{
	if (mTextCtrl) {
		BString text;
		text << time;
		mTextCtrl->SetText(text.String());
	}
}

void AmDurationControl::RefreshSplitToRaw()
{
	GetSplitTime(&mMultiplier, &mValue, &mDivider);
#if 0
	mMultiplier = 1;
	mValue = 1;
	mDivider = 2;
	if (mMultiplierCtrl)	mMultiplier = mMultiplierCtrl->Value();
	if (mQuantCtrl)			mValue = (AmTime)mQuantCtrl->ZoomY();
	if (mEighthsCtrl)		mDivider = (int32)mEighthsCtrl->ZoomY();
	
	if (mMultiplier <= 0)	mMultiplier = 1;
	if (mValue <= 0)		mValue = 1;
	if (mDivider != 2 && mDivider != 3 && mDivider != 5 && mDivider != 7)
		mDivider = 2;
#endif
	
	UpdateRawTime(mMultiplier, mValue, mDivider);
}

void AmDurationControl::RefreshRawToSplit()
{
	if (mTextCtrl) {
		AmTime time = atoll(mTextCtrl->Text());
		if (time <= 0) time = 1;
		UpdateSplitTime(time);
	}
}

void AmDurationControl::SendMultiplierUpdate()
{
	if (mMultiplierKey.Length() > 0) {
		BMessage	update;
		update.AddInt32(mMultiplierKey.String(), mMultiplier);
		mTarget->Implementation().SendConfiguration(&update);
		mTarget->Settings().Update(update);
	}
}

void AmDurationControl::SendValueUpdate()
{
	if (mQuantKey.Length() > 0)
		update_target(mQuantKey.String(), mTarget, mValue);
}

void AmDurationControl::SendDividerUpdate()
{
	if (mEighthsKey.Length() > 0) {
		BMessage	update;
		update.AddInt32(mEighthsKey.String(), mDivider);
		mTarget->Implementation().SendConfiguration(&update);
		mTarget->Settings().Update(update);
	}
}

/***************************************************************************
 * AM-CONTROL-CHANGE-LIST-VIEW
 ***************************************************************************/
static const float	I_SPACE_X			= 4;
static const float	I_ACTIVE_LINE_W		= 4;

class _AmControlChangeItem : public BStringItem
{
public:
	_AmControlChangeItem(const char* text, bool isActive = false)
			: BStringItem(text), mIsActive(isActive)
	{
	}

	bool IsActive() const
	{
		return mIsActive;
	}

	void SetActive(bool active)
	{
		mIsActive = active;
	}

	virtual	void DrawItem(BView *owner, BRect frame, bool complete)
	{
		owner->SetLowColor(255, 255, 255);
		if (IsSelected() || complete) {
			if ( IsSelected() ) {
				owner->SetHighColor(180, 180, 180);
				owner->SetLowColor(180, 180, 180);
			} else {
				owner->SetHighColor(255, 255, 255);
			}
			owner->FillRect(frame);
		}
		BFont	font;
		owner->GetFont(&font);
		BFont	alteredFont(font);

		if ( !IsEnabled() ) owner->SetHighColor(200, 200, 200);
		else if (mIsActive) {
			owner->SetHighColor(0, 0, 0);
			alteredFont.SetFace(B_BOLD_FACE);
			float		top = frame.top + ((frame.bottom - frame.top) / 2);
			owner->StrokeLine(	BPoint(frame.left + I_SPACE_X, top),
								BPoint(frame.left + I_SPACE_X + I_ACTIVE_LINE_W, top) );
		} else owner->SetHighColor(120, 120, 120);

		owner->SetFont(&alteredFont);
		font_height		height;
		alteredFont.GetHeight(&height);

		owner->MovePenTo(frame.left + I_SPACE_X + I_ACTIVE_LINE_W + I_SPACE_X, frame.bottom - height.descent);
		if ( Text() ) owner->DrawString( Text() );

		owner->SetFont(&font);
	}

private:
	bool		mIsActive;
};

/***************************************************************************
 * AM-CONTROL-CHANGE-LIST-VIEW
 * This BListView subclass displays all the active control changes.
 ***************************************************************************/
class AmControlChangeListView : public ArpListView
{
public:
	AmControlChangeListView(const char* name,
							AmFilterConfigLayout* target,
							const char* settingsKey = AM_CONTROL_CHANGE_KEY_STR,
							list_view_type type = B_SINGLE_SELECTION_LIST); 
	virtual ~AmControlChangeListView();

	virtual	void	AttachedToWindow();
	virtual void	SelectionChanged();
	void			GenerateItems(const BMessage* settings);

private:
	typedef ArpListView	inherited;
	AmFilterConfigLayout*	mTarget;
	BString				mSettingsKey;
};

AmControlChangeListView::AmControlChangeListView(	const char* name,
													AmFilterConfigLayout* target,
													const char* settingsKey,
													list_view_type type)
		: inherited(name, type), mTarget(target)
{
	if (!settingsKey) mSettingsKey = AM_CONTROL_CHANGE_KEY_STR;
	else mSettingsKey = settingsKey;
}

AmControlChangeListView::~AmControlChangeListView()
{
}

static int32 first_active_item(BListView* list)
{
	_AmControlChangeItem*	item;
	for (int32 k = 0; (item = dynamic_cast<_AmControlChangeItem*>( list->ItemAt(k) )); k++)
		if ( item->IsActive() ) return k;
	return -1;
}

void AmControlChangeListView::AttachedToWindow()
{
	inherited::AttachedToWindow();

	int32	item = first_active_item(this);
	if (item > 2) {
		BRect		frame = ItemFrame(item - 2);
		ScrollTo(0, frame.top);
	}
}

void AmControlChangeListView::SelectionChanged()
{
	inherited::SelectionChanged();
	int32	selection = CurrentSelection();
	BMessage	update('iupd');
	if (selection >= 0) {
		BListItem*	item;
		for (int32 k = 0; (item = ItemAt(k)); k++) {
			_AmControlChangeItem*	ccItem = dynamic_cast<_AmControlChangeItem*>(item);
			if (ccItem) {
				if ( IsItemSelected(k) ) {
					update.AddInt32(mSettingsKey.String(), k);
					if ( !ccItem->IsActive() ) {
						ccItem->SetActive(true);
						InvalidateItem(k);
					}
				} else {
					if ( ccItem->IsActive() ) {
						ccItem->SetActive(false);
						InvalidateItem(k);
					}
				}
			}
		}
	}

	if (mTarget) {
		mTarget->Implementation().SendConfiguration(&update);
		mTarget->Settings().Update(update);
	}
}

static ArpCRef<AmDeviceI> get_device(AmFilterHolderI* holder)
{
	if (!holder) return NULL;
	return holder->TrackDevice();
}

static bool settings_has_control(const BMessage* settings, const char* key, uint32 index)
{
	if (!settings || !key) return false;
	int32	cNumber;
	for (int32 k = 0; settings->FindInt32(key, k, &cNumber) == B_OK; k++) {
		if (cNumber == (int32)index) return true;
	}
	return false;
}

void AmControlChangeListView::GenerateItems(const BMessage* settings)
{
	BListItem*	item;
	while ( (item = RemoveItem((int32)0)) ) delete item;

	if (!mTarget) return;
	ArpCRef<AmDeviceI> device = get_device( mTarget->Target() );
	uint32		count = 128;
	if (device) count = device->CountControls();
	
	for (uint32 k = 0; k < count; k++) {
		BString		str;
		if (device) str << device->ControlName(k);
		if (str.Length() < 1) str << k;
		_AmControlChangeItem*	ccItem = new _AmControlChangeItem( str.String() );
		if (ccItem) {
			if ( settings_has_control(settings, mSettingsKey.String(), k) ) ccItem->SetActive(true);
			AddItem(ccItem);
		}
	}
}

/***************************************************************************
 * AM-CONTROL-CHANGE-LIST-PANEL
 ***************************************************************************/
AmControlChangeListPanel::AmControlChangeListPanel(	const char* name,
													AmFilterConfigLayout* target,
													const BMessage& initSettings,
													const char* settingsKey,
													list_view_type type)
		: inherited(name), mListView(NULL)
{
	AmControlChangeListView*	lv = new AmControlChangeListView("list_view", target, settingsKey, type);
	if (lv) {
		AddLayoutChild(lv);
		lv->GenerateItems(&initSettings);
		mListView = lv;
	}
}

AmControlChangeListPanel::~AmControlChangeListPanel()
{
}

BListView* AmControlChangeListPanel::ListView() const
{
	return mListView;
}

/***************************************************************************
 * AM-VELOCITY-CONTROL
 ***************************************************************************/
static const float	FF		= 127;
static const float	F		= 106;
static const float	MF		= 85;
static const float	MP		= 64;
static const float	P		= 43;
static const float	PP		= 22;
static const float	PPP		= 1;

AmVelocityControl::AmVelocityControl(	BPoint origin, const char* name,
										uint32 resizeMask, BMessage* finishedMsg)
		: inherited(BRect(origin, origin), name, resizeMask, 1, (float)1, ARP_DISPLAY_TEXT)
{
	ArpImageManagerI&	im = ImageManager();

	AddVerticalIcon(			FF,		FF,			im.FindBitmap(AM_VELOCITY_FF_STR) );
	AddVerticalIntermediate(	FF-1,	F+1,		10 );
	AddVerticalIcon(			F,		F, 			im.FindBitmap(AM_VELOCITY_F_STR) );
	AddVerticalIntermediate(	F - 1,	MF + 1, 	10 );
	AddVerticalIcon(			MF,		MF,			im.FindBitmap(AM_VELOCITY_MF_STR) );
	AddVerticalIntermediate(	MF - 1,	MP + 1,		10 );
	AddVerticalIcon(			MP,		MP, 		im.FindBitmap(AM_VELOCITY_MP_STR) );
	AddVerticalIntermediate(	MP - 1,	P + 1,		10 );
	AddVerticalIcon(			P,		P,			im.FindBitmap(AM_VELOCITY_P_STR) );
	AddVerticalIntermediate(	P - 1,	PP + 1,		10 );
	AddVerticalIcon(			PP,		PP,			im.FindBitmap(AM_VELOCITY_PP_STR) );
	AddVerticalIntermediate(	PP - 1,	PPP + 1,	10 );
	AddVerticalIcon(			PPP,	PPP,		im.FindBitmap(AM_VELOCITY_PPP_STR) );
	if (finishedMsg) SetFinishedMessage(finishedMsg);
	SetTextScale(1, 1);
	SetTextContext("", "");
	SetZoomY( AmVelocityControl::InitialVelocity() );
	SetRangeColor( tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT) );

	const BBitmap*		sizeMap = im.FindBitmap(AM_VELOCITY_FF_STR);
	BRect				f(0, 0, 20, 20);
	if (sizeMap) f = sizeMap->Bounds();
	// To account for the borders on the control
	f.right += 4;
	f.bottom += 4;

	ResizeTo( f.Width(), f.Height() );
}

uint8 AmVelocityControl::InitialVelocity()
{
	return (uint8)F;
}

uint8 AmVelocityControl::Velocity() const
{
	return (uint8)ZoomY();
}

/***************************************************************************
 * Some common formatters
 ***************************************************************************/
class _ArpPercentFormatter : public ArpIntFormatterI
{
public:
	_ArpPercentFormatter()		{ }
	virtual void FormatInt(int32 number, BString& out) const
	{
		out << number << '%';
	}
};

ArpIntFormatterI* arp_new_percent_formatter()
{
	return new _ArpPercentFormatter();
}


class _ArpFrequencyFormatter : public ArpIntFormatterI
{
public:
	_ArpFrequencyFormatter()	{ }
	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number <= 0) out << "Never";
		else if (number >= 100) out << "Always";
		else out << number << '%';
	}
};

ArpIntFormatterI* arp_new_frequency_formatter()
{
	return new _ArpFrequencyFormatter();
}
