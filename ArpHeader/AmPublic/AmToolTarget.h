/* AmToolTarget.h
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
#ifndef AMPUBLIC_AMTOOLTARGET_H
#define AMPUBLIC_AMTOOLTARGET_H

#include <vector.h>
#include <interface/View.h>
#include <ArpBuild.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmTrackHandler.h"
class AmPhraseEvent;
class AmSelectionsI;
class AmSong;
class AmTool;
class AmTrack;
class AmTrackWinPropertiesI;

struct am_trans_params
{
	size_t					size;					// amount of data in structure
	uint32					flags;					// transform flags
	int32					original_x, original_y;
	int32					delta_x, delta_y;
	int32					extra_data;				// data that has been supplied by the ToolTarget
	am_trans_params();
	am_trans_params(const am_trans_params& o);
	~am_trans_params();
	am_trans_params& operator=(const am_trans_params& o);

private:
	bool operator==(const am_trans_params& o) const;
	bool operator!=(const am_trans_params& o) const;
};

/* Return flags for the SetTransform() methods */
enum {
	AM_TRANS_NO_PLAY		= 0x00000001
};

/***************************************************************************
 * AM-TOOL-TARGET
 * This utility class provides all the behaviour that tools need to
 * function.  This plus the AmTrack is essentially the API that tools have
 * available to them.
 ***************************************************************************/
class AmToolTarget : public AmTrackHandler
{
public:
	/* The default 0 is provided as a convenience in certain situations,
	 * but be aware that until you set a valid view and time converter
	 * this class will not function.
	 */
	AmToolTarget(	AmTrackWinPropertiesI& trackWinProps,
					BView* view = 0,
					float leftFudge = AM_NO_FUDGE, float rightFudge = AM_NO_FUDGE);
	virtual ~AmToolTarget();

	AmTrackWinPropertiesI&	TrackWinProperties() const;
	
	enum {
		NO_CONTAINER		= 0x00000001,	// If set, this tells tool clients
											// that this target keeps its events
											// stored directly in the track's
											// phrase list.
		DRAG_TIME_ONLY		= 0x00000002	// This is a special flag.  In certain
											// situations, the tool model makes a
											// distinction between 'moving' and 'dragging'.
											// From the targets point of view, a drag is
											// a move on both the X and Y axis.  However,
											// sometimes the user wants to drag without
											// affecting the Y axis, for example when
											// dragging control changes.  This flag will
											// cause that mode to happen where desireable.
	};
	virtual uint32	Flags() const;
	
	/*---------------------------------------------------------
	 * EVENT ACCESSING
	 *---------------------------------------------------------*/
	/* Find whatever event lies at the supplied point.  Answer that
	 * event, if any, as well as the top level phrase that contains
	 * the event.  If no event is returned, then the AmPhraseEvent
	 * answer is not valid.
	 */
	AmEvent*		EventAt(const AmTrack* track,
							BPoint where,
							AmPhraseEvent** topPhraseAnswer,
							int32* extraData);
	AmEvent*		EventAt(const AmTrack* track,
							AmTime time,
							float y,
							AmPhraseEvent** topPhraseAnswer,
							int32* extraData);
	/* By default zero, subclasses can return a fudge time that increases
	 * the bounds of the EventAt() search.  This is useful for single-pixel
	 * events (like control changes) that want to give the user a little
	 * extra selection room.
	 */
	virtual AmTime	EventAtFudge() const;
	/* Answer the phrase event nearest to the supplied time.  There
	 * is a default tolerance within which the phrase must fall --
	 * if none of them do, then answer 0.
	 */
	AmPhraseEvent*	PhraseEventNear( const AmTrack* track, AmTime time );
	/* Subclasses must answer with whatever interesting event they found
	 * at the given location.
	 */
	virtual AmEvent* InterestingEventAt(const AmTrack* track,
										const AmPhraseEvent& topPhrase,
										const AmPhrase& phrase,
										AmTime time,
										float y,
										int32* extraData) const = 0;
	/* Answer true if the event intersects the MIDI data rectangle
	 * specified.  The left and right are MIDI time positions, the top
	 * and bottom are YValues for this target.  By default, answer with
	 * true if the event intersects the left and right (i.e. time) coordinates.
	 * Subclasses can augment to determine if it intersects with the top and bottom.
	 */
	virtual bool EventIntersects(	const AmEvent* event, AmRange eventRange,
									AmTime left, int32 top, AmTime right, int32 bottom) const;

