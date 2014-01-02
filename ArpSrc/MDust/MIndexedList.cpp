/* MIndexedList.cpp
 *
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 */

#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>
#include <support/ClassInfo.h>

#include "MDust/MIndexedList.h"

//#ifndef MIDI_CONTAINER_H
//#include "MidiContainer.h"
//#endif
//#ifndef MIDI_PLAYBACK_ARRAY_H
//#include "MidiPlaybackArray.h"
//#endif


// The number of beats we have in a chain
//#define MIDI_BEATS_IN_CHAIN			(40)

// The number of nodes we allot for the chain heads -- every time the list needs
// more chains, this is the number of chains we add.
#define ARP_CHAIN_INCREMENT		(10)
// The number of nodes we allot for the chain -- every time the chain
// needs to grow, this is the number we increment it by.
//#define MIDI_CHAIN_INCREMENT		(10)

// Surely there is someone who defines error as a value less than one
#define ARP_HASH_ERROR			(-1)

/***************************************************************************
* class ArpIndexedList
***************************************************************************/

MIndexedList::MIndexedList(int32 chainIndexesArg) {
	chain = 0;
	chainIndexes = chainIndexesArg;
	GrowToAtLeast(ARP_CHAIN_INCREMENT);
	tailNode = 0;
}

MIndexedList::~MIndexedList() {
//Fix: Why aren't we deleting events and letting subclasses who need to
//just do this do this?
	DeleteOnlyNodes();
}


/***************************************************************************
* Protected - CHAIN ACCESSING
***************************************************************************/

ArpIndexedNode* MIndexedList::HeadNode() const
{
	for (int32 i=0; i<count; i++) {
		if (chain[i] != 0) return chain[i];
	}
	return 0;
}

status_t MIndexedList::ChainHeadFrom(int32 index, ArpIndexedNode **node) {
	long hash = HashForIndex(index);

	// We used to hash to 1 minus the hash of the index, but we've removed that.
	// If clients want that, they can send it themselves.
	*node = 0;
	while (*node == 0) {
		if (! HashExists(hash)) return B_ERROR;
		*node = chain[hash];
		hash++;
	}
	return B_OK;
#if 0
	*node = chain[hash];
printf("MIndexedList::ChainHeadFrom() 5\n");  fflush(stdout);
Print();
	if (*node == 0) return B_ERROR;
	return B_OK;
#endif
}

int32 MIndexedList::IndexHeadFrom(int32 index)
{
	long hash = HashForIndex(index);
	if (! HashExists(hash)) return B_ERROR;
	return hash * ChainIndexes();
}

status_t MIndexedList::NodeAt(int32 index, ArpIndexedNode **node,
						int32 findNode, int32 direction) {
	if (direction == ARP_BACKWARDS) return(NodeBefore(index, node, findNode));
	if (direction == ARP_FORWARDS) return(NodeAfter(index, node, findNode));
	return B_ERROR;
}

status_t MIndexedList::NodeFor(int32 index, void *rawData, ArpIndexedNode **answer) {
	ArpIndexedNode	*originalNode, *node;
	// Assume that the user has supplied a useful index, and find the node closet to it.
	if (NodeAt(index, &node, 0, ARP_FORWARDS) == B_ERROR) {
		if (NodeAt(index, &node, 0, ARP_BACKWARDS) == B_ERROR) return B_ERROR;
	}
	if (node == 0) return B_ERROR;
	originalNode = node;
	
	// Seek forwards for the answer.
	while (node != 0)  {
		if (node->RawContents() == rawData) {
			*answer = node;
			return B_OK;
		}
		node = node->next;
	}

	// If that doesn't work, try seeking backwards.
	node = originalNode;
	while (node != 0)  {
		if (node->RawContents() == rawData) {
			*answer = node;
			return B_OK;
		}
		node = node->prev;
	}

	return B_ERROR;
}

int32 MIndexedList::Count()
{
	ArpIndexedNode		*node;
	int32	k = 0;
	if ((node = HeadNode()) == 0) return k;

	while (node != 0) {
		k++;
		node = node->next;
	}
	return k;
}

int32 MIndexedList::ChainIndexes()
{
	return chainIndexes;
}

/***************************************************************************
* Protected - CHAIN MANIPULATION
***************************************************************************/

status_t MIndexedList::AddNode(ArpIndexedNode *node)
{
	assert(node != 0);
	if (node == 0) return B_ERROR;
	if (node->IsValid() != true) return B_ERROR;
	return(pAddNode(node));
}

status_t MIndexedList::RemoveNode(ArpIndexedNode *node)
{
	assert(node != 0);
	return pRemoveNode(node);
}

