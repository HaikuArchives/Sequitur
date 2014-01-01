/* ArpUncertainChorus.h
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
 * 2001.05.16		hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include <be/support/String.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-UNCERTAIN-CHORUS-FILTER
 * This filter chorus all note events it finds, based on a combination
 * of the current Y value (i.e. this only works from a Tool) and a
 * random value.
 *****************************************************************************/
class ArpUncertainChorusAddOn;

class ArpUncertainChorusFilter : public AmFilterI
{
public:
	ArpUncertainChorusFilter(	ArpUncertainChorusAddOn* addon,
					 			AmFilterHolderI* holder,
								const BMessage* settings);
	virtual ~ArpUncertainChorusFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual void		Start(uint32 context);

private:
	ArpUncertainChorusAddOn*	mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mSeed;
	int32						mFrequency;
	enum {
		MIRROR_Y	= 0x00000001
	};
	uint32						mFlags;
	
	static const uint32			NOTE_SIZE = 128;
	bool						mNotes[NOTE_SIZE];

	bool						BuildTable(uint8 note, const am_tool_filter_params* toolParams);
};

/*****************************************************************************
 * ARP-UNCERTAIN-CHORUS-ADD-ON
 *****************************************************************************/
class ArpUncertainChorusAddOn : public AmFilterAddOn
{
public:
	ArpUncertainChorusAddOn(const void* cookie);
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Uncertain Chorus"; }
	virtual BString		Key() const						{ return "arp:UncertainChorus"; }
	virtual BString		ShortDescription() const		{ return "This Tool filter creates a chorus for all note events, based on the current Y mouse position and a random value"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const					{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpUncertainChorusFilter(this, holder, config); }
};
