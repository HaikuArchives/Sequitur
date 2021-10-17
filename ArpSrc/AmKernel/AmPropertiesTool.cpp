/* AmPropertiesTool.cpp
 */
#include <cstdio>
#include <vector>
#include <app/Clipboard.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmDeleteService.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmPropertiesTool.h"
#include "AmKernel/AmSong.h"

/*************************************************************************
 * AM-PROPERTIES-TOOL
 *************************************************************************/
AmPropertiesTool::AmPropertiesTool()
		: inherited("Properties", "arp:Properties", "Properties",
					NULL, NULL, true),
		  mMenu(0), mQuantizeTime(PPQN)
{
	mMenu = NewMenu();

	const BBitmap*		bm = ImageManager().FindBitmap("Property Tool");
	if (bm) {
		if (mIcon) delete mIcon;
		mIcon = new BBitmap(bm);
	}
}

AmPropertiesTool::~AmPropertiesTool()
{
	delete mMenu;
}

bool AmPropertiesTool::IsImmutable() const
{
	return true;
}

BView* AmPropertiesTool::NewPropertiesView() const
{
	return 0;
}

void AmPropertiesTool::MouseDown(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where)
{
	ArpASSERT(target);
	// WRITE TRACK BLOCK
	AmSong*		song = songRef.WriteLock();
	if (song) MouseDown(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmPropertiesTool::MouseUp(	AmSongRef songRef,
								AmToolTarget* target,
								BPoint where)
{
}

void AmPropertiesTool::MouseMoved(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where,
									uint32 code)
{
}

bool AmPropertiesTool::CanPaste()
{
	bool		canPaste = false;
	BClipboard*	clipboard = AmGlobals().Clipboard();
	if (clipboard && clipboard->Lock() ) {
		if (clipboard->Data() ) {
			canPaste = clipboard->Data()->HasMessage("sequitur_temp");
		}
		clipboard->Unlock();
	}
	return canPaste;
}

void AmPropertiesTool::Cut(AmSong* song, AmSelectionsI* selections, BMessenger sender)
{
	ArpASSERT(song && selections);
	Copy(selections);
	AmDeleteService			service;
	service.Prepare(song);
	uint32					trackCount = selections->CountTracks();
	for (uint32 ti = 0; ti < trackCount; ti++) {
		AmTrack*			track = NULL;
		track_id			tid;
		AmPhraseEvent*		topPhrase;
		AmEvent*			event;
		for (uint32 ei = 0; selections->EventAt(ti, ei, &tid, &topPhrase, &event) == B_OK; ei++) {
			if (!track) track = song->Track(tid);
			if (track) service.Delete(track, event, topPhrase);
		}
	}
	service.Finished(song, "Cut ");
}

#if 0
	void				Cut(	AmTrack* track,
								AmSelectionsI* selections,
								BMessenger sender);

void AmPropertiesTool::Cut(AmTrack* track, AmSelectionsI* selections, BMessenger sender)
{
	ArpASSERT(track && selections);
	Copy(selections);
	AmPhraseEvent*	topPhrase;
	AmEvent*		event;
	AmDeleteService	service;
	service.Prepare(track);
	for (uint32 k = 0; selections->EventAt(k, &topPhrase, &event) == B_OK; k++) {
		service.Delete(track, event, topPhrase);
	}
	service.Finished(track->Song(), "Cut ");
}
#endif

void AmPropertiesTool::Copy(AmSelectionsI* selections)
{
	ArpASSERT(selections);
	BClipboard*		clipboard = AmGlobals().Clipboard();
	if (!clipboard) return;

	BMessage		tracksMsg('trks');
	uint32			trackCount = selections->CountTracks();
	for (uint32 ti = 0; ti < trackCount; ti++) {
		AmPhrase*		phrase = new AmPhrase();
		if (phrase) {
			track_id		tid;
			AmPhraseEvent*	topPhrase;
			AmEvent*		event;
			for (uint32 ei = 0; selections->EventAt(ti, ei, &tid, &topPhrase, &event) == B_OK; ei++) {
				AmEvent*	copy = event->Copy();
				if (copy) phrase->Add(copy);
			}
			if (phrase->IsEmpty() ) delete phrase;
			else tracksMsg.AddPointer("sequitur_track", phrase);
		}
	}
	
	if (clipboard->Lock() ) {
		clipboard->Clear();
		BMessage*	clip = clipboard->Data();
		if (clip && !tracksMsg.IsEmpty() ) {
			clip->AddMessage("sequitur_temp", &tracksMsg);
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}

#if 0
void AmPropertiesTool::Copy(AmSelectionsI* selections)
{
	ArpASSERT(selections);
	BClipboard*		clipboard = AmGlobals().Clipboard();
	if (!clipboard) return;

	AmPhrase*		phrase = new AmPhrase();
	if (!phrase) return;
	AmEvent*		event;
	AmPhraseEvent*	topPhrase;
	
	for (uint32 k = 0; selections->EventAt(k, &topPhrase, &event) == B_OK; k++) {
		AmEvent*	copy = event->Copy();
		if (!copy) {
			delete phrase;
			return;
		}
		phrase->Add(copy);
	}
	
	if( clipboard->Lock() ) {
		clipboard->Clear();
		BMessage*	clip = clipboard->Data();
		if( clip ) {
			clip->AddPointer("sequitur_temp", phrase);
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}
#endif

status_t AmPropertiesTool::Paste(	AmSong* song,
									AmTrackWinPropertiesI& trackWinProps,
									AmTime atTime,
									AmSelectionsI* selections,
									BMessenger sender,
									AmToolTarget* target)
{
	ArpASSERT(song);
	AmTime			endTime = song->CountEndTime();
	mQuantizeTime = trackWinProps.GridTime();
	AmTime			newTime = atTime - (atTime % mQuantizeTime);
	BClipboard*		clipboard = AmGlobals().Clipboard();
	if (!clipboard) return B_NO_MEMORY;
	if (clipboard->Lock() ) {
		BMessage*	data = clipboard->Data();
		BMessage	tracksMsg;
		if (data && data->FindMessage("sequitur_temp", &tracksMsg) == B_OK) {
			AmPhrase*	phrase;
			for (int32 ti = 0; tracksMsg.FindPointer("sequitur_track", ti, (void**)&phrase) == B_OK; ti++) {
				AmTrack*	track = song->Track(trackWinProps.OrderedTrackAt(ti));
				if (track) {
					AmPhraseEvent*	pe = AmGlobals().PhraseEventNear(track, newTime);
					if (!pe) {
						pe = new AmRootPhraseEvent();
						if (pe) track->AddEvent(NULL, pe);
					}
					if (pe) {
						AmNode*		n = phrase->HeadNode();
						if (n) {
							AmTime	delta = newTime - n->StartTime();
							while (n) {
								AmEvent*	event;
								if (n->Event() && (event = n->Event()->Copy()) ) {
									event->SetStartTime(pe->EventRange(event).start + delta);
//									pe->SetEventStartTime(event, event->StartTime() + delta);
									if (target) target->PrepareForPaste(event);
									track->AddEvent(pe->Phrase(), event);
									if (selections) selections->AddEvent(track->Id(), pe, event);
								}
								n = n->next;
							}
						}
					}
				}
			}
		}
		clipboard->Unlock();
	}
	if (song->UndoContext() ) {
		song->UndoContext()->SetUndoName("Paste Events");
		song->UndoContext()->CommitState();
	}
	if (endTime != song->CountEndTime() ) song->EndTimeChangeNotice();

	return B_OK;
}

static AmTime new_event_time(float xPixel, const AmToolTarget* target, AmTime quantize)
{
	ArpASSERT(target);
	const AmTimeConverter&	mtc = target->TimeConverter();
	return mtc.PixelToTick(xPixel) - (mtc.PixelToTick(xPixel) % quantize);
}

void AmPropertiesTool::MouseDown(AmSong* song, AmToolTarget* target, BPoint where)
{
	ArpASSERT(target && target->View() );
	if (!mMenu) return;
	AmTrackWinPropertiesI&		props = target->TrackWinProperties();
	mQuantizeTime = props.GridTime();
	/* Select any events at the mouse point.
	 */
	AmSelectionsI*				selections = props.Selections();
	if (!selections || !modifiers()&B_SHIFT_KEY) selections = AmSelectionsI::NewSelections();
	if (selections) {
		track_id		tid;
		for (uint32 ti = 0; (tid = props.OrderedTrackAt(ti).TrackId()) != 0; ti++) {
			AmTrack*	track = song->Track(tid);
			if (track) {
				AmPhraseEvent*	topPhrase;
				int32			extraData;
				AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
				if (!event) event = target->EventAt(track, new_event_time(where.x, target, mQuantizeTime), where.y, &topPhrase, &extraData);
				if (event) {
					if (!selections->IncludesEvent(tid, topPhrase, event) )
						selections->AddEvent(tid, topPhrase, event, extraData);
				}
			}
		}
		props.SetSelections(selections);
	}
	/* Enable / disable my menu as appropriate.
	 */
	AmTrack*	track = song->Track(props.OrderedTrackAt(0) );
	if (!track) return;
	bool		canCopy = CanCopy(track, target, where);
	bool		canPaste = CanPaste();
	BMenuItem*	item;
	for (int32 k = 0; (item = mMenu->ItemAt(k)); k++ ) {
		if (item && item->Message() ) {
			if (item->Message()->what == B_CUT) {
			 	item->SetEnabled(canCopy);
			} else if (item->Message()->what == B_COPY ) {
				item->SetEnabled(canCopy);
			} else if (item->Message()->what == B_PASTE ) {
				item->SetEnabled(canPaste);
			}
		}
	}
	BRect		r(where, where);
	item = mMenu->Go(	target->View()->ConvertToScreen(where), false, false,
						target->View()->ConvertToScreen(r) );
	if (!item || !item->Message() ) return;
	if (item->Message()->what == B_CUT) {
		if( props.Selections() ) Cut(song, props.Selections(), BMessenger( target->View() ) );
		props.SetSelections(NULL);
	} else if (item->Message()->what == B_COPY) {
		if (props.Selections() ) Copy(props.Selections() );
	} else if (item->Message()->what == B_PASTE) {
		AmSelectionsI*	selections = AmSelectionsI::NewSelections();
		Paste(song, props, target->TimeConverter().PixelToTick(where.x), selections, BMessenger(target->View() ), target);
		props.SetSelections(selections);
	}
}

bool AmPropertiesTool::CanCopy(const AmTrack* track, AmToolTarget* target, BPoint where)
{
	ArpASSERT(target && target->View() );
	AmSelectionsI*	selections = target->TrackWinProperties().Selections();
	if (!selections) return false;
	/* Find an event where the mouse was clicked -- either exactly at the point at
	 * which the mouse was clicked, or, if that fails, the nearest quantized event.
	 */
	AmPhraseEvent*	topPhrase;
	int32			extraData;
	AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
	if (!event) {
		const AmTimeConverter&	mtc = target->TimeConverter();
		AmTime		newTime = mtc.PixelToTick(where.x) - ( mtc.PixelToTick(where.x) % mQuantizeTime );
		event = target->EventAt(track, newTime, where.y, &topPhrase, &extraData);
	}
	if (!event) return false;
	return selections->IncludesEvent(track->Id(), topPhrase, event);
}

BPopUpMenu* AmPropertiesTool::NewMenu() const
{
	BPopUpMenu*		menu = new BPopUpMenu( "menu" );
	if( !menu ) return 0;
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont( be_plain_font );

	BMenuItem*		item = new BMenuItem( "Cut", new BMessage(B_CUT) );
	if( item ) menu->AddItem( item );
	item = new BMenuItem( "Copy", new BMessage(B_COPY) );
	if( item ) menu->AddItem( item );
	item = new BMenuItem( "Paste", new BMessage(B_PASTE) );
	if( item ) menu->AddItem( item );

	return menu;
}
