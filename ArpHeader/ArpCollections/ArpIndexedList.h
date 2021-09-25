/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpIndexedList.h
 *
 * This class represents a list whose contents can be efficiently accessed.
 * The list stores subclasses of ArpIndexedNode -- node subclasses are responsible
 * for implementing Index() and returning a value.  This list class takes the Index
 * and hashes it into a chain.  Each chain will contain up to n indexes (set by the
 * constructor).
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• The list does not shrink as events are removed.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 11.01.98		hackborn
 * The function NodeAt(), when passed ARP_BACKWARDS, calls NodeBefore().  This
 * function seeks backwards through the list to find the first node before the time
 * supplied.  For some reason, I wrote it to respond with an error if the time
 * supplied is greater than the end of the chain.  This doesn't make sense to me
 * upon using it -- clients shouldn't need to know if a time passed in goes beyond
 * the end of the list, that situation would just be an efficient way of getting
 * the tail node.  So I've changed this to reassign an end-of-the-list time when
 * the supplied value is too large.
 *
 * 09.05.98		hackborn
 * Created this file
 */

#ifndef ARPCOLLECTIONS_ARPINDEXEDLIST_H
#define ARPCOLLECTIONS_ARPINDEXEDLIST_H

#ifndef _LOCKER_H
#include <Locker.h>
#endif

#ifndef ARPCOLLECTIONS_ARPINDEXEDNODE_H
#include <ArpCollections/ArpIndexedNode.h>
#endif

//#include "Synchronization.h"


#define ARP_FORWARDS		(1)
#define ARP_BACKWARDS		(2)

// The default number of indexes that we allow in a single chain.
// See the description for the chainIndexes variable.
#define ARP_DEFAULT_CHAIN_INDEXES			(40)


class ArpIndexedList {
	friend class ArpIndexedListAccess;

	public:
		ArpIndexedList(int32 chainIndexesArg = ARP_DEFAULT_CHAIN_INDEXES);
		virtual ~ArpIndexedList();

	protected:

		// CHAIN ACCESSING
		ArpIndexedNode* HeadNode();
		ArpIndexedNode* TailNode()				{ return tailNode; }
		virtual status_t ChainHeadFrom(int32 index, ArpIndexedNode **node);
		// Answer the index at the chain head for the supplied index.  B_ERROR is
		// returned if the index isn't in the chain.
		int32 IndexHeadFrom(int32 index);
		status_t NodeAt(int32 index, ArpIndexedNode **node,
						int32 findNode = 0, int32 direction = ARP_FORWARDS);
		status_t NodeFor(int32 index, void *rawData, ArpIndexedNode **answer);
		int32 Count();
		int32 ChainIndexes();

		// CHAIN MANIPULATION
		status_t AddNode(ArpIndexedNode *node);
		status_t RemoveNode(ArpIndexedNode *node);
		status_t DeleteNode(ArpIndexedNode *node);
		status_t RemoveHeadNode(ArpIndexedNode **removedNode);
		status_t RemoveTailNode(ArpIndexedNode **removedNode);

		virtual void AddedNode(ArpIndexedNode *node);
		/* The following two methods are notification that a node is about to
		 * be removed, and that it has been removed, respectively.  Subclasses
		 * overriding RemovingNode() must make sure to call the superclass method.
		 */
		virtual void RemovingNode(ArpIndexedNode *node);
		virtual void RemovedNode(ArpIndexedNode *node)		{ }
		void GrowToAtLeast(int32 countArg);		// Increase the size of chain to at least
										// the supplied value.
		void DeleteOnlyNodes();
		void DeleteNodeContents();

		// TESTING
		bool Includes(int32 index, void *rawData);

		// UTILITY
		virtual ArpIndexedList* Copy();
		virtual void CopyTo(ArpIndexedList *copy);
		virtual void ReplaceWith(ArpIndexedList *ail);
		void Print();
		// Sometimes the tail node will get out of sync.  The following can
		// be used in those situations to sync it back up.
		void SyncTailNode();
// Probably need these, have to wait and see.
//		void VerifyPosition(MidiEvent*);
//		void VerifyListPosition(MidiAbstractList*);

		virtual int32 HashForIndex(int32 index);		// Answer the proper index into the
														// chain array based on the supplied time

		// FILE IO
//		void KeepChainsConsistent(MidiListNode*);

