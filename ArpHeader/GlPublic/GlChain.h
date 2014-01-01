/* GlChain.h
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
 * 2004.01.28			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLCHAIN_H
#define GLPUBLIC_GLCHAIN_H

#include <ArpCore/String16.h>
#include <ArpSupport/ArpSafeDelete.h>
#include <GlPublic/GlDefs.h>
class GlChainPrivate;
class GlNode;
class GlNodeAction;
class GlNodeDataList;
class GlNodeReaderWriter;
class GlParamListI;
class GlAlgo;
class _GlStrainList;

/***************************************************************************
 * GL-CHAIN
 * Store a list of nodes.
 ***************************************************************************/
class GlChain : public ArpSafeDelete
{
public:
	GlChain(int32 key, uint32 io, const BString16* label, GlNode* parent,
			int32 dynamic = 0);
	GlChain(const BMessage* config);
	GlChain(const GlChain& o);

	status_t				Status() const;
	GlChain*				Clone() const;

	int32					Key() const;
	gl_chain_id				Id() const;
	uint32					Io() const;
	BString16				Label(bool appendDynamicCount = false) const;
	const GlNode*			Parent() const;
	int32					Dynamic() const;
	void					SetDynamicCount(uint32 dynCount);
	
	uint32					NodeCount() const;	
	const GlNode*			NodeAt(uint32 index) const;
	GlNode*					NodeAt(uint32 index);
	
	const GlChain*			FindChain(gl_chain_id id) const;
	GlChain*				FindChain(gl_chain_id id);
	const GlNode*			FindNode(gl_node_id nid, bool recurse = true) const;
	GlNode*					FindNode(gl_node_id nid, bool recurse = true);

	status_t				AddNode(GlNode* node);
	status_t				RemoveNode(gl_node_id nid);
	/* Insert the node before the given index -- if index is < 0,
	 * insert at the tail.
	 */
	status_t				InsertNode(GlNode* node, int32 index = -1);
	status_t				ReplaceNode(GlNode* node, int32 index);
	void					MakeEmpty();
	/* Delete all my nodes.
	 */
	status_t				DeleteNodes();
	
	int32					Walk(GlNodeAction& action, bool recurse = true);
	int32					Walk(GlNodeAction& action, bool recurse = true) const;
	
	status_t				StrainParams(_GlStrainList* strainer) const;
	/* This message is bubbled up from the node that actually had the param
	 * changed, so that node might not be contained directly in me.
	 */
	status_t				ParamChanged(gl_param_key key);
	status_t				ChainChanged(gl_chain_id id, int32 dynamic);	

	GlAlgo*					Generate(const gl_generate_args& args) const;

	status_t				ReadFrom(const BMessage& config);
	status_t				WriteTo(BMessage& config) const;

	status_t				ReadFrom(const BMessage& config, GlNodeReaderWriter& rw,
									bool recurse);
	status_t				WriteTo(BMessage& config, const GlNodeReaderWriter& rw,
									bool recurse) const;

protected:
	friend class GlAbstractNode;
	BString16				mLabel;
	
	virtual ~GlChain();
	
	void					SetParent(GlNode* node = 0);

private:
	GlChainPrivate*			mData;
	status_t				mStatus;
	int32					mKey;
	int32					mIo;
	/* I like having a separate node for the root behaviour, but it does
	 * make this parent thing a little lame -- if I were to use an abstract
	 * node, then you wouldn't have access to the params, which is important.
	 * So right now, chains owned by the root have no parent, instead of the
	 * root.  If I ever fold GlNode back into AbstractNode and subclass root
	 * from that -- a definite possibility -- this would clear up.
	 */
	GlNode*					mParent;
	/* Chains can be given a dynamic group (the default, 0, is just
	 * the static group).  When dynamic, the parent node will add or delete
	 * chains based on whether there are empty dynamic chains.  For example,
	 * if a chain is specified as dynamic, then the parent will require that
	 * there is always at least one empty chain with the same properties.
	 */
	int32					mDynamic;
	uint32					mDynamicCount;
	
public:
	void					Print(uint32 tabs = 0) const;
};

#endif
