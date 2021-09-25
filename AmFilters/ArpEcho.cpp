#include "ArpEcho.h"

#include <cstdio>
#include <cstdlib>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char*	MODE_STR				= "mode";
static const char*	GRID_STR				= "grid";
static const char*	VELOCITY_CHANGE_STR		= "velocity_change";
static const char*	CURVE_STR				= "curve";
static const char*	START_STR				= "start";
static const char*	STOP_STR				= "stop";
static const char*	EXPONENT_STR			= "exponent";
static const char*	PERIOD_STR				= "period";

static const float	NEAR_ZERO				= 0.000001;

/*****************************************************************************
 *	_ECHO-FILTER-SETTINGS
 *****************************************************************************/
class _EchoFilterSettings : public AmFilterConfigLayout
{
public:
	_EchoFilterSettings(AmFilterHolderI* target,
						const BMessage& initSettings);

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	BRadioButton*		mDurRadio;
	BRadioButton*		mDistRadio;
	BRadioButton*		mLocRadio;
	BCheckBox*			mGridCheck;
	ArpMenuField*		mVelocityField;
	ArpMenuField*		mCurveField;
	ArpTextControl*		mExponentText;
	ArpTextControl*		mPeriodText;

	void RefreshControls(const BMessage& settings);
};

/*****************************************************************************
 *	ARP-ECHO-FILTER
 *****************************************************************************/
ArpEchoFilter::ArpEchoFilter(ArpEchoFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  mMode(DURATION_MODE), mDepth(3),
	  mVelocityChange(DESCENDING_VELOCITY), mCurve(EXPONENT_CURVE),
	  mStart(1), mStop(.1), mExponent(1), mPeriod(1),
	  mGridChoice(MY_GRID), mMultiplier(1), mQuantize(PPQN), mEighths(2)
{
	if (config) PutConfiguration(config);
}

ArpEchoFilter::~ArpEchoFilter()
{
}

AmEvent* ArpEchoFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);

	AmTime			grid = mMultiplier * ((mQuantize*2)/mEighths);

	if (mMode == DURATION_MODE)
		PerformDurationMode(event, grid);

	if (event) event->SetNextFilter(mHolder->ConnectionAt(0) );
	return event;
}

AmEvent* ArpEchoFilter::HandleToolEvent(AmEvent* event,
										const am_filter_params* params,
										const am_tool_filter_params* toolParams)
{
	if (!event) return event;
	ArpVALIDATE(mHolder && toolParams, return event);
	if (event->Type() != event->NOTEON_TYPE) return event;
	AmNoteOn*		e = dynamic_cast<AmNoteOn*>(event);
	if (!e) return event;

	AmEvent*		head = event;
	AmTime			grid;
	if (mGridChoice == TOOL_GRID) grid = toolParams->GridTime();
	else grid = mMultiplier * ((mQuantize*2)/mEighths);

	if (mMode == MOUSE_DISTANCE_MODE)
		head = PerformMouseDistanceMode(e, toolParams, grid);
	else if (mMode == MOUSE_LOCATION_MODE)
		head = PerformMouseLocationMode(e, toolParams, grid);
	else
		PerformDurationMode(e, grid);
	
	if (event) event->SetNextFilter(mHolder->ConnectionAt(0) );
	return head;
}

status_t ArpEchoFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err = values->AddInt32(MODE_STR, mMode)) != B_OK) return err;
	if ((err = values->AddInt32(GRID_STR, mGridChoice)) != B_OK) return err;
	if ((err = values->AddInt32("depth", mDepth)) != B_OK) return err;
	if ((err = values->AddInt32(VELOCITY_CHANGE_STR, mVelocityChange)) != B_OK) return err;
	if ((err = values->AddInt32(CURVE_STR, mCurve)) != B_OK) return err;
	if ((err = values->AddFloat(START_STR, mStart)) != B_OK) return err;
	if ((err = values->AddFloat(STOP_STR, mStop)) != B_OK) return err;
	if ((err = values->AddFloat(EXPONENT_STR, mExponent)) != B_OK) return err;
	if ((err = values->AddFloat(PERIOD_STR, mPeriod)) != B_OK) return err;
	if ((err = values->AddInt32(AM_MULTIPLIER_CONTROL_KEY_STR, mMultiplier)) != B_OK ) return err;
	if ((err = add_time(*values, AM_QUANTIZE_CONTROL_KEY_STR, mQuantize)) != B_OK ) return err;
	if ((err = values->AddInt32(AM_EIGHTHS_CONTROL_KEY_STR, mEighths)) != B_OK ) return err;

	return B_OK;
}

