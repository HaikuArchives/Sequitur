#include "ArpEventMap.h"

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

static const uint32	_NOTE_VEL_MSG		= 'iNvl';
static const uint32	_NOTE_PITCH_MSG		= 'iNpt';
static const uint32	_CC_SRC_MSG			= 'iCSr';
static const uint32	_CC_DEST_MSG		= 'iCDs';
static const uint32	_PITCH_MSG			= 'iPit';
static const uint32	_AFTERTOUCH_MSG		= 'iAft';

static const char*	_NOTE_VEL_STR		= "nv";
static const char*	_NOTE_PITCH_STR		= "np";
static const char*	_CC_SRC_STR			= "ccsrc";
static const char*	_CC_DEST_STR		= "ccdest";
static const char*	_PITCH_STR			= "p";
static const char*	_AFTERTOUCH_STR		= "aft";

/* The possible destinations.  Values 0 and above are reserved
 * for the control changes.
 */
enum {
	_OFF_DEST				= -1,
	_AFTERTOUCH_DEST		= -2,
	_PITCH_DEST				= -3,
	_NOTE_PITCH_DEST		= -4
};

static inline uint8		arp_clip_255(float v)	{ return uint8(v >= 255 ? 255 : v < 0 ? 0 : v); }

static BPopUpMenu*		_new_src_menu(	AmFilterConfigLayout& target,
										uint32 menuMsg, const char* key, int32 init);
static BPopUpMenu*		_new_dest_menu(	AmFilterConfigLayout& target,
										uint32 msgWhat, const char* key, int32 init);
static AmEvent*			_new_dest(AmTime t, int32 dest, float v);

/*****************************************************************************
 *	_EVENT-MAP-SETTINGS
 *****************************************************************************/
class _EventMapSettings : public AmFilterConfigLayout
{
public:
	_EventMapSettings(	AmFilterHolderI* target,
						const BMessage& initSettings);

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	BPopUpMenu			*mCcSrcMenu, *mNoteVelMenu, *mNotePitchMenu,
						*mCcDestMenu, *mPitchMenu, *mAfterMenu;

	void				SendMenuMessage(BPopUpMenu* m);
};

/*****************************************************************************
 * ARP-EVENT-MAP
 *****************************************************************************/
ArpEventMap::ArpEventMap(	ArpEventMapAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder),
		  mNoteVel(_OFF_DEST), mNotePitch(_OFF_DEST), mCcSrc(0),
		  mCcDest(_OFF_DEST), mPitch(_OFF_DEST), mAftertouch(_OFF_DEST)
{
	if (config) PutConfiguration(config);

}

AmEvent* ArpEventMap::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	if (event->Type() == event->NOTEON_TYPE)
		event = HandleNoteOn(dynamic_cast<AmNoteOn*>(event));
	else if (event->Type() == event->CONTROLCHANGE_TYPE)
		event = HandleControlChange(dynamic_cast<AmControlChange*>(event));
	else if (event->Type() == event->PITCHBEND_TYPE)
		event = HandlePitchBend(dynamic_cast<AmPitchBend*>(event));
	else if (event->Type() == event->CHANNELPRESSURE_TYPE)
		event = HandleChannelPressure(dynamic_cast<AmChannelPressure*>(event));

	AmEvent*		e = event;
	while (e) {
		e->SetNextFilter(mHolder->ConnectionAt(0));
		e = e->NextEvent();
	}
	return event;
}

status_t ArpEventMap::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err = values->AddInt32(_NOTE_VEL_STR, mNoteVel)) != B_OK) return err;
	if ((err = values->AddInt32(_NOTE_PITCH_STR, mNotePitch)) != B_OK) return err;
	if ((err = values->AddInt32(_CC_SRC_STR, mCcSrc)) != B_OK) return err;
	if ((err = values->AddInt32(_CC_DEST_STR, mCcDest)) != B_OK) return err;
	if ((err = values->AddInt32(_PITCH_STR, mPitch)) != B_OK) return err;
	if ((err = values->AddInt32(_AFTERTOUCH_STR, mAftertouch)) != B_OK) return err;

	return B_OK;
}

