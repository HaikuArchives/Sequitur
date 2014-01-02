/* ArpLegato.h
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
 * 2001.05.26				hackborn@angryredplanet.com
 * Created this file
 */
#include <app/Message.h>
#include <interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-LEGATO-FILTER
 * This filter prevents notes from playing at the same time.
 *****************************************************************************/
class ArpLegatoAddOn;

class ArpLegatoFilter : public AmFilterI
{
public:
	ArpLegatoFilter(ArpLegatoAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* config);
	virtual ~ArpLegatoFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event,
									const am_filter_params* params = NULL);
	virtual void		Start(uint32 context);
	
private:
	ArpLegatoAddOn*			mAddOn;
	AmFilterHolderI*		mHolder;
	enum {
		ALLOW_DIFFERENT_PITCHES		= 0x00000001
	};
	uint32					mFlags;
	
	static const uint32		NOTE_SIZE = 128;

	AmRange					mNotes[NOTE_SIZE];
	AmRange					mLast;
		
	void					Init();
};

/*****************************************************************************
 * ARP-LEGATO-ADD-ON
 *****************************************************************************/
class ArpLegatoAddOn : public AmFilterAddOn
{
public:
	ArpLegatoAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Legato"; }
	virtual BString		Key() const						{ return "arp:Legato"; }
	virtual BString		ShortDescription() const		{ return "Allow only one note to play at a time"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