status_t ArpEchoFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	float f;
	if (values->FindInt32(MODE_STR, &i) == B_OK) {
		if (i < DURATION_MODE) i = DURATION_MODE;
		if (i > MOUSE_LOCATION_MODE) i = MOUSE_LOCATION_MODE;
		mMode = i;
	}
	if (values->FindInt32(GRID_STR, &i) == B_OK) {
		if (i < MY_GRID) i = MY_GRID;
		if (i > TOOL_GRID) i = TOOL_GRID;
		mGridChoice = i;
	}
	if (values->FindInt32("depth", &i) == B_OK) mDepth = i;
	if (values->FindInt32(VELOCITY_CHANGE_STR, &i) == B_OK) {
		if (i < DESCENDING_VELOCITY) i = DESCENDING_VELOCITY;
		if (i > EVEN_VELOCITY) i = EVEN_VELOCITY;
		mVelocityChange = i;
	}
	if (values->FindInt32(CURVE_STR, &i) == B_OK) {
		if (i < EXPONENT_CURVE) i = EXPONENT_CURVE;
		if (i > RANDOM_DECAY_CURVE) i = RANDOM_DECAY_CURVE;
		mCurve = i;
	}
	if (values->FindFloat(START_STR, &f) == B_OK) {
		if (f < 0.0) f = 0.0;
		if (f > 1.0) f = 1.0;
		mStart = f;
	}
	if (values->FindFloat(STOP_STR, &f) == B_OK) {
		if (f < 0.0) f = 0.0;
		if (f > 1.0) f = 1.0;
		mStop = f;
	}
	if (values->FindFloat(EXPONENT_STR, &f) == B_OK) {
		if (f < NEAR_ZERO) f = NEAR_ZERO;
		mExponent = f;
	}
	if (values->FindFloat(PERIOD_STR, &f) == B_OK) {
		if (f < NEAR_ZERO) f = NEAR_ZERO;
		mPeriod = f;
	}
	AmTime t;
	/* Backwards compatibility
	 */
	if (find_time(*values, "time", &t) == B_OK) {
		AmDurationControl::SplitTicks(t, &mMultiplier, &mQuantize, &mEighths);
	}
	
	if (values->FindInt32(AM_MULTIPLIER_CONTROL_KEY_STR, &i) == B_OK) mMultiplier = i;
	if (find_time(*values, AM_QUANTIZE_CONTROL_KEY_STR, &t) == B_OK) mQuantize = t;
	if (values->FindInt32(AM_EIGHTHS_CONTROL_KEY_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mEighths = i;
	}

	return B_OK;
}

status_t ArpEchoFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _EchoFilterSettings(mHolder, config));
	return B_OK;
}

