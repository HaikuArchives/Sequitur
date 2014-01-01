/* MParticle.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 04.14.00		hackborn
 * Created this file.
 */

#ifndef MDUST_MPARTICLE_H
#define MDUST_MPARTICLE_H

#include <be/opengl/GL/glu.h>
#include "MDustPublic/MParticleI.h"
#include "MDust/MCube.h"
//#include "MDust/MPoint.h"

/***************************************************************************
 * M-PARTICLE
 * This is the superclass for all types of particles in the MDust system.
 * Particles come in two flavours: planets and stars.  Planets produce
 * MIDI and / or audio data, stars, process MIDI and / or audio data.
 * All types of particles know how to draw themselves appropriately.
 ***************************************************************************/
class MParticle : public MParticleI
{
public:
	MParticle(	float startX,
				float startY,
				float startZ,
				float mass);
	virtual ~MParticle();

	virtual void SetFriction(float friction);

	virtual bool IsPlanet() const		{ return false; }
	virtual bool IsStar() const			{ return false; }

	virtual float DistanceTo(MParticleI* particle) const;

	virtual float X() const;
	virtual float Y() const;
	virtual float Z() const;
	virtual float Mass() const;
	virtual float AttractionStrength() const;
	virtual float DesiredDistance() const;

	virtual MPoint Pos() const;
	virtual void Cycle(MSpaceI& space);
	void Birth();

	virtual void Draw(	BView* view,
						float halfWidth,
						float halfHeight);

protected:
	static bool Walls;

	/* My coordinates in the current frame.
	 */
	MPoint		mPos;
	/* The quadric shape that describes me.
	 */
	GLUquadric*	mQuadric;

	/* -------- My particle properties --------
	 */
	/* The current rate at which I'm traveling.
	 */
	MPoint		mSpeed;
	/* My current velocity.
	 */
	MPoint		mVelocity;
	float		mFriction;
	float		mMass;
	float		mAttractionStrength;
	float		mDesiredDistance;
	/* -------- My display properties --------
	 */
	float		mSize;
	rgb_color	mColor;
};

#endif