status_t ArpEventMap::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32			i32;
	if (values->FindInt32(_NOTE_VEL_STR, &i32) == B_OK) mNoteVel = i32;
	if (values->FindInt32(_NOTE_PITCH_STR, &i32) == B_OK) mNotePitch = i32;
	if (values->FindInt32(_CC_SRC_STR, &i32) == B_OK) mCcSrc = i32;
	if (values->FindInt32(_CC_DEST_STR, &i32) == B_OK) mCcDest = i32;
	if (values->FindInt32(_PITCH_STR, &i32) == B_OK) mPitch = i32;
	if (values->FindInt32(_AFTERTOUCH_STR, &i32) == B_OK) mAftertouch = i32;
	
	if (mCcSrc < 0 || mCcSrc >= AM_CONTROLLER_SIZE) mCcSrc = 0;

	return B_OK;
}

status_t ArpEventMap::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _EventMapSettings(mHolder, config));
	return B_OK;
}

AmEvent* ArpEventMap::HandleNoteOn(AmNoteOn* e)
{
	ArpVALIDATE(e, return e);
	AmEvent*		newE = _new_dest(e->StartTime() + 1, mNoteVel, e->Velocity() / 127.0);
	if (newE) e->AppendEvent(newE);
	newE = _new_dest(e->StartTime() + 1, mNotePitch, e->Note() / 127.0);
	if (newE) e->AppendEvent(newE);

	return e;
}

AmEvent* ArpEventMap::HandleControlChange(AmControlChange* e)
{
	ArpVALIDATE(e, return e);
	if (mCcSrc != int32(e->ControlNumber())) return e;
	
	AmEvent*		newE = _new_dest(e->StartTime(), mCcDest, e->ControlValue() / 127.0);
	if (!newE) return e;
	e->Delete();
	return newE;
}

static float _float_from_pitch(int32 p)
{
	float		v = float(p) / (AM_PITCH_MAX + 1);
	v = (v + 1) * 0.5;
	if (v >= 1) return 1;
	if (v <= 0) return 0;
	return v;
}

AmEvent* ArpEventMap::HandlePitchBend(AmPitchBend* e)
{
	ArpVALIDATE(e, return e);
	
	AmEvent*		newE = _new_dest(e->StartTime(), mPitch, _float_from_pitch(e->Value()));
	if (!newE) return e;
	e->Delete();
	return newE;
}

AmEvent* ArpEventMap::HandleChannelPressure(AmChannelPressure* e)
{
	ArpVALIDATE(e, return e);

	AmEvent*		newE = _new_dest(e->StartTime(), mAftertouch, e->Pressure() / 127.0);
	if (!newE) return e;
	e->Delete();
	return newE;
}

// #pragma mark -

/*****************************************************************************
 * ARP-CONTROLLER-MAP-ADDON
 *****************************************************************************/
ArpEventMapAddOn::ArpEventMapAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpEventMapAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I transform each event type into a new event type.</p>";
}

void ArpEventMapAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpEventMapAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpEventMapAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpEventMap(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpEventMapAddOn(cookie);
	return NULL;
}

// #pragma mark -

/*****************************************************************************
 *	_EVENT-MAP-SETTINGS
 *****************************************************************************/
