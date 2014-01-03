/* ArpMotionControl.h
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
 * 2001.07.16		hackborn@angryredplanet.com
 * Created this file
 */
#include <app/Message.h>
#include <support/String.h>
#include "AmPublic/AmFilterI.h"
class AmMotionI;

/*****************************************************************************
 * ARP-MOTION-CONTROL-FILTER
 *****************************************************************************/
class ArpMotionControlAddOn;

class ArpMotionControlFilter : public AmFilterI
{
public:
	ArpMotionControlFilter(	ArpMotionControlAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	virtual ~ArpMotionControlFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

	enum {
		VELOCITY_AMOUNT		= 1,
		SELECTED_AMOUNT		= 2
	};

	enum {
		TOOL_AMOUNT_FLAG	= 0x00000001
	};

	
private:
	ArpMotionControlAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	AmMotionI*				mMotion;
	int32					mMode;
	int32					mFlags;
	float					mAmount;
	uint8					mControlNumber;

	AmTime					mGrid;

	AmEvent*		MotionControl(	AmTime time, float amount,
									const am_filter_params* params);
	AmEvent*		EnvelopeMotionControl(	AmTime time, float amount,
											AmSignature& sig);
	void			InitMotion();
};

/*****************************************************************************
 * ARP-MOTION-CONTROL-ADD-ON
 *****************************************************************************/
class ArpMotionControlAddOn : public AmFilterAddOn
{
public:
	ArpMotionControlAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Motion Control"; }
	virtual BString		Key() const						{ return "arp:MotionControl"; }
	virtual BString		ShortDescription() const		{ return "Transform notes into controllers, based on my motion"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpMotionControlFilter(this, holder, config); }
};
