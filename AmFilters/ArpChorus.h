/* ArpChorus.h
 * Copyright (c)2000 by Eric Hackborn
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
 * 09.03.00		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

/*****************************************************************************
 *
 *	ARP-CHORUS-FILTER and ARP-CHORUS-FILTER-ADDON CLASS
 *
 *	These classes are a simple filter to chorus note events.  By adjusting
 *  the time delay of the chorused events, this filter can also be used as
 *  an arpeggiator.
 *
 *****************************************************************************/

class ArpChorusFilterAddOn;

class ArpChorusFilter : public AmFilterI {
public:
	ArpChorusFilter(ArpChorusFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* settings);
	~ArpChorusFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& panels);

	enum {
		MAX_CHORUS		= 4
	};

private:
	ArpChorusFilterAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;

	int32						mOctaves[MAX_CHORUS];
	int32						mSteps[MAX_CHORUS];
	/* This is a scaled value from 1 to 200, where 1 equals 1 percent
	 * of the original note's velocity, 200 is 200 percent.  Ideally,
	 * this should be a float, but the int control used to display
	 * this value doesn't work with floats, so I'm stuck with this.
	 */
	int32						mVelocities[MAX_CHORUS];
};

class ArpChorusFilterAddOn : public AmFilterAddOn {
public:
	ArpChorusFilterAddOn(const void* cookie);
	
	virtual VersionType Version(void) const;
	virtual BString		Name() const;
	virtual BString		Key() const;
	virtual BString		ShortDescription() const;
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const;
	virtual BString		Email() const;
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const;
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
		
private:
	typedef AmFilterAddOn	inherited;
};
