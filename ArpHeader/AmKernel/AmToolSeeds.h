/* AmToolSeeds.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.02.05			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMKERNEL_AMTOOLSEEDS_H
#define AMKERNEL_AMTOOLSEEDS_H

#include "AmPublic/AmToolSeedI.h"
#include "AmPublic/AmToolTarget.h"
class AmChangeEventUndo;
class AmTrack;
class AmTrackEventUndo;

/**********************************************************************
 * _AM-MOVE-ENTRY and _AM-TRACK-MOVE-ENTRY
 * These classes encapsulates the state of one entry in the selections.
 **********************************************************************/
class _AmMoveEntry
{
public:
	_AmMoveEntry();
	_AmMoveEntry(	AmEvent* event,
					AmPhraseEvent* container,
					AmToolTarget* target);
	_AmMoveEntry(const _AmMoveEntry& o);

	_AmMoveEntry&	operator=(const _AmMoveEntry& o);

	AmEvent*		mEvent;
	AmPhraseEvent*	mContainer;
	AmTime			mStart;
	int32			mY;
};

class _AmTrackMoveEntry
{
public:
	_AmTrackMoveEntry();
	_AmTrackMoveEntry(	AmSong* song,
						AmToolTarget* target,
						uint32 trackIndex,
						AmSelectionsI* selections);
	_AmTrackMoveEntry(const _AmTrackMoveEntry& o);
	~_AmTrackMoveEntry();
	
	_AmTrackMoveEntry&	operator=(const _AmTrackMoveEntry& o);

	bool					IsEmpty() const;
	bool					Move(	AmSong* song,
									AmToolTarget* target,
									AmTime timeDelta, int32 yDelta,
									bool change);
	void					PostMove(	AmSong* song,
										AmSelectionsI* oldSelections,
										AmSelectionsI* newSelections);
	void					Finished(AmSong* song, bool moveOccurred);

	track_id				mTrackId;
	/* I cache an undo operation that stores the changed state to any
	 * number of events.  If, when I'm done transforming, there are
	 * any actual changes, then I place this undo into the song.
	 */
	AmChangeEventUndo*		mUndoCache;
	std::vector<_AmMoveEntry>	mEntries;

private:
	void			MoveEvent(	AmTrack* track,
								AmToolTarget* target,
								_AmMoveEntry* entry,
								AmTime timeDelta,
								int32 yDelta);
};

/***************************************************************************
 * AM-MOVE-TOOL-SEED
 * Take all selected events and Move() them via the target.
 ***************************************************************************/
class AmMoveToolSeed : public AmToolSeedI
{
public:
	AmMoveToolSeed();
	AmMoveToolSeed(const AmMoveToolSeed& o);
	virtual ~AmMoveToolSeed();

	/*---------------------------------------------------------
	 * TOOL SEED INTERFACE
	 *---------------------------------------------------------*/
	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual void			DrawOn(BView* view, BRect clip)		{ }

	virtual AmToolSeedI*	Copy() const;

	/*---------------------------------------------------------
	 * SERVICE INTERFACE
	 *---------------------------------------------------------*/
	/* Always call this before any move is initiated (typically in
	 * the MouseDown() ).  ALWAYS CALL Finished() AFTER THE MOVE
	 * IS FINISHED.
	 */
	void 					Prepare(AmSong* song,
									AmToolTarget* target,
									AmSelectionsI* selections,
									BPoint origin,
									AmTime quantizeTime);
	bool					IsReady();
	AmSelectionsI*			Move(	AmSong* song,
									AmToolTarget* target,
									BPoint where);
	AmSelectionsI*			Move( 	AmSong* song,
									AmToolTarget* target,
									AmTime timeDelta, int32 yDelta);
	/* Always call this after any move has finished (typically in
	 * the MouseUp() ).
	 */
	void					Finished(AmSong* song);
	/* clients can set this to true if they don't want the Y axis to drag.
	 */
	void					SetDragTimeOnly(bool dragTimeOnly);
	
private:
	typedef AmToolSeedI		inherited;
	std::vector<_AmTrackMoveEntry> mEntries;
	/* The grid to quantize to is cached with each mouse down.
	 */
	AmTime					mGridCache;
	/* Use the point supplied to Prepare() as the origin -- it's a
	 * delta value that gets added to the current point and used to
	 * offset each event's original secondary values.
	 */
	BPoint					mOrigin;
	/* Take the point supplied in Prepare(), convert it to
	 * a MIDI time, and quantize it.  This is then used to
	 * offset all events during the move.
	 */
	AmTime					mTimeOrigin;

