/* ArpReverb.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 08.08.00		hackborn
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
 *	ARP-REVERB-FILTER and ARP-REVERB-FILTER-ADDON CLASS
 *
 *	These classes are a simple filter to produce an echo for note events.
 *	The only event it processes is NOTEON; events of all
 *	other types are passed through untouched.  When it gets a NOTEON
 *	event, it echoes it based on the depth setting.  This was written so that
 *	there are actually TWO different non-output filters to work with.
 *
 *****************************************************************************/

class ArpReverbFilterAddOn;

class ArpReverbFilter : public AmFilterI {
public:
	ArpReverbFilter(ArpReverbFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* settings);
	~ArpReverbFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

private:
	ArpReverbFilterAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;
	// This controls the number of echoes I generate.
	int32						mDepth;
	AmTime						mTime;
};

class ArpReverbFilterAddOn : public AmFilterAddOn {
public:
	ArpReverbFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual const char* Name() const					{ return "Reverb"; }
	virtual const char* Key() const						{ return "arp:Reverb"; }
	virtual const char* ShortDescription() const		{ return 0; }
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpReverbFilter(this, holder, config); }
};
