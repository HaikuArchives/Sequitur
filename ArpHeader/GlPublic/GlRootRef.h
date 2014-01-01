/* GlRootRef.h
 * Copyright (c)2002-2004 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.14.00		hackborn
 * Created this file
 */
#ifndef GLPUBLIC_GLROOTREF_H
#define GLPUBLIC_GLROOTREF_H

#include <be/app/Messenger.h>
#include <GlPublic/GlDefs.h>
class GlRootNode;

/***************************************************************************
 * GL-ROOT-REF
 * This class represents a reference to a single matrix object.
 ****************************************************************************/
class GlRootRef
{
public:
	GlRootRef();
	GlRootRef(const GlRootNode* node);
	GlRootRef(const GlRootRef& ref);
	virtual ~GlRootRef();

	GlRootRef&			operator=(const GlRootRef& ref);
	/* Set this object to the supplied track impl and answer the
	 * result of IsValid().
	 */
	bool				SetTo(const GlRootNode* node);
	bool				SetTo(const GlRootRef& ref);
	/* Answer true if this ref can create AmTrack objects for reading and
	 * writing.
	 */
	bool				IsValid() const;
	/* Answer a unique value for the song.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	gl_node_id			NodeId() const;
	/* A root has one and only one chain, so no locking is necessary.
	 */
	gl_chain_id			ChainId() const;
		
	/* LOCKING
	 * ------- */
	const GlRootNode*	ReadLock() const;
	void				ReadUnlock(const GlRootNode* matrix) const;
	GlRootNode*			WriteLock(const char* name = NULL);
	void				WriteUnlock(GlRootNode* matrix);

	/* NOTIFICATION
	 * ------------ */
	status_t			AddObserver(uint32 code, const BMessenger& m);
	status_t			RemoveObserver(uint32 code, const BMessenger& m);
	status_t			RemoveAll();
	status_t			ReportMsgChange(uint32 code, BMessage* msg, BMessenger sender);

private:
	GlRootNode*			mRoot;
};

#endif 
