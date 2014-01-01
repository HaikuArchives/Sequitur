/* ArpTranspose.h
 * Copyright (c)2000 by Dianne Hackborn
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

class ArpTransposeFilterAddOn;

class ArpTransposeFilter : public AmFilterI {
public:
	ArpTransposeFilter(ArpTransposeFilterAddOn* addon,
					 AmFilterHolderI* holder,
					 const BMessage* settings);
	~ArpTransposeFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);
	
	virtual status_t GetToolTipText(BString* out) const {
		if(HasLabel()) {
			*out << "(" << Label() << ") ";
		}
		*out << "Transpose: " << mOctaves << " Oct, " << mSteps << " Steps";
		return B_OK;
	}
	
private:
	ArpTransposeFilterAddOn*			mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mOctaves;
	int32						mSteps;
};

class ArpTransposeFilterAddOn : public AmFilterAddOn {
public:
	ArpTransposeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Transpose"; }
	virtual BString		Key() const						{ return "arp:Transpose"; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne Hackborn"; }
	virtual BString		Email() const					{ return "hackbod@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpTransposeFilter(this, holder, config); }
};
