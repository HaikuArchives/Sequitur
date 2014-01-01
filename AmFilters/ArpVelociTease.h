/* ArpVelociTeaseFilter.h
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
 * ARP-VELOCI-TEASE-FILTER
 * This filter reassigns the velocity of all note events that come through.
 * It can be configured to:
 *		Set all velocities to an absolute value
 *		Scale all velocities by a percent
 *****************************************************************************/
class ArpVelociTeaseAddOn;

class ArpVelociTeaseFilter : public AmFilterI
{
public:
	ArpVelociTeaseFilter(	ArpVelociTeaseAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config);
	virtual ~ArpVelociTeaseFilter();
	
	virtual AmEvent*	HandleEvent(		AmEvent* event,
											const am_filter_params* params = NULL);
	virtual AmEvent* 	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual void		Start(uint32 context);

	enum {
		ABSOLUTE_MODE	= 1,
		SCALE_MODE		= 2,
		DELTA_MODE		= 3,
		RANDOM_MODE		= 4
	};

	virtual status_t GetToolTipText(BString* out) const {
		if(HasLabel()) {
			*out << "(" << Label() << ") ";
		}
		*out << "VelTease: ";
		switch(mMode) {
			case ABSOLUTE_MODE:
				*out << "Set to " << static_cast<int32>(mAbsolute);
				break;
			case SCALE_MODE:
				*out << "Scale to " << static_cast<int32>(mScale) << "%";
				break;
			case DELTA_MODE:
				*out << "Delta " << mDelta;
				break;
			case RANDOM_MODE:
				*out << "Random";
				break;
		}
		
		//[" << mFromRange.start << "," << mFromRange.end << "] -> [" << mToRange.start << "," << mToRange.end << "]";
		
		return B_OK;
	}

private:
	ArpVelociTeaseAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;

	int32					mMode;
	uchar					mAbsolute;
	float					mScale;
	int32					mDelta;
	bigtime_t				mSeed;
};

/*****************************************************************************
 * ARP-VELOCI-TEASE-ADDON
 *****************************************************************************/
class ArpVelociTeaseAddOn : public AmFilterAddOn
{
public:
	ArpVelociTeaseAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "VelociTease"; }
	virtual BString		Key() const						{ return "arp:Velocity"; }
	virtual BString		ShortDescription() const		{ return "Set the velocity of note events"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
