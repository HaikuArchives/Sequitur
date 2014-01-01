/* ArpRecombinant.h
 * Copyright (c)2002 by Eric Hackborn
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
 * 2002.04.28				hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <vector.h>
#include <Message.h>
#include <String.h>
#include <View.h>
class ArpRecombinantDestination;
class ArpRestCycle;

class ArpMidiValueTable
{
public:
	ArpMidiValueTable();
	virtual ~ArpMidiValueTable();

	bool		ControlValueMatches(uint8 number, uint8 value) const;
	bool		PitchMatches(int16 pitch) const;

	void		SetControlValue(uint8 number, uint8 value);
	void		SetPitch(int16 pitch);
	void		Clear();
	
private:
	static const uint32				CONTROL_SIZE = 128;

	int32		mControlValues[CONTROL_SIZE];
	int32		mPitch;
};

class ArpRecombination
{
public:
	ArpRecombination(	ArpRestCycle* rest,
						ArpRecombinantDestination* dest);
	virtual ~ArpRecombination();
	
	AmEvent*		Generate(	AmTime time, AmRange range,
								ArpMidiValueTable& table);
	
private:
	ArpRestCycle*				mRest;
	ArpRecombinantDestination*	mDest;
};

/*****************************************************************************
 * ARP-RECOMBINANT-FILTER
 *****************************************************************************/
class ArpRecombinantAddOn;

class ArpRecombinantFilter : public AmFilterI
{
public:
	ArpRecombinantFilter(	ArpRecombinantAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	~ArpRecombinantFilter();

	virtual AmTime		LookaheadTime() const;
	virtual void		Start(uint32 context);
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* 	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);

	// The ArpConfigurableI implementation.
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	ArpRecombinantAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;

	AmRange						mRange;
	vector<ArpRecombination*>	mRecombinations;
	ArpMidiValueTable			mTable;
};

class ArpRecombinantAddOn : public AmFilterAddOn {
public:
	ArpRecombinantAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Recombinant"; }
	virtual BString		Key() const						{ return "arp:Recombinant"; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpRecombinantFilter(this, holder, config); }
		
private:
	typedef AmFilterAddOn	inherited;
};
