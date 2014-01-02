#include "ArpRecombinant.h"

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

/*****************************************************************************
 *	ARP-RECOMBINANT-DESTINATION
 *****************************************************************************/
class ArpRecombinantDestination
{
public:
	virtual ~ArpRecombinantDestination()		{ }

	virtual bool		HasChange(float value, ArpMidiValueTable& mTable) const = 0;
	virtual void		SetChange(float value, ArpMidiValueTable& table) = 0;
	virtual AmEvent*	NewEvent(float percent) const = 0;

protected:
	ArpRecombinantDestination()					{ }
};

class ArpCcRecombinantDestination : public ArpRecombinantDestination
{
public:
	ArpCcRecombinantDestination(uint8 controlNumber = 7);

	virtual bool		HasChange(float value, ArpMidiValueTable& table) const;
	virtual void		SetChange(float value, ArpMidiValueTable& table);
	virtual AmEvent*	NewEvent(float percent) const;
	
private:
	typedef ArpRecombinantDestination	inherited;
	uint8		mControlNumber;

	uint8				ToCc(float percent) const;
};

/*****************************************************************************
 *	ARP-REST-CYCLE
 *****************************************************************************/
class ArpRestCycle
{
public:
	virtual ~ArpRestCycle()		{ }

	virtual void		SetPeriod(AmTime period) = 0;
	// 0 - 1
	virtual void		SetDepth(float depth) = 0;
	
	virtual float		GetValue(AmTime time, AmRange range) = 0;
	
protected:
	ArpRestCycle()				{ }
};

class ArpFlatRest : public ArpRestCycle
{
public:
	ArpFlatRest(float level) : mLevel(level)		{ }
	
	virtual void		SetPeriod(AmTime period)	{ }
	virtual void		SetDepth(float depth)		{ }

	virtual float		GetValue(AmTime time, AmRange range)	{ return mLevel; }

private:
	typedef ArpRestCycle	inherited;
	float			mLevel;
};

class ArpSineRest : public ArpRestCycle
{
public:
	ArpSineRest(AmTime period = PPQN, float depth = 1) : mPeriod(period), mDepth(depth) 	{ }
	
	virtual void		SetPeriod(AmTime period)	{ mPeriod = period; } 
	virtual void		SetDepth(float depth)		{ mDepth = depth; }

	virtual float		GetValue(AmTime time, AmRange range);

private:
	typedef ArpRestCycle	inherited;
	AmTime			mPeriod;
	float			mDepth;
};

/*****************************************************************************
 *	ARP-CURVE
 *****************************************************************************/
class ArpCurve
{
public:
	virtual ~ArpCurve()		{ }

	virtual float		GetValue(AmTime time, AmRange range) = 0;
	
protected:
	ArpCurve()				{ }
};

class ArpExponentialCurve : public ArpCurve
{
public:
	ArpExponentialCurve(float exponent = 2) : mExponent(exponent)		{ }
	
	virtual float		GetValue(AmTime time, AmRange range);

private:
	typedef ArpCurve	inherited;
	float			mExponent;
};



/*****************************************************************************
 *	ARP-RECOMBINANT-FILTER
 *****************************************************************************/
ArpRecombinantFilter::ArpRecombinantFilter(	ArpRecombinantAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* config)
	: AmFilterI(addon), mAddOn(addon), mHolder(holder)
{
	mRange.MakeInvalid();
	if (config) PutConfiguration(config);

	ArpRestCycle*				rest = new ArpSineRest(PPQN, 1);
	ArpRecombinantDestination*	dest = new ArpCcRecombinantDestination();
	if (rest && dest) {
		ArpRecombination*	recomb = new ArpRecombination(rest, dest);
		if (recomb) mRecombinations.push_back(recomb);
	}
}

ArpRecombinantFilter::~ArpRecombinantFilter()
{
	for (uint32 k = 0; k < mRecombinations.size(); k++)
		delete mRecombinations[k];
	mRecombinations.resize(0);
}

AmTime ArpRecombinantFilter::LookaheadTime() const
{
	return (PPQN * 2) - 1;
}

void ArpRecombinantFilter::Start(uint32 context)
{
	mTable.Clear();
	mRange.MakeInvalid();
}

AmEvent* ArpRecombinantFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(0);	

	AmEvent*			cur = event;
	if (event->TrackId() == mHolder->TrackId()) {
		AmTime			res(PPQN / 32);
		AmRange			range = event->TimeRange();
		for (AmTime time = range.start; time <= range.end; time += res) {

			for (uint32 index = 0; index < mRecombinations.size(); index++) {
				AmEvent*	e = mRecombinations[index]->Generate(time, range, mTable);
				if (e) {
//					e->Print();
					e->SetNextFilter(nextHolder);
					cur->AppendEvent(e);
					cur = e;
				}
			}
		}
	} else {
		event->Delete();
		return NULL;
	}
	event->SetNextFilter(nextHolder);
	return event;
}

AmEvent* ArpRecombinantFilter::HandleBatchEvents(	AmEvent* event,
													const am_filter_params* params,
													const AmEvent* lookahead)
{
#if 1
	if (lookahead) {
		printf("LOOKAHEAD:\n");
		const AmEvent*	e = lookahead;
		while (e) {
			e->Print();
			e = e->NextEvent();
		}
	} else printf("No lookahead\n");
	fflush(stdout);
#endif
	return event;
}

status_t ArpRecombinantFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
#if 0
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
#endif
	return B_OK;
}

status_t ArpRecombinantFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);
	if (err != B_OK) return err;
#if 0
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
#endif
	return B_OK;
}

#if 0
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