uint8 ArpEchoFilter::VelocityCurveAt(uint8 base, int32 depth, int32 which)
{
	double scale = 1.0 - ( ((double)which) / depth );

	switch (mCurve) {
		case EXPONENT_CURVE:
			scale = pow(scale, mExponent);
			break;
		case GEOMETRIC_CURVE: {
			scale = 1.0;
			for (int32 i=0; i<which; i++) scale /= 2.0;
			scale = pow(scale, mExponent);
		} break;
		case SINUSOIDAL_CURVE:
			scale = pow((1+cos((1.0-scale)*2*M_PI*mPeriod)) / 2, mExponent);
			break;
		case SINUSOIDAL_DECAY_CURVE:
			scale = pow((1+cos((1.0-scale)*2*M_PI*mPeriod)) / 2, mExponent) * scale;
			break;
		case RANDOM_CURVE:
			scale = pow(drand48(), mExponent);
			break;
		case RANDOM_DECAY_CURVE:
			scale = pow(drand48(), mExponent) * scale;
			break;
		default:
			break;
	}
	
	scale = mStop + (mStart-mStop)*scale;
	
	if (scale > 1.0) scale = 1.0;
	else if (scale < 0.0) scale = 0.0;
	
	uint8 velocity;
	
	switch (mVelocityChange) {
		case ASCENDING_VELOCITY:
			velocity = base + uint8((127-base)*(1.0-scale)+.5);
			break;
		case DESCENDING_VELOCITY:
			if (base < 2) base = 2;
			velocity = uint8(((base-1)*scale)+1+.5);
			break;
		default:
			velocity = base;
			break;
	}
	
	return velocity != 0 ? velocity : 1;
}

void ArpEchoFilter::PerformDurationMode(AmEvent* event, AmTime grid)
{
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(1);
	if (!nextHolder) nextHolder = mHolder->ConnectionAt(0);	

	if (event->Type() == event->NOTEON_TYPE || event->Type() == event->NOTEOFF_TYPE) {
		AmEvent*	prevEvent = event;
		
		AmNoteOn*	nextOn = dynamic_cast<AmNoteOn*>(event);
		AmNoteOff*	nextOff = nextOn ? NULL : dynamic_cast<AmNoteOff*>(event);
		const uint8	baseAttack = nextOn ? nextOn->Velocity() : NULL;
		const uint8	baseRelease = nextOn ? nextOn->ReleaseVelocity() : (nextOff ? nextOff->Velocity() : NULL);
		
		for (int32 k = 0; k <= mDepth; k++) {
			AmEvent* nextEvent = k == 0 ? event : event->Copy();
			if (k > 0) {
				nextEvent->SetStartTime(nextEvent->StartTime() + (grid * k));
				nextEvent->SetNextFilter(nextHolder);
			}
			
			/* Compute new velocities */
			const uint8 thisAttack = VelocityCurveAt(baseAttack, mDepth, k);
			const uint8 thisRelease = VelocityCurveAt(baseRelease, mDepth, k);
			
			nextOn = dynamic_cast<AmNoteOn*>(nextEvent);
			nextOff = nextOn ? NULL : dynamic_cast<AmNoteOff*>(nextEvent);
			if (nextOn) {
				/* Adjust the velocities.
				 */
				nextOn->SetVelocity(thisAttack);
				nextOn->SetReleaseVelocity(thisRelease);
				if (k > 0) {
					prevEvent->AppendEvent(nextEvent);
					// Adjust duration if intermediate note, so that it doesn't
					// go past next echo.
					if (k < (mDepth) && event->Duration() > grid)
						event->SetDuration(grid);
					prevEvent = nextEvent;
				}
			} else if (nextOff) {
				/* Adjust the velocities.
				 */
				nextOff->SetVelocity(thisRelease);
				if (k > 0) {
					// Add note to resulting chain.
					prevEvent->AppendEvent(nextEvent);
					prevEvent = nextEvent;
				}
			} else if (k > 0) {
				nextEvent->Delete();
			}
		}
	}
}

