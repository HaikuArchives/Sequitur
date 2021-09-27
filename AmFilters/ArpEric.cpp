/* ArpEric.cpp
 */
#include "ArpEric.h"

#include <cstdio>
#include <cstdlib>
#include <experimental/ResourceSet.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmSongObserver.h"

static AmStaticResources gRes;

static const char*		FOLLOW_X_STR	= "Follow X";

/*****************************************************************************
 * ARP-ERIC-FILTER
 * The filter implementation.  You don't need to change any of the methods
 * except HandleEvent().
 *****************************************************************************/
ArpEricFilter::ArpEricFilter(	ArpEricFilterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
}

ArpEricFilter::~ArpEricFilter()
{
}

/* Here's where all the work is done.  This method is given a single event
 * and needs to answer with 0 to n events.  Here, briefly, are the rules:
 *		1.  Multiple events are answered by chaining them together using
 * the AppendEvent() method.  YOU are responsible for making sure the events
 * are sorted by time -- that is, the event that is actually answered should
 * have the lowest Time() value of all the events chained together.  When you
 * append an event
 *				event1->AppendEvent( event2 );
 * event2 should always have a lower time value.
 *
 *		2,  If the event passed in as the argument is not part of the list
 * returned, then you are responsible for deleting it:
 *				event->Delete();
 *
 *		3.  For any new events called, you need to set their filter holder,
 * which is the same one this class stores in its mHolder variable:
 *				AmNoteOn*		new_event = new AmNoteOn( 64, 127, PPQN );
 * 				new_event->SetNextFilter( mHolder );
 *
 * And now, let's write a filter...
 */
AmEvent* ArpEricFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	/* This is an error condition -- if for some reason someone has passed
	 * an invalid event to me, just return it.
	 */
	if( !event ) return event;
	/* Here's some code to guarantee that this filter object has been setup
	 * correctly.
	 */
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	/* Now, everything from here down you are required to implement as desired...
	 */	

	/* Let's do a simple filter that takes the event supplied, and, if it's
	 * a note event, creates a new event one quarter note behind it at half the
	 * velocity.  Essentially, this is a simple echo.
	 */

	/* First, if the event I'm being supplied isn't a note, don't do anything with it,
	 */
	if( event->Type() != event->NOTEON_TYPE ) return event;
	/* Otherwise, get the actual note event so I can manipulate it.
	 */
	AmNoteOn* note = dynamic_cast<AmNoteOn*>( event );
	if( !note ) return event;
	
	/* Create the echoed note event, with the same note value as the current note,
	 * half the velocity, and a start time that is one quarter note (in 4/4 time) later
	 * then the note that came in.  PPQN stands for pulses per quarter note.
	 */
	AmEvent*	echoEv = event->Copy();
	AmNoteOn*	echo = dynamic_cast<AmNoteOn*>( echoEv );
	echoEv->SetStartTime( event->StartTime() + PPQN );
	echo->SetVelocity( note->Velocity() / 2 );
	/* The constructor for the AmNoteOn doesn't let us set the duration for the note,
	 * it uses a default, so here I make sure that the new note has the same duration
	 * as the one that came into the filter.
	 */
	echo->SetDuration( note->Duration() );
	/* Now hook the echoed note up with the current note.
	 */
	event->AppendEvent( echoEv );
	/* Finally, return the head of the chain of events I've created, which is just
	 * the event that was passed in.
	 */
	return event;
}
/* One final note:  If you're curious to see what types of MIDI events are available,
 * they're in the file ARP/ArpHeader/AmKernel/AmEvents.h.
 */

status_t ArpEricFilter::GetConfiguration(BMessage* values) const
{
	status_t	err = inherited::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err = values->AddBool(FOLLOW_X_STR, mFlags&FOLLOW_X)) != B_OK) return err;
	return B_OK;
}

status_t ArpEricFilter::PutConfiguration(const BMessage* values)
{
	status_t	err = inherited::PutConfiguration(values);
	if (err != B_OK) return err;
	bool		b;
	if (values->FindBool(FOLLOW_X_STR, &b) == B_OK) {
		if (b) mFlags |= FOLLOW_X;
		else mFlags &= ~FOLLOW_X;
	}
	return B_OK;
}

status_t ArpEricFilter::GetProperties(BMessage* properties) const
{
	status_t	err = inherited::GetProperties(properties);
	if (err != B_OK) return err;
	if ((err = properties->AddBool(FOLLOW_X_STR, true)) != B_OK) return err;
	return B_OK;
}

/*****************************************************************************
 * ARP-ERIC-FILTER-ADDON
 *****************************************************************************/
void ArpEricFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I am a template for writing your own C++ filters.  The source \n"
	"code can be found in the <i>DevKit/ExampleFilters/</i> folder.</P>\n";
}

void ArpEricFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpEricFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpEricFilterAddOn::NewInstance(	AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpEricFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpEricFilterAddOn(cookie);
	return NULL;
}
