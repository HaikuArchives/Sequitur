/* AmPhraseEvent.h
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.15.00			hackborn
 * Mutatated from its original incarnation.
 */

#ifndef AMKERNEL_AMPHRASEEVENT_H
#define AMKERNEL_AMPHRASEEVENT_H

#include "AmPublic/AmBankI.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmPhrase.h"

/***************************************************************************
 * AM-PHRASE-EVENT
 * This class holds onto an AmPhrase object -- i.e. this is its own sequence
 * of events.  If the holder is deleted, it deletes whatever phrase and
 * events it currently owns, as well.  Note that the inherited mTime variable
 * is not used by the phrase holder -- it punts to its phrase, or returns 0
 * if there isn't a phrase.
 *
 * The GetAsMessage function adds the following fields:
 *		"start_time"	An int32, the time the phrase begins.
 *		"end_time"		An int32, the time the phrase ends.
 ***************************************************************************/
class AmPhraseEvent : public AmEvent,
					  public AmEventParent
{
public:
	virtual EventType		Type() const  	{ return PHRASE_TYPE; }
	virtual AmTime			TimeOffset() const;
	/* Answer the time range for the given event, applying any offset
	 * that the phrase has.
	 */
	virtual AmRange			EventRange(const AmEvent* event) const;
	virtual status_t		SetEventStartTime(AmEvent* event, AmTime newTime);
	virtual status_t		SetEventEndTime(AmEvent* event, AmTime newTime);

	virtual AmEventParent*	Parent() const;
	virtual void			SetParent(AmEventParent* parent);
	virtual void			Invalidate(	AmEvent* changedEvent,
										AmRange oldRange, AmRange newRange);
	virtual void			TimeRangeDirty();

	bool					IsEmpty() const;
	AmPhrase*				Phrase();
	const AmPhrase*			Phrase() const;

	/* Convenience -- these are passthroughs to the phrase.
	 */
	virtual status_t		Add(AmEvent* event) = 0;
	virtual status_t		Remove(AmEvent* event) = 0;

	virtual status_t		GetAsMessage(BMessage& msg) const;
	virtual void			Print(void) const;

protected:
	AmPhrase*			mPhrase;

	AmPhraseEvent();
	AmPhraseEvent(AmTime startTime);
	AmPhraseEvent(const AmPhraseEvent& o);
	AmPhraseEvent(const BMessage& flatEvent);
//	AmPhraseEvent&		operator=(const AmPhraseEvent& o);
	virtual void		RealDelete();

private:
	typedef AmEvent		inherited;
};

/***************************************************************************
 * AM-ROOT-PHRASE-EVENT
 * This is a concrete implementation of the abstract class AmPhraseEvent.
 * It is the physical owner of a phrase which might contain links.
 ***************************************************************************/
class AmRootPhraseEvent : public AmPhraseEvent
{
public:
	AmRootPhraseEvent();
	AmRootPhraseEvent(const AmPhraseEvent& o);
	AmRootPhraseEvent(const AmPhrase& o);
	AmRootPhraseEvent(const BMessage& flatEvent);
	
	virtual EventSubtype	Subtype() const  	{ return ROOT_SUBTYPE; }
	virtual AmTime			StartTime() const;
	virtual void			SetStartTime(AmTime newTime);
//	virtual AmTime			Duration() const;
	virtual AmTime			EndTime() const;
	
	virtual status_t		Add(AmEvent* event);
	virtual status_t		Remove(AmEvent* event);

	virtual AmEvent*		Copy() const;
	virtual status_t		GetAsMessage(BMessage& msg) const;
	virtual bool			Equals(AmEvent* event) const;
	virtual void			Print(void) const;

protected:
	virtual ~AmRootPhraseEvent();
	AmRootPhraseEvent&		operator=(const AmRootPhraseEvent& o);

private:
	typedef AmPhraseEvent	inherited;
};

/***************************************************************************
 * AM-BANK-EVENT
 * This is simply a container for events that's typed to a BANK_SUBTYPE.
 ***************************************************************************/
