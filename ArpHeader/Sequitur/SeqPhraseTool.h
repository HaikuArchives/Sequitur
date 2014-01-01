/* SeqPhraseTool.h
 * Copyright (c)2000-2001 by Eric Hackborn.
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
 * 2001.03.05		hackborn@angryredplanet.com
 * Extracted from AmArrangeTool.
 */

#ifndef SEQUITUR_SEQPHRASETOOL_H
#define SEQUITUR_SEQPHRASETOOL_H

#include <be/app/MessageRunner.h>
#include <be/support/Locker.h>
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTimeConverter.h"
#include "AmPublic/AmTrackRef.h"
class AmPhraseEvent;
class SeqSongSelections;
class SeqSongToolTarget;

/*************************************************************************
 * SEQ-PHRASE-TOOL
 *************************************************************************/
class SeqPhraseTool
{	
public:
	SeqPhraseTool(AmSongRef songRef, const AmTimeConverter& mtc);
	virtual	~SeqPhraseTool();
		
	float			ScrollDelta() const;
	void			SetScrollDelta(float delta);

	virtual	void MouseDown(	SeqSongToolTarget* target,
							BPoint where);
	virtual	void MouseMoved(SeqSongToolTarget* target,
							BPoint where,
							uint32 code,
							const BMessage* dragMessage);
	virtual	void MouseUp(	SeqSongToolTarget* target,
							BPoint where);
	/* If this is a message I caused, then I should handle it
	 * and return true.  Otherwise return false.
	 */
	virtual bool HandleMessage(	const BMessage* msg,
								track_id trackId,
								SeqSongToolTarget* target);

private:
	AmSongRef	mSongRef;
	const AmTimeConverter&	mMtc;
	/* Set to the button mask while dragging.  If 0, then no buttons
	 * are pressed, and mouse moved should do nothing.
	 */
	int32		mButtons;
	/* The point pressed in the mouse down.
	 */
	BPoint		mOriginalPoint;
	/* Set to true essentially when the mouse moves after a mouse
	 * down -- i.e., a drag gesture is recognized.
	 */
	bool		mIsDrag;
	bool		mExited;
	
	void		CreateLink(	track_id trackId,
							AmRange range, int32 sourceTrack,
							vector<uint32>& tracks,
							AmTime dropTime);
	/* This is called when the user has not initiated a drag, but
	 * has just clicked and released on a specific point.
	 */
	bool		PointSelected(	SeqSongToolTarget* target,
								BPoint where,
								bool mergeOnShift = true);
	/* Answer true if something was actually selected, false
	 * otherwise.
	 */
	bool		PhraseSelected(	const AmTrack* track,
								SeqSongToolTarget* target,
								AmPhraseEvent* pe,
								SeqSongSelections** outSelections,
								bool mergeOnShift = true);

	void		MergeRange(AmPhraseEvent* pe, SeqSongSelections* selections) const;
	void		MergeTrack(const AmTrack* track, SeqSongSelections* selections) const;

	void		InitiateDrag(const SeqSongToolTarget* target, BPoint where);
	void		Drag(	SeqSongToolTarget* target, BPoint where,
						uint32 code, const BMessage* dragMessage);
	void		PerformDrop(SeqSongToolTarget* target,
							AmSong* srcSong,
							AmSong* destSong,
							AmRange range,
							vector<uint32>& tracks,
							int32 trackDelta,
							AmTime dropTime,
							AmTime oldMeasureStart,
							bool copy);
	uint32		DroppedMenu(BPoint where, bool enableLink) const;
	const AmTrack*	Track(const AmSong* song, BPoint where) const;
	
	/* This delivers messages as long as the mouse button is held down,
	 * providing me the opportunity to scroll if the user moves out of bounds.
	 */
	BMessageRunner*			mScrollRunner;
	void					HandleScrollMsg(const SeqSongToolTarget* target, const BMessage* msg);
	/* This is the current number of pixels the pointer is out of bounds,
	 * used to determine if the view should scroll.
	 */
	float					mScrollDelta;
};

#endif 
