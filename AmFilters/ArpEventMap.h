/* ArpEventMap.h
 * Copyright (c)2004 by Eric Hackborn
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
 * 2004.03.18		hackborn@angryredplanet.com
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"
class ArpEventMapAddOn;

/*****************************************************************************
 * ARP-EVENT-MAP
 * Transform events to a new type.
 *****************************************************************************/
class ArpEventMap : public AmFilterI
{
public:
	ArpEventMap(ArpEventMapAddOn* addon,
				AmFilterHolderI* holder,
				const BMessage* config);
	
	virtual AmEvent*		HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t		GetConfiguration(BMessage* values) const;
	virtual status_t		PutConfiguration(const BMessage* values);
	virtual status_t		Configure(ArpVectorI<BView*>& panels);

private:
	ArpEventMapAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;
	int32					mNoteVel;
	int32					mNotePitch;
	int32					mCcSrc;
	int32					mCcDest;
	int32					mPitch;
	int32					mAftertouch;

	AmEvent*				HandleNoteOn(AmNoteOn* e);
	AmEvent*				HandleControlChange(AmControlChange* e);
	AmEvent*				HandlePitchBend(AmPitchBend* e);
	AmEvent*				HandleChannelPressure(AmChannelPressure* e);
};

/*****************************************************************************
 * ARP-EVENT-MAP-ADD-ON
 *****************************************************************************/
class ArpEventMapAddOn : public AmFilterAddOn
{
public:
	ArpEventMapAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Event Map"; }
	virtual BString		Key() const						{ return "arp:EventMap"; }
	virtual BString		ShortDescription() const		{ return "Transform events to a different type"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
};
