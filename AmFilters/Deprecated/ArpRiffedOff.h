/* ArpRiffedOff.h
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
 * 09.03.00		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

class ArpVaccineFilter
{
public:
	ArpVaccineFilter(	AmFilterHolderI* holder, const char* key,
						int32 prox = 100, int32 freq = 50, int32 amt = 200);

	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	AmFilterHolderI* mHolder;
	/* The key is a hack -- a way of identifying vaccine filters uniquely
	 * from each other.  This will be removed once there is a proper
	 * multifilter filter.
	 */
	const char*		mKey;
	int32			mProximity;
	int32			mFrequency;
	int32			mAmount;

	bool			ShouldVaccinate(int32 frequency) const;
};

class ArpSubdividerFilter
{
public:
	ArpSubdividerFilter(AmFilterHolderI* holder);

	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

	enum {
		MAX_STEPS		= 3
	};
private:
	AmFilterHolderI* mHolder;
	int32		mStep[MAX_STEPS];
	int32		mFrequency[MAX_STEPS];

	/* Subdivide all events in the event chain.
	 */
	AmEvent*	Subdivide(AmEvent* event, int32 step, int32 frequency);
	AmEvent*	SubdivideLarger(AmEvent* event, AmTime newDuration);
	AmEvent*	SubdivideSmaller(AmEvent* event, AmTime newSub, AmTime newDuration);
	bool		ShouldSubdivide(int32 frequency) const;
	void		InitializeSteps();
};

/*****************************************************************************
 *
 *	ARP-RIFFED-OFF-FILTER and ARP-RIFFED-OFF-FILTER-ADDON CLASS
 *
 *	These classes are a filter to create random drum riffs and fills.
 *
 *****************************************************************************/

class ArpRiffedOffFilterAddOn;

class ArpRiffedOffFilter : public AmFilterI {
public:
	ArpRiffedOffFilter(	ArpRiffedOffFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	~ArpRiffedOffFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpRiffedOffFilterAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	mutable int32				mCounter;
	AmTime						mQuantizeTime;
	int32						mModifier;
	AmTime						mFullTime;
	int32						mMeasureCount;
	bigtime_t					mSeed;
	ArpVaccineFilter			mVaccine1;
	ArpVaccineFilter			mVaccine2;
	ArpSubdividerFilter			mSubdivider;
	
	void		GetSignature(AmTime time, const AmSignature* sig, AmSignature& answer);
};

class ArpRiffedOffFilterAddOn : public AmFilterAddOn {
public:
	ArpRiffedOffFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual const char* Name() const					{ return "Riffed Off"; }
	virtual const char* Key() const						{ return "arp:RiffedOff"; }
	virtual const char* ShortDescription() const		{ return "Transform each note into a simple drum riff"; }
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpRiffedOffFilter(this, holder, config); }
};
