#include "ArpChorus.h"

#include <String.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static void get_octave_string(uint32 number, BString& out)
{
	out << number << "_oct";
}

static void get_step_string(uint32 number, BString& out)
{
	out << number << "_stp";
}

static void get_velocity_string(uint32 number, BString& out)
{
	out << number << "_vel";
}

/*************************************************************************
 * _OCTAVE-MAP
 * A class that performs a mapping between ints and names octave -- i.e,
 * 0 is off, everything else is its number.
 *************************************************************************/
class _OctaveMap : public ArpIntToStringMapI
{
public:
	_OctaveMap()	{ ; }
	virtual status_t IdForName(const char *name, int32 *answer) const;
	virtual status_t NameForId(int32 id, char **answer) const;
};

/*************************************************************************
 * _PERCENT-FORMAT
 * A class that formats floats as percentages.
 *************************************************************************/
class _PercentFormat : public ArpIntFormatterI
{
public:
	_PercentFormat()	{ ; }
	virtual void FormatInt(int32 number, BString& out) const;
};

/*****************************************************************************
 * ARP-CHORUS-FILTER
 *****************************************************************************/
ArpChorusFilter::ArpChorusFilter(	ArpChorusFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder)
{
	for( uint32 k = 0; k < MAX_CHORUS; k++ ) mOctaves[k] = 0;
	for( uint32 k = 0; k < MAX_CHORUS; k++ ) mSteps[k] = 0;
	for( uint32 k = 0; k < MAX_CHORUS; k++ ) mVelocities[k] = 100;
	mOctaves[0] = 1;
	
	if (config) PutConfiguration(config);
}

ArpChorusFilter::~ArpChorusFilter()
{
}

AmEvent* ArpChorusFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);
	
	event->SetNextFilter(mHolder->FirstConnection() );
	AmEvent*	head = event;
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	note = dynamic_cast<AmNoteOn*>(event);
		if (!note) return event;

		for( uint32 k = 0; k < MAX_CHORUS; k++ ) {
			if( mOctaves[k] != 0 || mSteps[k] != 0 || mVelocities[k] != 100 ) {
				uint8 pitch = note->Note();
				int32 delta = (mOctaves[k]*12) + (mSteps[k]);
				if (pitch+delta > 0x7f) pitch = 0x7f;
				else if(pitch+delta < 0) pitch = 0;
				else pitch += delta;

				AmNoteOn*	e = dynamic_cast<AmNoteOn*>( note->Copy() );
				if( e ) {
					e->SetNote( pitch );
					float	newVel = e->Velocity() * (float)(float(mVelocities[k])/100);
					if( newVel < 0 ) newVel = 0;
					else if( newVel > 127 ) newVel = 127;
					e->SetVelocity( newVel );
					head->AppendEvent(e);
				}
			}
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff*	note = dynamic_cast<AmNoteOff*>(event);
		if( !note ) return event;

		for( uint32 k = 0; k < MAX_CHORUS; k++ ) {
			if( mOctaves[k] != 0 || mSteps[k] != 0 || mVelocities[k] != 100 ) {
				uint8 pitch = note->Note();
				int32 delta = (mOctaves[k]*12) + (mSteps[k]);
				if (pitch+delta > 0x7f) pitch = 0x7f;
				else if(pitch+delta < 0) pitch = 0;
				else pitch += delta;

				AmNoteOff*	e = dynamic_cast<AmNoteOff*>( note->Copy() );
				if( e ) {
					e->SetNote( pitch );
					float	newVel = e->Velocity() * (float)(float(mVelocities[k])/100);
					if( newVel < 0 ) newVel = 0;
					else if( newVel > 127 ) newVel = 127;
					e->SetVelocity( newVel );
					head->AppendEvent(e);
				}
			}
		}
	}
	
	return head;
}

static void add_chorus_box(ArpBaseLayout* toLayout, uint32 num, ArpConfigureImpl& impl, float labelW, float intW)
{
	BString			boxLabel("Note ");
	boxLabel << num + 1;
	ArpBaseLayout*	box = (new ArpBox("ChorusBox", boxLabel.String() ))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpNorthSouth));
	toLayout->AddLayoutChild( box );

	ArpBaseLayout*	vBar = (new ArpRunningBar("ChorusVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	box->AddLayoutChild( vBar );

	BString			octStr;
	get_octave_string( num, octStr );
	ArpKnobPanel*	kp = 0;
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( octStr.String(), "Octave:", impl.AttachControl( octStr.String() ), -10, 10, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,3)
			.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetStringMap( new _OctaveMap() );
	}
	
	BString			stepStr;
	get_step_string( num, stepStr );
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( stepStr.String(), "Step:", impl.AttachControl( stepStr.String() ), -12, 12, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetStringMap( new _OctaveMap() );
	}

	BString			offStr;
	get_velocity_string( num, offStr );
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( offStr.String(), "Velocity:", impl.AttachControl( offStr.String() ), 1, 200, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetFormatter( new _PercentFormat() );
	}
}