	/* Sort-of a hack to prevent us from adding an undo state if
	 * nothing happened.
	 */
	bool					mMoveOccurred;
	/* Set to true to suppress changes to the Y axis.
	 */
	bool					mDragTimeOnly;
	
	AmSelectionsI*	PostMove(AmSong* song, AmSelectionsI* oldSelections);

	AmSelectionsI*	MouseDown(	AmSong* song,
								AmToolTarget* target,
								BPoint where);
};

/***************************************************************************
 * AM-BOX-TOOL-SEED
 * Draw a box as the mouse drags and, on mouse up, answer all events in
 * the box.
 ***************************************************************************/
class AmBoxToolSeed : public AmToolSeedI
{
public:
	AmBoxToolSeed();
	virtual ~AmBoxToolSeed();

	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	/* I need to set the undo state for any events that have changed.
	 */
	virtual void			PostMouseUp(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);

	virtual void			DrawOn(BView* view, BRect clip);

	virtual AmToolSeedI*	Copy() const;

private:
	typedef AmToolSeedI		inherited;

	AmMoveToolSeed				mMoveSeed;
	BPoint						mOrigin;
	BPoint						mCorner;
	BList						mChanges;
	
	AmSelectionsI*	MouseDown(		AmSong* song, AmToolTarget* target,
									BPoint where);
	AmSelectionsI*	MouseUp(		AmSong* song, AmToolTarget* target,
									BPoint where);

	void			GetEvents(		AmToolTarget* target,
									const AmTrack& track,
									BRect rect,
									AmSelectionsI* selections) const;

	void			GetTopEvents(	AmToolTarget* target,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections) const;

	void			GetEvents(		AmToolTarget* target,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmPhraseEvent* phrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections) const;

	virtual void	ChangeEvent(AmTrack* track,
								AmPhraseEvent* topPhrase,
								AmEvent* event,
								uint32 flags);

	AmChangeEventUndo*	UndoForTrack(AmSong* song, track_id tid, bool create);
};

/***************************************************************************
 * AM-BEZIER-TOOL-SEED
 * Draw a line as the mouse drags and, on mouse up, transform all events
 * the line touches, then answer them.
 ***************************************************************************/
extern const char* AM_BEZIER_SEED_PT1START;
extern const char* AM_BEZIER_SEED_PT1END;
extern const char* AM_BEZIER_SEED_PT2START;
extern const char* AM_BEZIER_SEED_PT2END;
extern const char* AM_BEZIER_SEED_FRAME;
extern const char* AM_BEZIER_SEED_MODE;

class AmBezierToolSeed : public AmToolSeedI
{
public:
	AmBezierToolSeed();
	AmBezierToolSeed(const AmBezierToolSeed& o);
	virtual ~AmBezierToolSeed();

	virtual AmToolKeyHandler* NewToolKeyHandler() const;
	virtual void			KeyDown(	AmSongRef songRef,
										AmToolTarget* target, char byte);

	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	/* I need to set the undo state for any events that have changed.
	 */
	virtual void			PostMouseUp(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);

	virtual void			DrawOn(BView* view, BRect clip);

	virtual AmToolSeedI*	Copy() const;
	virtual	status_t		SetConfiguration(const BMessage* config);
	virtual	status_t		GetConfiguration(BMessage* config) const;

	enum {
		CREATE_MODE			= 0x00000001,
		MOVE_MODE			= 0x00000002,
		TRANFORM_MODE		= 0x00000004		
	};

private:
	typedef AmToolSeedI		inherited;
	uint32						mMode;
	
