/* ArpWipeOut.h
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
 * 08.14.00		hackborn
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-WIPE-OUT-FILTER
 * This filter decreases an arbitary value (the transform 'y' value) of
 * each event it hits, until the y goes below its limit, and then it gets
 * deleted.
 *****************************************************************************/
class ArpWipeOutFilterAddOn;

class ArpWipeOutFilter : public AmFilterI
{
public:
	ArpWipeOutFilter(	ArpWipeOutFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpWipeOutFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpWipeOutFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	int32					mAmount;
};

/*****************************************************************************
 * ARP-WIPE-OUT-FILTER-ADD-ON
 *****************************************************************************/
class ArpWipeOutFilterAddOn : public AmFilterAddOn
{
public:
	ArpWipeOutFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Wipe Out"; }
	virtual BString		Key() const						{ return "arp:WipeOut"; }
	virtual BString		ShortDescription() const		{ return "Decrease each event until it is deleted"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
