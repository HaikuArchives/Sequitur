/* ArpTimeStretch.h
 * Copyright (c)2002 by Eric Hackborn
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
 * 2002.10.21				hackborn@angryredplanet.com
 * Created this file
 */
#include <app/Message.h>
#include <interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-TIME-STRETCH-FILTER
 * Time stretch all selected events.
 *****************************************************************************/
class ArpTimeStretchFilterAddOn;

class ArpTimeStretchFilter : public AmFilterI
{
public:
	ArpTimeStretchFilter(	ArpTimeStretchFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpTimeStretchFilter();
	
	virtual status_t	PutConfiguration(const BMessage* values);
	
	virtual AmEvent*	HandleEvent(AmEvent* event,
									const am_filter_params* params = NULL);
	virtual AmEvent*	HandleBatchToolEvents(	AmEvent* event,
												const am_filter_params* params = NULL,
												const am_tool_filter_params* toolParams = NULL,
												const AmEvent* lookahead = NULL);
	
private:
	ArpTimeStretchFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
};

/*****************************************************************************
 * ARP-TIME-STRETCH-FILTER-ADD-ON
 *****************************************************************************/
class ArpTimeStretchFilterAddOn : public AmFilterAddOn
{
public:
	ArpTimeStretchFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "TimeStretch"; }
	virtual BString		Key() const						{ return "arp:TimeStretch"; }
	virtual BString		ShortDescription() const		{ return "Time stretch all events"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
