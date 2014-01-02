#include "ArpControllerMap.h"

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
class _ArpControllerMapList;

ArpMOD();
static AmStaticResources gRes;

/* Backwards compatibility (for v1.0)
 */
static const char*	BW_FROM_NUMBER		= "From Number";
static const char*	BW_TO_NUMBER		= "To Number";

static const char*	_FROM				= "f";
static const char*	_TO					= "t";
static const char*	_MSG				= "m";
static const char*	_CC_STR				= "cc";
static const uint32	_MENU_MSG			= '_Mnu';

static ArpMenuField* _new_controller_map_menu(AmFilterConfigLayout& target);

/***************************************************************************
 * _ARP_CONTROLLER_MAP_PANEL
 ***************************************************************************/
#define AM_CONTROL_CHANGE_KEY_STR			"control change key"

class _ArpControllerMapPanel : public ArpScrollArea
{
public:
	_ArpControllerMapPanel(	const char* name,
								AmFilterConfigLayout* target,
								const BMessage& initSettings,
								const char* settingsKey = AM_CONTROL_CHANGE_KEY_STR,
								list_view_type type = B_SINGLE_SELECTION_LIST); 
	virtual ~_ArpControllerMapPanel();

	/* Obviously, this class isn't the real list view, just a wrapper around
	 * it.  Here's a way to get at the actual list view, in case clients want
	 * to set selection and invocation messages.
	 */
	BListView*				ListView() const;
	int32					CurrentSelection() const;
	void					UpdateCc(int32 from, int32 to);
	
private:
	typedef ArpScrollArea	inherited;
	_ArpControllerMapList*	mListView;
	size_t					_reserved_data[4];
};

/*****************************************************************************
 *	_CONTROLLER-MAP-SETTINGS
 *****************************************************************************/
class _ControllerMapSettings : public AmFilterConfigLayout
{
public:
	_ControllerMapSettings(	AmFilterHolderI* target,
							const BMessage& initSettings);

	virtual void			AttachedToWindow();
	virtual void			MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	_ArpControllerMapPanel*	mTable;
	ArpMenuField*			mCcMenu;
	void RefreshControls(const BMessage& settings);
};

/*****************************************************************************
 * ARP-CONTROLLER-MAP-FILTER
 *****************************************************************************/
ArpControllerMapFilter::ArpControllerMapFilter(	ArpControllerMapAddOn* addon,
												AmFilterHolderI* holder,
												const BMessage* config)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder)
{
	for (uint32 k = 0; k < AM_CONTROLLER_SIZE; k++) mMap[k] = k;
	if (config) PutConfiguration(config);

}

AmEvent* ArpControllerMapFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	event->SetNextFilter(mHolder->ConnectionAt(0) );
	if (event->Type() != event->CONTROLCHANGE_TYPE) return event;
	AmControlChange*		e = dynamic_cast<AmControlChange*>(event);
	if (!e) return event;
	uint8					cc = e->ControlNumber();
	if (mMap[cc] == cc) return event;
	e->SetControlNumber(mMap[cc]);
	return event;
}

status_t ArpControllerMapFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err ROCK_ON

	for (uint32 k = 0; k < AM_CONTROLLER_SIZE; k++) {
		if (mMap[k] != k) {
			BMessage		msg;
			if ( (err = msg.AddInt32(_FROM, k)) != B_OK) return err;
			if ( (err = msg.AddInt32(_TO, mMap[k])) != B_OK) return err;
			if ( (err = values->AddMessage(_MSG, &msg)) != B_OK) return err;
		}
	}

	return B_OK;
}

status_t ArpControllerMapFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	/* backwards compatibility
	 */
	int32			f, t;
	if (values->FindInt32(BW_FROM_NUMBER, &f) == B_OK
			&& values->FindInt32(BW_TO_NUMBER, &t) == B_OK) {
		ArpASSERT(f >= 0 && f < AM_CONTROLLER_SIZE);
		ArpASSERT(t >= 0 && t < AM_CONTROLLER_SIZE);
		mMap[f] = t;
	}
	/* end backwards compatibility
	 */

	BMessage		msg;
	for (int32 k = 0; values->FindMessage(_MSG, k, &msg) == B_OK; k++) {
		if (msg.FindInt32(_FROM, &f) == B_OK && msg.FindInt32(_TO, &t) == B_OK)
			mMap[f] = t;
		msg.MakeEmpty();
	}

	return B_OK;
}

