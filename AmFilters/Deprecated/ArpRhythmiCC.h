/* ArpRhythmiCC.h
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
 * 2001.03.26			hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>
class AmMotionI;

/*****************************************************************************
 * ARP-RHYTHMICC-FILTER and ARP-RHYTHMICC-FILTER-ADDON CLASS
 * This filter transforms a rhythm into Control Change events.
 *****************************************************************************/
class ArpRhythmiCcFilterAddOn;

class ArpRhythmiCcFilter : public AmFilterI
{
public:
	ArpRhythmiCcFilter(	ArpRhythmiCcFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	virtual ~ArpRhythmiCcFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual status_t	GetProperties(BMessage* properties) const;

private:
	typedef AmFilterI	inherited;
	ArpRhythmiCcFilterAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	AmMotionI*					mMotion;
	uint8						mControlNumber;
	int32						mFrequency;
	float						mAmount;
	enum {
		FOLLOW_X			= 0x00000001
	};
	int32						mFlags;
	/* This is for the HandleEvent() method, i.e. when we're tracking the
	 * events as they come through.
	 */
	AmRange						mLastRange;
	void						InitMotion();
	AmEvent*					GenerateRhythm(	AmRange range, AmEvent* include,
												float amount, const am_filter_params* params,
												const am_tool_filter_params* toolParams);
};

class ArpRhythmiCcFilterAddOn : public AmFilterAddOn
{
public:
	ArpRhythmiCcFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "RhythmiCC"; }
	virtual BString		Key() const						{ return "arp:RhythmiCC"; }
	virtual BString		ShortDescription() const		{ return "Create a series of control change events based on a Motion"; }
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual subtype		Subtype() const  				{ return TOOL_SUBTYPE; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpRhythmiCcFilter(this, holder, config); }
};
