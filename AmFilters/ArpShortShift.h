/* ArpShortShift.h
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
#include <app/Message.h>
#include <interface/View.h>
#include <support/String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-SHORT-SHIFT-FILTER
 * Change event time and duration by absolute amounts.
 *****************************************************************************/
class ArpShortShiftAddOn;

class ArpShortShiftFilter : public AmFilterI
{
public:
	ArpShortShiftFilter(ArpShortShiftAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	virtual ~ArpShortShiftFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	typedef AmFilterI		inherited;
	ArpShortShiftAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;

	int32					mStartMult;
	AmTime					mStartQuant;
	int32					mStartDiv;
	bool					mStartToolUseGrid;
	bool					mStartNeg;

	int32					mEndMult;
	AmTime					mEndQuant;
	int32					mEndDiv;
	bool					mEndToolUseGrid;
	bool					mEndNeg;

	void				Shift(	AmEvent* event,
								int32 sMult, AmTime sQuant, int32 sDiv,
								int32 eMult, AmTime eQuant, int32 eDiv);
};

/*****************************************************************************
 * ARP-SHORT-SHIFT-ADD-ON
 *****************************************************************************/
class ArpShortShiftAddOn : public AmFilterAddOn
{
public:
	ArpShortShiftAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Short Shift"; }
	virtual BString		Key() const						{ return "arp:ShortShift"; }
	virtual BString		ShortDescription() const		{ return "Change event time and duration by absolute amounts"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = NULL)
		{ return new ArpShortShiftFilter(this, holder, config); }
};
