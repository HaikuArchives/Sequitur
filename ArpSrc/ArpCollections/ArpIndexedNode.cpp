/* ArpIndexedNode.cpp
 * This object wraps some data and provides a place in a list.
 *
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 */

#include <cassert>

#ifndef ARPCOLLECTIONS_ARPINDEXEDNODE_H
#include "ArpCollections/ArpIndexedNode.h"
#endif

#include <cstdio>

long	nodeCount = 0;


ArpIndexedNode::ArpIndexedNode(void *dataArg)
{
	next = prev = NULL;
	rawData = dataArg;

	// debug...  shared with other constructor
	nodeCount++;
#if 0
	printf("created node %li\n", nodeCount);
#endif
//if (nodeCount == 25) { nodeCount--; ArpIndexedNode	*n = new ArpIndexedNode(0); }
}

ArpIndexedNode::~ArpIndexedNode()
{
	if (prev != 0) prev->next = 0;
	if (next != 0) delete next;

	//  debug... make sure we're releasing all the data
#if 0
	if (nodeCount == 1) {
		printf("deleted FINAL node %li\n", nodeCount);
	} else {
		printf("deleted node %li\n", nodeCount);
	}
#endif
	nodeCount--;
}


/***************************************************************************
* Public - ACCESSING
***************************************************************************/

int32 ArpIndexedNode::Index() const
{
	assert(false);	// Subclass should have overwritten me.
	return 0;
}

void* ArpIndexedNode::RawContents() const
{
	return rawData;
}

ArpIndexedNode* ArpIndexedNode::HeadNode() const
{
	if (prev == 0) return (ArpIndexedNode*)this;
	return prev->HeadNode();
}

ArpIndexedNode* ArpIndexedNode::TailNode() const
{
	if (next == 0) return (ArpIndexedNode*)this;
	return next->TailNode();
}


/***************************************************************************
* Public - MANIPULATION
***************************************************************************/
// Automtically sort the node to be added -- if it is less then
// me and I'm at the head, add it as the new head.  If it is greater than
// me and I'm the tail, add it as the new tail.  If it is less then me
// and >= my prev, add it as the new prev.  If it is greater than me
// and <= my next, add it as my new next.
// if our time is the same, it always goes behind me.
status_t ArpIndexedNode::AddNode(ArpIndexedNode *node)
{
	assert(node != 0);

	if (node->Index() < Index()) {
		if (prev != 0) { return prev->AddNode(node); }
		node->prev = 0;
		node->next = this;
		prev = node;
		return B_OK;
	}
	
	if ( (next == 0)
			|| (node->Index() <= next->Index()) )
		return AddNext(node);

	return next->AddNode(node);
}

ArpIndexedNode* ArpIndexedNode::RemoveNode()
{
	if (prev != 0) prev->next = next;
	if (next != 0) next->prev = prev;
	next = prev = 0;
	return this;
}

void ArpIndexedNode::DeleteListContents()
{
	if (rawData != 0) {
		pDeleteContents();
		rawData = 0;
	}
	if (next != 0) next->DeleteListContents();
}


/***************************************************************************
* Public - TESTING
***************************************************************************/

bool ArpIndexedNode::IsValid() const
{
	return(rawData != 0);
}


/***************************************************************************
* Public - UTILITY
***************************************************************************/

ArpIndexedNode* ArpIndexedNode::Copy()
{
	ArpIndexedNode	*copy = new ArpIndexedNode(0);
	if (copy == 0) return 0;
	if (CopyTo(copy) == false) return 0;
	return copy;
}

void ArpIndexedNode::PrintListUntil(ArpIndexedNode *node) const
{
	if (this == node) return;
	PrintContents();
	if (next == 0) return;
	next->PrintListUntil(node);
}

void ArpIndexedNode::PrintContents() const
{
	assert(false);	// Subclass should have overriden
}



/***************************************************************************
* Protected - ACCESSING
***************************************************************************/

void* ArpIndexedNode::RawHeadContents() const
{
	return HeadNode()->RawContents();
}

void* ArpIndexedNode::RawTailContents() const
{
	return TailNode()->RawContents();
}


/***************************************************************************
* Protected - MANIPULATION
***************************************************************************/

status_t ArpIndexedNode::AddNext(ArpIndexedNode *node)
{
	assert(node != 0);

	if (next != 0) next->prev = node;

	node->prev = this;
	node->next = next;
	next = node;
	return B_OK;
}

/***************************************************************************
* Protected - UTILITY
***************************************************************************/

bool ArpIndexedNode::CopyTo(ArpIndexedNode *copy)
{
	CopyContentsTo(copy);

	if (next != 0) {
		ArpIndexedNode	*nextCopy = ((ArpIndexedNode*)next)->Copy();
		if (nextCopy == 0) {
			copy->DeleteListContents();
			delete copy;
			return false;
		}
		copy->next = (ArpIndexedNode*)nextCopy;
		nextCopy->prev = (ArpIndexedNode*)copy;
	}
	return true;
}

void ArpIndexedNode::CopyContentsTo(ArpIndexedNode *copy)
{
	assert(false);	// Subclass should have overriden me  to do something
}


/***************************************************************************
* Private - MANIPULATION
***************************************************************************/

// FUCK FUCK FUCK!  The subclasses need to cast the data properly in order to
// delete it.  Damn Be for getting rid of the fucking BObject!
void ArpIndexedNode::pDeleteContents()
{
	assert(false);	// Subclasses should override to delete the data.
//		delete data;
}

