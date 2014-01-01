/* ArpKeyboardSplitter.h
 * Copyright (c)2001 by Eric Hackborn.
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
#include <Message.h>
#include <String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-KEYBOARD-SPLITTER-FILTER
 * All events above the split point continue down my pipeline, all events
 * below it get sent to my first branch.
 *****************************************************************************/
class ArpVelocitySplitterAddOn;

class ArpVelocitySplitterFilter : public AmFilterI
{
public:
	ArpVelocitySplitterFilter(	ArpVelocitySplitterAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* settings);
	~ArpVelocitySplitterFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	ArpVelocitySplitterAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mSplitPoint;
};

/*****************************************************************************
 * ARP-KEYBOARD-SPLITTER-ADD-ON
 *****************************************************************************/
class ArpVelocitySplitterAddOn : public AmFilterAddOn
{
public:
	ArpVelocitySplitterAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Velocity Splitter"; }
	virtual BString		Key() const						{ return "arp:VelocitySplitter"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return "Send any notes below the split point to my branch"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpVelocitySplitterFilter(this, holder, config); }
};
