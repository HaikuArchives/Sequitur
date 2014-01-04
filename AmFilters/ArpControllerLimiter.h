/* ArpControllerLimiter.h
 * Copyright (c)2004 by Eric Hackborn
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
 * 2004.03.11		hackborn@angryredplanet.com
 * Created this file
 */
#include <app/Message.h>
#include <support/String.h>
#include "AmPublic/AmFilterI.h"
class AmMotionI;

/*****************************************************************************
 * ARP-MOTION-CONTROL-FILTER
 *****************************************************************************/
class ArpControllerLimiterAddOn;

class ArpControllerLimiterFilter : public AmFilterI
{
public:
	ArpControllerLimiterFilter(	ArpControllerLimiterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	virtual ~ArpControllerLimiterFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

	
private:
	ArpControllerLimiterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	AmMotionI*				mMotion1;
	AmMotionI*				mMotion2;
	uint8					mControlNumber;

	AmMotionI*				NewInitMotion(const BString& key) const;
};

/*****************************************************************************
 * ARP-MOTION-CONTROL-ADD-ON
 *****************************************************************************/
class ArpControllerLimiterAddOn : public AmFilterAddOn
{
public:
	ArpControllerLimiterAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Controller Limiter"; }
	virtual BString		Key() const						{ return "arp:ControllerLimiter"; }
	virtual BString		ShortDescription() const		{ return "Constrain control changes between two motions"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpControllerLimiterFilter(this, holder, config); }
};
