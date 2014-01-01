/* ArpMonophobic.h
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
 * 2001.06.12			hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include <be/support/String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-MONOPHOBIC-FILTER
 *****************************************************************************/
class ArpMonophobicAddOn;

class ArpMonophobicFilter : public AmFilterI
{
public:
	ArpMonophobicFilter(	ArpMonophobicAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	virtual ~ArpMonophobicFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	typedef AmFilterI		inherited;
	ArpMonophobicAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;
	float					mAmount;

	uint8					NewNote(uint8 oldNote, AmRange noteRange,
									const am_tool_filter_params* toolParams);
};

/*****************************************************************************
 * ARP-MONOPHOBIC-ADD-ON
 *****************************************************************************/
class ArpMonophobicAddOn : public AmFilterAddOn
{
public:
	ArpMonophobicAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Monophobic"; }
	virtual BString		Key() const						{ return "arp:Monophobic"; }
	virtual BString		ShortDescription() const		{ return "Notes follow the mouse as it is dragged"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual subtype		Subtype() const  				{ return TOOL_SUBTYPE; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = NULL)
		{ return new ArpMonophobicFilter(this, holder, config); }
};
