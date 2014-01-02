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
 * ArpIndexedNode.h
 *
 * This class represents one node in an ArpIndexedList.  It is an
 * abstract class:  Subclasses are responsible for answering an
 * actual index.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.05.98		hackborn
 * Created this file
 */

#ifndef ARPCOLLECTIONS_ARPINDEXEDNODE_H
#define ARPCOLLECTIONS_ARPINDEXEDNODE_H

#ifndef _SUPPORT_DEFS_H
#include <support/SupportDefs.h>
#endif

class ArpIndexedNode
{
public:
	ArpIndexedNode(void *dataArg);
	virtual ~ArpIndexedNode();
	
	ArpIndexedNode		*prev, *next;

	// ACCESSING
	virtual int32 Index() const;
	void* RawContents() const;
	ArpIndexedNode* HeadNode() const;
	ArpIndexedNode* TailNode() const;

	// MANIPULATION
	status_t AddNode(ArpIndexedNode *node);
	ArpIndexedNode* RemoveNode();
	void DeleteListContents();
//	void DeleteContents();

	// TESTING
	virtual bool IsValid() const;

	// UTILITY
	virtual ArpIndexedNode* Copy();
	void PrintListUntil(ArpIndexedNode *node) const;
	virtual void PrintContents() const;

protected:
	void			*rawData;

	// ACCESSING
	void* RawHeadContents() const;
	void* RawTailContents() const;

	// MANIPULATION
	status_t AddNext(ArpIndexedNode *node);

	// UTILITY
	virtual bool CopyTo(ArpIndexedNode *copy);
	virtual void CopyContentsTo(ArpIndexedNode *copy);

private:
	virtual void pDeleteContents();
};

#endif 