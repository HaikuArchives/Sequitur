/* ArpRubberStamp.h
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
class AmMotionI;

/*****************************************************************************
 * ARP-RUBBER-STAMP-FILTER
 * Transform notes into a series of control changes.
 *****************************************************************************/
class ArpRubberStampAddOn;

class ArpRubberStampFilter : public AmFilterI
{
public:
	ArpRubberStampFilter(	ArpRubberStampAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	virtual ~ArpRubberStampFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	typedef AmFilterI		inherited;
	ArpRubberStampAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	AmMotionI*				mMotion;
	uint8					mControlNumber;
	int32					mMultiplier;
	AmTime					mQuantizeTime;
	int32					mDivider;
	float					mAmount;
	enum {
		FOLLOW_X			= 0x00000001,
		USE_TOOL_BOUNDS		= 0x00000002
	};
	int32					mFlags;

	void					InitMotion();
	AmEvent*				GenerateMotion(	AmRange range, float amount,
											const am_filter_params* params,
											const am_tool_filter_params* toolParams);
};

/*****************************************************************************
 * ARP-RUBBER-STAMP-ADD-ON
 *****************************************************************************/
class ArpRubberStampAddOn : public AmFilterAddOn
{
public:
	ArpRubberStampAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Rubber Stamp"; }
	virtual BString		Key() const						{ return "arp:RubberStamp"; }
	virtual BString		ShortDescription() const		{ return "Transform notes into control changes"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual subtype		Subtype() const  				{ return TOOL_SUBTYPE; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = NULL)
		{ return new ArpRubberStampFilter(this, holder, config); }
};