	BPoint						mOrigin;
	BPoint						mPt1, mPt2;
	BPoint						mCorner;
	BList						mChanges;

	float						mFrame, mFrameStep;		// Frame 0 - 1
	BPoint						mPt1Frame0, mPt1Frame1,
								mPt2Frame0, mPt2Frame1;

	virtual void	DrawStraightOn(BView* view, BRect clip);
	virtual void	DrawBezierOn(BView* view, BRect clip);
	
	AmSelectionsI*	MouseUp(		AmSong* song, AmToolTarget* target,
									BPoint where);

	void			CreateEvents(	AmTrackWinPropertiesI& props, AmSong* song,
									AmToolTarget* target, AmTime time, float y,
									AmSelectionsI* selections);

	void			GetEvents(		AmToolTarget* target,
									AmTrack& track,
									BRect rect,
									AmSelectionsI* selections,
									AmChangeEventUndo* undo,
									const am_trans_params& params);

	void			GetTopEvents(	AmToolTarget* target,
									AmTrack& track,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections,
									AmChangeEventUndo* undo,
									const am_trans_params& params);

	void			GetEvents(		AmToolTarget* target,
									AmTrack& track,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmPhraseEvent* phrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections,
									AmChangeEventUndo* undo,
									const am_trans_params& params);

	virtual void	ChangeEvent(AmTrack* track,
								AmPhraseEvent* topPhrase,
								AmEvent* event,
								uint32 flags);

	AmChangeEventUndo*	UndoForTrack(AmSong* song, track_id tid);

	void			SetInteriorPoints();
};

/***************************************************************************
 * AM-TOUCH-TOOL-SEED
 * Answer with any event the mouse touches, as soon as it touches it.
 ***************************************************************************/
class AmTouchToolSeed : public AmToolSeedI
{
public:
	AmTouchToolSeed();
	virtual ~AmTouchToolSeed();

	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual void			DrawOn(BView* view, BRect clip)		{ }

	virtual AmToolSeedI*	Copy() const;
	virtual	status_t		SetConfiguration(const BMessage* config);
	virtual	status_t		GetConfiguration(BMessage* config) const;

private:
	typedef AmToolSeedI		inherited;

	AmSelectionsI*			FindSelections(	const AmSong* song,
											AmToolTarget* target,
											BPoint where);
};

/**********************************************************************
 * _AM-TRANS-ENTRY and _AM-TRACK-TRANS-ENTRY
 * This class encapsulates the state of one entry in the selections.
 **********************************************************************/
class _AmTransEntry
{
public:
	_AmTransEntry();
	_AmTransEntry(	AmPhraseEvent* topPhrase,
					AmEvent* event,
					AmToolTarget* target,
					int32 extraData);
	_AmTransEntry(const _AmTransEntry& o);

	_AmTransEntry&	operator=(const _AmTransEntry& o);

	AmPhraseEvent*	mTopPhrase;
	AmEvent*		mEvent;
	am_trans_params	mParams;
};

class _AmTrackTransEntry
{
public:
	_AmTrackTransEntry();
	_AmTrackTransEntry(	AmSong* song,
						AmToolTarget* target,
						uint32 trackIndex,
						AmSelectionsI* selections);
	/* This is the constructor used only for the TransformOneByOneEvent() method.
	 */
	_AmTrackTransEntry(	AmTrack* track, AmPhraseEvent* container,
						AmEvent* event, int32 extraData,
						AmToolTarget* target, BPoint where);
	_AmTrackTransEntry(const _AmTrackTransEntry& o);
	
	track_id				mTrackId;
	std::vector<_AmTransEntry>	mEntries;
	/* I cache an undo operation that stores the changed state to any
	 * number of events.  If, when I'm done transforming, there are
	 * any actual changes, then I place this undo into the song.
	 */
	AmChangeEventUndo*		mUndo;

	_AmTrackTransEntry&	operator=(const _AmTrackTransEntry& o);

