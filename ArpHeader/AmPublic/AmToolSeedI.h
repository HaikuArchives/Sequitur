/* AmToolSeed.h
 * Copyright (c)2001 by Angry Red Planet and Eric Hackborn.
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
 *	- None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.02.05			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMPUBLIC_AMTOOLSEEDI_H
#define AMPUBLIC_AMTOOLSEEDI_H

#include <interface/Point.h>
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmToolRef.h"
#include "AmPublic/AmTrackRef.h"
class AmPhraseEvent;
class AmSelectionsI;
class AmToolKeyHandler;
class AmToolTarget;
class BView;

enum {
	/* Sent by the property view of a tool seed to its
	 * window whenever a property changes.
	 */
	AM_TOOL_SEED_CHANGED_MSG		= 'Atsc'
};

/*************************************************************************
 * AM-TOOL-SEED-I
 * This class defines the framework for objects that translate user
 * gestures into events to be processed.
 *************************************************************************/
class AmToolSeedI
{
public:
	virtual ~AmToolSeedI();

	// --------------------------------------------------------
	// STATIC METHODS
	// --------------------------------------------------------
	/* From 0 to the number of seeds, fill in name with a user-friendly
	 * label and key with the unique tool seed name, and answer
	 * with B_OK.  Answer with B_ERROR if index is too high.
	 */
	static status_t			GetSeedInfo(uint32 index,
										BString& outLabel,
										BString& outKey);
	static status_t			GetSeedInfo(const BString& key,
										BString& outLabel);
	static AmToolSeedI*		NewSeed(const BString& key);
	/* Answer a new property view for the supplied tool seed class.
	 */
	static BView*			NewView(const BString& key);
	static status_t			ConfigureView(	BView* view,
											AmToolRef toolRef,
											const BString& factoryKey,
											const BString& viewKey,
											const BString& seedKey);

	// --------------------------------------------------------
	// AM-TOOL-SEED-I INTERFACE
	// --------------------------------------------------------
	/* Subclasses will make use of the flags variable as well.
	 * Subclass flag values start at 0x00010000.
	 */
	enum {
		AM_SCROLL_X_FLAG	= 0x00000001,
		AM_SCROLL_Y_FLAG	= 0x00000002,
		AM_SCROLL_FLAG		= 0x00000003
	};
	uint32					Flags() const;

	/* Subclasses can optionally handle key presses.  They must
	 * return an instance of a class that registers which key presses
	 * they will handle.
	 */
	virtual AmToolKeyHandler* NewToolKeyHandler() const;
	virtual void			KeyDown(	AmSongRef songRef,
										AmToolTarget* target, char byte);
	/* Subclasses override these messages to deal with mouse notifications
	 * coming from a view.
	 */
	virtual AmSelectionsI*	MouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where) = 0;
	virtual AmSelectionsI*	MouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where,
										uint32 code) = 0;
	virtual AmSelectionsI*	MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where) = 0;

	virtual void			PostMouseDown(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);
	virtual void			PostMouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);
	virtual void			PostMouseUp(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where);
	
	/* ARGH!  A stupid hack because there are certain situations where
	 * a seed will return selections, but not want them processed.
	 * This is called before processing to give the seed a chance
	 * to veto the processing -- by default it returns true.
	 */
	virtual bool			NeedsProcessHack() const;

	enum {
		NO_PROCESSING_FLAG		= 0x00000001
	};
	void					ProcessEvent(	AmTrack* track,
											AmPhraseEvent* topPhrase,
											AmEvent* event,
											uint32 flags);
	/* Subclasses can draw on the current view, if they like.  The
	 * drawing currently happens after all the background has been
	 * drawn and before the events are drawn.
	 */
	virtual void			DrawOn(BView* view, BRect clip) = 0;

	virtual AmToolSeedI*	Copy() const = 0;

	/* Tool seeds can have properties.
	 */
	virtual	status_t		SetConfiguration(const BMessage* config);
	virtual	status_t		GetConfiguration(BMessage* config) const;

protected:
	uint32		mFlags;

	virtual void			AddPhrase(	AmTrack* track,
										AmPhraseEvent* pe,
										uint32 flags);
	virtual void			AddEvent(	AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event,
										uint32 flags);
	virtual void			DeleteEvent(AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event,
										uint32 flags);
	/* This is a default that gets called if none of the others are
	 * during the process event.  It does nothing.
	 */
	virtual void			ChangeEvent(AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event,
										uint32 flags);
};

#endif 

