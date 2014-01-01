/* ArpOscillator.h
 * Copyright (c)2003 by Eric Hackborn
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.04.23				hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"
class ArpOscillatorAddOn;
class AmSweep;

/*****************************************************************************
 * ARP-OSCILLATOR-FILTER
 * Generate control changes based on a sweep.
 *****************************************************************************/
class ArpOscillatorFilter : public AmFilterI
{
public:
	ArpOscillatorFilter(ArpOscillatorAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpOscillatorFilter();
	
	virtual status_t	PutConfiguration(const BMessage* values);

	virtual AmEvent*	OscPulse(	AmTime start, AmTime end,
									const am_filter_params* params = NULL);

	virtual AmEvent*	HandleEvent(AmEvent* event,
									const am_filter_params* params = NULL);
	
private:
	ArpOscillatorAddOn*	mAddOn;
	AmFilterHolderI*	mHolder;
	AmSweep*			mSweep;
	AmTime				mResolution;
	float				mCurStep;
	int32				mLast;
};

/*****************************************************************************
 * ARP-OSCILLATOR-ADD-ON
 *****************************************************************************/
class ArpOscillatorAddOn : public AmFilterAddOn
{
public:
	ArpOscillatorAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Oscillator"; }
	virtual BString		Key() const						{ return "arp:Oscillator"; }
	virtual BString		ShortDescription() const		{ return "Generate control changes from a sweep"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
