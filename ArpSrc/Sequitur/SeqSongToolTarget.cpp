/* SeqSongToolTarget.cpp
*/
#define _BUILDING_AmKernel 1

#include "Sequitur/SeqSongToolTarget.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <interface/Window.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "Sequitur/SeqSongSelections.h"

/***************************************************************************
 * SEQ-SONG-TOOL-TARGET
 ****************************************************************************/
SeqSongToolTarget::SeqSongToolTarget(SeqSongWinPropertiesI* win, BView* view, const AmTimeConverter* mtc)
		: mWin(win), mView(view), mMtc(mtc)
{
}

SeqSongToolTarget::~SeqSongToolTarget()
{
}

void SeqSongToolTarget::SetOverViewWindow(SeqSongWinPropertiesI* win)
{
	mWin = win;
}

BView* SeqSongToolTarget::View() const
{
	return mView;
}

void SeqSongToolTarget::SetView(BView* view)
{
	mView = view;
}

const AmTimeConverter* SeqSongToolTarget::TimeConverter() const
{
	return mMtc;
}

void SeqSongToolTarget::SetTimeConverter(const AmTimeConverter* mtc)
{
	mMtc = mtc;
}

AmPhraseEvent* SeqSongToolTarget::PhraseEventAt(const AmTrack* track, BPoint where) const
{
	if( !mMtc ) return 0;
	return AmGlobals().PhraseEventNear( track, mMtc->PixelToTick( where.x ), 0 );
}

SeqSongSelections* SeqSongToolTarget::Selections() const
{
	if( !mWin ) return 0;
	return mWin->Selections();
}

void SeqSongToolTarget::SetSelections(SeqSongSelections* selections)
{
	if( !mWin ) return;
	mWin->SetSelections( selections );
}