class ArpRecombinantFilterSettings : public AmFilterConfigLayout
{
public:
	ArpRecombinantFilterSettings(AmFilterHolderI* target,
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
			for( uint32 k = 0; k < ArpRecombinantFilter::MAX_CHORUS; k++ )
				add_chorus_box( colHBar, k, mImpl, labelW, intW );
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
	}
	
protected:
	typedef AmFilterConfigLayout inherited;
};
#endif

status_t ArpRecombinantFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
//	panels.push_back(new ArpRecombinantFilterSettings(mHolder, config));
	return B_OK;
}

// #pragma mark -

/*****************************************************************************
 * ARP-RECOMBINANT-ADD-ON
 *****************************************************************************/
void ArpRecombinantAddOn::LongDescription(BString& name, BString& str) const
{
	inherited::LongDescription(name, str);
	str << "<P>I produce up to four additional notes for every note I receive.
	The chorused notes can be offset in pitch by octave and/or step amounts,
	and their velocities can be scaled. Typically, the velocity should be increased
	for notes lower in pitch and decreased for notes higher in pitch.</P>";
}

void ArpRecombinantAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpRecombinantAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpRecombinantAddOn(cookie);
	return NULL;
}

// #pragma mark -

/*****************************************************************************
 * ARP-RECOMBINATION
 *****************************************************************************/
ArpRecombination::ArpRecombination(	ArpRestCycle* rest,
									ArpRecombinantDestination* dest)
		: mRest(rest), mDest(dest)
{
}

ArpRecombination::~ArpRecombination()
{
	delete mRest;
	delete mDest;
}

AmEvent* ArpRecombination::Generate(AmTime time, AmRange range,
									ArpMidiValueTable& table)
{
	if (!mRest || !mDest) return NULL;
	float		v = mRest->GetValue(time, range);
//printf("1 Generate time %lld zero %lld value %f\n", time, zero, v);
	if (v < -1) v = -1; else if (v > 1) v = 1;
	ArpExponentialCurve		exp;
	v = v * exp.GetValue(time, range);

	if (!mDest->HasChange(v, table)) return NULL;
//printf("\t2 Generate\n");
	AmEvent*	e = mDest->NewEvent(v);
	if (!e) return NULL;
	mDest->SetChange(v, table);
	e->SetStartTime(time);
	return e;
}

// #pragma mark -

/*****************************************************************************
 *	ARP-RECOMBINANT-DESTINATION
 *****************************************************************************/
ArpCcRecombinantDestination::ArpCcRecombinantDestination(uint8 controlNumber)
		: mControlNumber(controlNumber)
{
}

bool ArpCcRecombinantDestination::HasChange(float value, ArpMidiValueTable& table) const
{
	return (!table.ControlValueMatches(mControlNumber, ToCc(value)));
}

void ArpCcRecombinantDestination::SetChange(float value, ArpMidiValueTable& table)
{
	table.SetControlValue(mControlNumber, ToCc(value));
}

AmEvent* ArpCcRecombinantDestination::NewEvent(float percent) const
{
	return new AmControlChange(mControlNumber, ToCc(percent), 0);
}

uint8 ArpCcRecombinantDestination::ToCc(float percent) const
{
	int32					value = int32( ( (1 + percent) * 0.5) * 127);
	if (value > 127) value = 127; else if (value < 0) value = 0;
	return uint8(value);
}

// #pragma mark -

/*****************************************************************************
 *	ARP-REST-CYCLE
 *****************************************************************************/
float ArpSineRest::GetValue(AmTime time, AmRange range)
{
//	float		mExponent = 1.0;
	double		scale = sin(double(time - range.start) / (float(mPeriod) / (2 * M_PI)) );

//	if (scale > 1.0) scale = 1.0;
//	else if (scale < 0.0) scale = 0.0;

	return float(scale * mDepth);
}

// #pragma mark -


/*****************************************************************************
 *	ARP-CURVE
 *****************************************************************************/
float ArpExponentialCurve::GetValue(AmTime time, AmRange range)
{
	double		scale = ( ((double)(time - range.start)) / (range.end - range.start) );
	scale = pow(scale, mExponent);

	if (scale > 1.0) scale = 1.0;
	else if (scale < 0.0) scale = 0.0;

//	scale = (scale * 2) - 1;
float	mDepth = 1;
	return float(scale * mDepth);
}

// #pragma mark -

/*****************************************************************************
 *	ARP-MIDI-VALUE-TABLE
 *****************************************************************************/
static const int32		ILLEGAL_PITCH = -10000;
static const int32		ILLEGAL_CONTROL_VALUE = -1;

ArpMidiValueTable::ArpMidiValueTable()
{
	Clear();
}

ArpMidiValueTable::~ArpMidiValueTable()
{
}

bool ArpMidiValueTable::ControlValueMatches(uint8 number, uint8 value) const
{
	ArpASSERT(number < CONTROL_SIZE);
	if (mControlValues[number] == ILLEGAL_CONTROL_VALUE) return false;
	return mControlValues[number] == int32(value);
}

bool ArpMidiValueTable::PitchMatches(int16 pitch) const
{
	if (mPitch == ILLEGAL_PITCH) return false;
	return mPitch == int32(pitch);
}

void ArpMidiValueTable::SetControlValue(uint8 number, uint8 value)
{
	ArpASSERT(number < CONTROL_SIZE);
	mControlValues[number] = int32(value);
}

void ArpMidiValueTable::SetPitch(int16 pitch)
{
	mPitch = int32(pitch);
}

void ArpMidiValueTable::Clear()
{
	for (uint32 k = 0; k < CONTROL_SIZE; k++)
		mControlValues[k] = ILLEGAL_CONTROL_VALUE;
	mPitch = ILLEGAL_PITCH;
}