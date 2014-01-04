/* ArpControllerMap.h
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

#define ROCK_ON						;

/*****************************************************************************
 * ARP-CONTROLLER-MAP-FILTER
 * Transform control changes of one value into another.
 *****************************************************************************/
class ArpControllerMapAddOn;

class ArpControllerMapFilter : public AmFilterI
{
public:
	ArpControllerMapFilter(	ArpControllerMapAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config);
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpControllerMapAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	uint8					mMap[AM_CONTROLLER_SIZE];
};

/*****************************************************************************
 * ARP-CONTROLLER-MAP-ADD-ON
 *****************************************************************************/
class ArpControllerMapAddOn : public AmFilterAddOn
{
public:
	ArpControllerMapAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Controller Map"; }
	virtual BString		Key() const						{ return "arp:ControllerMap"; }
	virtual BString		ShortDescription() const		{ return "Transform control changes of one value into another"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
};