	bool					IsEmpty() const;
	bool					Transform(	AmSong* song,
										AmToolTarget* target,
										int32 xDelta, int32 yDelta,
										uint32 targetFlags,
										bool change);
	void					TransformOneByOneEvent(	AmTrack* track, AmPhraseEvent* container,
													AmEvent* event, int32 extraData,
													AmToolTarget* target, BPoint where);
	void					Finished(AmSong* song);

private:
	/* If the event is a phrase event, then what we actually want to
	 * prepare is every event in the phrase.
	 */
	void				PrepareEvent(	AmPhraseEvent* topPhrase,
										AmEvent* event,
										AmToolTarget* target,
										int32 extraData);
	uint32				TransformEvent(	AmTrack* track,
										AmToolTarget* target,
										_AmTransEntry* entry,
										uint32 targetFlags);
};

/***************************************************************************
 * AM-TRANSFORM-TOOL-SEED
 * Take all the my target events and call Transform() on them.
 ***************************************************************************/
class AmTransformToolSeed : public AmToolSeedI
{
public:
	AmTransformToolSeed();
	AmTransformToolSeed(const AmTransformToolSeed& o);
	virtual ~AmTransformToolSeed();

	/*---------------------------------------------------------
	 * TOOL SEED INTERFACE
	 *---------------------------------------------------------*/
	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual void			DrawOn(BView* view, BRect clip)		{ }

	virtual AmToolSeedI*	Copy() const;
	virtual	status_t		SetConfiguration(const BMessage* config);
	virtual	status_t		GetConfiguration(BMessage* config) const;

	/*---------------------------------------------------------
	 * SERVICE INTERFACE
	 *---------------------------------------------------------*/
	/* Always call this before any transform is initiated (typically in
	 * the MouseDown() ).  ALWAYS CALL Finished() AFTER THE
	 * TRANSFORM IS FINISHED.
	 */
	void Prepare(	AmSong* song,
					AmToolTarget* target,
					AmSelectionsI* selections,
					BPoint origin,
					AmTime quantizeTime);
	bool IsReady();
	void Transform(	AmSong* song,
					AmToolTarget* target,
					BPoint where);
	void Transform( AmSong* song,
					AmToolTarget* target,
					int32 xDelta, int32 yDelta);
	/* Always call this after any transform has finished (typically in
	 * the MouseUp() ).  The song can be NULL, but if it is, the undo
	 * state won't get added.
	 */
	void Finished(AmSong* song);

	/* These are for mFlags.  They can't overlap with anything defined
	 * for the flags variable in AmToolSeedI.h.
	 */
	enum {
		ONE_BY_ONE_FLAG		= 0x00010000,
		EN_MASSE_FLAG		= 0x00020000
	};

private:
	typedef AmToolSeedI		inherited;
	/* This stores the AmToolTarget flags, like TRANSFORM_X.
	 */
	uint32					mTargetFlags;
	/* The grid to quantize to is cached with each mouse down.
	 */
	AmTime					mGridCache;
	std::vector<_AmTrackTransEntry> mEntries;
	/* Use the point supplied to Prepare() as the origin -- it's a
	 * delta value that gets added to the current point and used to
	 * offset each event's original secondary values.
	 */
	BPoint					mOrigin;
	/* The mAxisChangeOrigin gets set to the current point whenever
	 * the user does a motion that's larger on the axis that's not currently
	 * being tracked than the one that is.  If the user goes beyond the
	 * tolerance on the other axis, suddenly the axes flip.
	 */
	BPoint					mAxisChangeOrigin;
	BPoint					mLastPoint;

	AmSelectionsI*			MouseDown(	AmSong* song,
										AmToolTarget* target,
										BPoint where);

	/* Set the flag that determines which axis the user is moving on.
	 */
	void SetAxisFlag(const AmToolTarget* target, BPoint where);
	
