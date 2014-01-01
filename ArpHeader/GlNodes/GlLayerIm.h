/* GlLayerIm.h
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
 * 2004.20.21			hackborn@angryredplanet.com
 * Morphed GlComposite into layer, removed 1d, added image chain
 * and dynamic layer chains.  Also removed src selector, so it
 * defaults to the mask if that's all you have, otherwise the layers.
 *
 * 2003.02.26			hackborn@angryredplanet.com
 * Combined composite color and composite material
 */
#ifndef GLNODES_GLLAYERIM_H
#define GLNODES_GLLAYERIM_H

#include <GlPublic/GlImage.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlNode.h>
class GlLayerImAddOn;

/***************************************************************************
 * GL-LAYER-IM
 * Overlay a series of layers.  This node takes a destination image (which
 * is the image that arrives at its input), runs it through an optional
 * chain, then generates 1 or more layer images and overlays them on the
 * destination.  If there are no layer images, it uses the mask as the
 * source.  If there's no mask, no layers, and no image chain, it just
 * does nothing.
 *
 * Params
 *		Mode				-- The compositing method (average, replace, etc)
 *		R on, G on, etc.	-- The bools to tell me which properties are on
 *		Align sources		-- If there's no destination image and multiple
 *							   sources, align them to 0, 0.
 ***************************************************************************/
class GlLayerIm : public GlNode
{
public:
	GlLayerIm(const GlNodeAddOn* addon, const BMessage* config);
	GlLayerIm(const GlLayerIm& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;
	virtual status_t		Process(GlNodeDataList& list,
									const gl_process_args* args);

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-LAYER-IM-ADD-ON
 ***************************************************************************/
class GlLayerImAddOn : public GlNodeAddOn
{
public:
	GlLayerImAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const	{ return GL_IMAGE_IO; }
											
private:
	typedef GlNodeAddOn		inherited;
};


#endif
