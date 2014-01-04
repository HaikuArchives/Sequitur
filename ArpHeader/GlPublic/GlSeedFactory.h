/* GlSeedFactory.h
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.03.19			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLSEEDFACTORY_H
#define GLPUBLIC_GLSEEDFACTORY_H

#include <support/SupportDefs.h>
class ArpPoint3d;
class GlAlgo2d;
class GlPlanes;

/***************************************************************************
 * GL-ABSTRACT-SEED-FACTORY
 * I am a convenience class for those nodes that use surfaces to seed their
 * input.  Given a surface, I run it and pick pixels from the resulting
 * mask that can be used as seeds.
 *
 * Subclasses are responsible for implementing a NewSeeds() method for
 * allocating a new seed collection, and SetSeed() for each seed I pick.
 ****************************************************************************/
class GlAbstractSeedFactory
{
public:
	GlAbstractSeedFactory(int32 maxSeeds, uint8 low = 1, uint8 high = 255);

	status_t			MakeSeeds(const GlPlanes& pixels, GlAlgo2d* s);

	virtual status_t	NewSeeds(uint32 count) = 0;
	virtual void		SetSeed(uint32 index, float x, float y, uint8 val) = 0;

protected:
	int32				mMaxSeeds;
	uint8				mLow, mHigh;

private:
	/* Seeds are based on the distribution of values between low and high.
	 */
	status_t MakeDistributedSeeds(const uint8* mask, int32 w, int32 h);
	/* The simplest method, seeds are just the first mMaxSeeds that
	 * are bewtween low and high.
	 */
	status_t MakeAdjacentSeeds(const uint8* mask, int32 w, int32 h);
};

/***************************************************************************
 * GL-SEED-FACTORY
 * A simple utility for generating seeds for clients that just need the
 * basic info -- x, y, z.  I'm stored in an ArpPoint3d.
 ****************************************************************************/
class GlSeedFactory : public GlAbstractSeedFactory
{
public:
	uint32				size;
	ArpPoint3d*			seeds;

	GlSeedFactory(int32 maxSeeds, uint8 low = 1, uint8 high = 255);
	virtual ~GlSeedFactory();
	
	virtual status_t	NewSeeds(uint32 count);
	virtual void		SetSeed(uint32 index, float x, float y, uint8 val);

private:
	typedef GlAbstractSeedFactory	inherited;
};

#endif
