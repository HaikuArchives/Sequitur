/* GlEarthquakeIm.h
 * Copyright (c)2002 - 2004 by Eric Hackborn.
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
 * 2004.04.08				hackborn@angryredplanet.com
 * Ported to new chaining framework.
 *
 * 2002.08.22				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLEARTHQUAKEIM_H
#define GLNODES_GLEARTHQUAKEIM_H

#include <GlPublic/GlPlaneNode.h>
class GlEarthquakeImAddOn;
class _PlateInfo;

struct _FillLine
{
	int32 y, xl, xr, dy;
};

#define _MAX_FILL_LINE		(10000)

/***************************************************************************
 * GL-EARTHQUAKE-IM
 * Break the image into a series of plates, then reposition each plate.
 * By default, the plates are moved in a direction and distance according
 * to the plate number, where number 0 is moved along 0 degrees and with
 * 0 distance, and number n is moved along 359 degrees with the max distance.
 *
 * Parameters
 *		Length				The number of pixels to displace, in relative
 *							and absolute values.
 *		Maintain origin		If the earthquake changes the size of the image,
 *							offset the origin so the new image is in the same
 *							relative place.
 *
 * Chain matrix output
 *		Map - 0.			(OPT) A remapping for the plate directions
 *		Map - 1.			(OPT) A remapping for the plate distances
 *		Srf - 0.			The plate map.
 ***************************************************************************/
class GlEarthquakeIm : public GlPlaneNode
{
public:
	GlEarthquakeIm(const GlEarthquakeImAddOn* addon, const BMessage* config);
	GlEarthquakeIm(const GlEarthquakeIm& o);
	
	virtual GlNode*				Clone() const;
	virtual status_t			Process(GlNodeDataList& list,
										const gl_process_args* args);
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
				
private:
	typedef GlPlaneNode			inherited;
	const GlEarthquakeImAddOn*	mAddOn;
};

/***************************************************************************
 * GL-EARTHQUAKE-IM-ADD-ON
 ***************************************************************************/
class GlEarthquakeImAddOn : public GlNodeAddOn
{
public:
	GlEarthquakeImAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }
						
private:
	typedef GlNodeAddOn		inherited;
};


#endif
