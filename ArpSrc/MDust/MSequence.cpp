/* MSequence.cpp
 */
#define _BUILDING_AmKernel 1

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "MDust/MSequence.h"

/***************************************************************************
 * M-SEQUENCE
***************************************************************************/
MSequence::MSequence(int32 chainIndexesArg)
		: MIndexedList(chainIndexesArg)
{
}

MSequence::~MSequence()
{
}

status_t MSequence::Head(ArpMidiEvent** event)
{
	ArpMidiNode*	node;
	if ((node = dynamic_cast<ArpMidiNode*>(HeadNode())) == NULL) return B_ERROR;
	if (node->Event() == NULL) return B_ERROR;
	*event = node->Event();
	return B_OK;
}

status_t MSequence::Tail(ArpMidiEvent** event)
{
	ArpMidiNode*	node;
	if ((node = dynamic_cast<ArpMidiNode*>(TailNode())) == NULL) return B_ERROR;
	if (node->Event() == NULL) return B_ERROR;
	*event = node->Event();
	return B_OK;
}

status_t MSequence::Add(ArpMidiEvent* eventToAdd)
{
	ArpMidiNode*	node;
	node = new ArpMidiNode(eventToAdd);
	if (node == 0) return B_NO_MEMORY;
	if (AddNode(node) != B_OK) {
		delete node;
		return B_ERROR;
	}
	return B_OK;
}

status_t MSequence::Remove(ArpMidiEvent* eventToRemove)
{
	assert(eventToRemove != 0);
	ArpMidiNode*	node;
	if (NodeFor(eventToRemove->Time(), eventToRemove, (ArpIndexedNode**)&node) != B_OK)
		return B_ERROR;

	return DeleteNode(node);
}

status_t MSequence::RemoveHead(ArpMidiEvent** removedEvent)
{
	ArpMidiNode*	node;
	status_t		answer;
	if ((answer = RemoveHeadNode((ArpIndexedNode**)&node)) != B_OK) return answer;
	*removedEvent = node->Event();
	delete node;
	return B_OK;
}

status_t MSequence::RemoveTail(ArpMidiEvent** removedEvent)
{
	ArpMidiNode*	node;
	status_t		answer;
	if ((answer = RemoveTailNode((ArpIndexedNode**)&node)) != B_OK) return answer;
	*removedEvent = node->Event();
	delete node;
	return B_OK;
}

status_t MSequence::RemoveAt(ArpMidiT time, ArpMidiEvent** removedEvent)
{
	ArpMidiNode*	node = (ArpMidiNode*)HeadNode();
	while (node != 0) {
		if (node->Index() == time) {
			*removedEvent = node->Event();
			return DeleteNode(node);
		}
		node = (ArpMidiNode*)node->next;
	}
	return B_ERROR;
}

status_t MSequence::Merge(MSequence* sequence)
{
	assert(sequence != 0);
	ArpMidiEvent*	event;
	while ( sequence->RemoveTail(&event) == B_OK )
		Add(event);
	return B_OK;
}

status_t MSequence::SetTime(ArpMidiEvent* event, ArpMidiT newTime)
{
	assert(event != 0);
	ArpMidiNode*	node;
	if (NodeFor(event->Time(), event, (ArpIndexedNode**)&node) != B_NO_ERROR)
		return B_ERROR;
	RemoveNode(node);
	event->SetTime(newTime);
	AddNode(node);
	return B_NO_ERROR;
}