#if 0
void ArpEchoFilter::PerformDurationMode(AmEvent* event)
{
	AmTime				time = mMultiplier * ((mQuantize*2)/mEighths);
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(1);
	if (!nextHolder) nextHolder = mHolder->ConnectionAt(0);	

	if (event->Type() == event->NOTEON_TYPE || event->Type() == event->NOTEOFF_TYPE) {
		AmEvent*	prevEvent = event;
		
		AmNoteOn*	nextOn = dynamic_cast<AmNoteOn*>(event);
		AmNoteOff*	nextOff = nextOn ? NULL : dynamic_cast<AmNoteOff*>(event);
		const uint8	baseAttack = nextOn ? nextOn->Velocity() : NULL;
		const uint8	baseRelease = nextOn ? nextOn->ReleaseVelocity() : (nextOff ? nextOff->Velocity() : NULL);
		
		for (int32 k = 0; k <= mDepth; k++) {
			AmEvent* nextEvent = k == 0 ? event : event->Copy();
			if (k > 0) {
				nextEvent->SetStartTime(nextEvent->StartTime() + (time * k));
				nextEvent->SetNextFilter(nextHolder);
			}
			
			/* Compute new velocities */
			const uint8 thisAttack = VelocityCurveAt(baseAttack, mDepth, k);
			const uint8 thisRelease = VelocityCurveAt(baseRelease, mDepth, k);
			
			nextOn = dynamic_cast<AmNoteOn*>(nextEvent);
			nextOff = nextOn ? NULL : dynamic_cast<AmNoteOff*>(nextEvent);
			if (nextOn) {
				/* Adjust the velocities.
				 */
				nextOn->SetVelocity(thisAttack);
				nextOn->SetReleaseVelocity(thisRelease);
				if (k > 0) {
					prevEvent->AppendEvent(nextEvent);
					// Adjust duration if intermediate note, so that it doesn't
					// go past next echo.
					if (k < (mDepth) && event->Duration() > time)
						event->SetDuration(time);
					prevEvent = nextEvent;
				}
			} else if (nextOff) {
				/* Adjust the velocities.
				 */
				nextOff->SetVelocity(thisRelease);
				if (k > 0) {
					// Add note to resulting chain.
					prevEvent->AppendEvent(nextEvent);
					prevEvent = nextEvent;
				}
			} else if (k > 0) {
				nextEvent->Delete();
			}
		}
	}
}

#endif

static bool keep_going(float x, int32 xSteps, float y, int32 ySteps)
{
	if (xSteps == 0 && ySteps == 0) return false;
	if (xSteps == 0) return int32(y) <= ySteps;
	if (ySteps == 0) return int32(x) <= xSteps;
	return int32(x) <= xSteps && int32(y) <= ySteps;
}

static inline float new_float(float f)
{
	if ((f - floor(f) ) < 0.5) return floor(f);
	else return ceil(f);
}

AmEvent* ArpEchoFilter::PerformMouseDistanceMode(	AmNoteOn* orig,
													const am_tool_filter_params* toolParams,
													AmTime grid)
{
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(1);
	if (!nextHolder) nextHolder = mHolder->ConnectionAt(0);	
	AmEvent*			e = orig;
	AmTime				xDist = toolParams->cur_time - toolParams->start_time;
	AmTime				xOffset = grid;
	if (xDist < 0) {
		xOffset = -grid;
		xDist = llabs(xDist);
	}
	int32				yDist = toolParams->cur_y_value - toolParams->orig_y_value;
	int32				yOffset = 1;
	if (yDist < 0) {
		yOffset = -yOffset;
		yDist = abs(yDist);
	}
	const int32			xSteps = xDist / grid, ySteps = yDist;
	const int32			depth = std::min(mDepth, std::max(xSteps, ySteps));
	const uint8			origVelocity = orig->Velocity();
	float				xChange = 1, yChange = 1;
	if (ySteps == 0) yChange = 0;
	else if (ySteps < xSteps) yChange = float(ySteps) / float(xSteps);
	if (xSteps == 0) xChange = 0;
	else if (xSteps < ySteps) xChange = float(xSteps) / float(ySteps);
	float				x = xChange, y = yChange;
	AmTime				lastTime = orig->StartTime();
	int32				lastNote = orig->Note();
	int32				i = 1;
	orig->SetVelocity(VelocityCurveAt(origVelocity, depth, 0));
	while (keep_going(x, xSteps, y, ySteps) ) {
		AmTime		newTime = orig->StartTime() + AmTime(new_float(x) * xOffset);
		if (newTime < 0) newTime = 0;
		int32		newNote = orig->Note() + int32(new_float(y) * yOffset);
		if (newNote < 0) newNote = 0;
		else if (newNote > 127) newNote = 127;
		if (lastTime != newTime || lastNote != newNote) {
			AmNoteOn*		no = dynamic_cast<AmNoteOn*>(orig->Copy());
			if (no) {
				no->SetStartTime(newTime);
				no->SetNote(newNote);
				no->SetVelocity(VelocityCurveAt(origVelocity, depth, i));
				no->SetNextFilter(nextHolder);
				e = e->MergeEvent(no);
				if (i++ == mDepth) return e;
			}
			lastTime = newTime;
			lastNote = newNote;
		}
		x += xChange;
		y += yChange;
	}
	return e;
}

