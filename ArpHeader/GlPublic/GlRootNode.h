/* GlRootNode.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2004.03.03				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLROOTNODE_H
#define GLPUBLIC_GLROOTNODE_H

#include <GlPublic/GlNode.h>
#include <GlPublic/GlNotifier.h>

#include <Locker.h>

/***************************************************************************
 * GL-ROOT-NODE
 * A special node that forms the root of a hierarchy.  Handles locking
 * and other unique details.
 ***************************************************************************/
class GlRootNode : public GlNode,
				   public GlNotifier
{
public:
	GlRootNode(const GlNodeAddOn* addon, int32 type);
	GlRootNode(const GlNodeAddOn* addon, const BMessage* config);
	GlRootNode(const GlRootNode& o);
	
	bool						ReadLock() const;
	bool						WriteLock(const char* name = NULL);
	bool						ReadUnlock() const;
	bool						WriteUnlock();

	GlNode*						Clone() const;

	virtual uint32				Io() const;
	BString16					Creator() const;
	void						SetCreator(const BString16& str);
	int32						Key() const;
	void						SetKey(int32 k);
	BString16					Category() const;
	void						SetCategory(const BString16& str);
	BString16					Label() const;
	void						SetLabel(const BString16& str);

	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
	virtual status_t			ParamChanged(gl_param_key key);
	virtual status_t			ChainChanged(gl_chain_id id, int32 dynamic = 0);

	/* Return false if I've never been assigned the given property.
	 */
	bool						HasCreator() const;
	bool						HasKey() const;
	bool						HasLabel() const;

	/* Clear out any changes I've accumulated WITHOUT
	 * sending out notification.
	 */
	void						ClearChanges();

private:
	typedef GlNode				inherited;
	mutable BLocker				mReadLock;
	mutable BLocker				mWriteLock;
	BMessage					mInfoChanges, mNodeChanges,
								mChainChanges;
	
	void						FlushChanges(bool send = true);
	
public:
	void						Print(uint32 tabs = 0) const;
};

/***************************************************************************
 * GL-ROOT-NODE-ADD-ON
 * This isn't used with any of the system nodes -- this is an alternative
 * addon for when the root is loaded from a file.
 ***************************************************************************/
class GlRootNodeAddOn : public GlNodeAddOn
{
public:
	GlRootNodeAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return 0; }

private:
	typedef GlNodeAddOn		inherited;
};

#endif
