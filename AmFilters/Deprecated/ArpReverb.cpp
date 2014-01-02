#include "ArpReverb.h"

#ifndef AMPUBLIC_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#include <experimental/ResourceSet.h>

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

ArpMOD();

static BResourceSet gResources;
static int32 gInitResources = 0;
BResourceSet& Resources()
{
	if (atomic_or(&gInitResources, 1) == 0) {
		gResources.AddResources((void*)Resources);
		atomic_or(&gInitResources, 2);
	} else {
		while ((gInitResources&2) == 0) snooze(20000);
	}
	return gResources;
}

/*****************************************************************************
 * ARP-REVERB-FILTER
 *****************************************************************************/
ArpReverbFilter::ArpReverbFilter(	ArpReverbFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  mDepth(30), mTime(PPQN)
{
	if (config) PutConfiguration(config);
	float	t = PPQN / 30;
	if( t < 1 ) t = 1;
	mTime = (AmTime)t;
}

ArpReverbFilter::~ArpReverbFilter()
{
}

AmEvent* ArpReverbFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if( !event ) return event;
	
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);
	
	AmEvent* head = event;
	int32		noteChange = -1;
	int32		noteDelta = noteChange;
	
	if( event->Type() == event->NOTEON_TYPE ) {
		AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
		if( !note ) return event;
		
		AmEvent*	prevEvent = event;
		for (int32 k=0; k<mDepth; k++) {
			AmEvent* nextEvent = event->Copy();
			nextEvent->SetStartTime(nextEvent->StartTime() + (mTime * (k+1)));
			AmNoteOn* nextNote = dynamic_cast<AmNoteOn*>(nextEvent);
			if( !nextNote ) {
				nextEvent->Delete();
			} else {
				float	velocityDelta = float(nextNote->Velocity()) / (mDepth + 1);
				nextNote->SetVelocity(nextNote->Velocity() - uint8((k+1)*velocityDelta));
				int32	noteValue = nextNote->Note() + noteDelta;
				if( noteValue < 0 ) noteValue = 0;
				if( noteValue > 127 ) noteValue = 127;
				nextNote->SetNote( (uint8)noteValue );
				prevEvent->AppendEvent(nextEvent);
				if( k < (mDepth-1) && note->Duration() > mTime )
					note->SetDuration(mTime);
				note = nextNote;
				prevEvent = nextEvent;
			}
			noteDelta += noteChange;
		}
	}
	
	return head;
}

class ArpReverbFilterSettings : public AmFilterConfigLayout
{
public:
	ArpReverbFilterSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
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
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpTextControl(
										"depth", "Depth:","",
										mImpl.AttachTextControl("depth")))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "888")
						.SetString(ArpTextControl::PrefTextStringP, "88888888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpTextControl(
										"time", "Time:","",
										mImpl.AttachTextControl("time")))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "888")
						.SetString(ArpTextControl::PrefTextStringP, "88888888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
			);
		} catch(...) {
			throw;
		}
		
		Implementation().RefreshControls(mSettings);
	}
	
protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpReverbFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err=values->AddInt32("depth", mDepth)) != B_OK) return err;
	if ((err=add_time(*values, "time", mTime)) != B_OK) return err;
	return B_OK;
}

status_t ArpReverbFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if (values->FindInt32("depth", &i) == B_OK) mDepth = i;
	AmTime t;
	if (find_time(*values, "time", &t) == B_OK) mTime = t;
	return B_OK;
}

status_t ArpReverbFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpReverbFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-REVERB-FILTER-ADDON
 *****************************************************************************/
void ArpReverbFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpReverbFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpReverbFilterAddOn(cookie);
	return NULL;
}
