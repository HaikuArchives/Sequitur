/* AmNode.cpp
*/
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmNode.h"

/* For debugging
 */
static int32	node_count = 0;

/***************************************************************************
 * AM-NODE
 ****************************************************************************/
AmNode::AmNode( AmEvent* event )
		: prev(0), next(0), mEvent(event)
{
	if( mEvent ) mEvent->IncRefs();
	node_count++;
#if 0
	printf("created node %li\n", nodeCount);
#endif
//if (nodeCount == 25) { nodeCount--; ArpIndexedNode	*n = new ArpIndexedNode(0); }
}

AmNode::~AmNode()
{
	if( mEvent ) mEvent->DecRefs();
	mEvent = 0;
	
	if( prev ) prev->next = 0;
	
	AmNode* pos = next;
	while (pos) {
		AmNode* next = pos->next;
		pos->prev = pos->next = 0;
		delete pos;
		pos = next;
	}

	//  debug... make sure we're releasing all the data
#if 0
	if (nodeCount == 1) {
		printf("deleted FINAL node %li\n", nodeCount);
	} else {
		printf("deleted node %li\n", nodeCount);
	}
#endif
	node_count--;
}

AmTime AmNode::StartTime() const
{
	if (!mEvent) return -1;
	return mEvent->StartTime();
}

AmTime AmNode::EndTime() const
{
	if( !mEvent ) return -1;
	return mEvent->EndTime();
}

AmEvent* AmNode::Event() const
{
	return mEvent;
}

AmNode* AmNode::HeadNode() const
{
	const AmNode* pos = this;
	while (pos->prev) pos = pos->prev;
	return const_cast<AmNode*>(pos);
}

AmNode* AmNode::TailNode() const
{
	const AmNode* pos = this;
	while (pos->next) pos = pos->next;
	return const_cast<AmNode*>(pos);
}

bool AmNode::IsValid() const
{
	return mEvent != 0;
}

AmNode* AmNode::NodeBefore(AmTime time)
{
	if( StartTime() >= time ) return 0;
	
	AmNode* pos = this;
	while (pos && pos->next && pos->StartTime() < time) pos = pos->next;
	return pos;
}

// This is copied from AmEvent::MergeEvent().

status_t AmNode::AddNode(AmNode* node)
{
	assert( node );

	AmNode* destPos = this;
	
	#if NOISY
	ArpD(cdb << ADH << "Merging " << node << " into " << destPos << endl);
	#endif
	
	const AmTime nodeTime = node->StartTime();
	#if NOISY
	ArpD(cdb << ADH << "Node time is " << nodeTime << endl);
	#endif
	AmNode* tmp=NULL;
	bool searched = false;
	
	// If node time is after current position time, look forward for
	// the place to insert it.
	while( nodeTime >= destPos->StartTime() ) {
		#if NOISY
		ArpD(cdb << ADH << "Dest time is " << destPos->StartTime()
				<< ", going forward." << endl);
		#endif
		tmp = destPos->next;
		if( !tmp ) {
			// Whoops, ran to end of list -- place source at end.
			#if NOISY
			ArpD(cdb << ADH << "This is the last event; appending src.\n");
			#endif
			destPos->InsertNext(node);
			return B_OK;
		}
		destPos = tmp;
		searched = true;
	}
	
	if( searched ) {
		// We moved forward at least one event in the dest list, so
		// we know this is where to put it.
		#if NOISY
		ArpD(cdb << ADH << "Found dest time " << destPos->Time()
						<< ", inserting here." << endl);
		#endif
		destPos->InsertPrev(node);
		return B_OK;
	}
	
	// That didn't work -- node time is before current position, so look
	// back in the list for where this goes.
	while( nodeTime < destPos->StartTime() ) {
		#if NOISY
		ArpD(cdb << ADH << "Dest time is " << destPos->StartTime()
						<< ", going backward." << endl);
		#endif
		tmp = destPos->prev;
		if( !tmp ) {
			// Whoops, ran to end of list -- place source at front.
			#if NOISY
			ArpD(cdb << ADH << "This is the first event; inserting src.\n");
			#endif
			destPos->InsertPrev(node);
			return B_OK;
		}
		destPos = tmp;
	}
	
	// Okay, we absolutely positive know that this is the place to
	// put it.
	#if NOISY
	ArpD(cdb << ADH << "Found dest time " << destPos->StartTime()
					<< ", appending here." << endl);
	#endif
	destPos->InsertNext(node);
	return B_OK;
}

AmNode* AmNode::RemoveNode()
{
	if( prev ) prev->next = next;
	if( next ) next->prev = prev;
	next = prev = 0;
	return this;
}

void AmNode::DeleteListContents()
{
	AmNode* pos = this;
	
	while (pos) {
		if( pos->mEvent ) {
			pos->pDeleteContents();
			pos->mEvent = 0;
		}
		pos = pos->next;
	}
}

status_t AmNode::ReplaceRun(AmNode* end, AmNode* replacement)
{
	ArpASSERT(replacement);
	replacement->prev = prev;
	if (prev) prev->next = replacement;
	prev = NULL;
	if (!end) {
		replacement->next = NULL;
		return B_OK;
	}

	AmNode*		after = end->next;
	replacement->next = after;
	end->next = NULL;
	if (after) after->prev = replacement;

	return B_OK;
}

AmNode* AmNode::Copy() const
{
	const AmNode* pos = this;
	
	AmNode* head = NULL;
	AmNode* tail = NULL;
	
	while (pos) {
		AmNode*	copy = new AmNode(0);
		if (!copy) {
			if (head) {
				head->DeleteListContents();
				delete head;
			}
			return NULL;
		}
		
		pos->CopyContentsTo( copy );
		if (!tail) {
			head = tail = copy;
		} else {
			tail->InsertNext(copy);
			tail = copy;
		}
		
		pos = pos->next;
	}
	
	return head;
}

void AmNode::Print() const
{
	if( !mEvent ) {
		printf("AmNode (empty)\n");
		return;
	}
	printf("AmNode on ");  fflush(stdout);
	mEvent->Print();	
}

status_t AmNode::InsertNext(AmNode* node)
{
	assert( node );

	if( next ) next->prev = node;
	node->prev = this;
	node->next = next;
	next = node;
	return B_OK;
}

status_t AmNode::InsertPrev(AmNode* node)
{
	assert( node );

	if( prev ) prev->next = node;
	node->prev = prev;
	node->next = this;
	prev = node;
	return B_OK;
}

void AmNode::CopyContentsTo(AmNode* copy) const
{
	assert( copy );
	if( !copy || !mEvent ) return;
	AmNode*		node = copy;
	node->SetEvent( mEvent->Copy() );
}

void AmNode::SetEvent(AmEvent* event)
{
	if( mEvent ) mEvent->DecRefs();
	mEvent = event;
	if( mEvent ) mEvent->IncRefs();
}

void AmNode::pDeleteContents() {
	if ( mEvent ) mEvent->Delete();
}