		// These should be private, this is a convenience for the file IO.
		ArpIndexedNode	**chain;	// This dynamic array stores midi events
									// in spaced intervals -- the first element
									// will have 'next' elements from 00 to
									// nn, the next element will have events
									// from nn+1 to mm, etc.  When we need to
									// add higher events, we grow the chain.
		int32		count;			// the size of the chain

	private:
//		ReadWriteLock	chainLock;
		int32			chainIndexes;	// The number of indexes we allow between chains.
										// Note the word 'index' instead of 'node.' 
										// Multiple nodes can share the same index, so the
										// actual chain size might be larger.

		ArpIndexedNode	*tmpE;		// This is just a place to hold on to a tmp
									// event without always creating a new one.
									// Originally for use for Next(), so a new
									// event for comparison isn't allocated with
									// every call to that function.  NOTE: there
									// is never any guarantee what may be in this
									// event.  Functions should ALWAYS set before
									// using.

		ArpIndexedNode	*tailNode;	// This does NOT CURRENTLY represent the tail
									// node -- I just added it as a quick hack
									// for the MidiReader.  Don't use it!

		// CHAIN ACCESSING
		status_t NodeBefore(int32 index, ArpIndexedNode **answer,
						int32 findNode);
		status_t NodeAfter(int32 index, ArpIndexedNode **answer,
						int32 findNode);

		// CHAIN MANIPULATION
		status_t pAddNode(ArpIndexedNode *node);
		status_t pRemoveNode(ArpIndexedNode *node);
		void pAddNodeAtChainHead(ArpIndexedNode *node, int32 hash);
		status_t pRemoveChainHeadAt(int32 hash, ArpIndexedNode **node);
		void pFreeChain();

		// TESTING
		bool HashExists(int32 hash);

		// UTILITY
		ArpIndexedNode* NextChainHeadFrom(int32 hash);
};

class ArpIndexedListAccess {
	public:
		enum AccessMode { AccessRead, AccessWrite };
		
		ArpIndexedListAccess(ArpIndexedList& list, AccessMode=AccessWrite);
		virtual ~ArpIndexedListAccess();

		virtual void DeleteList();

		// CHAIN ACCESSING
		ArpIndexedNode* HeadNode() 				{ return List()->HeadNode(); }
		ArpIndexedNode* TailNode()				{ return List()->TailNode(); }
		status_t ChainHeadFrom(int32 index, ArpIndexedNode **node)
				{ return(List()->ChainHeadFrom(index, node)); }
		// Answer the index at the chain head for the supplied index.  B_ERROR is
		// returned if the index isn't in the chain.
		int32 IndexHeadFrom(int32 index)		{ return List()->IndexHeadFrom(index); }
		status_t NodeAt(int32 index, ArpIndexedNode **node,
						int32 findNode = 0, int32 direction = ARP_FORWARDS)
				{ return(List()->NodeAt(index, node, findNode, direction)); }
		status_t NodeFor(int32 index, void *rawData, ArpIndexedNode **answer)
				{ return(List()->NodeFor(index, rawData, answer)); }
		int32 Count() { return(List()->Count()); }
		
		// CHAIN MANIPULATION
		status_t AddNode(ArpIndexedNode *node)		{ return List()->AddNode(node); }
		status_t RemoveNode(ArpIndexedNode *node)	{ return List()->RemoveNode(node); }
		status_t DeleteNode(ArpIndexedNode *node)	{ return List()->DeleteNode(node); }
		void DeleteOnlyNodes()						{ List()->DeleteOnlyNodes(); }
		void DeleteNodeContents()					{ List()->DeleteNodeContents(); }

		// TESTING
		bool Includes(int32 index, void *rawData)	{ return List()->Includes(index, rawData); }

		// UTILITY
		ArpIndexedList* Copy()					{ return List()->Copy(); }
		void ReplaceWith(ArpIndexedList *ail)	{ List()->ReplaceWith(ail); }
		void Print()							{ List()->Print(); }
		virtual int32 HashForIndex(int32 index)	{ return List()->HashForIndex(index); }
		
	protected:
		ArpIndexedList& list;
		AccessMode mode;

		virtual ArpIndexedList* List();
		
//		ReadLockEntry* reader;
//		WriteLockEntry* writer;
};


#endif /* ARP_INDEXED_LIST_H */
