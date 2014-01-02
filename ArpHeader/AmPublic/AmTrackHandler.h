/* AmTrackHandler.h
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.18.00		hackborn
 * Created this file
 */
#ifndef AMPUBLIC_AMTRACKHANDLER_H
#define AMPUBLIC_AMTRACKHANDLER_H

#include <vector.h>
#include <app/Message.h>
#include <interface/View.h>
#include <ArpBuild.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmTimeConverter.h"

class AmPhrase;
class AmPhraseEvent;

enum {
	AM_NO_FUDGE			= 0,
	AM_FUDGE_TO_EDGE	= -1
};

/***************************************************************************
 * AM-TRACK-HANDLER
 * This utility class provides basic behaviour for dealing with the change
 * notifications sent out by tracks.  It's basic usefulness is to transform
 * one or more events into a rect that needs to be invalidated, which is
 * dealt with primarily through the MergeChange...() and DrawChange...()
 * methods.
 ***************************************************************************/
class AmTrackHandler
{
public:
	/* The default 0 is provided as a convenience in certain situations,
	 * but be aware that until you set a valid view and time converter
	 * this class will not function.
	 */
	AmTrackHandler(	const AmTimeConverter& mtc, BView* view = NULL,
					float leftFudge = AM_NO_FUDGE, float rightFudge = AM_NO_FUDGE);
	virtual ~AmTrackHandler();

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	BView*					View();
	/* When setting the view, you can also set the 'fugde' factor
	 * to the left and right.  This factor is used whenever the view
	 * is invalidated -- the invalidation rect is increased by the
	 * number of pixels supplied by the left and right fudge.  This
	 * is useful for clients who perform drawing to the left or right
	 * of their actual events.  For example, when notes draw sharps,
	 * the sharp is left of the actual note start time, but we want to
	 * make sure that we still draw the full sharp.  Both values should
	 * be positive -- i.e., a left fudge of 10 and right fudge of 3 will
	 * increase the invalidation area by 10 pixels left and 3 pixels right.
	 */
	void					SetView(BView* view, float leftFudge = AM_NO_FUDGE, float rightFudge = AM_NO_FUDGE);
	const AmTimeConverter&	TimeConverter() const;
	/* Subclasses can set a 'fudge,' which is how many pixels left and
	 * right to increase the typical invalidation bounds.  This is used
	 * when the size of an event and its display don't actually match up,
	 * like drawing sharps on a note.
	 */
	void					GetFudgeFactor(float* leftFudge, float* rightFudge) const;

	/* If the message is something I handle, then add its events to
	 * the change rect.  I handle messages that I have whats for and
	 * whose senders aren't me.  Answer true if I handled the message,
	 * false otherwise.
	 */
	bool HandleMessage(const BMessage* msg);

	/*---------------------------------------------------------
	 * EVENT INVALIDATION
	 * These methods are responsible for taking events and
	 * phrases and transforming them into a rectangle that needs
	 * to be invalidated.
	 *
	 * There are two variations of all events, one that takes
	 * actual AmEvent and AmPhrase objects, and one that takes
	 * BMessage flattened representations of them.  Having both
	 * systems is purely a performance consideration, so use
	 * whichever one is appropriate.
	 *
	 * Note that the default implementation of the AmEvent variants
	 * of IsInteresting() and RectFor() construct new AmEvent
	 * objects out of the BMessage, and then call the AmEvent
	 * variant.  A good subclass will override these methods,
	 * as well, and just operate directly on the flattened messages.
	 *---------------------------------------------------------*/
	/* These methods take events and transform them into a rect for drawing.
	 * Call either variant of MergeChange...() as much as you like until you
	 * are ready for all the changes to be drawn, then call either variant
	 * of DrawChange...().
	 */
	void			MergeChangeEvent(const AmEvent* event, const AmPhraseEvent& topPhrase);
	void			MergeChangePhrase(const AmPhrase* phrase, const AmPhraseEvent& topPhrase);
	void			DrawChangeEvent(const AmEvent* event, const AmPhraseEvent& topPhrase, bool drawImmediately = false);
	void			DrawChangePhrase(const AmPhrase* phrase, const AmPhraseEvent& topPhrase, bool drawImmediately = false);

	/* Implementors must answer true if the event is something they will
	 * display, false otherwise.  The event has been flattened in the
	 * format described for each event type in AmEventsI.h.
	 */
	virtual bool	IsInteresting(const AmEvent* event) const = 0;
	/* Implementors must answer with a rectangle for the event.
	 */
	virtual BRect	RectFor(const AmEvent* event, AmRange eventRange) const = 0;

protected:
	/* The view that will receive drawing commands.
	 */
	BView*					mView;
	const AmTimeConverter&	mMtc;

	virtual bool	HandleTrackMessage(const BMessage* msg);

private:
	/* Stores the rect that we build up of all the changes coming in.
	 */
	BRect					mChangeRect;
	float					mLeftFudge, mRightFudge;
	
	/* Cause any necessary drawing to be done.
	 */
	void FlushChangeRect(bool drawImmediately);
};

#endif
