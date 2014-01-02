/* ArpInverse.h
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
 * ARP-INVERSE-FILTER
 * Invert the pitch of all notes around the center of the range.
 *****************************************************************************/
class ArpInverseFilterAddOn;

class ArpInverseFilter : public AmFilterI
{
public:
	ArpInverseFilter(	ArpInverseFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpInverseFilter();
	
	virtual status_t	PutConfiguration(const BMessage* values);
	
	virtual AmEvent*	HandleEvent(AmEvent* event,
									const am_filter_params* params = NULL);
	virtual AmEvent*	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);
	
private:
	ArpInverseFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
};

/*****************************************************************************
 * ARP-INVERSE-FILTER-ADD-ON
 *****************************************************************************/
class ArpInverseFilterAddOn : public AmFilterAddOn
{
public:
	ArpInverseFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Inverse"; }
	virtual BString		Key() const						{ return "arp:Inverse"; }
	virtual BString		ShortDescription() const		{ return "Invert note pitch around the center"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
