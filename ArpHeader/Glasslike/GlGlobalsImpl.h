/* GlGlobalsImpl.h
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
 * 2002.08.02				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLGLOBALSIMPL_H
#define GLASSLIKE_GLGLOBALSIMPL_H

#include <be/StorageKit.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamTypeList.h>
#include <GlKernel/GlNodeRoster.h>
#include <Glasslike/GlImagePool.h>
class _ToolBarInit;

/***************************************************************************
 * GL-GLOBALS-IMPL
 ***************************************************************************/
class GlGlobalsImpl : public GlGlobalsI
{
public:
	GlGlobalsImpl();
	virtual ~GlGlobalsImpl();

	virtual void	Initialize();

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* Addon methods.
	 */
	virtual status_t			GetAddOnInfo(	uint32 index, gl_node_add_on_id* outId,
												uint32* outIo,
												BString16* outCreator, int32* outKey,
												BString16* outCategory, BString16* outLabel,
												const ArpBitmap** outImage) const;
	virtual const GlNodeAddOn*	GetAddOn(	gl_node_add_on_id id) const;
	virtual const GlNodeAddOn*	GetAddOn(	const BString16& creator,
											int32 key) const;
	virtual GlNode*				NewNode(const BString16& creator, int32 key,
										const BMessage* config) const;
	virtual GlAlgo*				NewAlgo(const BString16& creator, int32 key) const;

	virtual GlParamTypeList&	ParamTypes();

	virtual gl_image_id			AcquireImage(const BString16& filename);
	virtual gl_image_id			AcquireImage(GlImage* image);
	virtual gl_image_id			AcquireImage(gl_image_id id);
	virtual status_t			ReleaseImage(gl_image_id id);
	virtual GlImage*			CloneImage(gl_image_id id);
	virtual ArpBitmap*			CloneBitmap(gl_image_id id);
	virtual const GlImage*		SourceImage(gl_image_id id);

private:
	GlParamTypeList				mParamTypes;
	GlImagePool					mImagePool;

	GlNodeRoster				mNodes;
	
	void						InitializeParamTypes();
	void						InitializeAddOns();

	status_t					Install(GlNodeAddOn* addon, _ToolBarInit* tb);
	
	status_t					InitializeUserNodes(BPath path, _ToolBarInit* tb);
	status_t					AddUserNode(const BMessage& msg, _ToolBarInit* tb);
};

#endif
