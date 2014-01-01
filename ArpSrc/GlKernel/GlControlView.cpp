#include <stdio.h>
#include <be/interface/StringView.h>
#include <be/support/Errors.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ViewTools.h>
#include <GlPublic/GlControlView.h>

/***************************************************************************
 * _NODE-VIEW-PANEL
 ***************************************************************************/
class _ControlPanel
{
public:
	_ControlPanel(BView* inParent);
	virtual ~_ControlPanel();

	void				AttachedToWindow();
	void				GetPreferredSize(float* width, float* height) const;

	void				Show();
	void				Hide();

	ArpBitmapView*		AddBitmapView(		const BRect& frame, const char* name);
	BButton*			AddButton(			const BRect& frame, const char* name,
											const BString16* label, uint32 msg);
	BCheckBox*			AddCheckBox(		const BRect& frame, const char* name,
											const BString16* label, uint32 msg, bool on);
	ArpColourControl*	AddColourControl(	const BRect& frame, const char* name,
											const BString16* label, const ArpVoxel& c,
											uint32 changingMsg, uint32 changedMsg, float div);
//	GlOsNodePreviewView* AddNodePreviewView(const BRect& frame, const char* name, uint32 msg);
	ArpFileNameControl*	AddFileNameControl(	const BRect& frame, const char* name,
											const BString16& fileName, uint32 message);
	ArpFloatControl*	AddFloatControl(	const BRect& frame, const char* name,
											const BString16* label, uint32 msg, uint32 finishedMsg, float low,
											float high, float value, float steps = 0.1, float divider = 0);
	ArpFontControl*		AddFontControl(		const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											const ArpFont& font, float divider = 0);
	ArpIntControl*		AddIntControl(		const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											int32 low, int32 high, int32 value, float divider = 0);
	void				AddLabel(			const BRect& frame, const char* name, const BString16& text);
	ArpMenuControl*		AddMenuControl(		const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											const BMessage& items, float divider = 0);
	status_t			AddControl(			BControl* ctrl);
	status_t			AddView(			BView* view);
	/* If modificationMsg is 0, it won't be set.
	 */
	BTextControl*		AddTextControl(	const BRect& frame, const char* name,
										const BString16* label, uint32 msg, uint32 modificationMsg,
										const BString16& text,	float divider = 0);
	BTextView*			AddTextView(	const BRect& frame, const char* name,
										const BRect& textRect, const BString16& text);

private:
	BView*				mParent;
	/* The mViews, mControlsI / mControls, and mMenuFields all contain unique objects
	 * not found in the other vecs.  Between the three of them, they contain every object.
	 */
	vector<BView*>		mViews;
	vector<BInvoker*>	mControlsI;		// Two different views on
	vector<BView*>		mControlsV;		// the same objects.
	vector<ArpMenuControl*>	mMenuFields;
};

/***************************************************************************
 * GL-OS-NODE-VIEW
 ***************************************************************************/
GlControlView::GlControlView(const BRect& frame)
		: inherited(frame, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  mPanel(0)
{
	mPanel = new _ControlPanel(this);
}

GlControlView::~GlControlView()
{
	delete mPanel;
}

void GlControlView::AttachedToWindow()
{
	rgb_color		c = Prefs().GetColor(ARP_BG_C);
	c.red = arp_clip_255(int16(c.red) - 20);
	c.green = arp_clip_255(int16(c.green) - 20);
	c.blue = arp_clip_255(int16(c.blue) - 20);
	SetViewColor(c);
//	SetViewColor(Prefs().GetColor(ARP_BG_C));
	if (mPanel) mPanel->AttachedToWindow();
}

void GlControlView::GetPreferredSize(float* width, float* height)
{
	float					w = 0, h = 0;
	if (mPanel) mPanel->GetPreferredSize(&w, &h);
	*width = w + 2;
	*height = h + 2;
}

void GlControlView::MessageReceived(BMessage* msg)
{
	if (ControlMessage(msg->what) != B_OK) inherited::MessageReceived(msg);
}

ArpBitmapView* GlControlView::AddBitmapView(const BRect& frame, const char* name)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddBitmapView(frame, name);
}

BButton* GlControlView::AddButton(	const BRect& frame, const char* name,
									const BString16* label, uint32 msg)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddButton(frame, name, label, msg);
}

BCheckBox* GlControlView::AddCheckBox(	const BRect& frame, const char* name,
										const BString16* label, uint32 msg, bool on)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddCheckBox(frame, name, label, msg, on);
}

