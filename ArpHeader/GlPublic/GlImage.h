/* GlImage.h
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
 * 2003.08.22			hackborn@angryredplanet.com
 * Extracted from EnabledParamList, added labels to each entry
 */
#ifndef GLPUBLIC_GLIMAGE_H
#define GLPUBLIC_GLIMAGE_H

#include <support/SupportDefs.h>
// This is sooooo wrong...  find a better place for the pixel rules
#include <ArpInterface/ArpInterfaceDefs.h>
#include <ArpSupport/ArpVoxel.h>
#include "GlPublic/GlDefs.h"
class ArpBitmap;
class GlPlanes;

/***************************************************************************
 * GL-IMAGE
 * I contain bitplanes of image information.
 ****************************************************************************/
class GlImage
{
public:
	GlImage();
	/* You can not specify a bg, but the data will be uninitialized,
	 * so make sure you write over every pixel.  I always allocate
	 * the color fields, even if they aren't specified.
	 */
	GlImage(int32 width, int32 height, const ArpVoxel* bg = 0,
			const ArpVoxel* fg = 0, uint32 fields = 0);
	/* Create the image as a solid on the background color.
	 */
	GlImage(const ArpVoxel& bg);
	GlImage(const GlImage& o);
	GlImage(const ArpBitmap& bm);
	virtual ~GlImage();

	gl_image_id			Id() const;
	virtual GlImage*	Clone() const;

	status_t			InitCheck() const;
	int32				Width() const;
	int32				Height() const;
	status_t			SetSize(int32 width, int32 height);
	/* Answer a mask containing the fields I have allocated,
	 * like GL_PIXEL_R_MASK, etc.
	 */
	uint32				Fields() const;
	/* If enforce is true, I guarantee all the targets requested
	 * actually exist in the returned planes.  If I can't allocate
	 * something, I return nothing.
	 */
	GlPlanes*			LockPixels(uint32 targets, bool enforce = false);
	void				UnlockPixels(GlPlanes* pixels) const;

	/* VIRTUAL COORDINATES
	 */
	ArpPixelRules		Bounding() const;
	void				SetBounding(ArpPixelRules bounding);
	void				GetOrigin(int32* x, int32* y);
	void				SetOrigin(int32 x, int32 y);
	ArpVoxel			Property(ArpColourConstant constant) const;
	void				SetProperty(ArpColourConstant constant, const ArpVoxel& v);
	bool				IsSolid() const;

	inline void			GetLocation(int32* x, int32* y) const
	{
		// Optimization for color images
		if (mIsSolid) {
			*x = 0;
			*y = 0;
			return;
		}
		*x = *x - mX;
		*y = *y - mY;
		if (mBounding == ARP_CE) {
			if (*x < 0) *x = *x + mW;
			else if (*x >= mW) *x = *x - mW;
			if (*y < 0) *y = *y + mH;
			else if (*y >= mH) *y = *y - mH;
		} else if (mBounding == ARP_SE) {
			if (*x < 0) *x = 0 - *x;
			else if (*x >= mW) *x = 2 * mW - *x - 2;
			if (*y < 0) *y = 0 - *y;
			else if (*y >= mH) *y = 2 * mH - *y - 2;
		} else if (mBounding == ARP_TILED) {
			if (*x >= mW) *x = *x % mW;
			if (*y >= mH) *y = *y % mH;
		}
	}

	/* EDITING -  Pixels can't be locked
	   ------- */
	status_t			SetColor(uint8 r, uint8 g, uint8 b, uint8 a);

	/* UTILITIES
	 */
	/* This is a special utility message -- a common operation is for
	 * clients to want a copy of the image to use as a source, but
	 * the destination is going to change in size, so it doesn't need
	 * the original pixels.  Rather than going through all the work
	 * of copying, this method creates a new image with my pixel data,
	 * leaving me resized to the new dimensions.
	 */
	GlImage*			SourceClone(int32 width, int32 height);
	/* Take onwership of all data in the supplied image, leaving it empty.
	 */
	status_t			Take(GlImage& o);
	GlImage*			AsScaledImage(int32 newW, int32 newH, float quality = 0.0) const;
	/* The width and height are only valid if this image was
	 * created on a single pixel (i.e. a color image).  In that
	 * case, nothing will be returned if w or h is < 1, otherwise
	 * the requested size will be returned.  In all other cases,
	 * the image's width and height are used.
	 */
	ArpBitmap*			AsBitmap(int32 w = 50, int32 h = 50) const;
	status_t			FromTextHack(	const ArpBitmap& bm,
										const ArpVoxel& bg,
										const ArpVoxel& fg);

	status_t			LockColorHack(	const uint8** r, const uint8** g,
										const uint8** b, const uint8** a,
										const uint8** z) const;
	void				UnlockColorHack(const uint8* r) const;

protected:
	virtual GlPlanes*	NewPlanes() const;
	
private:
	status_t			mStatus;
	int32				mW, mH;
	uint8*				mColor;		// The color array always contains r, g, b, a, z
									// components.  The z is a bit of a stretch for
									// being in the color, but I know I always want it.
	uint8*				mLight;		// Contains diff and spec components.
	uint8*				mMaterial;	// Contains density, cohesion and fluidity.

	/* Properties
	 */
	ArpPixelRules		mBounding;
	int32				mX, mY;

	bool				mIsSolid;
	ArpVoxel			mBg, mFg;
	uint8*				mSolidCache;	// When locking, the pixels need something to reference
	status_t			CacheSolid();

	GlPlanes*			MakePixels(	uint32 targets, uint8* color,
									uint8* light, uint8* material, int32 w, int32 h) const;

	void				InitProperties();
	void				Free();
	
public:
	void				Print(uint32 tabs = 0) const;
};

#endif
