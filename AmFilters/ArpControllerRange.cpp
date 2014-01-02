#include "ArpControllerRange.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
class _ArpControllerRangeList;

ArpMOD();
static AmStaticResources gRes;

static const char*	_RANGE_MSG			= "rm";
static const char*	_CC_STR				= "cc";
static const char*	_LOW_VALUE			= "lv";
static const char*	_HIGH_VALUE			= "hv";

/***************************************************************************
 * _ARP_CONTROLLER_RANGE_PANEL
 ***************************************************************************/
#define AM_CONTROL_CHANGE_KEY_STR			"control change key"

class _ArpControllerRangePanel : public ArpScrollArea
{
public:
	_ArpControllerRangePanel(	const char* name,
								AmFilterConfigLayout* target,
								const BMessage& initSettings,
								const char* settingsKey = AM_CONTROL_CHANGE_KEY_STR,
								list_view_type type = B_SINGLE_SELECTION_LIST); 
	virtual ~_ArpControllerRangePanel();

	/* Obviously, this class isn't the real list view, just a wrapper around
	 * it.  Here's a way to get at the actual list view, in case clients want
	 * to set selection and invocation messages.
	 */
	BListView*				ListView() const;
	int32					CurrentSelection() const;
	status_t				GetCurrentRange(int32* cc, int32* low, int32* high) const;
	void					UpdateCc(int32 cc, int32 low, int32 high);
	
private:
	typedef ArpScrollArea	inherited;
	_ArpControllerRangeList*	mListView;
	size_t					_reserved_data[4];
};

/*****************************************************************************
 *	_CONTROLLER-RANGE-SETTINGS
 *****************************************************************************/
class _ControllerRangeSettings : public AmFilterConfigLayout
{
public:
	_ControllerRangeSettings(	AmFilterHolderI* target,
								const BMessage& initSettings);

	virtual void			MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	_ArpControllerRangePanel*	mTable;
	ArpIntControl*				mLowCtrl;
	ArpIntControl*				mHighCtrl;

	void						RefreshControls(const BMessage& settings);
	status_t					RefreshRangeControls();
};

/*****************************************************************************
 * ARP-CONTROLLER-RANGE-FILTER
 *****************************************************************************/
ArpControllerRangeFilter::ArpControllerRangeFilter(	ArpControllerRangeAddOn* addon,
												AmFilterHolderI* holder,
												const BMessage* config)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder)
{
	for (uint32 k = 0; k < ARP_CONTROLLER_SIZE; k++) {
		mLowMap[k] = 0;
		mHighMap[k] = 127;
	}
	if (config) PutConfiguration(config);
}

AmEvent* ArpControllerRangeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	event->SetNextFilter(mHolder->ConnectionAt(0) );
	if (event->Type() != event->CONTROLCHANGE_TYPE) return event;
	AmControlChange*		e = dynamic_cast<AmControlChange*>(event);
	if (!e) return event;
	uint8					cc = e->ControlNumber();
	if (mLowMap[cc] == 0 && mHighMap[cc] == 127) return event;

	int32					nv = mLowMap[cc];
	if (mLowMap[cc] != mHighMap[cc]) {
		int32				min =  (mLowMap[cc] <= mHighMap[cc]) ? mLowMap[cc] : mHighMap[cc],
							max =  (mHighMap[cc] >= mLowMap[cc]) ? mHighMap[cc] : mLowMap[cc];
		int32				range =  max - min;
		nv = min + (e->ControlValue() % range);
	}

	if (nv < 0) nv = 0;
	else if (nv > 127) nv = 127;
	e->SetControlValue(nv);
	return event;
}

status_t ArpControllerRangeFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	for (uint32 k = 0; k < ARP_CONTROLLER_SIZE; k++) {
		if (mLowMap[k] != 0 || mHighMap[k] != 127) {
			BMessage		msg;
			if ( (err = msg.AddInt32(_CC_STR, k)) != B_OK) return err;
			if ( (err = msg.AddInt32(_LOW_VALUE, mLowMap[k])) != B_OK) return err;
			if ( (err = msg.AddInt32(_HIGH_VALUE, mHighMap[k])) != B_OK) return err;
			if ( (err = values->AddMessage(_RANGE_MSG, &msg)) != B_OK) return err;
		}
	}

	return B_OK;
}