_EventMapSettings::_EventMapSettings(	AmFilterHolderI* target,
										const BMessage& initSettings)
		: inherited(target, initSettings),
		  mCcSrcMenu(0), mNoteVelMenu(0), mNotePitchMenu(0), mCcDestMenu(0),
		  mPitchMenu(0), mAfterMenu(0)
{
	int32		noteVel, notePitch, ccSrc, ccDest, pitch, after;
	if (initSettings.FindInt32(_NOTE_VEL_STR, &noteVel) != B_OK) noteVel = _OFF_DEST;
	if (initSettings.FindInt32(_NOTE_PITCH_STR, &notePitch) != B_OK) notePitch = _OFF_DEST;
	if (initSettings.FindInt32(_CC_SRC_STR, &ccSrc) != B_OK) ccSrc = 0;
	if (initSettings.FindInt32(_CC_DEST_STR, &ccDest) != B_OK) ccDest = _OFF_DEST;
	if (initSettings.FindInt32(_PITCH_STR, &pitch) != B_OK) pitch = _OFF_DEST;
	if (initSettings.FindInt32(_AFTERTOUCH_STR, &after) != B_OK) after = _OFF_DEST;

	mCcSrcMenu = _new_src_menu(*this, _CC_SRC_MSG, _CC_SRC_STR, ccSrc);
	mNoteVelMenu = _new_dest_menu(*this, _NOTE_VEL_MSG, _NOTE_VEL_STR, noteVel);
	mNotePitchMenu = _new_dest_menu(*this, _NOTE_PITCH_MSG, _NOTE_PITCH_STR, notePitch);
	mCcDestMenu = _new_dest_menu(*this, _CC_DEST_MSG, _CC_DEST_STR, ccDest);
	mPitchMenu = _new_dest_menu(*this, _PITCH_MSG, _PITCH_STR, pitch);
	mAfterMenu = _new_dest_menu(*this, _AFTERTOUCH_MSG, _AFTERTOUCH_STR, after);

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

			->AddLayoutChild((new ArpRunningBar("BoxVGroup"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, 0)
				)

				->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new ArpViewWrapper(new BStringView(BRect(0,0,10,10), "s", "Note velocity:",
										B_FOLLOW_NONE, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
					->AddLayoutChild((new ArpMenuField(_NOTE_VEL_STR, 0, mNoteVelMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
					->SetParams(ArpMessage().SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL).SetFloat(ArpRunningBar::IntraSpaceP, .5))
					->AddLayoutChild((new ArpViewWrapper(new BStringView(BRect(0,0,10,10), "s", "Note pitch:",
										B_FOLLOW_NONE, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
					->AddLayoutChild((new ArpMenuField(_NOTE_PITCH_STR, 0, mNotePitchMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
					->SetParams(ArpMessage().SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL).SetFloat(ArpRunningBar::IntraSpaceP, .5))
					->AddLayoutChild((new ArpMenuField(_CC_SRC_STR, "Controller:", mCcSrcMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
					->AddLayoutChild((new ArpMenuField(_CC_DEST_STR, 0, mCcDestMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
					->SetParams(ArpMessage().SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL).SetFloat(ArpRunningBar::IntraSpaceP, .5))
					->AddLayoutChild((new ArpViewWrapper(new BStringView(BRect(0,0,10,10), "s", "Pitch:",
										B_FOLLOW_NONE, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
					->AddLayoutChild((new ArpMenuField(_PITCH_STR, 0, mPitchMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
					->SetParams(ArpMessage().SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL).SetFloat(ArpRunningBar::IntraSpaceP, .5))
					->AddLayoutChild((new ArpViewWrapper(new BStringView(BRect(0,0,10,10), "s", "Aftertouch:",
										B_FOLLOW_NONE, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
					->AddLayoutChild((new ArpMenuField(_AFTERTOUCH_STR, 0, mAfterMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpSouth|ArpWest)
						)
					)
				)
			)
		);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}

void _EventMapSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mNoteVelMenu) mNoteVelMenu->SetTargetForItems(this);
	if (mNotePitchMenu) mNotePitchMenu->SetTargetForItems(this);
	if (mCcSrcMenu) mCcSrcMenu->SetTargetForItems(this);
	if (mCcDestMenu) mCcDestMenu->SetTargetForItems(this);
	if (mPitchMenu) mPitchMenu->SetTargetForItems(this);
	if (mAfterMenu) mAfterMenu->SetTargetForItems(this);
}

void _EventMapSettings::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case _NOTE_VEL_MSG:
			SendMenuMessage(mNoteVelMenu);
			break;
		case _NOTE_PITCH_MSG:
			SendMenuMessage(mNotePitchMenu);
			break;
		case _CC_SRC_MSG:
			SendMenuMessage(mCcSrcMenu);
			break;
		case _CC_DEST_MSG:
			SendMenuMessage(mCcDestMenu);
			break;
		case _PITCH_MSG:
			SendMenuMessage(mPitchMenu);
			break;
		case _AFTERTOUCH_MSG:
			SendMenuMessage(mAfterMenu);
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void _EventMapSettings::SendMenuMessage(BPopUpMenu* m)
{
	if (!m) return;
	BMenuItem*		item = m->FindMarked();
	if (!item) return;
	BMessage*		msg = item->Message();
	if (!msg) return;
	Implementation().SendConfiguration(msg);
	mSettings.Update(*msg);
}

// #pragma mark -

/***************************************************************************
 * Misc
 ***************************************************************************/
static ArpCRef<AmDeviceI> get_device(AmFilterHolderI* holder)
{
	if (!holder) return NULL;
	return holder->TrackDevice();
}

static status_t _add_item(	uint32 msgWhat, const char* msgKey, int32 msgVal,
							int32 initVal, const char* itemLabel, BMenu* m)
{
	BMessage*			msg = new BMessage(msgWhat);
	if (!msg) return B_NO_MEMORY;
	msg->AddInt32(msgKey, msgVal);
	BMenuItem*			item =  new BMenuItem(itemLabel, msg, 0, 0);
	if (!item) {
		delete msg;
		return B_NO_MEMORY;
	}
	m->AddItem(item);
	if (msgVal == initVal) item->SetMarked(true);
	return B_OK;
}

static void _add_cc_items(	BMenu* m, AmFilterConfigLayout& target,
							uint32 msgWhat, const char* msgKey,
							int32 initVal)
{
	ArpCRef<AmDeviceI>	device = get_device(target.Target());
	uint32				count = AM_CONTROLLER_SIZE;
	if (device) count = device->CountControls();

	for (uint32 k = 0; k < count; k++) {
		BString		str;
		if (device) str << device->ControlName(k);
		if (str.Length() < 1) str << k;
		_add_item(msgWhat, msgKey, k, initVal, str.String(), m);
	}
}

static BPopUpMenu* _new_src_menu(	AmFilterConfigLayout& target,
									uint32 menuMsg, const char* key, int32 init)
{
	const char*			name = "src";
	BPopUpMenu*			m = new BPopUpMenu(name);
	if (!m) return 0;

	_add_cc_items(m, target, menuMsg, key, init);
	
	m->SetLabelFromMarked(true);
	m->SetRadioMode(true);
	return m;
}

static BPopUpMenu* _new_dest_menu(	AmFilterConfigLayout& target,
									uint32 menuMsg, const char* key, int32 init)
{
	const char*			name = "dest";
	BPopUpMenu*			m = new BPopUpMenu(name);
	if (!m) return 0;

	_add_item(menuMsg, key,	_OFF_DEST,				init, "Off", m);
	m->AddSeparatorItem();
	_add_item(menuMsg, key,	_AFTERTOUCH_DEST,		init, "Aftertouch", m);
	_add_item(menuMsg, key,	_PITCH_DEST,			init, "Pitch", m);
	_add_item(menuMsg, key,	_NOTE_PITCH_DEST,		init, "Note", m);

	m->AddSeparatorItem();
	_add_cc_items(m, target, menuMsg, key, init);
	
	m->SetLabelFromMarked(true);
	m->SetRadioMode(true);
	return m;
}

static int16 _get_pitch(float v)
{
	v = (v * 2) - 1;
	int32		p = int32(v * 8192);
	if (p <= AM_PITCH_MIN) return AM_PITCH_MIN;
	if (p >= AM_PITCH_MAX) return AM_PITCH_MAX;
	return int16(p);
}

static AmEvent* _new_dest(AmTime t, int32 dest, float v)
{
	if (dest == _OFF_DEST) return 0;
	if (dest == _AFTERTOUCH_DEST) return new AmChannelPressure(arp_clip_255(v * 127), t);
	if (dest == _PITCH_DEST) return new AmPitchBend(_get_pitch(v), t);
	if (dest == _NOTE_PITCH_DEST) {
		AmNoteOn*		e = new AmNoteOn(arp_clip_255(v * 127), 127, t);
		if (e) e->SetEndTime(t + PPQN);
		return e;
	}
	if (dest >= 0 && dest < AM_CONTROLLER_SIZE) return new AmControlChange(dest, arp_clip_255(v * 127), t);
	return 0;
}
