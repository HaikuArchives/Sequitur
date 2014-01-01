/* ArpUncertainShuttle.h
 * Copyright (c)2001 by Eric Hackborn
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
 * 2001.05.16		hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include <be/support/String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-FILTER
 * This filter can have any number of connections.  Each event it receives
 * is sent to a random connection.
 *****************************************************************************/
class ArpUncertainShuttleAddOn;

class ArpUncertainShuttleFilter : public AmFilterI
{
public:
	ArpUncertainShuttleFilter(	ArpUncertainShuttleAddOn* addon,
					 			AmFilterHolderI* holder,
								const BMessage* settings);
	virtual ~ArpUncertainShuttleFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual void		Start(uint32 context);

private:
	ArpUncertainShuttleAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mSeed;
};

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-ADD-ON
 *****************************************************************************/
class ArpUncertainShuttleAddOn : public AmFilterAddOn
{
public:
	ArpUncertainShuttleAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Uncertain Shuttle"; }
	virtual BString		Key() const						{ return "arp:UncertainShuttle"; }
	virtual int32		MaxConnections() const			{ return -1; }
	virtual BString		ShortDescription() const		{ return "Randomly chooses a connection for each event"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpUncertainShuttleFilter(this, holder, config); }
};