status_t ArpControllerRangeFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	BMessage		msg;
	int32			cc, low, high;
	for (int32 k = 0; values->FindMessage(_RANGE_MSG, k, &msg) == B_OK; k++) {
		if (msg.FindInt32(_CC_STR, &cc) == B_OK && msg.FindInt32(_LOW_VALUE, &low) == B_OK
				&& msg.FindInt32(_HIGH_VALUE, &high) == B_OK) {
			ArpASSERT(cc >= 0 && cc < ARP_CONTROLLER_SIZE);
			if (cc >= 0 && cc < ARP_CONTROLLER_SIZE) {
				mLowMap[cc] = low;
				mHighMap[cc] = high;
			}
		}
		msg.MakeEmpty();
	}

	return B_OK;
}

status_t ArpControllerRangeFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _ControllerRangeSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-CONTROLLER-RANGE-ADDON
 *****************************************************************************/
void ArpControllerRangeAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I am a map of control change ranges.  Every controller can be
	given a new range.  For example, if the range is 0 9, then the controller is
	mapped so that it stays within this range.  If the incoming value is 10, it's
	remapped to 0, 11 to 1, etc.</p>
	<p>The intended use for this filter is to map button presses from keyboards
	to a range suitable for the destination control.  For example, if you have
	a keyboard with a button configured to send values of 0 - 127 on controller
	12, you can use a Controller Range to map those values into something useful.
	Say controller 12 controls the filter type on a synthesizer, and their are
	10 types, so the valid range is 0 - 9.  This filter will step through the range,
	always keeping the control value valid.</p>";
}

void ArpControllerRangeAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpControllerRangeAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpControllerRangeAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpControllerRangeFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpControllerRangeAddOn(cookie);
	return NULL;
}

/***************************************************************************
 * _ARP_CONTROLLER_RANGE_ITEM
 ***************************************************************************/
static const float	I_SPACE_X			= 4;
static const float	I_ACTIVE_LINE_W		= 4;

class _ArpControllerRangeItem : public BStringItem
{
public:
	int32			low, high;

	_ArpControllerRangeItem(const char* text, int32 inLow, int32 inHigh,
							bool isActive = false)
			: BStringItem(text), low(inLow), high(inHigh), mIsActive(isActive)
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

// #pragma mark -

/***************************************************************************
 * _ARP_CONTROLLER_RANGE_PANEL
 ***************************************************************************/
class _ArpControllerRangeList : public ArpListView
{
public:
	_ArpControllerRangeList(const char* name,
							AmFilterConfigLayout* target,
							const char* settingsKey = AM_CONTROL_CHANGE_KEY_STR,
							list_view_type type = B_SINGLE_SELECTION_LIST); 
	virtual ~_ArpControllerRangeList();