class ArpChorusFilterSettings : public AmFilterConfigLayout
{
public:
	ArpChorusFilterSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		float	labelW = -1, intW = -1;
		const BFont*	font = be_plain_font;
		if( font ) {
			labelW = font->StringWidth( "Octaves:");
			intW = font->StringWidth("200%") + 5;
		}

		try {
			ArpBaseLayout*	topVBar = (new ArpRunningBar("TopVBar"))
											->SetParams(ArpMessage()
												.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
												.SetFloat(ArpRunningBar::IntraSpaceP, .5)
											);
			AddLayoutChild( topVBar );
			topVBar->AddLayoutChild((new ArpTextControl(
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
					);
			ArpBaseLayout*	colHBar = (new ArpRunningBar("ColHBar"))
											->SetParams(ArpMessage()
												.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
												.SetFloat(ArpRunningBar::IntraSpaceP, .5)
											);
			topVBar->AddLayoutChild( colHBar );
			for( uint32 k = 0; k < ArpChorusFilter::MAX_CHORUS; k++ )
				add_chorus_box( colHBar, k, mImpl, labelW, intW );
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
	}
	
protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpChorusFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	for( uint32 k = 0; k < MAX_CHORUS; k++ ) {
		BString		octStr;
		get_octave_string(k, octStr);
		if ((err=values->AddInt32(octStr.String(), mOctaves[k])) != B_OK) return err;
		BString		stepStr;
		get_step_string(k, stepStr);
		if ((err=values->AddInt32(stepStr.String(), mSteps[k])) != B_OK) return err;
		BString		velStr;
		get_velocity_string(k, velStr);
		if ((err=values->AddInt32(velStr.String(), mVelocities[k])) != B_OK) return err;
	}
	return B_OK;
}

status_t ArpChorusFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	int32 		i;
	for( uint32 k = 0; k < MAX_CHORUS; k++ ) {
		BString		octStr;
		get_octave_string(k, octStr);
		if (values->FindInt32(octStr.String(), &i) == B_OK) mOctaves[k] = i;
		BString		stepStr;
		get_step_string(k, stepStr);
		if (values->FindInt32(stepStr.String(), &i) == B_OK) mSteps[k] = i;
		BString		velStr;
		get_velocity_string(k, velStr);
		if (values->FindInt32(velStr.String(), &i) == B_OK) mVelocities[k] = i;
	}
	return B_OK;
}

status_t ArpChorusFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpChorusFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-CHORUS-FILTER-ADD-ON
 *****************************************************************************/
void ArpChorusFilterAddOn::LongDescription(BString& name, BString& str) const
{
	inherited::LongDescription(name, str);
	str << "<P>I produce up to four additional notes for every note I receive.
	The chorused notes can be offset in pitch by octave and/or step amounts,
	and their velocities can be scaled. Typically, the velocity should be increased
	for notes lower in pitch and decreased for notes higher in pitch.</P>";
}

void ArpChorusFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpChorusFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpChorusFilterAddOn(cookie);
	return NULL;
}

/*************************************************************************
 * _OCTAVE-MAP
 *************************************************************************/
status_t _OctaveMap::IdForName(const char *name, int32 *answer) const
{
	if( strcmp(name, "Off") == 0 ) {
		*answer = 0;
		return B_OK;
	}
	*answer = -1;
	return B_ERROR;
}

status_t _OctaveMap::NameForId(int32 id, char **answer) const
{
	if( id == 0 ) {
		*answer = "Off";
		return B_OK;
	}
	return B_ERROR;
}

/*************************************************************************
 * _PERCENT-FORMAT
 *************************************************************************/
void _PercentFormat::FormatInt(int32 number, BString& out) const
{
	out << number << '%';
};

ArpChorusFilterAddOn::ArpChorusFilterAddOn(const void* cookie)
	: AmFilterAddOn(cookie)
{
	
}

ArpChorusFilterAddOn::VersionType ArpChorusFilterAddOn::Version() const
{
	return VERSION_CURRENT;
}

BString ArpChorusFilterAddOn::Name() const
{
	return "Chorus";
}

BString ArpChorusFilterAddOn::Key() const
{
	return "arp:Chorus";
}

BString ArpChorusFilterAddOn::ShortDescription() const
{
	return "";
}

BString ArpChorusFilterAddOn::Author() const
{
	return "Eric Hackborn";
}
BString ArpChorusFilterAddOn::Email() const
{
	return "hackborn@angryredplanet.com";
}
ArpChorusFilterAddOn::type ArpChorusFilterAddOn::Type() const
{
	return THROUGH_FILTER;
}

AmFilterI* ArpChorusFilterAddOn::NewInstance(AmFilterHolderI* holder,
	const BMessage* config)
{
		return new ArpChorusFilter(this, holder, config);
}
