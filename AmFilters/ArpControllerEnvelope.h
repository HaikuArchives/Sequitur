/* AmControllerEnvelopFilter.h
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
 * 2004.03.17		hackborn@angryredplanet.com
 * Updated to 1.1 -- added a mask for turning events on and off.
 *
 * 06.10.00			hackborn
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-CONTROLLER-ENVELOPE-FILTER
 * This filter envelope consumes note on events and replaces them with a series
 * of control changes.
 *****************************************************************************/
class ArpControllerEnvelopeFilterAddOn;

class ArpControllerEnvelopeFilter : public AmFilterI
{
public:
	ArpControllerEnvelopeFilter(ArpControllerEnvelopeFilterAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* config);
	virtual ~ArpControllerEnvelopeFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual BView* NewEditView(BPoint requestedSize) const;
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpControllerEnvelopeFilterAddOn*	mAddOn;
	AmFilterHolderI*					mHolder;

	uint8			mControlNumber;
	uint8			mInitialValue;
	int32			mDensity;
	
	int32			mMultAtk;
	AmTime			mQuantizeAtk;
	int32			mEighthsAtk;

	int32			mMultDcy;
	AmTime			mQuantizeDcy;
	int32			mEighthsDcy;
};

/*****************************************************************************
 * ARP-CONTROLLER-ENVELOPE-FILTER-ADD-ON
 *****************************************************************************/
class ArpControllerEnvelopeFilterAddOn : public AmFilterAddOn
{
public:
	ArpControllerEnvelopeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Controller Envelope"; }
	virtual BString		Key() const						{ return "arp:ControllerEnvelope"; }
	virtual BString		ShortDescription() const		{ return "Replace notes with a series of control changes"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