	virtual	void			AttachedToWindow();
	virtual void			SelectionChanged();
	void					GenerateItems(const BMessage* settings);
	status_t				GetCurrentRange(int32* cc, int32* low, int32* high) const;
	void					UpdateCc(int32 cc, int32 low, int32 high);

private:
	typedef ArpListView		inherited;
	AmFilterConfigLayout*	mTarget;
	BString					mSettingsKey;
};

_ArpControllerRangeList::_ArpControllerRangeList(	const char* name,
													AmFilterConfigLayout* target,
													const char* settingsKey,
													list_view_type type)
		: inherited(name, type), mTarget(target)
{
	if (!settingsKey) mSettingsKey = AM_CONTROL_CHANGE_KEY_STR;
	else mSettingsKey = settingsKey;
}

_ArpControllerRangeList::~_ArpControllerRangeList()
{
}

static int32 first_active_item(BListView* list)
{
	_ArpControllerRangeItem*	item;
	for (int32 k = 0; (item = dynamic_cast<_ArpControllerRangeItem*>( list->ItemAt(k) )); k++)
		if ( item->IsActive() ) return k;
	return -1;
}

void _ArpControllerRangeList::AttachedToWindow()
{
	inherited::AttachedToWindow();

	int32	item = first_active_item(this);
	if (item > 2) {
		BRect		frame = ItemFrame(item - 2);
		ScrollTo(0, frame.top);
	}
}

void _ArpControllerRangeList::SelectionChanged()
{
	inherited::SelectionChanged();
	int32	selection = CurrentSelection();
	BMessage	update('iupd');
	if (selection >= 0) {
		BListItem*	item;
		for (int32 k = 0; (item = ItemAt(k)); k++) {
			_ArpControllerRangeItem*	ccItem = dynamic_cast<_ArpControllerRangeItem*>(item);
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

static void _add_cc_label(BString& str, uint32 cc, ArpCRef<AmDeviceI> device)
{
	str << cc;
	if (!device) return;
	BString		s = device->ControlName(cc, false);
	if (s.Length() < 1) return;
	str << " (" << s << ")";
}

static void _build_label(BString& str, int32 cc, int32 low, int32 high, ArpCRef<AmDeviceI> device)
{
	_add_cc_label(str, cc, device);
	if (low != 0 || high != 127)
		str << " ---> " << low << " to " << high;
}

void _ArpControllerRangeList::GenerateItems(const BMessage* settings)
{
	BListItem*			item;
	while ( (item = RemoveItem((int32)0)) ) delete item;

	if (!mTarget) return;
	ArpCRef<AmDeviceI> device = get_device(mTarget->Target());
	uint32				count = ARP_CONTROLLER_SIZE, k;
	if (device) count = device->CountControls();

	/* Fill in a cache of the map supplied by settings.
	 */
	uint8				lowCache[ARP_CONTROLLER_SIZE],
						highCache[ARP_CONTROLLER_SIZE];
	for (k = 0; k < ARP_CONTROLLER_SIZE; k++) {
		lowCache[k] = 0;
		highCache[k] = 127;
	}
	if (settings) {
		BMessage		msg;
		int32			cc, low, high;
		for (k = 0; settings->FindMessage(_RANGE_MSG, k, &msg) == B_OK; k++) {
			if (msg.FindInt32(_CC_STR, &cc) == B_OK && msg.FindInt32(_LOW_VALUE, &low) == B_OK
					 && msg.FindInt32(_HIGH_VALUE, &high) == B_OK) {
				ArpASSERT(cc >= 0 && cc < ARP_CONTROLLER_SIZE);
				if (cc >= 0 && cc < ARP_CONTROLLER_SIZE) {
					lowCache[cc] = low;
					highCache[cc] = high;
				}
			}
			msg.MakeEmpty();
		}
	}

	for (k = 0; k < count; k++) {
		BString			str;
		_build_label(str, k, lowCache[k], highCache[k], device);
		_ArpControllerRangeItem*	ccItem = new _ArpControllerRangeItem(str.String(), lowCache[k], highCache[k]);
		if (ccItem) AddItem(ccItem);
	}
}

status_t _ArpControllerRangeList::GetCurrentRange(int32* cc, int32* low, int32* high) const
{
	int32							selection = CurrentSelection();
	if (selection < 0) return B_ERROR;
	_ArpControllerRangeItem*		item = dynamic_cast<_ArpControllerRangeItem*>(ItemAt(selection));
	if (!item) return  B_ERROR;
	if (cc) *cc = selection;
	if (low) *low = item->low;
	if (high) *high = item->high;
	return B_OK;
}

void _ArpControllerRangeList::UpdateCc(int32 cc, int32 low, int32 high)
{
	if (!mTarget) return;

	if (cc < 0 || cc >= ARP_CONTROLLER_SIZE) return;
	if (low < 0 || low >= ARP_CONTROLLER_SIZE) return;
	if (high < 0 || high >= ARP_CONTROLLER_SIZE) return;

	_ArpControllerRangeItem*		item = (_ArpControllerRangeItem*)ItemAt(cc);
	if (!item) return;

	ArpCRef<AmDeviceI>	device = get_device(mTarget->Target());
	BString				str;
	_build_label(str, cc, low, high, device);
	item->SetText(str.String());
	item->low = low;
	item->high = high;
	InvalidateItem(cc);
}

// #pragma mark -

/***************************************************************************
 * _ARP_CONTROLLER_RANGE_PANEL
 ***************************************************************************/
_ArpControllerRangePanel::_ArpControllerRangePanel(	const char* name,
												AmFilterConfigLayout* target,
												const BMessage& initSettings,
												const char* settingsKey,
												list_view_type type)
		: inherited(name), mListView(NULL)
{
	_ArpControllerRangeList*	lv = new _ArpControllerRangeList("list_view", target, settingsKey, type);
	if (lv) {
		AddLayoutChild(lv);
		lv->GenerateItems(&initSettings);
		mListView = lv;
	}
}

_ArpControllerRangePanel::~_ArpControllerRangePanel()
{
}

BListView* _ArpControllerRangePanel::ListView() const
{
	return mListView;
}

int32 _ArpControllerRangePanel::CurrentSelection() const
{
	if (!mListView) return -1;
	return mListView->CurrentSelection();
}

status_t _ArpControllerRangePanel::GetCurrentRange(int32* cc, int32* low, int32* high) const
{
	if (cc) *cc = -1;
	return mListView->GetCurrentRange(cc, low, high);
}

void _ArpControllerRangePanel::UpdateCc(int32 cc, int32 low, int32 high)
{
	if (mListView) mListView->UpdateCc(cc, low, high);
}

// #pragma mark -

/*****************************************************************************
 *	_CONTROLLER-RANGE-SETTINGS
 *****************************************************************************/
_ControllerRangeSettings::_ControllerRangeSettings(	AmFilterHolderI* target,
												const BMessage& initSettings)
		: inherited(target, initSettings),
		  mTable(0), mLowCtrl(0), mHighCtrl(0)
{
	try {
		AddLayoutChild((new ArpRunningBar("TopVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpTextControl(
									SZ_FILTER_LABEL, "Label:","",
									mImpl.AttachTextControl(SZ_FILTER_LABEL)))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "8")
					.SetString(ArpTextControl::PrefTextStringP, "8888888888")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,false)
				)
			)

			->AddLayoutChild((mTable = new _ArpControllerRangePanel("Control Changes", this, initSettings, _RANGE_MSG))
				->SetParams(ArpMessage()
					.SetBool(ArpScrollArea::ScrollHorizontalP,false)
					.SetBool(ArpScrollArea::ScrollVerticalP,true)
					.SetBool(ArpScrollArea::InsetCornerP,false)
					.SetInt32(ArpScrollArea::BorderStyleP,B_FANCY_BORDER)
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpWest)
					.SetBool(ArpRunningBar::AlignLabelsC,false)
				)
			)

			->AddLayoutChild((mLowCtrl = new ArpIntControl(
									_LOW_VALUE, "Low value:",
									mImpl.AttachControl(_LOW_VALUE),
									0, 127))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
			->AddLayoutChild((mHighCtrl = new ArpIntControl(
									_HIGH_VALUE, "High value:",
									mImpl.AttachControl(_HIGH_VALUE),
									0, 127))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
		);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}

void _ControllerRangeSettings::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case ArpConfigureImpl::CONFIG_REPORT_MSG: {
			/* Interrupt reports from the int controls, because we need to
			 * annotate them with more info.
			 */
			const char*			p;
			if (mTable && mLowCtrl && mHighCtrl
					&& msg->FindString("arp:param", &p) == B_OK && p
					&& (strcmp(p, _LOW_VALUE) == 0 || strcmp(p, _HIGH_VALUE) == 0)) {
				int32			cc = mTable->CurrentSelection(),
								low = mLowCtrl->Value(),
								high = mHighCtrl->Value();
				if (cc >= 0 && cc < ARP_CONTROLLER_SIZE) {
					BMessage	upd, m;
					if (m.AddInt32(_CC_STR, cc) == B_OK && m.AddInt32(_LOW_VALUE, low) == B_OK
							&& m.AddInt32(_HIGH_VALUE, high) == B_OK
							&& upd.AddMessage(_RANGE_MSG, &m) == B_OK) {
						Implementation().SendConfiguration(&upd);
						RefreshControls(upd);
					}				
				}
				break;
			}
		}
		case ARP_PUT_CONFIGURATION_MSG: {
			/* Right now this only happens as a result of the table being
			 * selected, but if there are ever more controls generating
			 * this message this will need to be a little smarter.
			 */
			RefreshRangeControls();
			/* Don't break -- let the superclass handle, too.
			 */
		}
		default:
			inherited::MessageReceived(msg);
	}
}

void _ControllerRangeSettings::RefreshControls(const BMessage& settings)
{
	if (mTable) {
		BMessage		msg;
		for (int32 k = 0; settings.FindMessage(_RANGE_MSG, k, &msg) == B_OK; k++) {
			int32		cc, low, high;
			if (msg.FindInt32(_CC_STR, &cc) == B_OK && msg.FindInt32(_LOW_VALUE, &low) == B_OK
					&& msg.FindInt32(_HIGH_VALUE, &high) == B_OK)
				mTable->UpdateCc(cc, low, high);
			msg.MakeEmpty();
		}
	}
}

status_t _ControllerRangeSettings::RefreshRangeControls()
{
	if (!mTable || !mLowCtrl || !mHighCtrl) return B_NO_MEMORY;
	int32		cc, low, high;
	mTable->GetCurrentRange(&cc, &low, &high);
	ArpVALIDATE(cc >= 0 && cc < ARP_CONTROLLER_SIZE, return B_ERROR);
	mLowCtrl->SetValue(low);
	mHighCtrl->SetValue(high);
	return B_OK;
}