	/* This is a special method for the en masse transformation of events.
	 * When doing this kind of transformation, the transform happens on
	 * any events the mouse touches, whether or not they're selected.  This
	 * means that the events need to be added to the undo operation, since
	 * they weren't known about in the Prepare() method.
	 */
	void	TransformOneByOneEvent(AmSong* song, AmToolTarget* target, BPoint where);
};

/***************************************************************************
 * _AM-TRACK-REFILTER-ENTRY
 ***************************************************************************/
class _AmTrackRefilterEntry
{
public:
	_AmTrackRefilterEntry();
	_AmTrackRefilterEntry(	AmSong* song, AmToolTarget* target,
							AmSelectionsI* selections, uint32 trackIndex);
	_AmTrackRefilterEntry(AmSong* song, track_id tid);
	_AmTrackRefilterEntry(const _AmTrackRefilterEntry& o);
	
	track_id					mTrackId;
	AmChangeEventUndo*			mUndo;
	std::vector<_AmMoveEntry>		mEntries;
	std::vector<AmTrackEventUndo*>	mUndoList;

	_AmTrackRefilterEntry&	operator=(const _AmTrackRefilterEntry& o);

	bool					IsEmpty() const;

	void					UndoEverything(AmSong* song);
	void					PostMove(AmSong* song, AmSelectionsI* newSelections);
	void					Finished(AmSong* song);

	void					AddPhrase(AmTrack* track, AmPhraseEvent* pe);
	void					AddEvent(	AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event);
	void					DeleteEvent(AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event);

	void					Print(uint32 tabs = 0) const;
};

/***************************************************************************
 * AM-REFILTER-TOOL-SEED
 * Take all selected events and, well, do nothing.  With each mouse move,
 * restore them to their original state.
 ***************************************************************************/
class AmCreateToolSeed;

class AmRefilterToolSeed : public AmToolSeedI
{
public:
	AmRefilterToolSeed();
	AmRefilterToolSeed(const AmRefilterToolSeed& o);
	virtual ~AmRefilterToolSeed();

	/*---------------------------------------------------------
	 * TOOL SEED INTERFACE
	 *---------------------------------------------------------*/
	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual void			PostMouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual void			DrawOn(BView* view, BRect clip)		{ }

	virtual AmToolSeedI*	Copy() const;

	/*---------------------------------------------------------
	 * SERVICE INTERFACE
	 *---------------------------------------------------------*/
	/* Always call this before any move is initiated (typically in
	 * the MouseDown() ).  ALWAYS CALL Finished() AFTER THE MOVE
	 * IS FINISHED.
	 */
	void 					Prepare(AmSong* song,
									AmToolTarget* target,
									AmSelectionsI* selections,
									BPoint origin,
									AmTime quantizeTime);
	bool					IsReady();
	AmSelectionsI*			Move(	AmSong* song,
									AmToolTarget* target);
	/* Always call this after any move has finished (typically in
	 * the MouseUp() ).
	 */
	void					Finished(AmSong* song);

	enum {
	 	// If true, Undo() before every refilter.
	 	RESTORE_FLAG		= 0x00010000
	};

protected:
	friend class			AmCreateToolSeed;
	
	virtual void			AddPhrase(AmTrack* track, AmPhraseEvent* pe, uint32 flags);
	virtual void			AddEvent(	AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event, uint32 flags);
	virtual void			DeleteEvent(AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event, uint32 flags);

	/* The grid to quantize to is cached with each mouse down.
	 */
	AmTime					mGridCache;
	/* This seed works by basically undo-ing any changes that the pipeline
	 * is creating, which means of course that I need to cache all those
	 * changes.
	 */
	std::vector<_AmTrackRefilterEntry> mEntries;

	AmSelectionsI*	PostMove(AmSong* song, AmSelectionsI* oldSelections);
	AmSelectionsI*	MouseDown(	AmSong* song,
								AmToolTarget* target,
								BPoint where);

private:
	typedef AmToolSeedI		inherited;
	/* Make sure the track is in my list of entries.  If it isn't, add it.
	 */
	void					ValidateTrackEntry(AmTrack* track);
};

