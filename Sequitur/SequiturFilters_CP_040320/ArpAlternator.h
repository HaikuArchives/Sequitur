/* ArpAlternate.h
 * Copyright (c) 2004 by Christian Packmann
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
 * 2004.02.08 CPackmann
 * 2001.05.16		hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include <be/support/String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-ALTERNATOR-FILTER
 * This filter can have any number of connections.  Received events are sent
 * alternatingly to all connections.
 *****************************************************************************/
class ArpAlternateAddOn;

class ArpAlternateFilter : public AmFilterI
{
public:
	ArpAlternateFilter(	ArpAlternateAddOn* addon,
					 			AmFilterHolderI* holder,
								const BMessage* settings);
	virtual ~ArpAlternateFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual void		Start(uint32 context);

private:
	ArpAlternateAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mSeed;
	uint32 lastOut;
};

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-ADD-ON
 *****************************************************************************/
class ArpAlternateAddOn : public AmFilterAddOn
{
public:
	ArpAlternateAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Alternator"; }
	virtual BString		Key() const						{ return "arp:Alternator"; }
	virtual int32		MaxConnections() const			{ return -1; }
	virtual BString		ShortDescription() const		{ return "Alternates Note events between all connections"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Christian Packmann"; }
	virtual BString		Email() const					{ return "Christian.Packmann@gmx.de"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpAlternateFilter(this, holder, config); }
};
