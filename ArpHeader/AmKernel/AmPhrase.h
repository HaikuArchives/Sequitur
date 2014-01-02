/* AmPhrase.h
 * Copyright (c)1998-2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This class defines the basic container structure for AmEvent objects.
 * AmEvents are stored as a series of doubly-linked AmNodes, but they
 * are also indexed from an array based on their start time.  The head of
 * each array index is an _AmChain.  In addition to providing fast access
 * to predetermined parts of the list, _AmChains also store 'span nodes':
 * nodes whose time extend from a previous chain into them.  This means that
 * clients never have to look prior to a requested chain to find all events
 * that overlap with that chain.  Note that the events in the span list are
 * not in any way sorted.
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
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.15.00		hackborn
 * Created this file from its original incarnation as ArpIndexedList.
 * Removed the locking.
 */

#ifndef AMKERNEL_AMPHRASE_H
#define AMKERNEL_AMPHRASE_H

#include <vector.h>
#include <support/SupportDefs.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmRange.h"
#include "AmKernel/AmNode.h"
class AmLinkEvent;
class AmNoteOn;
class AmPhraseEvent;
class AmPhrase;
class AmFilterHolderI;
class _AmPhraseSelectionEntry;

/* Several accessing methods include the option to seek forwards
 * or backwards if data isn't found where requested.
 */
enum PhraseSearchType {
	NO_SEARCH,
	FORWARDS_SEARCH,
	BACKWARDS_SEARCH,
};

/***************************************************************************
 * _AM-CHAIN
 ****************************************************************************/
class _AmChain
{
public:
	_AmChain();
	virtual ~_AmChain();
	
	_AmChain*	mNext;
	_AmChain*	mPrev;

	_AmChain&	operator=(const _AmChain &c);

	/* Set my start and end times.  This should never change once set, since
	 * that would seriously screw with the events in this chain.
	 */
	void		SetRange(AmTime startTime, AmTime endTime);

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* Answer the end time of this chain (or any chains after it, whatever is
	 * last).
	 */
	AmTime		EndTime() const;
	/* Answer the first event in this chain.  Optionally, the caller can
	 * have me seek forwards or backwards if this chain has no head.
	 */
	AmNode*		HeadNode(PhraseSearchType search = NO_SEARCH) const;
	/* Answer the first span event in this chain.  Optionally, the caller can
	 * have me seek forwards or backwards if this chain has no span head.
	 */
	AmNode*		SpanNode(PhraseSearchType search = NO_SEARCH) const;
	
	AmNode*		FindNode(	AmTime time,
							PhraseSearchType search) const;

	/*---------------------------------------------------------
	 * NODE MANIPULATING
	 *---------------------------------------------------------*/
	/* Attempt to add the event to my list of events, which are in time-sorted
	 * order.  Answer the AmNode created, or none if there's a problem.  This
	 * method assumes the caller has verified that the event actually belongs
	 * in this chain.  This method will also add spans to any chains after this
	 * one that the event exists in (although it will NOT span phrase events).
	 */
	AmNode*		Add(AmEvent* event);
	/* Remove the supplied event.  Answer the removed node, or 0 if something
	 * goes wrong.  The event needs to be in this chain.
	 */
	AmNode*		Remove(AmEvent* event);
	/* Answer the phrase event that contains the supplied phrase, or 0 if none.  If the
	 * phrase isn't in this chain it won't be answered.
	 */
	AmPhraseEvent* PhraseEventFor(AmPhrase* phrase) const;