status_t MIndexedList::DeleteNode(ArpIndexedNode *node)
{
	assert(node != 0);
	status_t	answer;

	if ((answer = RemoveNode(node)) != B_OK) return answer;
	node->RemoveNode();

	delete node;
	return B_OK;
}

status_t MIndexedList::RemoveHeadNode(ArpIndexedNode **removedNode)
{
	if ((*removedNode = HeadNode()) == 0) return B_ERROR;
	return RemoveNode(*removedNode);
}

status_t MIndexedList::RemoveTailNode(ArpIndexedNode **removedNode)
{
	if ((*removedNode = TailNode()) == 0) return B_ERROR;
	return RemoveNode(*removedNode);
}

void MIndexedList::AddedNode(ArpIndexedNode *node)
{
	assert(node != 0);
	// If the node is the new last node in the list then assign it
	// to the cached tailNode.
	if ((tailNode == 0) || (tailNode->Index() < node->Index())) {
		tailNode = node;
	}
}

void MIndexedList::RemovingNode(ArpIndexedNode *node)
{
	assert(node != 0);
	if (node == tailNode) {
		tailNode = node->prev;
	}
}

void MIndexedList::GrowToAtLeast(int32 countArg)
{
	int32 oldcount = count;
	count = countArg;
	// FIX: this isn't deleting anyone... is that what we want?
	if (count < 1) {
		if (chain != 0) pFreeChain();
		return;
	}
	
	if (chain == 0) {
		chain = (ArpIndexedNode**)malloc(sizeof(ArpIndexedNode)*count+2);
		for (long i=0; i<count; i++) chain[i] = 0;

	} else {
		ArpIndexedNode	**tmp;
		tmp = (ArpIndexedNode**)malloc(sizeof(ArpIndexedNode)*count+2);
		for (long i=0; i<count; i++) tmp[i] = 0;
		if (oldcount < count) {
			for (long i=0; i<oldcount; i++) tmp[i] = chain[i];
		} else {
			for (long i=0; i<count; i++) tmp[i] = chain[i];
		}
		free(chain);
		chain = tmp;
	}
}

void MIndexedList::DeleteOnlyNodes()
{
	if (chain == 0) return;

	ArpIndexedNode	*node;
	if ((node = HeadNode()) != 0) delete node;

	pFreeChain();
}

void MIndexedList::DeleteNodeContents()
{
	if (chain == 0) return;

	ArpIndexedNode	*node;
	if ((node = HeadNode()) != 0) {
		node->DeleteListContents();
		delete node;
	}

	pFreeChain();
}


/***************************************************************************
* Protected - TESTING
***************************************************************************/

bool MIndexedList::Includes(int32 index, void *rawData)
{
	ArpIndexedNode	*node;
	if (NodeFor(index, rawData, &node) == B_OK) return true;
	return false;
}

bool MIndexedList::IsEmpty() const
{
	if ( HeadNode() == 0 ) return true;
	return false;
}

/***************************************************************************
* Protected - UTILITY
***************************************************************************/

MIndexedList* MIndexedList::Copy()
{
	MIndexedList	*copy = new MIndexedList;
	if (copy == 0) return 0;
	CopyTo(copy);
	return copy;
}

void MIndexedList::CopyTo(MIndexedList *copy)
{
	copy->GrowToAtLeast(count);	

	for (long k=0; k<count; k++) {
		if (chain[k] != 0) copy->chain[k] = chain[k]->Copy();
	}
}

// A convenience for the file IO
void MIndexedList::ReplaceWith(MIndexedList *ail)
{
	chain = ail->chain;
	count = ail->count;
	ail->chain = 0;
	ail->count = -1;
}

//fix:  We can't we just tell the head to print?  Are nodes not connected
//between chains?  If not, why not?
void MIndexedList::Print()
{
	//printf("Printing MIndexedList..\n");
	
	if (chain == 0) {
		printf("MIndexedList::Print() chain was NULL\n");
		return;
	}

	for(long i=0; i<count; i++) {
		printf("Chain %d", (int)i);
		if (chain[i] == NULL) {
			printf("\tNULL\n");
		} else {
			printf("\n");
			chain[i]->PrintListUntil(NextChainHeadFrom(i+1));
		}
	}		
	if (tailNode == 0) {
		printf("No tail node\n");
	} else {
		const char	*cn = class_name(tailNode);
		if (cn == 0) { printf("Tail node class: No class name\n");
		} else { printf("Tail node class: %s\n", cn); }
		fflush(stdout);

		tailNode->PrintContents();
	}
}

void MIndexedList::SyncTailNode()
{
	for (int32 i=count-1; i>=0; i--) {
		if (! HashExists(i)) assert(false);
		if (chain[i] != 0) {
			tailNode = chain[i]->TailNode();
			return;
		}
	}
	tailNode = 0;
}