class AmBankChange : public AmRootPhraseEvent
{
public:
	AmBankChange(uint32 bankNumber = 0, const char* name = NULL);
	AmBankChange(const AmBankChange& o);
	AmBankChange(const BMessage& flatEvent);
	
	uint32					BankNumber() const;
	void					SetBankNumber(uint32 bankNumber);
	const char*				Name() const;
	void					SetName(const char* name);
	void					Set(uint32 bankNumber, const char* name);
	/* Replace my bank info (i.e. all events and my cached info)
	 * with the relevant info from bank.  Do NOT replace my
	 * last program change, if any.  Return B_OK for success,
	 * otherwise B_ERROR.
	 */
	status_t				SetTo(ArpCRef<AmBankI> bank);
	/* Answer my program change, if any.  The program change is
	 * not technically part of the bank, so any bank changes that
	 * INCLUDE a program change (like the program 100 method) will
	 * answer the second, final program change.
	 */
	AmProgramChange*		ProgramChange() const;
	/* Answer true if these banks have the same types of events
	 * in the same order.  What it means for events to be the
	 * same 'type' varies -- program changes by default are the
	 * same type, whereas control changes need to have the same
	 * control number (but not value).  If exactly is true, then
	 * the events have to match exactly, including their data.
	 */
	bool					Matches(const AmBankChange* be, bool exactly = false) const;
	bool					Matches(const AmNode* head, const AmNode* tail, bool exactly = false) const;
	
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------		
	virtual EventSubtype	Subtype() const  	{ return BANK_SUBTYPE; }
	virtual AmEvent*		Copy() const;
	virtual BView*			NewView(ViewType type, BRect frame) const;
	virtual void			Print(void) const;

private:
	typedef AmRootPhraseEvent inherited;
	uint32					mBankNumber;
	BString					mName;
};

/***************************************************************************
 * AM-INNER-PHRASE-EVENT
 * This is identical to a root phrase event, curently, it just has a
 * different subtype.  It's been awhile since I worked on sequitur -- I
 * THINK the original intent of the root phrase was that they would be
 * the highest level of phrase in the track.  Realistically I don't see
 * anything in the code that would cause problems with stuffing root
 * phrases in root phrases, but just to be safe I'm separating them.
 ***************************************************************************/
class AmInnerPhraseEvent : public AmRootPhraseEvent
{
public:
	AmInnerPhraseEvent();
	AmInnerPhraseEvent(const AmInnerPhraseEvent& o);
	AmInnerPhraseEvent(const AmPhrase& o);
	AmInnerPhraseEvent(const BMessage& flatEvent);
	
	virtual EventSubtype	Subtype() const  	{ return INNER_SUBTYPE; }
	
	virtual AmEvent*		Copy() const;
	virtual void			Print(void) const;

protected:
	AmInnerPhraseEvent&		operator=(const AmInnerPhraseEvent& o);

private:
	typedef AmRootPhraseEvent	inherited;
};

/***************************************************************************
 * AM-LINK-EVENT
 * This event creates a link to a phrase.
 ***************************************************************************/
class AmLinkEvent : public AmPhraseEvent
{
public:
	AmLinkEvent();
	AmLinkEvent(AmTime startTime, AmPhrase* phrase);
	
	virtual EventSubtype	Subtype() const  	{ return LINK_SUBTYPE; }
	virtual AmTime			TimeOffset() const;
	virtual AmTime			EndTime() const;

	virtual AmRange			EventRange(const AmEvent* event) const;
	virtual status_t		SetEventStartTime(AmEvent* event, AmTime newTime);
	virtual status_t		SetEventEndTime(AmEvent* event, AmTime newTime);

	virtual status_t		Add(AmEvent* event);
	virtual status_t		Remove(AmEvent* event);

	virtual AmEvent*		Copy() const;
	virtual bool			Equals(AmEvent* event) const;
	virtual void			Print(void) const;

	virtual void			AddedToPhrase();
	virtual void			RemovedFromPhrase();

protected:
	virtual ~AmLinkEvent();
	AmLinkEvent&			operator=(const AmLinkEvent& o);

private:
	typedef AmPhraseEvent	inherited;
};

#endif
