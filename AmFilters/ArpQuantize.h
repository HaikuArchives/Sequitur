/* ArpQuantizeFilter.h
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
 * 08.08.00		hackborn
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-QUANTIZE-FILTER
 * This filter assigns the start and end times of any events that come through
 * it to align with its current quantize grid.
 *****************************************************************************/
class ArpQuantizeFilterAddOn;

class ArpQuantizeFilter : public AmFilterI
{
public:
	ArpQuantizeFilter(	ArpQuantizeFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpQuantizeFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* HandleToolEvent(	AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

	enum {
		MY_GRID					= 1,
		TOOL_GRID				= 2
	};
	enum {
		LEFT_SHIFT			= 0,
		RIGHT_SHIFT			= 1,
		CLOSEST_SHIFT		= 2
	};

private:
	ArpQuantizeFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;

	int32					mMultiplier;
	AmTime					mQuantizeTime;
	int32					mDivider;
	AmTime					mFullTime;
	int32					mGridChoice;
	bool					mDoStartTime;
	bool					mDoEndTime;
	int32					mShift;
	int32					mAttraction;	// 1 - 100%
	
	void					DoQuantize(AmEvent* event, AmTime fullTime, const int64 smallest);
};

/*****************************************************************************
 * ARP-QUANTIZE-FILTER-ADDON
 *****************************************************************************/
class ArpQuantizeFilterAddOn : public AmFilterAddOn
{
public:
	ArpQuantizeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Quantize"; }
	virtual BString		Key() const						{ return "arp:Quantize"; }
	virtual BString		ShortDescription() const		{ return "Snap event times to a grid"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne and Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
