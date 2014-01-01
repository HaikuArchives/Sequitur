/* ArpAccentRandomizer.h
 * Copyright (c) 2004 by Christian Packmann
 * Copyright (c)2000 by Dianne Hackborn
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 *
 * History
 * ~~~~~~~
 * 2004.02.10  CPackmann
 * 09.07.98		hackborn
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
 *	ARP-TRANSPOSE-FILTER and ARP-TRANSPOSE-FILTER-ADDON CLASS
 *
 *	These classes are a simple filter to shift note events up or down
 *	in pitch.
 *
 *****************************************************************************/

class ArpAccentRandomizerFilterAddOn;

class ArpAccentRandomizerFilter : public AmFilterI {
public:
	ArpAccentRandomizerFilter(ArpAccentRandomizerFilterAddOn* addon,
					 AmFilterHolderI* holder,
					 const BMessage* settings);
	~ArpAccentRandomizerFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);
	virtual void	 Start(uint32 context);
	
	virtual status_t GetToolTipText(BString* out) const {
		if(HasLabel()) {
			*out << "(" << Label() << ") ";
		}
		*out << "AccRandom +-" << mRandomizeAmount;
		return B_OK;
	}
	
private:
	ArpAccentRandomizerFilterAddOn*			mAddOn;
	AmFilterHolderI*			mHolder;
	int32 mSeed;
	int32 mRandomizeAmount;
};

class ArpAccentRandomizerFilterAddOn : public AmFilterAddOn {
public:
	ArpAccentRandomizerFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Accent Randomizer"; }
	virtual BString		Key() const						{ return "arp:AccentRandomizer"; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Christian Packmann"; }
	virtual BString		Email() const					{ return "Christian.Packmann@gmx.de"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpAccentRandomizerFilter(this, holder, config); }
};
