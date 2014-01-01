/* GlGlobalsI.h
 * Copyright (c)2002 by Eric Hackborn.
 * All rights reserved.
 *
 * This file defines classes that provide global, application-wide
 * behaviour, along with functions for accessing instances of each
 * of these classes.  The application implementing this library needs
 * to implement the accessing functions.
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
 * 2002.08.02				hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef GLPUBLIC_GLGLOBALSI_H
#define GLPUBLIC_GLGLOBALSI_H

#include <ArpCore/String16.h>
#include <GlPublic/GlDefs.h>
class ArpBitmap;
class GlImage;
class GlNode;
class GlNodeAddOn;
class GlParamTypeList;
class GlAlgo;

enum {
	GL_USER_NODES		= 0,
	GL_IMG_GEN_NODES	= 1,
	GL_IMG_TRANS_NODES	= 2,
	GL_2D_GEN_NODES		= 3,
	GL_2D_TRANS_NODES	= 4
};

extern gl_image_id		GL_NODE_IMAGE_ID;
extern gl_image_id		GL_PIXEL_TARGET_IMAGE_ID;

/***************************************************************************
 * GL-GLOBALS-I
 * A class that stores all the globally-available methods and data.
 ***************************************************************************/
class GlGlobalsI;

GlGlobalsI& GlGlobals();
void SetGlobals(GlGlobalsI& globals);

class GlGlobalsI
{
public:
	virtual ~GlGlobalsI()			{ }

	virtual void	Initialize()	{ }

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* Add ons
	 */
	virtual status_t			GetAddOnInfo(	uint32 index, gl_node_add_on_id* outId,
												uint32* outIo,
												BString16* outCreator, int32* outKey,
												BString16* outCategory, BString16* outLabel,
												const ArpBitmap** outImage) const = 0;
	virtual const GlNodeAddOn*	GetAddOn(	gl_node_add_on_id id) const = 0;
	virtual const GlNodeAddOn*	GetAddOn(	const BString16& creator,
											int32 key) const = 0;
	virtual GlNode*				NewNode(const BString16& creator, int32 key,
										const BMessage* config) const = 0;
	virtual GlAlgo*				NewAlgo(const BString16& creator, int32 key) const = 0;
	// Convenience -- use default "arp" creator
	const GlNodeAddOn*			GetAddOn(int32 key) const;
	GlNode*						NewNode(int32 key, const BMessage* config) const;

	/* Params
	 */
	virtual GlParamTypeList&	ParamTypes() = 0;

	/*---------------------------------------------------------
	 * POOLED IMAGE RESOURCES
	 *---------------------------------------------------------*/
	virtual gl_image_id			AcquireImage(const BString16& filename) = 0;
	virtual gl_image_id			AcquireImage(GlImage* image) = 0;
	virtual gl_image_id			AcquireImage(gl_image_id id) = 0;
	virtual status_t			ReleaseImage(gl_image_id id) = 0;
	/* Copies -- don't need to release.
	 */
	virtual GlImage*			CloneImage(gl_image_id id) = 0;
	virtual ArpBitmap*			CloneBitmap(gl_image_id id) = 0;
	/* Some client knowledge is required here -- the client must have Acquired
	 * the image, and must not Release it while they're referencing it.
	 */
	virtual const GlImage*		SourceImage(gl_image_id id) = 0;
};

#endif
