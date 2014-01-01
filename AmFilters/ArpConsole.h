/* AmConsole.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * This class represents a class that events can be sprayed to (it will probably
 * end up being a subclass of some sort of AmSteam object or something).
 * Subclasses can modify the data they receive in any way they like.
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
 * 09.07.98		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif
#include "AmPublic/AmDeviceI.h"

#include <Message.h>
#include <String.h>
#include <View.h>

/*****************************************************************************
 *
 *	ARP-CONSOLE-FILTER and ARP-CONSOLE-FILTER-ADDON CLASS
 *
 *	This is a filter that simply prints out some limited information
 *	about every event that passes through it.
 *
 *****************************************************************************/

class ArpConsoleFilterAddOn;

class ArpConsoleFilter : public AmFilterI {
public:
	ArpConsoleFilter(	ArpConsoleFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	~ArpConsoleFilter();
	
	virtual AmEvent*			StartSection(	AmTime firstTime, AmTime lastTime,
												const am_filter_params* params = NULL);
	virtual AmEvent*			FinishSection(	AmTime firstTime, AmTime lastTime,
												const am_filter_params* params = NULL);
	virtual void				Stop(uint32 context);
	virtual AmEvent*			HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
		
	virtual status_t			GetConfiguration(BMessage* values) const;
	virtual status_t			PutConfiguration(const BMessage* values);
	virtual status_t			Configure(ArpVectorI<BView*>& panels);

private:
	int32						print_params(const am_filter_params* params) const;
	
	ArpRef<AmDeviceI>			mDevice;
	ArpConsoleFilterAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;

	int32						mTypeMask;		// Which events are on
};

class ArpConsoleFilterAddOn : public AmFilterAddOn {
public:
	ArpConsoleFilterAddOn(const void* cookie, const char* name="Console")
		: AmFilterAddOn(cookie),
		  mName(name)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return mName.String(); }
	virtual BString		Key() const						{ return "arp:Console"; }
	virtual BString		ShortDescription() const		{ return "Print all events to standard out"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne Hackborn"; }
	virtual BString		Email() const					{ return "hackbod@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = NULL)
		{ return new ArpConsoleFilter(this, holder, config); }

private:
	BString mName;
};