status_t ArpControllerMapFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _ControllerMapSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-CONTROLLER-MAP-ADDON
 *****************************************************************************/
void ArpControllerMapAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I am a map of control changes.  Every controller can be
	mapped to a new control value.</p>";
}

void ArpControllerMapAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpControllerMapAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpControllerMapAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpControllerMapFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpControllerMapAddOn(cookie);
	return NULL;
}

/***************************************************************************
 * _ARP_CONTROLLER_MAP_ITEM
 ***************************************************************************/
static const float	I_SPACE_X			= 4;
static const float	I_ACTIVE_LINE_W		= 4;

class _ArpControllerMapItem : public BStringItem
{
public:
	_ArpControllerMapItem(const char* text, bool isActive = false)
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

// #pragma mark -

/***************************************************************************
 * _ARP_CONTROLLER_MAP_PANEL
 ***************************************************************************/
class _ArpControllerMapList : public ArpListView
{
public:
	_ArpControllerMapList(	const char* name,
							AmFilterConfigLayout* target,
							const char* settingsKey = AM_CONTROL_CHANGE_KEY_STR,
							list_view_type type = B_SINGLE_SELECTION_LIST); 
	virtual ~_ArpControllerMapList();

	virtual	void			AttachedToWindow();
	virtual void			SelectionChanged();
	void					GenerateItems(const BMessage* settings);
	void					UpdateCc(int32 from, int32 to);

private:
	typedef ArpListView		inherited;
	AmFilterConfigLayout*	mTarget;
	BString					mSettingsKey;
};

_ArpControllerMapList::_ArpControllerMapList(	const char* name,
												AmFilterConfigLayout* target,
												const char* settingsKey,
												list_view_type type)
		: inherited(name, type), mTarget(target)
{
	if (!settingsKey) mSettingsKey = AM_CONTROL_CHANGE_KEY_STR;
	else mSettingsKey = settingsKey;
}

_ArpControllerMapList::~_ArpControllerMapList()
{
}

static int32 first_active_item(BListView* list)
{
	_ArpControllerMapItem*	item;
	for (int32 k = 0; (item = dynamic_cast<_ArpControllerMapItem*>( list->ItemAt(k) )); k++)
		if ( item->IsActive() ) return k;
	return -1;
}

void _ArpControllerMapList::AttachedToWindow()
{
	inherited::AttachedToWindow();

	int32	item = first_active_item(this);
	if (item > 2) {
		BRect		frame = ItemFrame(item - 2);
		ScrollTo(0, frame.top);
	}
}

void _ArpControllerMapList::SelectionChanged()
{
	inherited::SelectionChanged();
	int32	selection = CurrentSelection();
	BMessage	update('iupd');
	if (selection >= 0) {
		BListItem*	item;
		for (int32 k = 0; (item = ItemAt(k)); k++) {
			_ArpControllerMapItem*	ccItem = dynamic_cast<_ArpControllerMapItem*>(item);
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

static void _build_label(BString& str, int32 from, int32 to, ArpCRef<AmDeviceI> device)
{
	_add_cc_label(str, from, device);
	if (from != to) {
		str << " ---> ";
		_add_cc_label(str, to, device);
	}
}

void _ArpControllerMapList::GenerateItems(const BMessage* settings)
{
	BListItem*			item;
	while ( (item = RemoveItem((int32)0)) ) delete item;

	if (!mTarget) return;
	ArpCRef<AmDeviceI> device = get_device(mTarget->Target());
	uint32				count = AM_CONTROLLER_SIZE, k;
	if (device) count = device->CountControls();

	/* Fill in a cache of the map supplied by settings.
	 */
	uint8				mapCache[AM_CONTROLLER_SIZE];
	for (k = 0; k < AM_CONTROLLER_SIZE; k++) mapCache[k] = k;
	if (settings) {
		BMessage		msg;
		int32			f, t;
		for (k = 0; settings->FindMessage(_MSG, k, &msg) == B_OK; k++) {
			if (msg.FindInt32(_FROM, &f) == B_OK && msg.FindInt32(_TO, &t) == B_OK)
				mapCache[f] = t;
			msg.MakeEmpty();
		}
	}

	for (k = 0; k < count; k++) {
		BString			str;
		_build_label(str, k, mapCache[k], device);
		_ArpControllerMapItem*	ccItem = new _ArpControllerMapItem(str.String() );
		if (ccItem) AddItem(ccItem);
	}
}

void _ArpControllerMapList::UpdateCc(int32 from, int32 to)
{
	if (!mTarget) return;

	if (from < 0 || from >= AM_CONTROLLER_SIZE) return;
	if (to < 0 || to >= AM_CONTROLLER_SIZE) return;

	BStringItem*		item = (BStringItem*)ItemAt(from);
	if (!item) return;

	ArpCRef<AmDeviceI>	device = get_device(mTarget->Target());
	BString				str;
	_build_label(str, from, to, device);
	item->SetText(str.String());
	InvalidateItem(from);
}

// #pragma mark -

/***************************************************************************
 * _ARP_CONTROLLER_MAP_PANEL
 ***************************************************************************/
_ArpControllerMapPanel::_ArpControllerMapPanel(	const char* name,
												AmFilterConfigLayout* target,
												const BMessage& initSettings,
												const char* settingsKey,
												list_view_type type)
		: inherited(name), mListView(NULL)
{
	_ArpControllerMapList*	lv = new _ArpControllerMapList("list_view", target, settingsKey, type);
	if (lv) {
		AddLayoutChild(lv);
		lv->GenerateItems(&initSettings);
		mListView = lv;
	}
}

_ArpControllerMapPanel::~_ArpControllerMapPanel()
{
}

BListView* _ArpControllerMapPanel::ListView() const
{
	return mListView;
}

int32 _ArpControllerMapPanel::CurrentSelection() const
{
	if (!mListView) return -1;
	return mListView->CurrentSelection();
}

void _ArpControllerMapPanel::UpdateCc(int32 from, int32 to)
{
	if (mListView) mListView->UpdateCc(from, to);
}

// #pragma mark -

/*****************************************************************************
 *	_CONTROLLER-MAP-SETTINGS
 *****************************************************************************/
_ControllerMapSettings::_ControllerMapSettings(	AmFilterHolderI* target,
												const BMessage& initSettings)
		: inherited(target, initSettings),
		  mTable(0), mCcMenu(0)
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

			->AddLayoutChild((mTable = new _ArpControllerMapPanel("Control Changes", this, initSettings, BW_FROM_NUMBER))
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
			->AddLayoutChild((mCcMenu = _new_controller_map_menu(*this))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}


void _ControllerMapSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mCcMenu) mCcMenu->Menu()->SetTargetForItems(this);
}

void _ControllerMapSettings::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case _MENU_MSG: {
			int32				cc, sel = (mTable) ? mTable->CurrentSelection() : -1;
			if (sel >= 0 && msg->FindInt32(_CC_STR, &cc) == B_OK) {
				BMessage		upd, m;
				if (m.AddInt32(_FROM, sel) == B_OK && m.AddInt32(_TO, cc) == B_OK
						&& upd.AddMessage(_MSG, &m) == B_OK) {
					Implementation().SendConfiguration(&upd);
					RefreshControls(upd);
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void _ControllerMapSettings::RefreshControls(const BMessage& settings)
{
	if (mTable) {
		BMessage		msg;
		for (int32 k = 0; settings.FindMessage(_MSG, k, &msg) == B_OK; k++) {
			int32		f, t;
			if (msg.FindInt32(_FROM, &f) == B_OK && msg.FindInt32(_TO, &t) == B_OK)
				mTable->UpdateCc(f, t);
			msg.MakeEmpty();
		}
	}
}

/***************************************************************************
 * Misc
 ***************************************************************************/
static ArpMenuField* _new_controller_map_menu(AmFilterConfigLayout& target)
{
	const char*			name = "cmap menu";
	const char*			label = "Controller:";
	BMenu*				menu = new BMenu(name);
	if (!menu) return NULL;

	ArpCRef<AmDeviceI>	device = get_device(target.Target());
	uint32				count = AM_CONTROLLER_SIZE;
	if (device) count = device->CountControls();

	bool				first = true;
	for (uint32 k = 0; k < count; k++) {
		BString		str;
		if (device) str << device->ControlName(k);
		if (str.Length() < 1) str << k;

		BMessage*		msg = new BMessage(_MENU_MSG);
		if (msg) {
			msg->AddInt32(_CC_STR, k);
			BMenuItem*	item = new BMenuItem(str.String(), msg, 0, 0);
			if (item) {
				if (first) item->SetMarked(true);
				first = false;
				menu->AddItem(item);
			}
		}
	}

	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);	
	ArpMenuField*		field = new ArpMenuField(name, label, menu);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}
