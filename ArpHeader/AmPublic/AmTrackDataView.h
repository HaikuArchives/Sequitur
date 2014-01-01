/* AmTrackDataView.h
 * Copyright (c)1998-2000 by Eric Hackborn.
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
 * Jan 23, 1999		hackborn
 * Changed GetEvents() so that it will flip the start and the end if the
 * rect passed in caused the end to be before the start.
 *
 * Jan 16, 1999		hackborn
 * Removed the CreateEvent() func.  That behaviour now appears shared
 * between the new NewEventStrategy() func and NewEvent().
 *
 * 11.11.98			hackborn
 * Mutated this file from the original SeqTrackDataView
 */


#ifndef AMPUBLIC_AMTRACKDATAVIEW_H
#define AMPUBLIC_AMTRACKDATAVIEW_H

#include <be/app/Message.h>
#include <be/app/MessageRunner.h>
#include <be/interface/View.h>
#include "AmPublic/AmTimeConverter.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmToolRef.h"
class ArpBackground;
class AmGraphicEffect;
class AmPhraseEvent;
class AmSelectionsI;
class AmSignature;
class AmTool;
class AmToolControlList;
class AmToolKeyHandler;
class AmToolTarget;
class AmTrackWinPropertiesI;
class AmViewPropertyI;

enum {
	/* The window containing the track data view understands this as
	 * a command to scroll the view.
	 */
	AM_SCROLL_MSG			= 'Ascr'
		// "x"				A float that scrolls on the x axis by the supplied amount
		// "pri y"			A float that scrolls the primary view on the y axis
		// "sec y"			A float that scrolls the secondary hrz manager view on the y axis
};

/*************************************************************************
 * AM-TRACK-DATA-VIEW
 * This class is the abstract base class for all views that present
 * editable MIDI data.
 *
 * Implementors will probably want to handle to following messages:
 *		AM_PRIMARY_TRACK_MSG.  Sent when the user selects a different
 * primary track -- i.e., a different track for editing.  If the view
 * caches the primary track info (available from
 * mTrackWinProps.OrderedTrackAt(0) ), then it should recache it at this
 * time.  Also, just about every view will probably want to invalidate
 * itself.
 *************************************************************************/
class AmTrackDataView : public BView
{
public:
	AmTrackDataView(AmSongRef songRef,
					AmTrackWinPropertiesI& trackWinProps,
					const AmViewPropertyI& viewProp,
					TrackViewType viewType,
					BRect frame,
					const char *name,
					uint32 resizeMask,
					uint32 flags);
	virtual ~AmTrackDataView();

	virtual void		AttachedToWindow();
	virtual void		Draw(BRect clip);
	virtual	void		GetPreferredSize(float *width, float *height);
	virtual	void		KeyDown(const char *bytes, int32 numBytes);
	virtual	void		KeyUp(const char *bytes, int32 numBytes);
	virtual void		MessageReceived(BMessage* msg);
	virtual	void		MouseDown(BPoint where);
	virtual	void		MouseUp(BPoint where);
	virtual	void		MouseMoved(	BPoint where,
									uint32 code,
									const BMessage *a_message);

	void				DrawSongPosition(AmTime time, bool redraw = true);

	/* Subclasses should answer with a BMessage containing any parameters
	 * that they want to be persistant.  Note that subclasses do not own
	 * the BMessage they answer -- it will be deleted by someone else.
	 * By default this method returns NULL.
	 */
	virtual BMessage*	ConfigurationData();
	BString				FactorySignature()	 const;
	BString				ViewName() const;

protected:
	AmSongRef				mSongRef;
	AmTrackWinPropertiesI&	mTrackWinProps;
	/* This is stored in the track win props, but pulled out as a convenience,
	 * since it gets accessed so often.
	 */
	const AmTimeConverter&	mMtc;
	/* Subclasses are responsible for filling this with an
	 * appropriate target.
	 */
	AmToolTarget*			mTarget;
	/* If the user is operating (ie, one of the mouse buttons is depressed),
	 * then activeTool will be whatever tool they are using.
	 */
	AmToolRef				mActiveTool;

	/* These values are all information that my subclasses can determine
	 * based on asking either the mTrackWinProps are the topLevelPhrase,
	 * but they are cached here because they are accessed so often while
	 * drawing.
	 */
	/* Saturation is the transparencey with which ordered and shadow
	 * events should be drawn -- 0, don't draw them, 1 completely opaque.
	 */
	float					mOrderedSaturation;
	float					mShadowSaturation;
	rgb_color				mEventColor;
	rgb_color				EventColor(uint8 velocity = 127);

