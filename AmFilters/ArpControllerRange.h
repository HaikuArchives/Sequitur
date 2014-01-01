/* ArpControllerRange.h
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

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/* This is probably defined somewhere but it's been too long.
 */
#define ARP_CONTROLLER_SIZE			(128)

/*****************************************************************************
 * ARP-CONTROLLER-RANGE-FILTER
 * Transform control changes of one value into another.
 *****************************************************************************/
class ArpControllerRangeAddOn;

class ArpControllerRangeFilter : public AmFilterI
{
public:
	ArpControllerRangeFilter(	ArpControllerRangeAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* config);
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpControllerRangeAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	uint8						mLowMap[ARP_CONTROLLER_SIZE];
	uint8						mHighMap[ARP_CONTROLLER_SIZE];
};

/*****************************************************************************
 * ARP-CONTROLLER-RANGE-ADD-ON
 *****************************************************************************/
class ArpControllerRangeAddOn : public AmFilterAddOn
{
public:
	ArpControllerRangeAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Controller Range"; }
	virtual BString		Key() const						{ return "arp:ControllerRange"; }
	virtual BString		ShortDescription() const		{ return "Map the selected controllers into a new range"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
};
