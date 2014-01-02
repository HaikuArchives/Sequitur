/* MSequence.h
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
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 04.27.00		hackborn
 * Created this file
 */

#ifndef MDUST_MSEQUENCE_H
#define MDUST_MSEQUENCE_H


#include "MDust/MIndexedList.h"
#ifndef ARPMIDI_ARPMIDIEVENTS_H
#include "ArpMidi2/ArpMidiEvents.h"
#endif
#ifndef ARPMIDI_ARPMIDINODE_H
#include "ArpMidi2/ArpMidiNode.h"
#endif

#ifndef _SUPPORT_DEFS_H
#include <support/SupportDefs.h>
#endif

/***************************************************************************
 * M-SEQUENCE
 * A simple list for storing and ordering a sequence of MIDI events.  This
 * is just the ArpMidiList class, but with the locking and ghosts removed.
 ***************************************************************************/
class MSequence : public MIndexedList
{
public:
	MSequence(int32 chainIndexesArg = ARP_DEFAULT_CHAIN_INDEXES);
	virtual ~MSequence();

	// CHAIN ACCESSING
	status_t Head(ArpMidiEvent** event);
	status_t Tail(ArpMidiEvent** event);
		
	// CHAIN MANIPULATION
	status_t Add(ArpMidiEvent* eventToAdd);
	status_t Remove(ArpMidiEvent* eventToRemove);
	status_t RemoveHead(ArpMidiEvent** removedEvent);
	status_t RemoveTail(ArpMidiEvent** removedEvent);
	/* Remove the first event at the given time.
	 */
	status_t RemoveAt(ArpMidiT time, ArpMidiEvent** removedEvent);
	/* Remove all events from the supplied sequence and add them into
	 * myself.  The result is that the supplied sequence is completely
	 * empty.
	 */
	status_t Merge(MSequence* sequence);

	/* Use this method to change the time of an ArpMidiEvent.
	 * DO NOT call the SetTime() method implemented in the event
	 * if that event is in an ArpMidiList.
	 */
	status_t SetTime(ArpMidiEvent* event, ArpMidiT newTime);
};

#endif 
