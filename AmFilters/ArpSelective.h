/* ArpSelective.h
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
 * 2001.08.14			hackborn@angryredplanet.com
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"
class ArpSelectiveAddOn;

/*****************************************************************************
 * ARP-SELECTIVE-FILTER
 * Only allow through the selected events.
 *****************************************************************************/
class ArpSelectiveFilter : public AmFilterI
{
public:
	ArpSelectiveFilter(	ArpSelectiveAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpSelectiveFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpSelectiveAddOn*	mAddOn;
	AmFilterHolderI*	mHolder;
	int32				mTypeMask;
	vector<uint8>		mControlNumbers;

	bool				IncludesControlNumber(uint8 number) const;
};

/*****************************************************************************
 * ARP-SELECTIVE-ADD-ON
 *****************************************************************************/
class ArpSelectiveAddOn : public AmFilterAddOn
{
public:
	ArpSelectiveAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Selective"; }
	virtual BString		Key() const						{ return "arp:Selective"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return "Only allow through the selected events"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
