/* ArpMerge.h
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
 * 2001.05.12				hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-MERGE-FILTER
 * This filter is a straight passthrough, taking all events and sending
 * them to its next connection.  It can be used to merge in several sources.
 *****************************************************************************/
class ArpMergeFilterAddOn;

class ArpMergeFilter : public AmFilterI
{
public:
	ArpMergeFilter(	ArpMergeFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* config);
	virtual ~ArpMergeFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
private:
	ArpMergeFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
};

/*****************************************************************************
 * ARP-MERGE-FILTER-ADD-ON
 *****************************************************************************/
class ArpMergeFilterAddOn : public AmFilterAddOn
{
public:
	ArpMergeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Merge"; }
	virtual BString		Key() const						{ return "arp:Merge"; }
	virtual BString		ShortDescription() const		{ return "Merge all incoming events and send them to the next filter"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
