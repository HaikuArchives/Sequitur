/* AmNoteRenderer.h
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
 *	- Drawing is not as efficient as it could be -- drawing ghost events and
 * drawing normal events are chunked into two completely separate functions.
 * To speed it up, we could only lock the list we're drawing from once, plus
 * we could actually make use of the event being returned by DrawGhostEvents()
 * and use that as the starting point for the DrawEvents() function.
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.03.05		hackborn@angryredplanet.com
 * Created this file from the old AmTrackArrangeView.
 */

#ifndef AMSTDFACTORY_AMNOTERENDERER_H
#define AMSTDFACTORY_AMNOTERENDERER_H

#include <vector>
#include <interface/View.h>
#include "ArpKernel/ArpLineArrayCache.h"
#include "AmPublic/AmPhraseRendererI.h"
#include "AmPublic/AmTimeConverter.h"
class AmNode;
class AmNoteOn;
class AmPhraseEvent;
class AmTrack;

/*************************************************************************
 * _FLOAT-RANGE
 *************************************************************************/
class _FloatRange
{
public:
	_FloatRange();
	float			start, end;
	bool			IsValid() const;
	void			MakeInvalid();
	bool			Touches(const _FloatRange& r) const;
	_FloatRange&	operator=(const _FloatRange& r);
	_FloatRange&	operator+=(const _FloatRange& r);
};
typedef std::vector<_FloatRange>		floatrange_vec;

/*************************************************************************
 * AM-NOTE-RENDERER
 *************************************************************************/
class AmNoteRenderer : public AmPhraseRendererI
{	
public:
	AmNoteRenderer(const AmTimeConverter& mtc);
	virtual	~AmNoteRenderer();
	
	virtual void	BeginTrack(	BRect clip,
								BView* view,
								const AmTrack* track,
								ArpLineArrayCache& lineCache);
	virtual void	DrawPhrase(	BRect clip,
								BView* view,
								const AmTrack* track,
								const AmPhraseEvent* phrase,
								AmTime end,
								AmPhraseEvent* topPhrase,
								ArpLineArrayCache& lineCache);
	virtual void	EndTrack(	BRect clip,
								BView* view,
								const AmTrack* track,
								ArpLineArrayCache& lineCache);

	virtual AmPhraseRendererI*	Copy() const;

private:
	const AmTimeConverter&	mMtc;
	floatrange_vec			mRanges;
	/* This is used during drawing, so I don't have to construct
	 * the range object each time I draw an event.
	 */
	_FloatRange				mTmpRange;
	/* Store an array that maps notes to pixels for performance.
	 */
	float					mNoteTable[128];

	void	DrawEvents(	BRect clip,
						BView* view,
						const AmTrack* track,
						AmNode* n,
						AmTime end,
						AmPhraseEvent* topPhrase,
						ArpLineArrayCache& lineCache);
	void	DrawEvent(	BRect clip,
						BView* view,
						const AmNoteOn* noteOn,
						AmRange eventRange,
						ArpLineArrayCache& lineCache);

	/* Cache the height of the current note table (i.e., how many
	 * pixels used to draw the notes) so I only have to recompute my note
	 * table when the height changes.
	 */
	float	mNoteTableHeight;
	void	GenerateNoteTable(float newHeight);
};


#endif 