ArpColourControl* GlControlView::AddColourControl(	const BRect& frame, const char* name,
													const BString16* label, const ArpVoxel& c,
													uint32 changingMsg, uint32 changedMsg, float div)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddColourControl(frame, name, label, c, changingMsg, changedMsg, div);
}
#if 0
GlOsNodePreviewView* GlControlView::AddNodePreviewView(const BRect& frame, const char* name, uint32 msg)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddNodePreviewView(frame, name, msg);
}
#endif
ArpFileNameControl* GlControlView::AddFileNameControl(	const BRect& frame, const char* name,
														const BString16& fileName, uint32 message)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddFileNameControl(frame, name, fileName, message);
}

ArpFloatControl* GlControlView::AddFloatControl(const BRect& frame, const char* name,
												const BString16* label, uint32 msg, uint32 finishedMsg,
												float low, float high, float value, float steps,
												float divider)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddFloatControl(frame, name, label, msg, finishedMsg, low, high, value, steps, divider);
}

ArpFontControl* GlControlView::AddFontControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message,
												const ArpFont& font, float divider)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddFontControl(frame, name, label, message, font, divider);
}

ArpIntControl* GlControlView::AddIntControl(const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											int32 low, int32 high, int32 value, float divider)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddIntControl(frame, name, label, message, low, high, value, divider);
}

void GlControlView::AddLabel(const BRect& frame, const char* name, const BString16& text)
{
	ArpVALIDATE(mPanel, return);
	mPanel->AddLabel(frame, name, text);
}

ArpMenuControl* GlControlView::AddMenuControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message,
												const BMessage& items, float divider)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddMenuControl(frame, name, label, message, items, divider);
}

#if 0
	ArpPixelControl*	AddPixelControl(	const BRect& frame, const char* name,
											const BString16* label, const ArpPixel& p,
											uint32 changingMsg, uint32 changedMsg, float div);

ArpPixelControl* GlControlView::AddPixelControl(	const BRect& frame, const char* name,
												const BString16* label, const ArpPixel& p,
												uint32 changingMsg, uint32 changedMsg, float div)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddPixelControl(frame, name, label, p, changingMsg, changedMsg, div);
}
#endif

status_t GlControlView::AddControl(BControl* ctrl)
{
	ArpVALIDATE(mPanel, return B_NO_MEMORY);
	return mPanel->AddControl(ctrl);
}

status_t GlControlView::AddView(BView* view)
{
	ArpVALIDATE(mPanel, return B_NO_MEMORY);
	return mPanel->AddView(view);
}

BTextControl* GlControlView::AddTextControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message, uint32 modificationMsg,
												const BString16& text, float divider)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddTextControl(frame, name, label, message, modificationMsg, text, divider);
}

BTextView* GlControlView::AddTextView(	const BRect& frame, const char* name,
										const BRect& textRect, const BString16& text)
{
	ArpVALIDATE(mPanel, return 0);
	return mPanel->AddTextView(frame, name, textRect, text);
}

float GlControlView::ViewFontHeight() const
{
	return arp_get_font_height(this);
}

float GlControlView::CheckBoxW(const BString16* label)
{
	return StringWidth(label) + 20;
}

float GlControlView::MenuW(const BString16* label, const BMessage& items)
{
	const char*		item;
	const char*		longItem = 0;
	int32			longLen = 0;
	for (int32 k = 0; items.FindString("item", k, &item) == B_OK; k++) {
		int32		len = int32(strlen(item));
		if (len > longLen) {
			longItem = item;
			longLen = len;
		}
	}
	if (!longItem) return StringWidth(label) + 40;
	return MenuW(label, longItem);
}

float GlControlView::MenuW(const BString16* label, const char* longItem)
{
	ArpASSERT(label && longItem);
	return StringWidth(label) + 5 + StringWidth(longItem) + 30;
}

// #pragma mark -

/***************************************************************************
 * _CONTROL-PANEL
 ***************************************************************************/
_ControlPanel::_ControlPanel(BView* inParent)
		: mParent(inParent)
{
}

_ControlPanel::~_ControlPanel()
{
	uint32	k;
	for (k = 0; k < mViews.size(); k++) {
		if (mViews[k]->Window() == 0) delete mViews[k];
	}
	for (k = 0; k < mControlsV.size(); k++) {
		if (mControlsV[k]->Window() == 0) delete mControlsV[k];
	}
	for (k = 0; k < mMenuFields.size(); k++) {
		if (mMenuFields[k]->Window() == 0) delete mMenuFields[k];
	}
	mViews.resize(0);
	mControlsI.resize(0);
	mControlsV.resize(0);
	mMenuFields.resize(0);
}

void _ControlPanel::AttachedToWindow()
{
	for (uint32 k = 0; k < mControlsI.size(); k++) {
		if (mControlsI[k]) mControlsI[k]->SetTarget(BMessenger(mParent));
	}
	for (uint32 k = 0; k < mMenuFields.size(); k++) {
		if (mMenuFields[k] && mMenuFields[k]->Menu()) {
			mMenuFields[k]->Menu()->SetTargetForItems(BMessenger(mParent));
			mMenuFields[k]->SetCachedTarget(BMessenger(mParent));
		}
	}
}