// convert a ppqn into its index into the chain.
// Return ARP_HASH_ERROR if the value is below zero.  Otherwise
// return the index -- note that it is possible for the index to
// exceed count.  Most functions will want to treat that as an error,
// but in the case of the adding functions, they need to grow to that size.
int32 MIndexedList::HashForIndex(int32 index)
{
	long hash = (long)floor(index / chainIndexes);
	if (hash < 0) return(ARP_HASH_ERROR);
	return(hash);

//	long index = floor( ((ppqnTime / PPQN) / chainIndexes) );
//	if (index < 0) return(ARP_HASH_ERROR);
//	return(index);
}


/**********************************************************************
* Public - utilities
**********************************************************************/

// A utility that checks to see if eArg is in the right place
// in the list.  If not, it repositions it.
//void MidiAbstractList::VerifyPosition(MidiEvent *eArg) {
//	Remove(eArg);
//	Add(eArg);

/* I decided the below won't work if the event has been repositioned
to a different chain, but I can't think of a decent way to check to
see if that condition has occurred, so for now, we're doing this the
REALLY cheap way.
	MidiListNode	*node = HeadNode();
	while ( (node != NULL) && (node->Contents() != eArg)) {
		node = (MidiListNode*)node->next;
	}
	if (node->Contents() != eArg) return;

	MidiListNode	*prev, *next;
	prev = (MidiListNode*)node->prev;
	next = (MidiListNode*)node->next;	

	if ( ((prev != NULL) && (prev->Time() > node->Time()))	
			|| ((next != NULL) && (next->Time() < node->Time())) ) {
		// For now we'll do this the cheap way
		Remove(eArg);
		Add(eArg);
	}
*/
//}

/*
void MidiAbstractList::VerifyListPosition(MidiAbstractList *lArg) {
	MidiListNode	*node = lArg->HeadNode();
	while (node != NULL) {
		Remove(node->Contents());
		node = (MidiListNode*)node->next;
	}
	node = lArg->HeadNode();
	while (node != NULL) {
		Add(node->Contents());
		node = (MidiListNode*)node->next;
	}
}
*/

/**********************************************************************
* Protected - file IO
**********************************************************************/


// Check to see if node should be the head of a chain.
/*
void MidiAbstractList::KeepChainsConsistent(MidiListNode *node) {
	long index = IndexForPPQN(node->Time());
	if (index >= count)
		GrowToAtLeast(index + ARP_CHAIN_INCREMENT);	

	if (chain[index] == NULL)
		chain[index] = node;
}
*/

/***************************************************************************
* Private - CHAIN ACCESSING
***************************************************************************/

status_t MIndexedList::NodeBefore(int32 index, ArpIndexedNode **answer,
						int32 findNode)
{
	long hash = HashForIndex(index);
	if (hash >= count) hash = count - 1;
	if (! HashExists(hash)) return B_ERROR;

	ArpIndexedNode	*nodeBefore = 0,
					*node = 0;
	
	// Find the first chain before the index that isn't NULL
	bool		keepLooking = true;
	for (int32 k = hash; keepLooking; k--) {
		if (! HashExists(k)) {
			keepLooking = false;
		} else if ( (chain[k] != 0) && (chain[k]->Index() <= index) ) {
			node = chain[k];
			keepLooking = false;
		}
	}
	if (node == 0) return B_ERROR;

	// Find the first node on or before index.
	while (node != 0) {
		if (node->next == 0) {
			nodeBefore = node;
			node = 0;
		} else if (node->Index() == index) {
			nodeBefore = node;
			node = 0;
		} else if (node->next->Index() == index) {
			nodeBefore = node->next;
			node = 0;
		} else if ( (node->Index() <= index)
				&& (node->next->Index() >= index) ) {
			nodeBefore = node;
			node = 0;
		} else {
			node = node->next;
		}
	}
	if (nodeBefore == 0) return B_ERROR;
	node = nodeBefore;

	// The above routines might put us on the last node in a
	// series of nodes with the same index -- no real way around
	// that, I don't think.  If that's the case, rewind us back
	// to the first node.
	while ( (node->prev != 0)
			&& (node->prev->Index() == node->Index()) )
		node = node->prev;

	// Now find whichever of the n nodes with the same index that
	// the client has requested.
	for (int32 k=1; k<= findNode; k++) {
		node = node->next;
		if (node == 0) return B_ERROR;
	}
	if (node->Index() != nodeBefore->Index()) return B_ERROR;
	*answer = node;
	return B_OK;
}

