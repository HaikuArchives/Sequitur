#include <stdio.h>
#include <interface/StringView.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ArpColourControl.h>

static const uint32		_R_MSG		= 'iiR ';
static const uint32		_G_MSG		= 'iiG ';
static const uint32		_B_MSG		= 'iiB ';
static const uint32		_A_MSG		= 'iiA ';

class _ArpRgbaControl : public ArpIntControl
{
public:
	_ArpRgbaControl(	BRect frame,
					const char* name,
					const BString16* label,
					BMessage* message)
			: ArpIntControl(frame, name, label, message), mC(0),
			  mR(0), mG(0), mB(0)
	{
		if (message) mC = message->what;
	}

	void		SetColor(int32 r, int32 g, int32 b)
	{
		if (r >= 0 && r < 256) mR = arp_clip_255(r);
		if (g >= 0 && g < 256) mG = arp_clip_255(g);
		if (b >= 0 && b < 256) mB = arp_clip_255(b);
	}

protected:
	 virtual void GratuitousShadeOn(BView* view, BRect bounds,
									rgb_color color, int16 delta)
	{
		rgb_color		c;
		uint8			v = arp_clip_255(Value());
		c.red	= (mC == _R_MSG) ? v : 0;
		c.green	= (mC == _G_MSG) ? v : 0;
		c.blue	= (mC == _B_MSG) ? v : 0;
		c.alpha = (mC == _A_MSG) ? v : 255;
		if (mC == _A_MSG) {
//printf("Draw colour r %d g %d b %d a %d\n", c.red, c.green, c.blue, c.alpha);
			c.red = mR;	c.green = mG; c.blue = mB;
		}
		view->SetLowColor(c);
		view->SetHighColor(c);
		view->FillRect(bounds);
	}

	virtual void SetValueColor(BView* v)
	{
		v->SetHighColor(255 - mR, 255 - mG, 255 - mB, 255);
//		v->SetHighColor(255, 255, 255, 255);
	}

private:
	uint32		mC;
	uint8		mR, mG, mB;
};

static ArpIntControl* new_component(BRect& f, float iw,
									const char* n, uint32 msg)
{
	f.left = f.right + 2;
	f.right = f.left + iw;
	ArpIntControl*	ic = new _ArpRgbaControl(f, n, NULL, new BMessage(msg));
	if (!ic) return 0;
	ic->SetLimits(0, 255);
	return ic;
}

/*******************************************************
 * ARP-COLOUR-CONTROL
 *******************************************************/
ArpColourControl::ArpColourControl(	BRect frame,
									const char* name,
									const BString16* label,
									float div)
		: inherited(frame, name, 0, 0, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_FRAME_EVENTS),
		  mR(0), mG(0), mB(0), mA(0), mChangingMsg(0),
		  mChangedMsg(0)
{
	BRect		f(0, 0, 0, frame.Height());
	f.right = 0;
	if (label || div > 0) {
		if (div <= 0) div = StringWidth(label) + 2;
		f.right = div;
		BStringView*	sv = new BStringView(f, "label", label);
		if (sv) AddChild(sv);
//		f.left = f.right + 1;
	}

	float		iw = StringWidth("255") + 5;
	if ((mR = new_component(f, iw, "r", _R_MSG)) != 0) AddChild(mR);
	if ((mG = new_component(f, iw, "g", _G_MSG)) != 0) AddChild(mG);
	if ((mB = new_component(f, iw, "b", _B_MSG)) != 0) AddChild(mB);
	if ((mA = new_component(f, iw, "a", _A_MSG)) != 0) AddChild(mA);
}

ArpColourControl::~ArpColourControl()
{
	delete mChangingMsg;
//	delete mChangedMsg;
}

void ArpColourControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(Prefs().GetColor(ARP_BG_C));
	if (mR) mR->SetTarget(this);
	if (mG) mG->SetTarget(this);
	if (mB) mB->SetTarget(this);
	if (mA) mA->SetTarget(this);
}

void ArpColourControl::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case _R_MSG:
			Invoke();
			if (mR && mA) {
				((_ArpRgbaControl*)mA)->SetColor(mR->Value(), -1, -1);
				mA->Invalidate();
			}
//			printf("R Changed\n");
			break;
		case _G_MSG:
			Invoke();
			if (mG && mA) {
				((_ArpRgbaControl*)mA)->SetColor(-1, mG->Value(), -1);
				mA->Invalidate();
			}
//			printf("G Changed\n");
			break;
		case _B_MSG:
			Invoke();
			if (mB && mA) {
				((_ArpRgbaControl*)mA)->SetColor(-1, -1, mB->Value());
				mA->Invalidate();
			}
//			printf("B Changed\n");
			break;
		case _A_MSG:
			Invoke();
//			printf("A Changed\n");
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void ArpColourControl::SetChangingMessage(BMessage* msg)
{
	mChangingMsg = msg;
}

void ArpColourControl::SetChangedMessage(BMessage* msg)
{
	SetMessage(msg);
//	mChangedMsg = msg;
}

ArpVoxel ArpColourControl::Color() const
{
	if (!mR || !mG || !mB || !mA) return ArpVoxel(0, 0, 0, 255);
	return ArpVoxel(arp_clip_255(mR->Value()),
					arp_clip_255(mG->Value()),
					arp_clip_255(mB->Value()),
					arp_clip_255(mA->Value()), 0);
}

status_t ArpColourControl::SetColor(const ArpVoxel& c)
{
	if (!mR || !mG || !mB || !mA) return B_NO_MEMORY;
	mR->SetValue(c.r);
	mG->SetValue(c.g);
	mB->SetValue(c.b);
	mA->SetValue(c.a);
	((_ArpRgbaControl*)mA)->SetColor(mR->Value(), mG->Value(), mB->Value());
	return B_OK;
}
