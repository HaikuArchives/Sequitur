/* GlArrayF.h
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
 * 2004.04.16				hackborn@angryredplanet.com
 * Extracted from GlNumberSet, added support for Algo1d caching.
 *
 * 2003.03.28				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLARRAYF_H
#define GLPUBLIC_GLARRAYF_H

#include <support/SupportDefs.h>

/*******************************************************
 * GL-ARRAY-F
 * Store an array of floats.  This class has various
 * useful functions for different clients.
 *
 * WARNING:  The array of points is exposed for fast
 * manipulation, but CLIENTS SHOULD NEVER ALLOCATE OR
 * DEALLOCATE THE ARRAY DIRECTLY.  Use the various
 * memory methods for that.
 *******************************************************/
class GlArrayF
{
public:
	uint32			size;		// Size of the array
	float*			n;

	GlArrayF(uint32 size = 0);
	GlArrayF(const GlArrayF& o);
	~GlArrayF();

	GlArrayF&		operator=(const GlArrayF& o);
	GlArrayF*		Clone() const;

	/* Answer a value from my array as if that array
	 * had a normalized size of 1.  This means v must
	 * be between 0 and 1.
	 */
	float			At(float v) const;

	void			Sort();
	
	/* MEMORY MANAGEMENT.
	   ------------------ */
	 /* Use these methods to alter the size of the n array,
	  * never do that directly.
	  */
	status_t		Resize(uint32 newSize);
	status_t		Add(float n);

private:
	uint32			mAllocated;		// Allocated size of n

	void			FreeAll();
	
public:
	void			Print() const;
};

#endif
