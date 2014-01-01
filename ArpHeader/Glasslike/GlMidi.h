/* GlMidi.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
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
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.01.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLMIDI_H
#define GLASSLIKE_GLMIDI_H

#include <be/midi2/MidiConsumer.h>
#include <be/app/Messenger.h>
#include <be/support/Locker.h>
#include <GlPublic/GlMidiEvent.h>
class GlEventConsumer;
class GlMidiProducerList;

/***************************************************************************
 * GL-MIDI
 ***************************************************************************/
class GlMidi
{
public:
	GlMidi();
	~GlMidi();

	/* Set target does a blind set, replacing what's there.  Unset
	 * will only cause a change if the argument is the same as the
	 * current target.
	 */
	void					SetTarget(const BMessenger& target);
	void					UnsetTarget(const BMessenger& target);

protected:
	friend class			GlEventConsumer;
	status_t				ReceiveMidi(GlMidiEvent event, bigtime_t time);

private:
	GlMidiProducerList*		mList;
	BMessenger				mTarget;

	mutable BLocker			mAccess;
};

#endif