void _ControlPanel::GetPreferredSize(float* width, float* height) const
{
	float	w = *width, h = *height;
	uint32	k;
	for (k = 0; k < mViews.size(); k++) {
		BRect		f = mViews[k]->Frame();
		if (f.right > w) w = f.right;
		if (f.bottom > h) h = f.bottom;
	}
	for (k = 0; k < mControlsV.size(); k++) {
		BRect		f = mControlsV[k]->Frame();
		if (f.right > w) w = f.right;
		if (f.bottom > h) h = f.bottom;
	}
	for (k = 0; k < mMenuFields.size(); k++) {
		BRect		f = mMenuFields[k]->Frame();
		if (f.right > w) w = f.right;
		if (f.bottom > h) h = f.bottom;
	}
	*width = w;
	*height = h;
}

void _ControlPanel::Show()
{
	uint32		k;
	for (k = 0; k < mViews.size(); k++) {
		if (mViews[k]->Window() == 0) mParent->AddChild(mViews[k]);
	}
	for (k = 0; k < mControlsV.size(); k++) {
		if (mControlsV[k]->Window() == 0) mParent->AddChild(mControlsV[k]);
	}
	for (k = 0; k < mMenuFields.size(); k++) {
		if (mMenuFields[k]->Window() == 0) mParent->AddChild(mMenuFields[k]);
	}
}

void _ControlPanel::Hide()
{
	uint32		k;
	for (k = 0; k < mViews.size(); k++) {
		if (mViews[k]->Window()) mParent->RemoveChild(mViews[k]);
	}
	for (k = 0; k < mControlsV.size(); k++) {
		if (mControlsV[k]->Window()) mParent->RemoveChild(mControlsV[k]);
	}
	for (k = 0; k < mMenuFields.size(); k++) {
		if (mMenuFields[k]->Window()) mParent->RemoveChild(mMenuFields[k]);
	}
}

ArpBitmapView* _ControlPanel::AddBitmapView(const BRect& frame, const char* name)
{
	ArpBitmapView*		v = new ArpBitmapView(frame, name);
	if (v) {
		mViews.push_back(v);
		mParent->AddChild(v);
	}
	return v;
}

