#include <InterfaceKit.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ArpFontControl.h>

static const uint32				FONT_IMSG	= 'iFnt';
static const uint32				SIZE_IMSG	= 'iSze';

class _ArpFontControlFamilies : public BFontFamilies
{
public:
	_ArpFontControlFamilies(BMenu*  m) : mMenu(m)   { }

protected:
	virtual status_t	Do(const BString16* familyName)
	{
		ArpASSERT(familyName);
		BMenuItem*		item = new BMenuItem(familyName, new BMessage(FONT_IMSG));
		if (item) mMenu->AddItem(item);
#if 0
		int32	numFamilies = count_font_families();
		for (int32 i = 0; i < numFamilies; i++) { 
			font_family		family; 
			uint32			flags; 
			if (get_font_family(i, &family, &flags) == B_OK) { 
				BMenuItem*	item = new BMenuItem(family, new BMessage(FONT_IMSG));
				if (item) m->AddItem(item);
#if 0
				int32 numStyles = count_font_styles(family); 
				for (int32 j = 0; j < numStyles; j++) { 
					font_style style; 
					if (get_font_style(family, j, &style, &flags) == B_OK) { 
						printf("FONT: %s %s\n", family, style);
					} 
				} 
#endif
			} 
		}
#endif
		return B_OK;
	}

private:
	BMenu*		mMenu;
};

/*******************************************************
 * ARP-FONT-CONTROL
 *******************************************************/
ArpFontControl::ArpFontControl(	BRect frame,
								const char* name,
								const BString16* label,
								uint32 message,
								float divider)
		: inherited(frame, name, 0, new BMessage(message),
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW),
		  mFontCtrl(0), mSizeCtrl(0), mMsgWhat(message)
{
	if (label) {
		if (divider <= 0) divider = StringWidth(label);
		BRect			f(0, 0, divider, frame.Height());
		BStringView*	sv = new BStringView(f, "sv", label);
		if (sv) AddChild(sv);
	}

	float				sizeW = StringWidth("000") + 5;
	float				sizeL = frame.Width() - sizeW;
	BRect				f(divider + 1, 0, sizeL - 1, frame.Height());
	mFontCtrl = new BMenuField(f, "fonts", 0, new BMenu("font"), true,
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (!mFontCtrl) return;

	mFontCtrl->SetDivider(0);
	BMenu*		m = mFontCtrl->Menu();
	if (m) {
		m->SetLabelFromMarked(true);
		_ArpFontControlFamilies		families(m);
		families.ForEach();
	}
	AddChild(mFontCtrl);

	float		iH = float(Prefs().GetInt32(ARP_INTCTRL_Y));
	float		iT = 0, iB = frame.Height();
	if (iH < frame.Height()) {
		iT = (frame.Height() - iH) / 2;
		iB = iT + iH;
	}
	f.Set(sizeL, iT, sizeL + sizeW, iB);
	mSizeCtrl = new ArpIntControl(	f, "size", 0, new BMessage(SIZE_IMSG),
									B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	if (mSizeCtrl) {
		mSizeCtrl->SetLimits(1, 512);
		AddChild(mSizeCtrl);
	}

}

void ArpFontControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mFontCtrl && mFontCtrl->Menu())
		mFontCtrl->Menu()->SetTargetForItems(this);
	if (mSizeCtrl) mSizeCtrl->SetTarget(this);
}

void ArpFontControl::MessageReceived(BMessage* msg)
{
	/* Redirect any messages I receive from my font ctrl to
	 * whomever I'm supposed to deliver to.
	 */
	if (msg->what == FONT_IMSG || msg->what == SIZE_IMSG) {
		BMessenger		messenger = Messenger();
		if (messenger.IsValid()) messenger.SendMessage(mMsgWhat);
		return;
	}
	inherited::MessageReceived(msg);
}

ArpFont ArpFontControl::Font() const
{
	ArpFont				f;
	if (!mFontCtrl || !mSizeCtrl) return f;
	BMenu*				m = mFontCtrl->Menu();
	if (!m) return f;
	BMenuItem*			item = m->FindMarked();
	if (!item) return f;
	const BString16*	lbl = item->Label();
	if (lbl) {
		BString16		s(*lbl);
		f.SetFamilyAndStyle(&s);
	}
	f.SetSize(float(mSizeCtrl->Value()));
	return f;
}

void ArpFontControl::SetFont(const ArpFont& font)
{
	if (!mFontCtrl || !mSizeCtrl) return;
	mSizeCtrl->SetValue(int32(font.Size()));
	BMenu*		m = mFontCtrl->Menu();
	if (!m) return;
	BString16	family = font.Family();
	BMenuItem*	item;
	for (int32 k = 0; (item = m->ItemAt(k)) != 0; k++) {
		if (family == item->Label()) {
			item->SetMarked(true);
		}
	}
}