/**********************************************************************
 * AM-CREATE-TOOL-SEED
 * This seed places new events on mouse down.  Its behaviour on mouse
 * move varies depending upon its current properties setting -- it will
 * either place new events, move the first event, transform the first
 * event, or stop altogether.
 **********************************************************************/
class _CreateAndFilterEntry
{
public:
	track_id		mTrackId;
	AmPhraseEvent*	mTopPhrase;
	AmEvent*		mEvent;

	_CreateAndFilterEntry();
	_CreateAndFilterEntry(const _CreateAndFilterEntry& o);
	_CreateAndFilterEntry(track_id tid, AmPhraseEvent* topPhrase, AmEvent* event);
	virtual ~_CreateAndFilterEntry();

	_CreateAndFilterEntry& operator=(const _CreateAndFilterEntry& o);

	void			AddToSelections(AmSelectionsI* selections) const;
};


class AmCreateToolSeed : public AmToolSeedI
{
public:
	AmCreateToolSeed();
	AmCreateToolSeed(const AmCreateToolSeed& o);
	virtual ~AmCreateToolSeed();

	/*---------------------------------------------------------
	 * TOOL SEED INTERFACE
	 *---------------------------------------------------------*/
	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code);
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where);

	virtual void			PostMouseDown(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);
	virtual void			PostMouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);

	virtual bool			NeedsProcessHack() const;

	virtual void			DrawOn(BView* view, BRect clip)		{ }

	virtual AmToolSeedI*	Copy() const;

	/* These are for mFlags.  They can't overlap with anything defined
	 * for the flags variable in AmToolSeedI.h.
	 */
	enum {
	 	// After adding the event, move all selected events.
	 	MOVE_FLAG			= 0x00010000,
	 	// Add events for as longs as the user is pressing the mouse button.
	 	ADDN_FLAG			= 0x00020000,
	 	// After adding the event, transform all selected events.
		TRANSFORM_FLAG		= 0x00040000,
	 	// After adding the event, refilter all selected events.
		REFILTER_FLAG		= 0x00080000
	};
	
protected:
	/* This does the actual work of the MouseDown() function.  The public
	 * method is just a shell to guarantee the AmTrack gets deleted.
	 */
	AmSelectionsI*			MouseDown(	AmSong* song,
										AmToolTarget* target,
										BPoint where);
	/* This does the actual work of the MouseMoved() function.  The public
	 * method is just a shell to guarantee the AmTrack gets deleted.
	 */
	AmSelectionsI*			MouseMoved(	AmSong* song,
										AmToolTarget* target,
										BPoint where);

	virtual void			AddPhrase(AmTrack* track, AmPhraseEvent* pe, uint32 flags);
	virtual void			AddEvent(	AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event, uint32 flags);
	virtual void			DeleteEvent(AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event, uint32 flags);

private:
	typedef AmToolSeedI		inherited;
	/* These provide the services for moving and transforming events,
	 * which the create tool makes use of depending upon what its tool
	 * target tells it.
	 */
	AmMoveToolSeed			mMoveSeed;
	/* This is a little hack -- the mouse down knows whether or not the
	 * current selections are going to be moved, but they are actually
	 * prepared for moving in the post mouse down.
	 */
	bool					mForceMoveHack;
	AmTransformToolSeed		mTransformSeed;
	AmRefilterToolSeed		mRefilterSeed;
	/* This is the duration of notes the user is currently placing,
	 * which I use as a quantize time.
	 */
	AmTime					mGridCache;
	/* This happens when the user clicks on an existing event.
	 */
	AmSelectionsI*	MouseDownOnEvent(	AmSong* song,
										AmToolTarget* target,
										BPoint where);
	
	/* Create a new event and add it to the selections.  Answer the
	 * selections.
	 */
	AmSelectionsI*	CreateEvents(	const AmSong* song,
									AmToolTarget* target,
									BPoint where);

	AmSelectionsI*					NewCreatedEvents() const;
	std::vector<_CreateAndFilterEntry>	mCreateEntries;
};

#endif