	/* Subclasses must return a new MIDI event of whatever data they hold.
	 * The mouse point has already been translated, so AmTime is the time of
	 * the new event, and float is whatever data goes in the y value for
	 * the particular subclass.  The track is provided in case anyone needs it.
	 */
	virtual AmEvent* NewEvent(const AmTrack& track, AmTime time, float y) = 0;

	/*---------------------------------------------------------
	 * MOVING
	 * The term 'y' value is used to describe what changes in
	 * the event as it is dragged on the y axis.  Not all
	 * subclasses will actually do something with the y value --
	 * many will use the transform y value, which is just
	 * something ELSE that happens to the event on the y axis.
	 *---------------------------------------------------------*/
	/* Subclasses must answer whatever y value resides at the supplied
	 * pixel.
	 */
	virtual int32	MoveYValueFromPixel(float y) const = 0;
	virtual void	GetMoveValues(	const AmPhraseEvent& topPhrase, const AmEvent* event,
									AmTime* x, int32* y) const = 0;
	virtual void	GetMoveDelta(	BPoint origin, BPoint where,
									AmTime* xDelta, int32* yDelta) const = 0;
	virtual void	SetMove(AmPhraseEvent& topPhrase,
							AmEvent* event,
							AmTime originalX, int32 originalY,
							AmTime deltaX, int32 deltaY,
							uint32 flags) = 0;

	/*---------------------------------------------------------
	 * TRANSFORMATION
	 * These methods define a second type of edit that subclasses
	 * might want to perform.  These edits are utilized by the
	 * transform tool.  Ironically, unlike the primary edits,
	 * most every class should implement transform edits.  If
	 * they don't, then they basically have no data other than
	 * time that changes, and that's just weird.
	 *---------------------------------------------------------*/
	/* Subclasses must implement to fill the originalX and
	 * originalY values with whatever data in the event will be
	 * transformed on the X and Y axes.
	 */
	virtual void GetOriginalTransform(	AmEvent* event,
										am_trans_params& params) const = 0;
	/* Subclasses must implement to fill the transformation deltaX
	 * and deltaY based on the distance traveled.
	 */
	virtual void GetDeltaTransform(	BPoint origin, BPoint where,
									am_trans_params& params) const = 0;
	enum {
		TRANSFORM_X			= 0x00000001,
		TRANSFORM_Y			= 0x00000002,
		TRANSFORM_BOTH		= 0x00000003	// The transform tool defaults to a transform
											// of both axes.  As soon as the user starts
											// dragging the mouse, it sets itself to whatever
											// axis the user is moving along.  This allows
											// programmatic changes, which don't set the axis,
											// to occur along both the x and y.
	};
	/* The top level phrase that holds the event is included for any implementors
	 * that need it.
	 */
	virtual uint32 SetTransform(const AmTrack& track,
								AmPhraseEvent& topPhrase,
								AmEvent* event,
								const am_trans_params& params) = 0;
	virtual uint32 SetTransform(const AmTrack& track,
								AmPhraseEvent& topPhrase,
								AmEvent* event,
								BPoint where,
								const am_trans_params& params) = 0;

	/* Subclasses should implement to actually perform the selected events.
	 * Events are performed automatically in certain situations, like
	 * when they are moved.
	 */
	virtual void Perform(const AmSong* song, const AmSelectionsI* selections); 
	/* Obviously, this only applies if the target can actually perform.  But
	 * if it can, subclasses should take this opportunity to call Stop() on
	 * their performer.
	 */
	virtual void Stop();
	/* A very late hack addition -- when pasting, in certain situations the
	 * data should change.  For example, users should be able to copy CC
	 * data from the volume view and paste it to the pan view.  This allows that.
	 */
	virtual void PrepareForPaste(AmEvent* event);
	
private:
	typedef AmTrackHandler		inherited;
	AmTrackWinPropertiesI&		mTrackWinProps;
	
	AmEvent*		RecurseEventAt(	const AmTrack* track,
									const AmPhraseEvent& topPhrase,
									const AmPhraseEvent& phrase,
									AmTime time, float y,
									int32* extraData) const;
};

#endif
