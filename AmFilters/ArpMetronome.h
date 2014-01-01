/* ArpMetronome.h
 * Copyright (c)2000 by Dianne Hackborn.
 * All rights reserved.
 *
 * An echo filter.
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
 *	• None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.07.98		hackborn
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
 *	ARP-METRONOME-FILTER and ARP-METRONOME-FILTER-ADDON CLASS
 *
 *	These classes are a simple filter that produces a steady stream of
 *	events while a song is playing.
 *
 *****************************************************************************/

class ArpMetronomeFilterAddOn;

class ArpMetronomeFilter : public AmFilterI {
public:
	ArpMetronomeFilter(ArpMetronomeFilterAddOn* addon,
					 AmFilterHolderI* holder,
					 const BMessage* settings);
	~ArpMetronomeFilter();
	
	virtual AmEvent* StartSection(AmTime firstTime, AmTime lastTime,
								  const am_filter_params* params = NULL);
	virtual AmEvent* FinishSection(AmTime firstTime, AmTime lastTime,
								   const am_filter_params* params = NULL);
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

private:
	ArpMetronomeFilterAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	AmTime						mBeat;
	int32						mBeatsPerMeasure;
	uint8						mMeasureNote;
	uint8						mMeasureVelocity;
	uint8						mBeatNote;
	uint8						mBeatVelocity;
	uint8						mOffbeatNote;
	uint8						mOffbeatVelocity;

	int32						mFlags;
};

class ArpMetronomeFilterAddOn : public AmFilterAddOn {
public:
	ArpMetronomeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Metronome"; }
	virtual BString		Key() const						{ return "arp:Metronome"; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne Hackborn"; }
	virtual BString		Email() const					{ return "hackbod@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpMetronomeFilter(this, holder, config); }
};