status_t MIndexedList::NodeAfter(int32 index, ArpIndexedNode **answer,
						int32 findNode)
{
	long hash = HashForIndex(index);
	if (! HashExists(hash)) return B_ERROR;

	ArpIndexedNode	*nodeAfter = NULL,
					*node = NULL;
	node = NextChainHeadFrom(hash);

	// Find the first node on or after index.
	while (node != NULL) {
		if (node->Index() >= index) {
			nodeAfter = node;
			node = NULL;
		} else {
			node = node->next;
		}
	}
	if (nodeAfter == NULL) return B_ERROR;
	node = nodeAfter;

	// Now find whichever of the n nodes with the same index that
	// the client has requested.
	for (int32 k=0; k< findNode; k++) {
		node = node->next;
		if (node == NULL) return B_ERROR;
	}
	if (node->Index() != nodeAfter->Index()) return B_ERROR;
	*answer = node;
	return B_OK;
}


/**********************************************************************
* Private - CHAIN MANIPULATION
**********************************************************************/

// Add the node to the chain.  First we must verify that the chain
// is large enough for nArg; if not, grow it.  Than add it at
// the appropriate index.
status_t MIndexedList::pAddNode(ArpIndexedNode *node)
{
	long hash = HashForIndex(node->Index());
#if 0 	// why did I write this?  Isn't the POINT that if the hash is
		// too large, then the list grows?
	if (not HashExists(hash)) {
		delete node;
		return B_ERROR;
	}
#endif

	if (hash >= count) {
		GrowToAtLeast(hash + ARP_CHAIN_INCREMENT);	
	}

	if (chain[hash] == NULL) {
		chain[hash] = node;
		pAddNodeAtChainHead(node, hash);
	} else if (node->Index() < chain[hash]->Index()) {
		chain[hash]->AddNode(node);
		chain[hash] = node;
	} else {
		chain[hash]->AddNode(node);
	}
	
	// If the node is the new last node in the list then assign it
	// to the cached tailNode.
//	if ((tailNode == NULL) or (tailNode->Index() < node->Index()))
//		tailNode = node;

	AddedNode(node);
	return B_OK;
}

status_t MIndexedList::pRemoveNode(ArpIndexedNode *node)
{
	assert(node != 0);
	int32	hash = HashForIndex(node->Index());
	if (! HashExists(hash)) return B_ERROR;
	if (chain[hash] == 0) return B_ERROR;

	if (node == chain[hash]) {
		status_t	answer;
		ArpIndexedNode	*unusedVar;
		answer = pRemoveChainHeadAt(hash, &unusedVar);
		if (answer != B_OK) return answer;
	}

	RemovingNode(node);
	node->RemoveNode();
	RemovedNode(node);
	return B_OK;
}


// A node is being added as the head of an empty chain.  We need to find
// the nearest node to add it to, so it gets inserted in the list.
void MIndexedList::pAddNodeAtChainHead(ArpIndexedNode *node, int32 hash)
{
	long	k = hash;
	while (++k < count) {
		if (chain[k] != NULL) {
			chain[k]->AddNode(node);
			return;
		}
	}
	k = hash;
	while (--k >= 0) {
		if (chain[k] != NULL) {
			chain[k]->AddNode(node);
			return;
		}
	}
}

// A node is being removed from the head of a chain.
status_t MIndexedList::pRemoveChainHeadAt(int32 hash, ArpIndexedNode **node)
{
	assert( (hash >= 0) && (hash < count) );

	*node = 0;
	ArpIndexedNode	*foundNode;
	assert( (foundNode = chain[hash]) != 0);
	if ( (foundNode = chain[hash]) == 0) return B_ERROR;

	// If the foundNode has a next that is in the same chain,
	// we need to point to it.
	if ( (foundNode->next != 0)
			&& (hash == (HashForIndex(foundNode->next->Index() )))) {
		chain[hash] = foundNode->next;
	} else {
		chain[hash] = 0;
	}

	*node = foundNode;
	return B_OK;
}

void MIndexedList::pFreeChain()
{
	free(chain);
	chain = 0;
	count = -1;
	tailNode = 0;
}


/**********************************************************************
* Private - TESTING
**********************************************************************/

// answer false if the hash falls beyond the bounds of the current chain.
bool MIndexedList::HashExists(int32 hash)
{
	if ( (hash == ARP_HASH_ERROR)
			|| (hash < 0)
			|| (hash >= count) )
		return false;
	return true;
}


/**********************************************************************
* Private - UTILITY
**********************************************************************/

ArpIndexedNode* MIndexedList::NextChainHeadFrom(int32 hash)
{
	int32	k = hash;
	while (HashExists(k)) {
		if (chain[k] != NULL) return chain[k];
		k++;
	}
	return NULL;
}

