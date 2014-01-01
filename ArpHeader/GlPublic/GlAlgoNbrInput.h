/* GlAlgoNbrInput.h
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
 * 2004.04.23				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLALGONBRINPUT_H
#define GLPUBLIC_GLALGONBRINPUT_H

#include <ArpCore/String16.h>
#include <GlPublic/GlAlgoNbr.h>
class _GlAlgoNbrInputData;
class GlArrayF;

/*************************************************************************
 * GL-ALGO-NBR-IN
 * I am a common object for storing a list of values that I set
 * as the result of the process.
 *************************************************************************/
class GlAlgoNbrIn : public GlAlgoNbr
{
public:
	BString16				key;
	/* I do not own the values, so my client must know enough
	 * not to delete me before it deletes the values.
	 */
	GlArrayF*				values;
	/* There are essentially two systems, for two kinds of
	 * clients.  Some clients might have a static values array,
	 * so they just use SetValues() in the GlAlgoNbrInList to
	 * set this.  Other clients might change the value list.
	 * They use SetIndex() so they can identify the inputs easily.
	 */
	int32					index;
	
	GlAlgoNbrIn(int32 key, gl_node_id nid, const BString16& sKey, int32 token = GL_NO_TOKEN);
	GlAlgoNbrIn(const GlAlgoNbrIn& o);

	virtual GlAlgoNbrIn*	AsInput();

	virtual status_t		Process(GlArrayF& set);

private:
	typedef GlAlgoNbr		inherited;
};

/*************************************************************************
 * GL-ALGO-NBR-IN-LIST
 * A convenience class for those clients that work with ins.
 *************************************************************************/
class GlAlgoNbrInList
{
public:
	GlAlgoNbrIn**			ins;
	uint32					size;
	
	GlAlgoNbrInList();
	~GlAlgoNbrInList();

	status_t				Add(GlAlgoNbrIn* a);
	void					MakeEmpty();

	/* For each input, see if its key matches any of the keys
	 * (const char*) supplied.  If so, assign it the values
	 * or the index, respectively.  A typical call would look like
	 *		SetValue(anArray, 2, "width", "w");
	 */
	void					SetValues(GlArrayF* values, uint32 keyCount, ...);
	void					SetIndex(uint32 index, uint32 keyCount, ...);
};

#endif