	/*---------------------------------------------------------
	 * NODE MANIPULATING
	 *---------------------------------------------------------*/
	/* Answer true if this chain has neither a head nor spanning node.
	 */
	bool		IsEmpty() const;
	/* This method answers the number of events in this chain.  It is primarily
	 * a debugging tool.
	 */
	uint32		CountEvents() const;
	/* This method answers the number of spans in this chain.  It is primarily
	 * a debugging tool.
	 */
	uint32		CountSpans() const;
	/* The event needs to be synced.  That means the phrase has no clue
	 * what chain it might be in, or where it might have spans, so rifle
	 * through all of my events and remove it from wherever I find it.
	 */
	void		Sync(AmEvent* event);
	void		Print(bool recurse, uint32 tabs) const;
	

private:
	friend class AmPhrase;
	/* My start and end times, inclusive.  That means this chain holds events
	 * that start on or after start time, and on or before end time.
	 */
	AmTime		mStartTime, mEndTime;
	/* This is the head node of all the events this chain contains.
	 */
	AmNode*		mHeadNode;
	/* This is the head node of all the events that are in previous changes
	 * that extend over into this one.
	 */
	AmNode*		mSpanNode;

	/* Answer the next non-null head after the current chain.  If there is none,
	 * answer the obvious null.
	 */
	AmNode*		NextHead() const;
	/* Attempt to add the event into my list of spanning events.
	 * If the event doesn't actually span me -- it ends before I start --
	 * or it was added fine, answer with B_OK.  Otherwise, answer with B_ERROR
	 * or B_MEMORY.  This method also adds the spanning event to all subsequent
	 * chains it exists in.
	 */
	status_t	AddSpanEvent(AmEvent* event);
	/* Remove the span event from my span list.  I attempt to remove the event
	 * from myself and every chain after me, regardless of the end time of
	 * the event.  This is because the event might have been moved, so its
	 * end time might be deceptive.
	 */
	void		RemoveSpanEvent(AmEvent* event);

	/* These method are called when the first head node is added to this
	 * chain (doesn't count if there was already a head node that was just]
	 * replaced).  They hunt through the prev and next chains in my list
	 * and connect up the node with the closest nodes in the other chains.
	 */
	void		ConnectNewHeadNode(AmNode* node);
	/* Do the work of the various search styles supplied to FindNode().
	 */
	AmNode*		FindNodeForwards(AmTime time) const;
	AmNode*		FindNodeBackwards(AmTime time) const;
	/* The first non-null head node, starting with me and continuing backwards
	 * through the other chains.
	 */
	AmNode*		PrevAvailableHeadNode() const;
	/* The first non-null head node, starting with me and continuing forwards
	 * through the other chains.
	 */
	AmNode*		NextAvailableHeadNode() const;

	/* The check methods are all just used for debugging.
	 */
	bool		CheckIncludesSpan(AmEvent* event) const;
};
typedef vector<_AmChain>		chain_vec;
typedef vector<AmLinkEvent*>	link_vec;

/***************************************************************************
 * AM-PHRASE
 * This is based on the ArpIndexedList class, which stores nodes of linked
 * lists in a series of chains, based on the nodes' Indexes().  AmPhrases
 * can in turn store other AmPhrases, via the AmPhraseEvent class.  However,
 * since AmPhrases store a reference to their parent AmPhrase, it's an error
 * to store the same AmPhrase in multiple AmPhrases.
 ****************************************************************************/
class AmPhrase : public AmEventParent
{
public:
	/* Default to four measures of 4 / 4 time
	 */
	AmPhrase(uint32 chainSize = PPQN * 4 * 4);
	AmPhrase(const AmPhrase& o);
	virtual ~AmPhrase();

