/* ArpDrone.h - a port of the Bars&Pipes tool of the same name.
 * Copyright (c) 2004 by Christian Packmann
 * Copyright (c) 2001 by Eric Hackborn.
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
 * ARP-DRONE-FILTER
 * Sends a copy of passing Note events to the second connection, forcing
 * them to the same note value - the note value is selected via GUI.
 *****************************************************************************/
class ArpDroneAddOn;

class ArpDroneFilter : public AmFilterI
{
public:
	ArpDroneFilter(	ArpDroneAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* settings);
	~ArpDroneFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	ArpDroneAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mSplitPoint;
};

/*****************************************************************************
 * ARP-KEYBOARD-SPLITTER-ADD-ON
 *****************************************************************************/
class ArpDroneAddOn : public AmFilterAddOn
{
public:
	ArpDroneAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Drone"; }
	virtual BString		Key() const						{ return "arp:Drone"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return "Sends copies of notes to 2nd connection, forcing them to same note value."; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Christian Packmann"; }
	virtual BString		Email() const					{ return "Christian.Packmann@gmx.de"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpDroneFilter(this, holder, config); }
};