AmEvent* ArpEchoFilter::PerformMouseLocationMode(	AmNoteOn* orig,
													const am_tool_filter_params* toolParams,
													AmTime grid)
{
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(1);
	if (!nextHolder) nextHolder = mHolder->ConnectionAt(0);	
	AmEvent*			e = orig;
	AmTime				xDist = toolParams->cur_time - orig->StartTime();
	AmTime				xOffset = grid;
	if (xDist < 0) {
		xOffset = -grid;
		xDist = llabs(xDist);
	}
	int32				yDist = toolParams->cur_y_value - orig->Note();
	int32				yOffset = 1;
	if (yDist < 0) {
		yOffset = -yOffset;
		yDist = abs(yDist);
	}
	const int32			xSteps = xDist / grid, ySteps = yDist;
	const int32			depth = std::min(mDepth, std::max(xSteps, ySteps));
	const uint8			origVelocity = orig->Velocity();
	float				xChange = 1, yChange = 1;
	if (ySteps == 0) yChange = 0;
	else if (ySteps < xSteps) yChange = float(ySteps) / float(xSteps);
	if (xSteps == 0) xChange = 0;
	else if (xSteps < ySteps) xChange = float(xSteps) / float(ySteps);
	float				x = xChange, y = yChange;
	AmTime				lastTime = orig->StartTime();
	int32				lastNote = orig->Note();
	int32				i = 1;
	orig->SetVelocity(VelocityCurveAt(origVelocity, depth, 0));
	while (keep_going(x, xSteps, y, ySteps) ) {
		AmTime		newTime = orig->StartTime() + AmTime(new_float(x) * xOffset);
		if (newTime < 0) newTime = 0;
		int32		newNote = orig->Note() + int32(new_float(y) * yOffset);
		if (newNote < 0) newNote = 0;
		else if (newNote > 127) newNote = 127;
		if (lastTime != newTime || lastNote != newNote) {
			AmNoteOn*		no = dynamic_cast<AmNoteOn*>(orig->Copy() );
			if (no) {
				no->SetStartTime(newTime);
				no->SetNote(newNote);
				no->SetVelocity(VelocityCurveAt(origVelocity, depth, i));
				no->SetNextFilter(nextHolder);
				e = e->MergeEvent(no);
				if (i++ == mDepth) return e;
			}
			lastTime = newTime;
			lastNote = newNote;
		}
		x += xChange;
		y += yChange;
	}
	return e;
}

// #pragma mark -

/*****************************************************************************
 *	ARP-ECHO-FILTER-ADD-ON
 *****************************************************************************/
void ArpEchoFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I echo note events based on my parameters.</p>"
	"<h4>Types</h4>"
	"<UL>"
	"	<LI><I>Traditional</I>.  This is a standard echo, where each note is"
	"	repeated depth number of times."
	"	<LI><I>Mouse distance</I>.  This works only when the filter is in a"
	"	tool pipeline.  Notes are echoed based on the distance the mouse has"
	"	traveled since being clicked.  For example, if a single note is selected,"
	"	as the mouse is dragged a tail forms that stretches from that note to"
	"	the mouse location.  If multiple notes are selected, they will all have"
	"	tails that mimic the one attached to the note that was clicked."
	"	<LI><I>Mouse location</I>.  This works only when the filter is in a"
	"	tool pipeline.  Notes are echoed toward the current mouse location.  For"
	"	example, if three notes are selected, then tails will be drawn from those"
	"	three notes to the current mouse location."
	"</UL>"
	"<h4>Duration</h4>"
	"	This is the amount of time between each echo.  If you select the \"Tools"
	"	use grid duration\" option, then when the Echo filter is used in a tool"
	"	its duration will come from the current note length of the track window"
	"	you are working in."
	"<h4>Depth</h4>"
	"	The number of additional notes to generate <em>after</em> the first.  In"
	"	<I>Mouse distance</I> and <I>Mouse location</I> modes, this is the maximum"
	"	number of additional notes to generate."
	"<h4>Velcocity</h4>"
	"<UL>"
	"	<LI><I>Decending</I> Note velocity decreases with time."
	"	<LI><I>Ascending</I> Note velocity increases with time."
	"	<LI><I>No Change</I> Note velocity is constant over time."
	"</UL>"
	"<h4>Start and Stop</h4>"
	"	These are the relative locations in the echo curve, ranging from 1.0 to"
	"	0.0, through which to spread note velocities.  For the full curve,"
	"	start is 1.0 and stop is 0.0; for a reversed curve, start is 0.0 and"
	"	stop is 1.0."
	"<h4>Curve</h4>"
	"<UL>"
	"	<LI><I>Exponential</I>. Linear curve raised to the <I>Exponent</I> power."
	"	<LI><I>Geometric</I>. Velocity is halved at each step."
	"	<LI><I>Sinusoidal</I>. Sin curve repeated <I>Period</I> times raised to the <I>Exponent</I> power."
	"	<LI><I>Sinusoidal Decay</I>. Sin curve repeated <I>Period</I> times"
	"	with linear modifier, raised to the <I>Exponent</I> power."
	"	<LI><I>Random</I>. Random curve raised to the <I>Exponent</I> power."
	"	<LI><I>Random Decay</I>. Random curve with linear modifier, raised to the <I>Exponent</I> power."
	"</UL>"
	"<h4>Exponent</h4>"
	"	This is a modifier for all curves, raising the final value (ranging from 0.0"
	"	to 1.0) to this exponent's power.  A value of 1.0 here is a linear curve.  A value"
	"	greater than 1.0 creates a faster curve (it drops from 1.0 more quickly), while a"
	"	value less than 1.0 creates a slower curve."
	"<h4>Period</h4>"
	"	For a sinusoidal curve, this is the number of iterations of the sin functions"
	"	to include in the curve.  A value of 1.0 creates a single sin iteration, going"
	"	from 1.0 down to 0.0 and back up to 1.0."
	"<h4>Connections</h4>"
	"	This filter can have either one or two connections.  If it has a single connection,"
	"	then all events it receives and generates are sent out that connection.  If it has"
	"	two connections, then all events it receives are sent out the first connection, and"
	"	all events it generates are sent out the second.";
}

void ArpEchoFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 2;
	*minor = 0;
}

BBitmap* ArpEchoFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpEchoFilterAddOn(cookie);
	return NULL;
}

// #pragma mark -

/*****************************************************************************
 *	_ECHO-FILTER-SETTINGS
 *****************************************************************************/
static const uint32 	DURATION_MSG		= 'iDur';
static const uint32 	GRID_MSG			= 'iGrd';
static const uint32		MOUSE_DISTANCE_MSG	= 'iDst';
static const uint32		MOUSE_LOCATION_MSG	= 'iLoc';
static const uint32		VELOCITY_MSG		= 'iVel';
static const uint32		CURVE_MSG			= 'iCur';

struct velocity_change_info
{
	const char* name;
	int32 value;
};

static const velocity_change_info velocity_change_types[] = {
	{ "Descending", ArpEchoFilter::DESCENDING_VELOCITY },
	{ "Ascending", ArpEchoFilter::ASCENDING_VELOCITY },
	{ "No change", ArpEchoFilter::EVEN_VELOCITY },
	{ NULL, 0 }
};

