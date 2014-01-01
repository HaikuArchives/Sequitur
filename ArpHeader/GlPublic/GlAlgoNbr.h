/* GlAlgoNbr.h
 * Copyright (c)2003-2004 by Eric Hackborn.
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
 * 2003.03.28				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLALGONBR_H
#define GLPUBLIC_GLALGONBR_H

#include <GlPublic/GlAlgo.h>
class GlAlgoNbrIn;
class GlAlgoNbrInList;
class GlArrayF;

/*******************************************************
 * GL-ALGO-NBR
 * Take a number set and transform it.
 *******************************************************/
class GlAlgoNbr : public GlAlgo
{
public:
	virtual ~GlAlgoNbr();

	/* ACCESSING
	   ---------- */
	virtual uint32				Io() const		{ return GL_NUMBER_IO; }

	/* CONVERTING
	   --------- */
	virtual GlAlgoNbr*			AsNbr();
	virtual const GlAlgoNbr*	AsNbr() const;

	virtual GlAlgoNbrIn*		AsInput();	

	/* A unique value for the algo subclass.
	 */
	int32						Key() const;

	/* Run v through myself and any subsequent curves and
	 * return the result.  Subsequent curves are subtractive.
	 */
	virtual status_t			Process(GlArrayF& set) = 0;
	
protected:
	GlAlgoNbr(int32 key, gl_node_id nid, int32 token = GL_NO_TOKEN);
	GlAlgoNbr(const GlAlgoNbr& o);
	
	int32						mKey;

private:
	typedef GlAlgo				inherited;
	
public:
	/* This function just adds the key to a master list of keys,
	 * answering true if successful and false if the key already
	 * exists.  Objects that create new algos should call this
	 * in an assert -- it's just to verify that the same int32
	 * isn't used twice during development, after development it
	 * doesn't matter.
	 */
	static bool					RegisterKey(int32 key);
};

/*******************************************************
 * GL-ALGO-NBR-WRAP
 * Perform operations on algo nbr chains.  I DO NOT own
 * the algo I'm assigned -- I do cache it, BUT I DON'T
 * PERFORM ANY SORT OF LOCK, SO IT MUST EXIST AS LONG
 * AS I DO.
 *******************************************************/
class GlAlgoNbrWrap
{
public:
	GlAlgoNbrWrap(GlAlgo* algo = 0);
	~GlAlgoNbrWrap();

	status_t					InitCheck() const;
	status_t					SetAlgo(GlAlgo* algo);		

	status_t					GetInputs(GlAlgoNbrInList& list);
	status_t					Process(GlArrayF& set);

private:
	GlAlgoNbr**					mCache;
	uint32						mSize;

	status_t					Cache(GlAlgo* algo);
	void						Free();

	status_t					GetInputs(GlAlgo* a, GlAlgoNbrInList& list);
};

#endif
