/* ArpNote.h
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
 * 2001.07.08			hackborn@angryredplanet.com
 * Created this file
 */
#include <Message.h>
#include <String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-NOTE-FILTER
 *****************************************************************************/
class ArpNoteAddOn;

class ArpNoteFilter : public AmFilterI
{
public:
	ArpNoteFilter(	ArpNoteAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* settings);
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpNoteAddOn*		mAddOn;
	AmFilterHolderI*	mHolder;

	BString				mNote;
};

/*****************************************************************************
 * ARP-NOTE-ADD-ON
 *****************************************************************************/
class ArpNoteAddOn : public AmFilterAddOn
{
public:
	ArpNoteAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Note"; }
	virtual BString		Key() const						{ return "arp:Note"; }
	virtual BString		ShortDescription() const		{ return "I perform no processing, I just store text"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpNoteFilter(this, holder, config); }
		
private:
	typedef AmFilterAddOn	inherited;
};
