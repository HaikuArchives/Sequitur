/* ArpWipeOut.cpp
 */
#include "ArpWipeOut.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <be/interface/MenuField.h>
#include <be/interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char* AMOUNT_STR		= "amount";

/*****************************************************************************
 * ARP-WIPE-OUT-FILTER
 *****************************************************************************/
ArpWipeOutFilter::ArpWipeOutFilter(	ArpWipeOutFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mAmount(-100)
{
	if (config) PutConfiguration(config);
}

ArpWipeOutFilter::~ArpWipeOutFilter()
{
}

AmEvent* ArpWipeOutFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	event->SetNextFilter(mHolder->FirstConnection() );

	if (mAmount <= -100) {
		event->Delete();
		return NULL;
	}

	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*		e = dynamic_cast<AmNoteOn*>(event);
		if (e) {
			int32		vel = int32(e->Velocity() + (float(mAmount) / 100 * 127));
			if (vel <= 0) {
				e->Delete();
				return NULL;
			} else if (vel > 127) vel = 127;
			e->SetVelocity(vel);
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff*		e = dynamic_cast<AmNoteOff*>(event);
		if (e) {
			int32		vel = int32(e->Velocity() + (float(mAmount) / 100 * 127));
			if (vel <= 0) {
				e->Delete();
				return NULL;
			} else if (vel > 127) vel = 127;
			e->SetVelocity(vel);
		}
	} else if (event->Type() == event->CONTROLCHANGE_TYPE) {
		AmControlChange*	e = dynamic_cast<AmControlChange*>(event);
		if (e) {
			int32		val = int32(e->ControlValue() + (float(mAmount) / 100 * 127));
			if (val <= 0) {
				e->Delete();
				return NULL;
			} else if (val > 127) val = 127;
			e->SetControlValue(val);
		}
	} else if (event->Type() == event->PITCHBEND_TYPE) {
		AmPitchBend*	e = dynamic_cast<AmPitchBend*>(event);
		if (e) {
			int32		val = int32(e->Value() + (float(mAmount) / 100 * 16382));
			if (val <= 0) {
				e->Delete();
				return NULL;
			} else if (val > 127) val = 127;
			e->SetValue(val);
		}
	} else if (mAmount != 0) {
		event->Delete();
		return NULL;
	}
	return event;
}

status_t ArpWipeOutFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ( (err = values->AddInt32(AMOUNT_STR, mAmount)) != B_OK) return err;

	return B_OK;
}

status_t ArpWipeOutFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32		i;
	if (values->FindInt32(AMOUNT_STR, &i) == B_OK) mAmount = i;

	return B_OK;
}

/*************************************************************************
 * _PERCENT-FORMAT
 *************************************************************************/
class _PercentFormat : public ArpIntFormatterI
{
public:
	_PercentFormat()	{ ; }
	virtual void FormatInt(int32 number, BString& out) const
	{
		out << number << '%';
	}
};

class ArpWipeOutSettings : public AmFilterConfigLayout
{
public:
	ArpWipeOutSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		ArpIntControl*	ic = NULL;
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
				->AddLayoutChild((ic = new ArpIntControl(
										AMOUNT_STR, "Amount:",
										mImpl.AttachControl(AMOUNT_STR),
										-100, 100))
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
		if (ic) ic->SetFormatter(new _PercentFormat() );
		Implementation().RefreshControls(mSettings);
	}

protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpWipeOutFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpWipeOutSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-WIPE-OUT-FILTER-ADDON
 *****************************************************************************/
void ArpWipeOutFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I delete events that are at their minimum values, as defined by each
	event, below.  The Amount: control can cause events to change by the given
	amount.  If Amount: is set to -100%, I automatically delete all events I receive.
	The Amount: controls the following values:</P>
	<UL>
		<LI><I>Notes</I> change by velocity.</LI>
		<LI><I>Control changes</I> change by control value.</LI>
		<LI><I>Pitch bend events</I> change by pitch.</LI>
		<LI><I>Any other events are deleted.</I></LI>
	</UL>&nbsp;";
}

void ArpWipeOutFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpWipeOutFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpWipeOutFilterAddOn::NewInstance(	AmFilterHolderI* holder,
												const BMessage* config)
{
	return new ArpWipeOutFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpWipeOutFilterAddOn(cookie);
	return NULL;
}
