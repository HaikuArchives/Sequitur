/* MParticle.cpp
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "MDust/MParticle.h"
#include "MDust/MSpace.h"

static uint8	red = 255;
static uint8	green = 0;
static uint8	blue = 0;
static uint8	color_delta = 30;

/***************************************************************************
 * M-PARTICLE
 ***************************************************************************/
MParticle::MParticle(	float startX,
						float startY,
						float startZ,
						float mass)
		: mQuadric(0), mMass(mass),
		mSize(4)
{
	Birth();
//	Walls = true;
	// Set myself for 'neon'
	mDesiredDistance = 0.005;
	mAttractionStrength = 0.0025;
	mMass = 100.0;
	mFriction = 0.99;
	mPos.x = startX;
	mPos.y = startY;
	mPos.z = startZ;

	if ((mQuadric = gluNewQuadric()) != 0) {
		gluQuadricDrawStyle(mQuadric, GLU_FILL);
		gluQuadricNormals(mQuadric, GLU_SMOOTH);
	}

	mSize = 0.2;
	
	mColor.red = red;
	mColor.green = green;
	mColor.blue = blue;
	if (green == 0) {
		if (blue + color_delta > 255) {
			blue = 255;
			green = color_delta;
		} else {
			blue += color_delta;
		}
	} else {
		if (green + color_delta > 255) {
			green = 0;
			blue = 0;
		} else {
			green += color_delta;
		}
	}

}

MParticle::~MParticle()
{
	if (mQuadric != 0) gluDeleteQuadric(mQuadric);
}

void MParticle::SetFriction(float friction)
{
	mFriction = friction;
}

float MParticle::DistanceTo(MParticleI* particle) const
{
	float	xDist = (particle->X() - mPos.x);
	float	yDist = (particle->Y() - mPos.y);
	float	zDist = (particle->Z() - mPos.z);
	return sqrt(xDist*xDist + yDist*yDist + zDist*zDist);
}

float MParticle::X() const
{
	return mPos.x;
}

float MParticle::Y() const
{
	return mPos.y;
}

float MParticle::Z() const
{
	return mPos.z;
}

float MParticle::Mass() const
{
	return mMass;
}

float MParticle::AttractionStrength() const
{
	return mAttractionStrength;
}

float MParticle::DesiredDistance() const
{
	return mDesiredDistance;
}

void MParticle::Birth()
{
	mFriction = 0.9;
	mMass = 1;
	mVelocity.Set(0.0000001, 0.00000001, 0.00000001);
	mPos.Set(0.5, 0.5, 0.5);
	mSpeed.Set(0.0, 0.0, 0.0);
}

MPoint MParticle::Pos() const
{
	return mPos;
}

void MParticle::Cycle(MSpaceI& space)
{
	// for reactto
	float dl;
	MPoint diff;

	// bechleunigen
	mVelocity.x /= mMass;
	mVelocity.y /= mMass;
	mVelocity.z /= mMass;
	mSpeed += mVelocity;
	mVelocity.Set(0.0, 0.0, 0.0);

	// friction
	mSpeed.x *= mFriction;
	mSpeed.y *= mFriction;
	mSpeed.z *= mFriction;

#if 0
	if (Walls) {
		if (pos.y < 0.05 && speed.y < 0) speed.y*=-0.5;
		else if (pos.y > 0.95 && speed.y > 0) speed.y*=-0.5;
		if (pos.x < 0.05 && speed.x < 0) speed.x*=-0.5;
		else if (pos.x > 0.95 && speed.x > 0) speed.x*=-0.5;
	}
#endif
	// movement
	mPos += mSpeed;

	// react to others
	MParticleI*	particle;
	for (uint32 k=0; (particle = space.ParticleAt(k)) != 0; k++) {
		if (particle != this) {
			diff = particle->Pos() - mPos;
			dl = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
			
			if (dl > 0.000001) {
				mVelocity.x += diff.x * (mAttractionStrength)/dl*(dl-(particle->DesiredDistance()+mDesiredDistance))*particle->Mass();
				mVelocity.y += diff.y * (mAttractionStrength)/dl*(dl-(particle->DesiredDistance()+mDesiredDistance))*particle->Mass();
				mVelocity.z += diff.z * (mAttractionStrength)/dl*(dl-(particle->DesiredDistance()+mDesiredDistance))*particle->Mass();
			}
		}
	}
}

void MParticle::Draw(	BView* view,
						float halfWidth,
						float halfHeight)
{
	BPoint	trans;
	trans.x = (mPos.x + 1) / 2;
	trans.y = (mPos.y + 1) / 2;

//	BPoint	pt(	(mPos.x * halfWidth) + halfWidth,
//				(mPos.y * halfHeight) + halfHeight );
	BPoint	pt(	(trans.x * (halfWidth * 2)),
				(trans.y * (halfHeight * 2)) );

	float	size2d = 40;
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	float	delta = (size2d / 2) / 5;
	float	size = size2d / 2;
	view->SetHighColor(mColor.red, mColor.green, mColor.blue, 44);
	view->FillEllipse(	BPoint(pt.x, pt.y),
						size,
						size);
	size -= delta;
	view->SetHighColor(mColor.red, mColor.green, mColor.blue, 74);
	view->FillEllipse(	BPoint(pt.x, pt.y),
						size,
						size);
	size -= delta;
	view->SetHighColor(mColor.red, mColor.green, mColor.blue, 104);
	view->FillEllipse(	BPoint(pt.x, pt.y),
						size,
						size);
	size -= delta;
	view->SetHighColor(mColor.red, mColor.green, mColor.blue, 144);
	view->FillEllipse(	BPoint(pt.x, pt.y),
						size,
						size);
	size -= delta;
	view->SetHighColor(mColor.red, mColor.green, mColor.blue, 255);
	view->FillEllipse(	BPoint(pt.x, pt.y),
						size,
						size);

}
