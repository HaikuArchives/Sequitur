/* GlCache1d.h
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
 * 2003.08.04				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLCACHE1D_H
#define GLPUBLIC_GLCACHE1D_H

#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlRect.h>
class GlImage;

/***************************************************************************
 * GL-CACHE-1D
 * I cache the state of a single 1D.  I have a lot of different conditions
 * I vary based on -- if the algo is MORPHING_F, whether the client wanted
 * to cache all frames or do them one at a time, and other things.  But
 * the interaction model is fairly simple:
 *
 * In the Init(), pass in the size of the cache and the number of
 * frames to cache.  The number of frames will be ignored if the algo is
 * not a MORPHING_F or RANDOM_F.  If you don't want to cache multiple
 * frames regardless of the properties, pass in 0 for frames.
 *
 * The, call SetStep() for each step, if you have anything that qualifies
 * as a step.
 ***************************************************************************/
class GlCache1d
{
public:
	GlCache1d();
	GlCache1d(const GlCache1d& o);
	~GlCache1d();

	float*			n;
	uint32			w, h;		// Dimensions of n -- if either value
								// is 0 then n is unallocated

	GlCache1d*		Clone() const;

	/* PERFORMING
	   ---------- */
	/* Begin a new cache.  Frames is the number of frames to make if
	 * this is a morphing algo.  You don't have to check -- if it's
	 * not morphing, this value is ignored.  If it is morphing and the
	 * value is 0, then it will only make one frame at a time.
	 */
	status_t		Init(	GlAlgo1d* algo, uint32 size, uint32 frames = 0,
							uint32 flags = GlAlgo1d::INIT_LINE_F);
	/* Set the current step and rerun the cache.  You should always
	 * call this method.  It will be ignored if you supplied multiple
	 * frames in the Init(), or this isn't a morphing algo.
	 */
	status_t		SetStep(GlAlgo1d* algo, float step);

	/* ACCESSING
	   --------- */
	/* Answer a value from my array as if the array had a normalized
	 * size of 1.  The coords must be between 0 and 1.  The first
	 * variant answers as if the h is 1, the second variant uses
	 * the height info, as well.  The second variant is necessary
	 * if a value > 0 was supplied to frames in the init.
	 */
	float			At(float x) const;
	float			At(float x, float y) const;

	/* UTILITIES
	   --------- */
	/* Render myself to the supplied area.  These methods assume
	 * the image has been cleared to some blank state.  The alpha
	 * can be set to a value less than 255 to render as a transparent
	 * overlay.
	 */
	status_t		Render(	GlAlgo1d* a1d, GlImage* image, GlRect algo,
							GlRect area, uint8 alpha = 255,
							uint32 flags = GlAlgo1d::INIT_LINE_F);
	status_t		Render(	GlAlgo1d* a1d, uint8* r, uint8* g, uint8* b, uint8* a,
							int32 w, int32 h, GlRect algo, GlRect area,
							uint8 alpha = 255, uint32 flags = GlAlgo1d::INIT_LINE_F);

private:
	uint32			mProperties;
	gl_id			mOwner;		// Just for debugging -- make sure
								// a client doesn't switch algos on me,
								// although there's no real reason why
								// you couldn't

	float			Interpolate(float x, uint32 frame = 0) const;

	void			Free();

	float			NextV(int32 x, int32 areaW) const;
};


#endif
