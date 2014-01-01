/* MIndexedList.h
 * Copyright (c)2000 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 04.27.00		hackborn
 * Created this file
 */

#ifndef MDUST_MINDEXEDLIST_H
#define MDUST_MINDEXEDLIST_H

#ifndef ARPCOLLECTIONS_ARPINDEXEDLIST_H
#include <ArpCollections/ArpIndexedList.h>
#endif

/***************************************************************************
 * M-INDEXED-LIST
 * This is just the ArpIndexedList with the locking removed.
 ***************************************************************************/
class MIndexedList {
	public:
		MIndexedList(int32 chainIndexesArg = ARP_DEFAULT_CHAIN_INDEXES);
		virtual ~MIndexedList();

		// CHAIN ACCESSING
		ArpIndexedNode* HeadNode() const;
		ArpIndexedNode* TailNode() const			{ return tailNode; }
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
		bool IsEmpty() const;
		
		// UTILITY
		virtual MIndexedList* Copy();
		virtual void CopyTo(MIndexedList *copy);
		virtual void ReplaceWith(MIndexedList *ail);
		void Print();
		// Sometimes the tail node will get out of sync.  The following can
		// be used in those situations to sync it back up.
		void SyncTailNode();

		virtual int32 HashForIndex(int32 index);		// Answer the proper index into the
														// chain array based on the supplied time

	protected:
		// These should be private, this is a convenience for the file IO.
		ArpIndexedNode	**chain;	// This dynamic array stores midi events
									// in spaced intervals -- the first element
									// will have 'next' elements from 00 to
									// nn, the next element will have events
									// from nn+1 to mm, etc.  When we need to
									// add higher events, we grow the chain.
		long		count;			// the size of the chain

	private:
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

#endif