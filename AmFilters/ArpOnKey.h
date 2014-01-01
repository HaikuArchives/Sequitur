/* AmOnKeyFilter.h
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
 * ARP-ON-KEY
 * This filter performs a mapping between notes.  It presents users with a
 * complete scale and allows them to choose which notes in the scale are
 * active.  Additionally, it provides convenience macros for setting all
 * the keys to a selected root and scale (like C Major).
 *****************************************************************************/
class ArpOnKeyFilterAddOn;

class ArpOnKeyFilter : public AmFilterI
{
public:
	ArpOnKeyFilter(	ArpOnKeyFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* config);
	virtual ~ArpOnKeyFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual BView*		NewEditView(BPoint requestedSize) const;
	virtual status_t	GetToolTipText(BString* out) const;
	
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);

	/* Wrong notes strategy
	 */
	enum {
		DISCARD			= 0,
		SHIFT_DOWN		= 1,
		SHIFT_UP		= 2,
		SHIFT_CLOSEST	= 3
	};
	
private:
	typedef AmFilterI		inherited;
	ArpOnKeyFilterAddOn*	mAddOn;
	AmFilterHolderI*		mHolder;

	/* This array stores which notes are on or off for the entire octave.
	 */
	bool					mKey[12];
	int32					mWrongNotes;
	/* The root and scale are just convenience macros for selecting a
	 * particular key.
	 */
	int32					mRoot;
	int32					mScale;
	/* Answer true if the supplied note is my current scale scale.
	 */
	bool					NoteResult(uint8 oldNote, uint8* newNote);
	int32					DistanceToNewNote(bool scale[], uint8 note, int32 step);
};

/*****************************************************************************
 * AM-ON-KEY-FILTER-ADDON
 *****************************************************************************/
class ArpOnKeyFilterAddOn : public AmFilterAddOn
{
public:
	ArpOnKeyFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "On Key"; }
	virtual BString		Key() const						{ return "arp:On Key"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return "Only allow notes in the selected scale"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
