/* ArpMotionSplitter.h
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
 * 2001.08.12			hackborn@angryredplanet.com
 * Created this file
 */
#include <Message.h>
#include <String.h>
#include <View.h>
#include "AmPublic/AmFilterI.h"
class AmMotionI;
class ArpMotionSplitterAddOn;

/*****************************************************************************
 * ARP-MOTION-SPLITTER-FILTER
 * This filter finds each event's y value on a motion, and sends all events
 * with a y of less than the split point to the second output.
 *****************************************************************************/
class ArpMotionSplitterFilter : public AmFilterI
{
public:
	ArpMotionSplitterFilter(ArpMotionSplitterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	virtual ~ArpMotionSplitterFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* 	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);
	virtual AmEvent*	HandleBatchToolEvents(	AmEvent* event,
												const am_filter_params* params = NULL,
												const am_tool_filter_params* toolParams = NULL,
												const AmEvent* lookahead = NULL);

	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

	enum {
		FOLLOW_X_FLAG				= 0x00000001,
		FOLLOW_Y_FLAG				= 0x00000002,

		MOTION_FROM_TRACK_FLAG		= 0x00000020
	};
	
private:
	ArpMotionSplitterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	AmMotionI*				mMotion;
	uint32					mChangeFlags;
	int32					mFrequency;
	float					mAmount;
	int32					mSplitPoint;

	void					VaccinateBatchEvents(	AmEvent* event,
													const am_filter_params* params,
													AmMotionChange* curMotion,
													const am_tool_filter_params* toolParams);
	bool					ShouldVaccinate() const;
	void					InitMotion();
};

/*****************************************************************************
 * ARP-MOTION-SPLITTER-ADD-ON
 *****************************************************************************/
class ArpMotionSplitterAddOn : public AmFilterAddOn
{
public:
	ArpMotionSplitterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Motion Splitter"; }
	virtual BString		Key() const						{ return "arp:MotionSplitter"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return "Send to the second output every event whose y value (according to the motion) is below the split point"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = NULL)
		{ return new ArpMotionSplitterFilter(this, holder, config); }
};