BButton* _ControlPanel::AddButton(	const BRect& frame, const char* name,
									const BString16* label, uint32 msgWhat)
{
	BMessage*			msg = 0;
	if (msgWhat > 0) msg = new BMessage(msgWhat);
	BButton*			ctrl = new BButton(frame, name, label, msg);
	if (ctrl) {
		mParent->AddChild(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

BCheckBox* _ControlPanel::AddCheckBox(	const BRect& frame, const char* name,
										const BString16* label, uint32 msgWhat, bool on)
{
	BMessage*			msg = 0;
	if (msgWhat > 0) msg = new BMessage(msgWhat);
	BCheckBox*			ctrl = new BCheckBox(frame, name, label, msg);
	if (ctrl) {
		if (on) ctrl->SetValue(B_CONTROL_ON);
		else ctrl->SetValue(B_CONTROL_OFF);
		mParent->AddChild(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

ArpColourControl* _ControlPanel::AddColourControl(	const BRect& frame, const char* name,
													const BString16* label, const ArpVoxel& c,
													uint32 changingMsg, uint32 changedMsg, float div)
{
	ArpColourControl*		v = new ArpColourControl(frame, name, label, div);
	if (v) {
		v->SetColor(c);
		if (changingMsg != 0) v->SetChangingMessage(new BMessage(changingMsg));
		if (changedMsg != 0) v->SetChangedMessage(new BMessage(changedMsg));
		mParent->AddChild(v);
		mControlsI.push_back(v);
		mControlsV.push_back(v);
	}
	return v;
}
#if 0
GlOsNodePreviewView* _ControlPanel::AddNodePreviewView(const BRect& frame, const char* name, uint32 msg)
{
	GlOsNodePreviewView*	v = new GlOsNodePreviewView(frame, name);
	if (v) {
		if (msg > 0) v->SetChangedMsg(new BMessage(msg));
		mViews.push_back(v);
		mParent->AddChild(v);
	}
	return v;
}
#endif
ArpFileNameControl* _ControlPanel::AddFileNameControl(	const BRect& frame, const char* name,
														const BString16& fileName, uint32 message)
{
	ArpFileNameControl*	ctrl = new ArpFileNameControl(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (ctrl) {
		ctrl->SetMessage(new BMessage(message));
		ctrl->SetFileName(&fileName);
		mParent->AddChild(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

ArpFloatControl* _ControlPanel::AddFloatControl(	const BRect& frame, const char* name,
													const BString16* label, uint32 msg, uint32 finishedMsg,
													float low, float high, float value, float steps,
													float divider)
{
	ArpFloatControl*	ctrl = new ArpFloatControl(frame, name, label, NULL);
	if (ctrl) {
		ctrl->SetLimits(low, high);
		ctrl->SetValue(value);
		ctrl->SetSteps(steps);
		ctrl->SetDivider(divider);
		ctrl->SetMessage(new BMessage(msg));
		ctrl->SetFinishedMessage(new BMessage(finishedMsg));
		mParent->AddChild(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

ArpFontControl* _ControlPanel::AddFontControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message,
												const ArpFont& font, float divider)
{
	ArpFontControl*		ctrl = new ArpFontControl(frame, name, label, message, divider);
	if (ctrl) {
//		ctrl->SetDivider(divider);
		ctrl->SetFont(font);
		mParent->AddChild(ctrl);
//		mMenuFields.push_back(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

ArpIntControl* _ControlPanel::AddIntControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message,
												int32 low, int32 high, int32 value, float divider)
{
	ArpIntControl*	ctrl = new ArpIntControl(frame, name, label, NULL);
	if (ctrl) {
		ctrl->SetLimits(low, high);
		ctrl->SetValue(value);
		ctrl->SetDivider(divider);
		ctrl->SetMessage(new BMessage(message));
		mParent->AddChild(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

void _ControlPanel::AddLabel(const BRect& frame, const char* name, const BString16& text)
{
	BStringView*	label = new BStringView(frame, name, text.String());
	if (label) {
		mParent->AddChild(label);
		mViews.push_back(label);
	}
}

#if 0
	ArpPixelControl*	AddPixelControl(	const BRect& frame, const char* name,
											const BString16* label, const ArpPixel& p,
											uint32 changingMsg, uint32 changedMsg, float div);
ArpPixelControl* _ControlPanel::AddPixelControl(	const BRect& frame, const char* name,
													const BString16* label, const ArpPixel& p,
													uint32 changingMsg, uint32 changedMsg, float div)
{
	ArpPixelControl*		v = new ArpPixelControl(frame, name, label, div);
	if (v) {
		v->SetPixel(p);
		if (changingMsg != 0) v->SetChangingMessage(new BMessage(changingMsg));
		if (changedMsg != 0) v->SetChangedMessage(new BMessage(changedMsg));
		mParent->AddChild(v);
		mControlsI.push_back(v);
		mControlsV.push_back(v);
	}
	return v;
}
#endif

ArpMenuControl* _ControlPanel::AddMenuControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message,
												const BMessage& items, float divider)
{
	ArpMenuControl*		ctrl = new ArpMenuControl(frame, name, label, message, items);
	if (ctrl) {
		ctrl->SetDivider(divider);
		mParent->AddChild(ctrl);
		mMenuFields.push_back(ctrl);
	}
	return ctrl;
}

status_t _ControlPanel::AddControl(BControl* ctrl)
{
	ArpVALIDATE(ctrl, return B_ERROR);
	mParent->AddChild(ctrl);
	mControlsI.push_back(ctrl);
	mControlsV.push_back(ctrl);
	return B_OK;
}

status_t _ControlPanel::AddView(BView* view)
{
	ArpVALIDATE(view, return B_ERROR);
	ArpASSERT(view->Parent() == 0);
	if (view->Parent() == 0) mParent->AddChild(view);
	mViews.push_back(view);
	return B_OK;
}

BTextControl* _ControlPanel::AddTextControl(	const BRect& frame, const char* name,
												const BString16* label, uint32 message, uint32 modificationMsg,
												const BString16& text, float divider)
{
	BTextControl*		ctrl = new BTextControl(frame, name, label, text.String(), new BMessage(message));
	if (ctrl) {
		ctrl->SetDivider(divider);
		if (modificationMsg != 0) ctrl->SetModificationMessage(new BMessage(modificationMsg));
		mParent->AddChild(ctrl);
		mControlsI.push_back(ctrl);
		mControlsV.push_back(ctrl);
	}
	return ctrl;
}

BTextView* _ControlPanel::AddTextView(	const BRect& frame, const char* name,
										const BRect& textRect, const BString16& text)
{
	BTextView*		view = new BTextView(frame, name, textRect, B_FOLLOW_ALL);
	if (view) {
		if (text.String()) view->SetText(text.String());
//		if (modificationMsg != 0) ctrl->SetModificationMessage(new BMessage(modificationMsg));
		mParent->AddChild(view);
//		mControlsI.push_back(ctrl);
		mControlsV.push_back(view);
	}
	return view;
}
