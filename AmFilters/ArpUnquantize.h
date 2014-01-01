/* ArpEcho.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * An echo filter.
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
 *	• None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.07.98		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

class ArpUnquantizeFilterAddOn;

class ArpUnquantizeFilter : public AmFilterI {
public:
	ArpUnquantizeFilter(ArpUnquantizeFilterAddOn* addon,
					 AmFilterHolderI* holder,
					 const BMessage* settings);
	~ArpUnquantizeFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

private:
	ArpUnquantizeFilterAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	
	// Configuration
	bool						mDrifting;

	int32						mAmtMult;
	AmTime						mAmtQuant;
	int32						mAmtEighths;
	int32						mStepMult;
	AmTime						mStepQuant;
	int32						mStepEighths;
	int32						mRecoveryMult;
	AmTime						mRecoveryQuant;
	int32						mRecoveryEighths;

	// State
	AmTime						mLastTime;
	AmTime						mDrift;
};

class ArpUnquantizeFilterAddOn : public AmFilterAddOn {
public:
	ArpUnquantizeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Unquantize"; }
	virtual BString		Key() const						{ return "arp:Unquantize"; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne Hackborn"; }
	virtual BString		Email() const					{ return "hackbod@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpUnquantizeFilter(this, holder, config); }
};
