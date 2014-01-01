/* ArpBitmap.h
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
 * 2002.07.15				hackborn@angryredplanet.com
 * Created this file.
 */
 
#ifndef OSINTERFACESUPPORT_OSBITMAP_H
#define OSINTERFACESUPPORT_OSBITMAP_H

#include <ArpCore/String16.h>
#include <be/experimental/BitmapTools.h>
#include <be/interface/Bitmap.h>
#include <ArpSupport/ArpVoxel.h>
#include <ArpInterface/ArpInterfaceDefs.h>

enum {
	ARP_BMP_FORMAT		= 0,
	ARP_PNG_FORMAT		= 1,
	ARP_JPG_FORMAT		= 2,
	ARP_TGA_FORMAT		= 3
};

/*******************************************************
 * ARP-BITMAP
 * Wrap the BBitmap.
 *******************************************************/
class ArpBitmap
{
public:
	ArpBitmap();
	ArpBitmap(float width, float height);
	ArpBitmap(const ArpBitmap& o);
	ArpBitmap(const BBitmap* o);
	ArpBitmap(const BString16& filename);
	ArpBitmap(uint8* r, uint8* g, uint8* b, uint8* a, int32 w, int32 h);
	ArpBitmap(uint8 r, uint8 g, uint8 b, uint8 a, int32 w, int32 h);
	virtual ~ArpBitmap();

	/* Overwrite myself with the supplied plane data.
	 */
	status_t			Set(uint8* r, uint8* g, uint8* b, uint8* a,
							int32 w, int32 h);
	status_t			Set(uint8 r, uint8 g, uint8 b, uint8 a,
							int32 w, int32 h);
	/* Fill in the planes with my data.  If w or h doesn't match
	 * my dimensions, then error.
	 */
	status_t			Get(uint8* r, uint8* g, uint8* b, uint8* a,
							int32 w, int32 h) const;
	/* A special method for filling the plane info from a bitmap
	 * that was generated from text -- i.e. has the supplied
	 * background and foreground.
	 */
	status_t			GetTextHack(uint8* r, uint8* g, uint8* b, uint8* a,
									uint8* z, int32 inW, int32 inH,
									const ArpVoxel& bg, const ArpVoxel& fg) const;

	int32				Width() const;
	int32				Height() const;
	BRect				Bounds() const;

	status_t			Save(const BString16& filename, int32 format);

	/* Ugh...  when I make a bitmap from text, it's never lined up
	 * properly, there are always blank pixels around the edges.
	 * This creates a new bitmap with new edges.
	 */
	status_t			TrimFromTextHack();
	
public:
	BBitmap*		mBitmap;
private:
	friend class	ArpPainter;
	pixel_access	mPa;

	/* My bitmap must be in a 32-bit format right now.
	 */
	status_t		ConformColorSpace();

	/* BEOS SPECIFIC
	 */
public:
	const BBitmap*	Bitmap() const;
	status_t		TakeBitmap(BBitmap* bm);		// Take ownership

public:
	void			Print(uint32 tabs = 0) const;
};


#endif
