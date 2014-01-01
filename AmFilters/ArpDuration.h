/* AmDurationFilter.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 06.02.00		hackborn
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-DURATION-FILTER
 * This filter reassigns the duration of all note events that come through.
 * It can be configured to:
 *		Set all durations to an absolute value
 *		Scale all durations by a percent
 *****************************************************************************/
class ArpDurationFilterAddOn;

class ArpDurationFilter : public AmFilterI
{
public:
	ArpDurationFilter(	ArpDurationFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpDurationFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* HandleToolEvent(	AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);

	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

	enum {
		ABSOLUTE_MODE	= 1,
		SCALE_MODE		= 2,
		GRID_MODE		= 3
	};

private:
	ArpDurationFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;

	int32					mMode;
	AmTime					mAbsolute;
	float					mScale;
};

/*****************************************************************************
 * ARP-DURATION-FILTER-ADDON
 *****************************************************************************/
class ArpDurationFilterAddOn : public AmFilterAddOn
{
public:
	ArpDurationFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Duration"; }
	virtual BString		Key() const						{ return "arp:Duration"; }
	virtual BString		ShortDescription() const		{ return "Set the duration of note events"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