	AmPhrase& operator=(const AmPhrase& o);
	
	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* Answer the start and end time of this phrase.
	 */
	AmRange					TimeRange() const;
	AmTime					StartTime() const;
	void					SetStartTime(AmTime newTime);
	AmTime					EndTime() const;
	AmNode*					HeadNode() const;
	AmNode*					TailNode() const;
	/* Answer the first node in the chain located at the supplied time.  If the chain
	 * has no head, by default seek forward until I find one.
	 */
	AmNode*					ChainHeadNode(AmTime time, PhraseSearchType search = FORWARDS_SEARCH) const;
	/* Answer the first span node in the chain located at the supplied time.  By default,
	 * do no searching and just answer zero if that chain has no span head.  This is
	 * because there's really no reason for clients to care about span nodes that exist
	 * before the time requested or after the time requested -- mostly the option is
	 * there to keep the interface consistent.
	 */
	AmNode*					ChainSpanNode(AmTime time, PhraseSearchType search = NO_SEARCH) const;
	/* Answer the first node at the supplied time.  If no node exists at that time,
	 * use the search type to determine if I should look after or before the supplied
	 * time for the nearest node.  FORWARDS finds the next node on or after the supplied
	 * time, BACKWARDS finds the next node on or before the supplied time.  The NO_SEARCH
	 * is not currently implemented.
	 */
	AmNode*					FindNode(	AmTime time,
										PhraseSearchType search = FORWARDS_SEARCH) const;
	/* Answer the node for the supplied event, or 0 if none.
	 */
	AmNode*					FindNode(AmEvent* event) const;
	/* Answer true if this phrase contains the supplied event, false otherwise.
	 */
	bool					Includes(AmEvent* event) const;
	bool					IsEmpty() const;
	uint32					CountNodes() const;
	bool					IncludesOnly(AmEvent::EventType type) const;
	
	void					MergeInto(	AmEvent** eventList, const AmPhraseEvent* topPhrase = NULL,
										AmFilterHolderI* initialFilter = NULL,
										AmTime startTime = 0, AmTime stopTime = -1) const;

	uint32					CountLinks() const;
	void					Link(AmLinkEvent* link);
	void					Unlink(AmLinkEvent* link);
	/*---------------------------------------------------------
	 * PROPERTIES
	 *---------------------------------------------------------*/
	enum ColorCode {
		BACKGROUND_C,
		FOREGROUND_C
	};
	rgb_color				Color(ColorCode code) const;
	void					SetColor(ColorCode code, rgb_color c);

	BString					Name() const;
	void					SetName(const BString& name);

	status_t				GetProperties(BMessage& properties) const;
	status_t				SetProperties(const BMessage& properties);
	/*---------------------------------------------------------
	 * NODE MANIPULATION
	 *---------------------------------------------------------*/
	status_t				Add(AmEvent* event);
	/* Add every event in the supplied phrase to myself.  If adding
	 * any of the events fails, stop the operation and answer the error.
	 */
	status_t				AddAll(AmPhrase* phrase);
	status_t				Remove(AmEvent* event);
	/* Clients should never directly manipulate the time and end time in
	 * an event -- they should always go through the phrase to provide
	 * a context for setting the time values.
	 */
	status_t				SetEventStartTime(AmEvent* event, AmTime newTime);
	status_t				SetEventEndTime(AmEvent* event, AmTime newTime);
	status_t				SetEndTime(AmEvent* event, AmTime newTime, AmTime oldTime);

	virtual AmEventParent*	Parent() const;
	virtual void			SetParent(AmEventParent* parent);
	virtual void			Invalidate(	AmEvent* changedEvent,
										AmRange oldRange, AmRange newRange);
	virtual void			TimeRangeDirty();

	/* Remove all my current events and replace them with the linked list
	 * supplied by head.  Supplying a head of 0 just empties this list.
	 * Currently, this method does not copy head, but instead takes ownership
	 * of it.  If the client doesn't want to give up ownership, pass in head->Copy().
	 */
	status_t				SetList(AmNode* head = 0);
	/* Delete all nodes and events in this phrase.  There is a special
	 * meaning for this:  The Delete() method will be called on each event,
	 * which marks the event deleted but only actually deletes it if it
	 * has no references.
	 */
	void					DeleteEvents();

