/* GlAlgo.h
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
 * 2004.03.09				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLALGO_H
#define GLPUBLIC_GLALGO_H

#include <be/support/SupportDefs.h>
#include <GlPublic/GlActions.h>
#include <GlPublic/GlDefs.h>
class GlAlgo1d;
class GlAlgo2d;
class GlAlgoIm;
class GlAlgoNbr;
class GlNode;
class GlNodeDataList;
class GlParamWrap;
class _GlAlgoData;

enum {
	GL_NO_TOKEN			= 0,
	GL_OPEN_PAREN		= 1,
	GL_CLOSE_PAREN		= 2,
	GL_UNARY			= 3,
	GL_BINARY			= 4
};

/*******************************************************
 * GL-ALGO
 * An object that can be placed in a parse tree.
 *******************************************************/
class GlAlgo
{
public:
	virtual ~GlAlgo();

	virtual GlAlgo*				Clone() const = 0;

	/* ACCESSING
	   --------- */
	/* Concrete subclasses must return an IO (GL_IMAGE_IO, etc.)
	 */
	virtual uint32				Io() const = 0;
	gl_node_id					NodeId() const;
	enum {
		REDIRECTOR_F			= 0x00000001	// This parse node redirects (typically
												// in the walk_tree).  Redirectors need
												// to implement UpdateSource().
	};
	virtual uint32				Flags() const;

	/* Step is between 0 and 1.  Implementors must answer true if they've
	 * changed, false otherwise.
	 */
	bool						SetStep(float v);

	/* Some subclasses know which of their member data correspond to
	 * their node params.  This is primarily support for realtime
	 * stuff, like recording.
	 */
	virtual status_t			SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t			GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

	status_t					AddTail(GlAlgo* tail);

	/* CONVERTING
	   --------- */
	/* These methods are essentially casts.  The issue is that any subclass
	 * can respond anyway they like for the Io(), so someone might respond
	 * as being a GL_1D_IO when they're not.  This is used to guide some of
	 * the tree walking behaviour, but there are other situations where the
	 * client needs to know if they are dealing with an appropriate subclass.
	 * Since casting is out due to the problem described, this is used as
	 * a sort of final check on the object's identity.  Note that no new
	 * instance is created, this is the same object, just cast.
	 */
	virtual GlAlgo1d*			As1d();
	virtual const GlAlgo1d*		As1d() const;
	virtual GlAlgo2d*			As2d();
	virtual const GlAlgo2d*		As2d() const;
	virtual GlAlgoIm*			AsIm();
	virtual const GlAlgoIm*		AsIm() const;
	virtual GlAlgoNbr*			AsNbr();
	virtual const GlAlgoNbr*	AsNbr() const;

	/* UTILITIES
	   --------- */
	status_t					PerformAll(GlNodeDataList& list, const gl_process_args* args);


	/* An IO of 0 is a wildcard -- every type of algo is walked.
	 * Otherwise only walk the algos for the specified type.
	 */
	int32						Walk(GlAlgoAction& action, uint32 io = 0);

	/* Anyone who sets the REDIRECTOR flag needs to implement this
	 * to install themselves on their source node.
	 */
	virtual void				UpdateSource(GlNode* node);

	/* PARSE TREE
	   --------- */

	int32						Token() const;
	float						Precedence() const;
	/* Do a parse of the list before performing -- this lets the
	 * algos arrange themselves appropriately.  Answer the new
	 * root of the tree.
	 */
	GlAlgo*						Parse();

	virtual status_t			AssignBinary(GlAlgo* lh, GlAlgo* rh);
	virtual status_t			AssignUnary(GlAlgo* v);

protected:
	friend class GlAlgo1d;
	friend class GlAlgo2d;
	friend class GlAlgoIm;
	friend class GlAlgoNbr;
	friend class GlAlgo1dWrap;
	friend class GlAlgo2dWrap;
	friend class GlAlgoNbrWrap;

	GlAlgo(gl_node_id nid, int32 token = GL_NO_TOKEN);
	GlAlgo(const GlAlgo& o);
	
	gl_node_id				mNid;
	int32					mToken;
	float					mPrecedence;
	GlAlgo*					mNext;

	virtual status_t		Perform(GlNodeDataList& list, const gl_process_args* args);

	/* Subclasses that are composed of chains of other data
	 * store it here.  You want to store it here rather than
	 * in some instance variable of the subclass so it can
	 * be included in the WalkTree().
	 */
	status_t				SetChain(GlAlgo* node, uint32 index);
	uint32					ChainSize() const;
// OBSOLETE: replaced with AlgoAt()
	GlAlgo*					ChainAt(uint32 index);
	const GlAlgo*			ChainAt(uint32 index) const;

	GlAlgo*					AlgoAt(uint32 index);
	const GlAlgo*			AlgoAt(uint32 index) const;
	/* Chain accessing conveniences 
	 */
	GlAlgo1d*				Algo1dAt(uint32 index);
	const GlAlgo1d*			Algo1dAt(uint32 index) const;
	GlAlgo2d*				Algo2dAt(uint32 index);
	const GlAlgo2d*			Algo2dAt(uint32 index) const;
		
	/* Step is between 0 and 1.  Implementors must answer true if they've
	 * changed, false otherwise.  If you've got any chains, make sure
	 * to call the inherited.
	 */
	virtual bool			set_step(float v);
	virtual int32			walk(GlAlgoAction& action, uint32 io);

	/* Subclasses implement _print() by displaying info with no
	 * adornment -- no tabs, spaces, newlines before or after.
	 * _print_additional() is for any additional info that requires
	 * its own line(s).
	 */
	virtual void			_print() const;
	virtual void			_print_additional(uint32 tabs) const;
	
private:
	_GlAlgoData*			mData;

	/* Parse tree construction
	 */
	GlAlgo*					ParseNext() const;
	status_t				ParseDetach();
	
public:
	void					Print(uint32 tabs = 0) const;
};

#endif
