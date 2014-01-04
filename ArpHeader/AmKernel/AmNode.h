/* AmNode.h
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
 * 05.15.00		hackborn
 * Mutated this file from its original incarnation, renamed class,
 * added const's, changed the contents from an event to an event interface.
 *
 * 09.07.98		hackborn
 * Created this file
 */

#ifndef AMKERNEL_AMNODE_H
#define AMKERNEL_AMNODE_H

#include <support/SupportDefs.h>
#include "AmPublic/AmDefs.h"
class AmEvent;

/***************************************************************************
 * AM-NODE
 * This class represents one node that appears in an AmPhrase.
 ****************************************************************************/
class AmNode
{
public:
	AmNode( AmEvent *event );
	virtual ~AmNode();

	AmNode		*prev, *next;

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	AmTime			StartTime() const;
	AmTime			EndTime() const;
	AmEvent*		Event() const;

	AmNode* 		HeadNode() const;
	AmNode* 		TailNode() const;

	bool			IsValid() const;

	/* Answer the furthest node down my chain before the supplied
	 * time.  If my node and on are after time, answer 0.
	 */
	AmNode*			NodeBefore(AmTime time);

	/*---------------------------------------------------------
	 * MANIPULATION
	 *---------------------------------------------------------*/
	/* Automatically sort the added node, based on our Index() values.
	 * If our times are the same, it always goes behind me.
	 */
	status_t		AddNode(AmNode* node);
	AmNode*			RemoveNode();
	void			DeleteListContents();
	/* This does not guarantee that the sort order is correctly, it
	 * simple inserts node as the next node in the chain.
	 */
	status_t		InsertNext(AmNode* node);
	status_t		InsertPrev(AmNode* node);
	/* Replace from myself to end (which can be me, NULL, or
	 * something after me) with the replacement node.  If end
	 * is NULL, then replace me and everything after me with it.
	 */
	status_t		ReplaceRun(AmNode* end, AmNode* replacement);
	
	/*---------------------------------------------------------
	 * UTILITY
	 *---------------------------------------------------------*/
	/* Copy myself and all subsequent nodes, answer the copy of
	 * myself.
	 */
	AmNode*			Copy() const;
	void			Print() const;

protected:
	AmEvent*		mEvent;
	
	/*---------------------------------------------------------
	 * COPYING
	 *---------------------------------------------------------*/
	void			CopyContentsTo(AmNode* copy) const;
	void			SetEvent(AmEvent* event);

private:
	void			pDeleteContents();
};

#endif
