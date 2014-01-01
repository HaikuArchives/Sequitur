/* ArpVaccine.h
 * Copyright (c)2000 by Eric Hackborn
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
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
 * 12.04.00		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

/*****************************************************************************
 *
 *	ARP-VACCINE-FILTER and ARP-VACCINE-FILTER-ADDON CLASS
 *
 *	These classes are a filter that use velocity to accent notes on the beat.
 *
 *****************************************************************************/
class ArpVaccineFilterAddOn;

class ArpVaccineFilter : public AmFilterI {
public:
	ArpVaccineFilter(	ArpVaccineFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	virtual ~ArpVaccineFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpVaccineFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	int32					mProximity;
	int32					mFrequency;
	int32					mAmount;
	
	bool			ShouldVaccinate(int32 frequency) const;
};

class ArpVaccineFilterAddOn : public AmFilterAddOn {
public:
	ArpVaccineFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual const char* Name() const					{ return "Vaccine"; }
	virtual const char* Key() const						{ return "arp:Vaccine"; }
	virtual const char* ShortDescription() const		{ return "Accent the velocity of notes depending on proximity to the beat"; }
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpVaccineFilter(this, holder, config); }
};