	rgb_color				mSecondaryBase;
	/* I store the name of the factory that created me, along with the
	 * name that is sent to that factory to instantiate me.  This should be
	 * sent in by the population message.
	 */
	BString					mFactorySignature;
	BString					mViewName;
	TrackViewType			mViewType;
	
	ArpBackground*			mHeadBackground;
	/* Subclasses can plug into drawing on the background by
	 * supplying background objects.  This method adds the
	 * background as the tail of any existing backgrounds.
	 */
	void					AddBackground(ArpBackground* background);

	/*---------------------------------------------------------
	 * DRAWING
	 *---------------------------------------------------------*/
	virtual void DrawOn(BRect clip, BView* view);
	/* Subclasses are given the option to do any necessary processing
	 * before and after drawing any events.  These hook methods are empty.
	 */
	virtual void PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	/* These are the various flags that might be supplied to the
	 * DrawEvent() method.
	 */
	enum EventFlags {
		ARPEVENT_PRIMARY		= 0x00000001,	// Draw the event as a primary event
		ARPEVENT_ORDERED		= 0x00000002,	// Draw the event as an ordered event
		ARPEVENT_SHADOW			= 0x00000004,	// Draw the event as a shadow event
		ARPEVENT_SELECTED		= 0x00000008,	// Draw the event selected
		ARPEVENT_HIGHLIGHTED	= 0x00000010	// Draw the event highlighted
	};
	/* The drawing methods happen as a result of Draw().  They
	 * all happen within one read transaction of AmTrack.
	 */
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song);
	virtual void	DrawOrderedEvents(BRect clip, BView* view, const AmSong* song);
	virtual void	DrawPrimaryEvents(BRect clip, BView* view, const AmTrack* track);
	virtual void	DrawTrack(	const AmTrack* track,
								BRect clip,
								BView* view,
								int32 properties,
								AmSelectionsI* selections = NULL);
	virtual void	DrawPhrase(	BRect clip,
								BView* view,
								track_id trackId,
								const AmPhraseEvent& topPhrase,
								AmPhraseEvent* event,
								AmTime start,
								AmTime end,
								int32 properties,
								AmSelectionsI* selections = 0);
	/* Subclasses must implement to actually draw the event.
	 */
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties) = 0;

	/* The cleanup code after a mouse interaction is encapsulated here since it
	 * needs to be called both on mouse up and if the mouse moved with no button.
	 */
	void			MouseCleanup(BPoint where);
	
private:
	typedef BView			inherited;
	/* This delivers messages as long as the mouse button is held down,
	 * providing me the opportunity to scroll if the user moves out of bounds.
	 */
	BMessageRunner*			mScrollRunner;
	int32					mFlags;
	float					mSongPosition;
	AmToolControlList*		mToolControls;
	AmGraphicEffect*		mToolGraphic;
	AmToolKeyHandler*		mToolKeys;
	rgb_color				mLowEventColor;
	/* The last point from a MouseMoved().
	 */
	BPoint					mLastPt;
	/* Everytime MouseDown() occurs, the current graphic (if any) gets
	 * placed on the stack until it finishes up.  Once finished, it is
	 * deleted and gone forever.
	 */
	vector<AmGraphicEffect*> mPrevToolGraphics;
	
	void		HandleScrollMsg(const BMessage* msg);
	bool		IsOrdered(track_id tid) const;
	
	/* The code for editing events programmatically.
	 */
	void		DeleteSelectedEvents();
	void		MoveEventsBy(AmTime timeDelta, int32 yDelta);
	void		TransformEventsBy(int32 xDelta, int32 yDelta);
	/* These are used to facilitate key presses.
	 */
	int32		mTransStep, mTransCount;
	AmTime		QuantizeTime() const;
	
	/* Ahhhh yeah, these are the commands that get invoked from key presses
	 * that cause events to get selected.  This is a mess right now, hacked in
	 * at the last minute.
	 */
	void		SelectRightEvent();
	status_t	NextRightEvent(	const AmTrack* track,
								AmEvent* event, AmPhraseEvent* container,
								AmEvent** eventAnswer, AmPhraseEvent** containerAnswer) const;
	/* Create a new selections object around the requested event.  The caller is responsible
	 * for actually calling SetSelections() on the target -- which CAN NOT be done while
	 * the song is locked.
	 */
	AmSelectionsI* SelectFirstEvent(const AmTrack* track);
	AmSelectionsI* SelectEvent(AmEvent* event, AmPhraseEvent* container);

	AmEvent*	FirstEvent(const AmTrack* track, AmPhraseEvent** answer) const;
};

#endif 
