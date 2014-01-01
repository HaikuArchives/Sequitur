/* GlMaskMap.h
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
 * 2003.01.15			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLMASKMAP_H
#define GLPUBLIC_GLMASKMAP_H

#include "GlPublic/GlDefs.h"
class GlImage;
class _MaskMapData;

/***************************************************************************
 * GL-MASK-MAP
 * I store a list of masks keyed on an ID -- any ID, but the most common
 * key will be a gl_image_id, so there are conveniences for it.  I am a small
 * utility for clients that need to cache plane information.
 ****************************************************************************/
class GlMaskMap
{
public:
	GlMaskMap();
	~GlMaskMap();

	uint32			FrameCount(gl_id id) const;
	/* Answer the mask at the given id.  If no entry exists for the
	 * id, create it.  If a mask exists at the id but the dimensions
	 * don't match, nothing is returned.  Multiple frames of data can
	 * be stored at the same image index -- frames of less than 0 will
	 * always create a new mask.  Frames greater than 0 will always answer
	 * the requested mask.  Frame 0 is a special case -- it will answer
	 * the first mask if it exists, and create it if it doesn't.
	 *
	 * Clients can alternatively store a float mask.  Obviously,
	 * mixing the two will [bleep] things up.  Really gotta resolve
	 * this float vs. uint8 battle.
	 */
	uint8*			GetMask(const GlImage* image, int32 frame = 0);
	uint8*			GetMask(gl_id id, int32 w, int32 h, int32 frame = 0);
	float*			GetMaskF(const GlImage* image, int32 frame = 0);
	float*			GetMaskF(gl_id id, int32 w, int32 h, int32 frame = 0);
	
	void			MakeEmpty();
	
private:
	_MaskMapData*	mData;
};

#endif
