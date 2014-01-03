/* ArpVelocityMap.h
 * Copyright (c)2001 by Eric Hackborn
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
 * 2001.06.05		hackborn@angryredplanet.com
 * Created this file
 */

#include <app/Message.h>
#include <interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-VELOCITY-MAP-FILTER
 * Transform a range of velocities into a different range of velocities.
 *****************************************************************************/
class ArpVelocityMapAddOn;

class ArpVelocityMapFilter : public AmFilterI
{
public:
	ArpVelocityMapFilter(	ArpVelocityMapAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config);
	virtual ~ArpVelocityMapFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpVelocityMapAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	AmRange					mFromRange;
	AmRange					mToRange;
};

/*****************************************************************************
 * ARP-VELOCITY-MAP-ADD-ON
 *****************************************************************************/
class ArpVelocityMapAddOn : public AmFilterAddOn
{
public:
	ArpVelocityMapAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Velocity Map"; }
	virtual BString		Key() const						{ return "arp:VelocityMap"; }
	virtual BString		ShortDescription() const		{ return "Transform a range of velocities into a different range of velocities"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
};