static const velocity_change_info curve_types[] = {
	{ "Exponential", ArpEchoFilter::EXPONENT_CURVE },
	{ "Geometric", ArpEchoFilter::GEOMETRIC_CURVE },
	{ "Sinusoidal", ArpEchoFilter::SINUSOIDAL_CURVE },
	{ "Sinusoidal decay", ArpEchoFilter::SINUSOIDAL_DECAY_CURVE },
	{ "Random", ArpEchoFilter::RANDOM_CURVE },
	{ "Random decay", ArpEchoFilter::RANDOM_DECAY_CURVE },
	{ NULL, 0 }
};

static ArpMenuField* new_menu_field(const char* name, const char* label,
		uint32 what, const velocity_change_info* info)
{
	BMenu*		menu = new BMenu(name);
	if (!menu) return NULL;
	while (info->name) {
		BMessage*	msg = new BMessage(what);
		if (msg) {
			msg->AddInt32("type", info->value);
			BMenuItem*	item = new BMenuItem(info->name, msg, 0, 0);
			if (item) menu->AddItem(item);
		}
		info++;
	}

	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);	
	ArpMenuField*	field = new ArpMenuField(name, label, menu);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

static ArpMenuField* new_velocity_change_field()
{
	return new_menu_field("velocity_field", "Velocity:", VELOCITY_MSG, velocity_change_types);
}

static ArpMenuField* new_curve_field()
{
	return new_menu_field("curve_field", "Curve:", CURVE_MSG, curve_types);
}

