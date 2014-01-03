/* GlPseudoRandomizer.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.02.21				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GL_PSEUDORANDOMIZER_H
#define GL_PSEUDORANDOMIZER_H

#include <kernel/OS.h>
#include <support/SupportDefs.h>
#include <ArpMath/ArpDefs.h>

/***************************************************************************
 * A class to make predictable random numbers avoiding
 * the rand() func, so nodes don't step on each other.
 *
 * You can either seed with 0, which will seed with a random number,
 * or you can seed with a non zero value, which creates reproducible results.
 ***************************************************************************/
class GlPseudoRandomizer
{
public:
	GlPseudoRandomizer(uint32 seed = 0, uint32 gen1 = 45653, uint32 max = 400000000)
	{
		Init(seed, gen1, max);
	}

	GlPseudoRandomizer(const GlPseudoRandomizer& o)
			: mGen1(o.mGen1), mGen2(o.mGen2), mMax(o.mMax), mSeed(o.mSeed)
	{
	}

	/* Answer 0 to 1.
	 */	
	float inline Next() const
	{
		mSeed = ((mGen1 * mSeed) * mGen2) % mMax;
		return float(mSeed) / mMax;
	}

	/* Answer -1 to 1.
	 */
	float inline NextOffset() const
	{
		mSeed = ((mGen1 * mSeed) * mGen2) % mMax;
		return (float(mSeed) / mMax) * 2 - 1;
	}

	void Init(uint32 seed = 0, uint32 gen1 = 45653, uint32 max = 400000000)
	{
		mGen1 = gen1;
		mGen2 = gen1 * 2;
		mSeed = seed;
		mMax = max;
		
		ArpASSERT(mGen1 != 0);
		if (mSeed == 0) {
//			srand(time(0));
ArpFINISH();
			srand(uint32(system_time()));
			mSeed = uint32(ARP_RAND * (max / 10));
			if (mSeed == 0) mSeed = 1;
		}
	}

private:
	uint32			mGen1, mGen2, mMax;
	mutable	uint32	mSeed;
};


#endif
