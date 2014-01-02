/* GlFilterKernel.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.09.02				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLFILTERKERNEL_H
#define GLPUBLIC_GLFILTERKERNEL_H

#include <support/SupportDefs.h>

/*******************************************************
 * GL-FILTER-KERNEL
 * A simple array of values that can be used to process
 * images.  The array is always centered on a pixel, so
 * for example supplying sides of 1, 1, 1, 1 will produce
 * a kernel with 8 taps around the center pixel:
 *		x x x
 *		x P x
 *		x x x
 *******************************************************/
class GlFilterKernel
{
public:
	GlFilterKernel();
	GlFilterKernel(	uint32 left, uint32 top,
					uint32 right, uint32 bottom);
	GlFilterKernel(	const GlFilterKernel& o);
	virtual ~GlFilterKernel();

	GlFilterKernel*	Clone() const;
	status_t		InitCheck() const;
	
	int32			Left() const;
	int32			Top() const;
	int32			Right() const;
	int32			Bottom() const;
	int32			Width() const;
	int32			Height() const;
	int32			Size() const;

	float*			LockTaps(int32* width, int32* height);
	void			UnlockTaps(float* taps);

//	int32			XRange() const;
//	int32			YRange() const;
//	status_t		Set(int32 x, int32 y, float value);
//	status_t		Get(int32 x, int32 y, float* value);

	/* Values of 2, 2 would end it up with 25 taps, two
	 * on either side of the center.
	 */
	status_t		Init(	uint32 width, uint32 height);
	status_t		Init(	uint32 left, uint32 top,
							uint32 right, uint32 bottom);

private:
	status_t		mStatus;
	float*			mTaps;
	uint32			mL, mT, mR, mB;
		
	/* Debugging
	 */
public:
	void			Print() const;
};

#endif
