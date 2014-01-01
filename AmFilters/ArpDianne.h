/* ArpExample.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * An example filter.
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
 *	• None!  Ha ha!
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

#include <Message.h>
#include <String.h>
#include <View.h>

/*****************************************************************************
 *
 *	ARP-EXAMPLE-FILTER and ARP-EXAMPLE-FILTER-ADDON CLASS
 *
 *	These classes are an example of a fairly simple MIDI event
 *	filter.  The only event it processes is NOTEON; events of all
 *	other types are passed through untouched.  When it gets a NOTEON
 *	event, it duplicates it, creating softer notes one beat before and
 *	one beat after the input note.
 *
 *****************************************************************************/

class ArpExampleFilterAddOn;

class ArpExampleFilter : public AmFilterI {
public:
	ArpExampleFilter(ArpExampleFilterAddOn* addon,
					 AmFilterHolderI* holder,
					 const BMessage* settings);
	~ArpExampleFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

private:
	ArpExampleFilterAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;
};

class ArpExampleFilterAddOn : public AmFilterAddOn {
public:
	ArpExampleFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Dianne"; }
	virtual BString		Key() const						{ return "arp:Example"; }
	virtual BString		ShortDescription() const		{ return "Code example:  Play a soft note, then the original note, then a soft note"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne Hackborn"; }
	virtual BString		Email() const					{ return "hackbod@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = NULL)
			{ return new ArpExampleFilter(this, holder, config); }
};
