/* ArpUnstack.h
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
 * 2001.08.16			hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include <be/support/String.h>
#include "AmPublic/AmFilterI.h"
class ArpUnstackAddOn;

/*****************************************************************************
 * ARP-UNSTACK-FILTER
 * Take stacks of notes and thin them out.
 *****************************************************************************/
class ArpUnstackFilter : public AmFilterI
{
public:
	ArpUnstackFilter(	ArpUnstackAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	virtual ~ArpUnstackFilter();
	
	virtual AmEvent*	HandleEvent(			AmEvent* event,
												const am_filter_params* params = NULL);
	virtual AmEvent* 	HandleBatchEvents(		AmEvent* event,
												const am_filter_params* params = NULL,
												const AmEvent* lookahead = NULL);
	virtual AmEvent*	HandleBatchToolEvents(	AmEvent* event,
												const am_filter_params* params = NULL,
												const am_tool_filter_params* toolParams = NULL,
												const AmEvent* lookahead = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	typedef AmFilterI	inherited;
	ArpUnstackAddOn*	mAddOn;
	AmFilterHolderI*	mHolder;

	int32				mThreshold;
	int32				mFrequency;
	bool				mShiftOn;
	int32				mShiftMult;
	AmTime				mShiftQuant;
	int32				mShiftDiv;
	bool				mShiftToolUseGrid;
	bool				mShiftNeg;

	AmEvent*			Unstack(AmEvent* event, int32 shiftMult, AmTime shiftQuant,
								int32 shiftDiv);
	AmEvent*			ThinAndMerge(AmEvent* thin, AmEvent* ans, uint32 thinCount, AmTime shift);
};

/*****************************************************************************
 * ARP-UNSTACK-ADD-ON
 *****************************************************************************/
class ArpUnstackAddOn : public AmFilterAddOn
{
public:
	ArpUnstackAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Unstack"; }
	virtual BString		Key() const						{ return "arp:Unstack"; }
	virtual BString		ShortDescription() const		{ return "Thin out stacks of notes"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = NULL)
		{ return new ArpUnstackFilter(this, holder, config); }
};
