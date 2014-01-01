/* ArpTrackStamp.h
 * Copyright (c)2002 by Eric Hackborn
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
 * 2002.04.28				hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

/*****************************************************************************
 * ARP-TRACK-STAMP-FILTER
 *****************************************************************************/
class ArpTrackStampAddOn;

class ArpTrackStampFilter : public AmFilterI
{
public:
	ArpTrackStampFilter(	ArpTrackStampAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* settings);
	~ArpTrackStampFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

private:
	ArpTrackStampAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;

};

class ArpTrackStampAddOn : public AmFilterAddOn {
public:
	ArpTrackStampAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Track Stamp"; }
	virtual BString		Key() const						{ return "arp:TrackStamp"; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpTrackStampFilter(this, holder, config); }
		
private:
	typedef AmFilterAddOn	inherited;
};
