/* ArpCopy.h
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
 * 2001.05.07		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>

/*****************************************************************************
 * ARP-COPY-FILTER and ARP-COPY-FILTER-ADDON CLASS
 * This simple filter takes any events it receives and copies them to all its
 * connections.
 *****************************************************************************/
class ArpCopyFilterAddOn;

class ArpCopyFilter : public AmFilterI {
public:
	ArpCopyFilter(	ArpCopyFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* settings);
	~ArpCopyFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

private:
	ArpCopyFilterAddOn*			mAddOn;
	AmFilterHolderI*			mHolder;
};

class ArpCopyFilterAddOn : public AmFilterAddOn {
public:
	ArpCopyFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Copy"; }
	virtual BString		Key() const						{ return "arp:Copy"; }
	virtual int32		MaxConnections() const			{ return -1; }
	virtual BString		ShortDescription() const		{ return "Copy all events to all of my connections"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpCopyFilter(this, holder, config); }
};