_EchoFilterSettings::_EchoFilterSettings(	AmFilterHolderI* target,
											const BMessage& initSettings)
		: inherited(target, initSettings),
		  mDurRadio(NULL), mDistRadio(NULL), mLocRadio(NULL),
		  mGridCheck(NULL),
		  mVelocityField(NULL), mCurveField(NULL),
		  mExponentText(NULL), mPeriodText(NULL)
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
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
			->AddLayoutChild((new ArpRunningBar("ModeHBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)

				->AddLayoutChild((new ArpRunningBar("RadioVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new ArpViewWrapper(mDurRadio = 
						new BRadioButton(BRect(0,0,10,10), "duration_radio", "Traditional",
								new BMessage(DURATION_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mDistRadio = 
						new BRadioButton(BRect(0,0,10,10), "distance_radio", "Mouse distance (tool only)",
								new BMessage(MOUSE_DISTANCE_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mLocRadio = 
						new BRadioButton(BRect(0,0,10,10), "location_radio", "Mouse location (tool only)",
								new BMessage(MOUSE_LOCATION_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
				)
			)
			->AddLayoutChild((new ArpBox("DurationBox", "Duration"))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					.SetBool(ArpRunningBar::AlignLabelsC,false)
				)
				->AddLayoutChild((new ArpRunningBar("DurationVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new ArpViewWrapper(mGridCheck =
						new BCheckBox(BRect(0,0,10,10), "grid", "Tools use grid duration",
								new BMessage(GRID_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
							.SetBool(ArpRunningBar::AlignLabelsC,false)
						)
					)
					->AddLayoutChild((new AmDurationControl("duration", "", this, initSettings))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							.SetBool(ArpRunningBar::AlignLabelsC,false)
						)
					)
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
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
			->AddLayoutChild((mVelocityField = new_velocity_change_field())
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpRunningBar("LimitsHBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
				->AddLayoutChild((new ArpTextControl(
										START_STR, "Start:","",
										mImpl.AttachTextControl(START_STR)))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "88")
						.SetString(ArpTextControl::PrefTextStringP, "888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpTextControl(
										STOP_STR, "Stop:","",
										mImpl.AttachTextControl(STOP_STR)))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "88")
						.SetString(ArpTextControl::PrefTextStringP, "888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
			)
			->AddLayoutChild((mCurveField = new_curve_field())
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((mExponentText = new ArpTextControl(
									EXPONENT_STR, "Exponent:","",
									mImpl.AttachTextControl(EXPONENT_STR)))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "888")
					.SetString(ArpTextControl::PrefTextStringP, "88888888")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
			->AddLayoutChild((mPeriodText = new ArpTextControl(
									PERIOD_STR, "Period:","",
									mImpl.AttachTextControl(PERIOD_STR)))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "888")
					.SetString(ArpTextControl::PrefTextStringP, "88888888")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
		);
	} catch(...) {
		throw;
	}
	
	Implementation().RefreshControls(mSettings);
	RefreshControls(initSettings);
}

void _EchoFilterSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mVelocityField) mVelocityField->Menu()->SetTargetForItems(this);
	if (mCurveField) mCurveField->Menu()->SetTargetForItems(this);
}

void _EchoFilterSettings::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case DURATION_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpEchoFilter::DURATION_MODE) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case MOUSE_DISTANCE_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpEchoFilter::MOUSE_DISTANCE_MODE) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case MOUSE_LOCATION_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpEchoFilter::MOUSE_LOCATION_MODE) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case GRID_MSG: {
			BMessage		upd;
			if (upd.AddInt32(GRID_STR, mGridCheck->Value()
							? ArpEchoFilter::TOOL_GRID : ArpEchoFilter::MY_GRID) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case VELOCITY_MSG: {
			int32 type;
			if (msg->FindInt32("type", &type) == B_OK) {
				BMessage		upd;
				if (upd.AddInt32(VELOCITY_CHANGE_STR, type) == B_OK)
					Implementation().SendConfiguration(&upd);
			}
		} break;
		case CURVE_MSG: {
			int32 type;
			if (msg->FindInt32("type", &type) == B_OK) {
				BMessage		upd;
				if (upd.AddInt32(CURVE_STR, type) == B_OK) {
					RefreshControls(upd);
					Implementation().SendConfiguration(&upd);
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void _EchoFilterSettings::RefreshControls(const BMessage& settings)
{
	int32		i;
	if (settings.FindInt32(MODE_STR, &i) == B_OK) {
		if (i == ArpEchoFilter::DURATION_MODE) {
			if (mDurRadio) mDurRadio->SetValue(B_CONTROL_ON);
		} else if (i == ArpEchoFilter::MOUSE_DISTANCE_MODE) {
			if (mDistRadio) mDistRadio->SetValue(B_CONTROL_ON);
		} else if (i == ArpEchoFilter::MOUSE_LOCATION_MODE) {
			if (mLocRadio) mLocRadio->SetValue(B_CONTROL_ON);
		}
	}

	if (settings.FindInt32(GRID_STR, &i) == B_OK) {
		mGridCheck->SetValue(i == ArpEchoFilter::MY_GRID ? B_CONTROL_OFF : B_CONTROL_ON);
	}

	if (mVelocityField && settings.FindInt32(VELOCITY_CHANGE_STR, &i) == B_OK) {
		BMenu* menu = mVelocityField->Menu();
		const int32 N = menu->CountItems();
		for (int32 j=0; j<N; j++) {
			BMenuItem* it = menu->ItemAt(j);
			int32 type;
			if (it && it->Message() && it->Message()->FindInt32("type", &type) == B_OK) {
				if (type == i) {
					it->SetMarked(true);
					break;
				}
			}
		}
	}
	if (mCurveField && settings.FindInt32(CURVE_STR, &i) == B_OK) {
		BMenu* menu = mCurveField->Menu();
		const int32 N = menu->CountItems();
		for (int32 j=0; j<N; j++) {
			BMenuItem* it = menu->ItemAt(j);
			int32 type;
			if (it && it->Message() && it->Message()->FindInt32("type", &type) == B_OK) {
				if (type == i) {
					it->SetMarked(true);
					break;
				}
			}
		}
		//if (mExponentText) mExponentText->SetEnabled(i == ArpEchoFilter::EXPONENT_CURVE);
		if (mPeriodText) mPeriodText->SetEnabled(i == ArpEchoFilter::SINUSOIDAL_CURVE
												|| i == ArpEchoFilter::SINUSOIDAL_DECAY_CURVE);
	}
}
