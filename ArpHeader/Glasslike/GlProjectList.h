/* GlProjectList.h
 * Copyright (c)2003 by Eric Hackborn.
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
 *	-� None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.08.24				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLPATHDATA_H
#define GLASSLIKE_GLPATHDATA_H

#include <ArpCore/StlVector.h>
#include <GlPublic/GlRootRef.h>
class GlChain;
class GlProjectList;
class _GlProjectEntry;

typedef		void*		gl_path_node_id;

/***************************************************************************
 * GL-PATH-NODE
 * A node represents a single GlNode, along with -- optionally -- a
 * chain in that node.
 ***************************************************************************/
class GlPathNode
{
public:
	GlPathNode();
	GlPathNode(int32 editor);
	~GlPathNode();

	GlPathNode&	operator=(const GlPathNode& node);

	gl_path_node_id		Id() const;
	bool				IsRoot() const;

protected:
	friend class GlProjectList;
	friend class GlMainPathView;

	int32				editor;
	gl_node_id			mNid;
	gl_chain_id			mCid;
	uint32				mChains;

public:
	void				Print(uint32 tabs = 0) const;
};

/***************************************************************************
 * GL-PROJECT-LIST
 * Store a list of root refs.  I have a context -- there's a current
 * ref, and a path for it.
 ***************************************************************************/
class GlProjectList
{
public:
	GlProjectList();
	~GlProjectList();

	GlRootRef					At(uint32 index) const;
	GlRootRef					At(gl_id id) const;
	/* Answer the number of roots.
	 */
	uint32						Size() const;
	
	GlRootRef					Current() const;
	status_t					SetCurrent(gl_id id);
	
	/* Set the cid for the path node with the given nid.  If no node,
	 * return error.
	 */
	status_t					SetChain(gl_node_id nid, gl_chain_id cid);

	GlChain*					Tail(GlRootNode* root) const;
	const GlChain*				Tail(const GlRootNode* root) const;
	status_t					GetTail(gl_node_id* outNid, gl_chain_id* outCid) const;
	
	/* Add it to the list if it doesn't exist.
	 */
	status_t					SetRef(const GlRootRef& ref);
	status_t					AddRef(const GlRootRef& ref);

	status_t					AddNode(gl_node_id nid, gl_chain_id cid = 0, uint32 chains = 0);

	/* Answer the size of the path for the current root.
	 */
	uint32						PathSize() const;
	status_t					Unset();
	/* Pop off the tail of the path.  I pop until I hit a node
	 * that has an editor, or the first node (which should always
	 * be the Grid node).
	 */
	status_t					Pop();
	status_t					PopTo(uint32 size);
	/* Pop everyone after the node matching the supplied id.
	 */
	status_t					PopTo(gl_path_node_id pid);

	status_t					MakeEmpty();

protected:
	std::vector<GlPathNode*>			mNodes;
	// This is just for the editor, and only the editor sets it to false.
	bool						mDirty;
	
	status_t					Truncate(uint32 index);

	status_t					AddEntry(const GlRootRef& ref);
	status_t					AddNode(GlPathNode* node);
	
private:
	int32						mCurrent;
	std::vector<_GlProjectEntry*>	mList;

	void						FreeList();
	void						FreePath();
	
public:
	void						Print(uint32 tabs = 0) const;
};

#endif