	/*---------------------------------------------------------
	 * UTILITY
	 *---------------------------------------------------------*/
	/* Filter processing may cause my end times and events to be
	 * incorrectly hashed.  This fixes that.  This will only be
	 * coming from a selections object, so I optimize myself by
	 * supplying just those events that might need rehashing.
	 */
	void		Sync(_AmPhraseSelectionEntry* entry);
	void		Print(bool recurse = false, uint32 tabs = 0) const;
	void		PrintNoChains(bool recurse = false, uint32 tabs = 0) const;

void do_check() const;

private:
	AmEventParent*	mParent;
	AmPhrase*		mOldParent;
	chain_vec		mChains;
	/* The range of time in a single chain.  For example, with a chain
	 * size of 100, mChain[0] would contain nodes whose times are 0 to 99.
	 */
	int32			mChainSize;
	link_vec		mLinks;

	mutable AmTime	mCachedEndTime;
	mutable bool	mEndTimeIsDirty;
	AmTime			CountEndTime() const;
	
	BString			mName;
	/* The background and foreground colours of this event.
	 */
	mutable bool		mColorsNeedInit;
	mutable rgb_color	mBgC, mFgC;
	void		InitializeColors() const;
	/* Answer a chain for the supplied event.
	*/
	_AmChain*	ChainFor(AmEvent* event);
	/* Answer true if the hash exists, false otherwise.  Also, if the
	 * hash is beyond my current bounds, resize myself so it exists.
	 */
	bool		ValidateHash(uint32 hash);

	uint32		HashFromTime(AmTime time) const;

	/* It's been reported that a phrase that exists in me has had its start
	 * time change.  I want to rehash it and reorder my node list.
	 */
	void		RangeChange(AmPhrase* phrase, AmTime oldStart, AmTime newStart);

	/* This method does most of the work of the add.  It needs to:
	 *		Add the event in its appropriate chain.
	 *		Add spanning events in all necessary subsequent chains (which
	 *			is handled by the chains themselves)
	 *		Make sure the head node stays current
	 *		Set the parent if the added event is a phrase event.
	 */
	status_t	AddNoHash(AmEvent* event);
	/* Remove the event, which must be in the chain supplied by the time.  This
	 * is only to be used internally, since it doesn't cause this phrase's
	 * parent to rehash it.
	 */
	status_t	RemoveNoHash(AmEvent* event, AmTime time);
	/* This is a hack that causes all my parents to set the mEndTimeIsDirty
	 * flag to true.  Whenever a change comes in that affects my dirty flag,
	 * for now it will automatically affect all my parents -- obviously, this
	 * can be optimized.
	 */
	void		SetEndTimeIsDirty();
	/* The start and / or end time in this event has changed.  Make sure it
	 * is correctly hashed in all my chains.
	 */
	void		Sync(AmEvent* event);
	
	/* Answer true if all my data looks good.
	 */
	bool		CheckAll() const;
	bool		CheckNoEmptyNodes() const;
	AmNode*		CheckHead() const;
	AmNode*		CheckTail() const;
	bool		CheckEndTime() const;
	bool		CheckChainConnections() const;
	uint32		CheckNodeCount() const;
	uint32		CheckChainCount() const;
	bool		CheckSpanEvents() const;
	AmEvent*	CheckForDuplicates() const;
	bool		ContainsLink(AmLinkEvent* linkEvent) const;
};

/***************************************************************************
 * AM-SIGNATURE-PHRASE
 * A phrase that only stores signatures.  Mostly a convenience, but any
 * one who stores a list of signatures will find it a significant convenience.
 ****************************************************************************/
class AmSignaturePhrase : public AmPhrase
{
public:
	AmSignaturePhrase(uint32 chainSize = PPQN * 4 * 4);
	AmSignaturePhrase(const AmSignaturePhrase& o);

	status_t		SetSignature(int32 measure, uint32 beats, uint32 beatValue);
	status_t		GetSignature(AmTime time, AmSignature& signature) const;
	status_t		GetSignatureForMeasure(int32 measure, AmSignature& signature) const;

private:
	typedef AmPhrase	inherited;
};

#endif 
