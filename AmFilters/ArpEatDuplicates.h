/* ArpEatDuplicates.h
 * Copyright (c)2001 by Eric Hackborn
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
 * 2001.05.14				hackborn@angryredplanet.com
 * Created this file
 */
#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-EAT-DUPLICATES-FILTER
 * This filter deletes all duplicate events.
 *****************************************************************************/
class ArpEatDuplicatesFilterAddOn;

class ArpEatDuplicatesFilter : public AmFilterI
{
public:
	ArpEatDuplicatesFilter(	ArpEatDuplicatesFilterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config);
	virtual ~ArpEatDuplicatesFilter();
	
	virtual status_t	PutConfiguration(const BMessage* values);
	
	virtual AmEvent*	StartSection(	AmTime firstTime, AmTime lastTime,
									  const am_filter_params* params = NULL);
	virtual void		Start(uint32 context);
	virtual AmEvent*	HandleEvent(AmEvent* event,
									const am_filter_params* params = NULL);
	virtual AmEvent* 	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);
	
private:
	ArpEatDuplicatesFilterAddOn*	mAddOn;
	AmFilterHolderI*				mHolder;

	static const uint32				NOTE_SIZE = 128;
	static const uint32				CONTROL_SIZE = 128;

	BLocker							mLock;
	AmNoteOn*						mNotes[NOTE_SIZE];
	AmRange							mNoteTimes[NOTE_SIZE];
	AmControlChange*				mControls[CONTROL_SIZE];
	int32							mControlValues[CONTROL_SIZE];
	int32							mPitch;
	int32							mChannelPressure;

	AmEvent*						EatIt(AmEvent* event);	
	void							Init(bool first = false);
};

/*****************************************************************************
 * ARP-EAT-DUPLICATES-FILTER-ADD-ON
 *****************************************************************************/
class ArpEatDuplicatesFilterAddOn : public AmFilterAddOn
{
public:
	ArpEatDuplicatesFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Eat Duplicates"; }
	virtual BString		Key() const						{ return "arp:EatDuplicates"; }
	virtual BString		ShortDescription() const		{ return "Delete all duplicate events"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
