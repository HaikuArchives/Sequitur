/* ArpPandemic.h
 * Copyright (c)2000 by Eric Hackborn
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
 * 08.14.00		hackborn
 * Created this file
 */

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-PANDEMIC-FILTER
 * This filter generates a control change with each note on.
 *****************************************************************************/
class ArpPandemicFilterAddOn;

class ArpPandemicFilter : public AmFilterI
{
public:
	ArpPandemicFilter(	ArpPandemicFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config);
	virtual ~ArpPandemicFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

private:
	ArpPandemicFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;
	uint8						mPrevControlValue;
	/* The control number that gets generated with each note on.
	 */
	uint8						mControlNumber;
	/* When generating a random control value, this is the range I'm
	 * confined to.
	 */
	uint8						mMinValue, mMaxValue;

	int32						mMultiplier;
	AmTime						mQuantize;
	int32						mEighths;
	int32						mDensity;

	uint8						RandomControlValue() const;
};

/*****************************************************************************
 * ARP-PANDEMIC-FILTER-ADD-ON
 *****************************************************************************/
class ArpPandemicFilterAddOn : public AmFilterAddOn
{
public:
	ArpPandemicFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Pandemic"; }
	virtual BString		Key() const						{ return "arp:Pandemic"; }
	virtual BString		ShortDescription() const		{ return "Generate a random control change with each note"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
