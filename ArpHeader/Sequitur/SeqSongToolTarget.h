/* SeqSongToolTarget.h
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 06.29.00		hackborn
 * Created this file
 */
#ifndef SEQUITUR_SEQSONGTOOLTARGET_H
#define SEQUITUR_SEQSONGTOOLTARGET_H

#include <interface/View.h>
#include <ArpBuild.h>
#include "AmPublic/AmRange.h"
#include "AmPublic/AmTimeConverter.h"
class SeqSongSelections;
class SeqSongWinPropertiesI;
class AmPhraseEvent;
class AmTrack;

/***************************************************************************
 * SEQ-SONG-TOOL-TARGET
 * This utility class provides all the behaviour that tools editing arrange
 * views need to function.  This is analogous to the AmToolTarget, but
 * simplified, since arrange views only work on phrases, not on events.
 ***************************************************************************/
class SeqSongToolTarget
{
public:
	/* The default 0 is provided as a convenience in certain situations,
	 * but be aware that until you set a valid view and time converter
	 * this class will not function.
	 */
	SeqSongToolTarget(	SeqSongWinPropertiesI* win = NULL,
						BView* view = NULL,
						const AmTimeConverter* mtc = NULL);
	virtual ~SeqSongToolTarget();
	
	void					SetOverViewWindow(SeqSongWinPropertiesI* win);
	BView*					View() const;
	void					SetView(BView* view);
	const AmTimeConverter* TimeConverter() const;
	void					SetTimeConverter(const AmTimeConverter* mtc);

	/*---------------------------------------------------------
	 * PHRASE ACCESSING
	 *---------------------------------------------------------*/
	/* Answer the phrase event exactly at the point supplied.
	 */
	AmPhraseEvent*			PhraseEventAt(const AmTrack* track, BPoint where) const;

	/*---------------------------------------------------------
	 * SELECTION
	 *---------------------------------------------------------*/
	/* Clients should never delete the answered selections object.  If
	 * they want to clear the selections, they should send 0 to SetSelections().
	 */
	SeqSongSelections*		Selections() const;
	/* Clients should never get the selections, modify them, and do nothing
	 * else -- they should always call set if they've changed them.
	 */
	void					SetSelections(SeqSongSelections* selections);

	/*---------------------------------------------------------
	 * DRAWING
	 *---------------------------------------------------------*/
	virtual void			ShowDragMark(	AmTime time, BPoint where,
											const BMessage* dragMessage) = 0;
	virtual void			ClearDragMark() = 0;
	
protected:
	SeqSongWinPropertiesI*		mWin;
	BView*						mView;
	const AmTimeConverter*		mMtc;
};

#endif
