/* GlNewImage.h
 * Copyright (c)2002-2003 by Eric Hackborn.
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
 * 2004.02.24			hackborn@angryredplanet.com
 * Converted to new chaining framework.
 *
 * 2003.03.13			hackborn@angryredplanet.com
 * Extracted from NewImage, parameterized.
 *
 * 2002.08.21			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLIMAGENODES_H
#define GLNODES_GLIMAGENODES_H

#include <support/Locker.h>
#include <GlPublic/GlNode.h>
class GlImage;
class GlNewImageAddOn;

/***************************************************************************
 * GL-IMAGE-NODE
 * Abstract class for a couple nodes that generate images.
 ***************************************************************************/
class GlImageNode : public GlNode
{
public:
	GlImageNode(const GlNodeAddOn* addon, const BMessage* config);
	GlImageNode(const GlImageNode& o);
	virtual ~GlImageNode();
	
	virtual status_t			ParamChanged(gl_param_key key);
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
	virtual BView*				NewView(gl_new_view_params& params) const;
	virtual GlNodeVisual*		NewVisual(const GlRootRef& ref) const;

protected:
	friend class _GlImageVisual;
	gl_image_id					mImageId;
	GlImage*					mPreview;	// Cache the preview too.
	int32						mPreviewX,	// The dimensions the preview was
								mPreviewY;	// created to conform to.

	/* Subclasses can answer with true if they need to rebuild the images.
	 */
	virtual bool				IsDirty() const;

	mutable BLocker				mAccess;

	virtual void				CacheImage() = 0;
	void						UncacheImages();

private:
	typedef GlNode				inherited;
};

/***************************************************************************
 * GL-LOAD-IMAGE and GL-LOAD-IMAGE-ADD-ON
 ***************************************************************************/
class GlLoadImage : public GlImageNode
{
public:
	GlLoadImage(const GlNodeAddOn* addon, const BMessage* config);
	GlLoadImage(const GlLoadImage& o);

	virtual GlNode*				Clone() const;

protected:
	virtual void				CacheImage();

private:
	typedef GlImageNode			inherited;
};

// GL-LOAD-IMAGE-ADD-ON
class GlLoadImageAddOn : public GlNodeAddOn
{
public:
	GlLoadImageAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const { return GL_IMAGE_IO; }

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
};

/***************************************************************************
 * GL-NEW-IMAGE and GL-NEW-IMAGE-ADD-ON
 ***************************************************************************/
class GlNewImage : public GlImageNode
{
public:
	GlNewImage(const GlNodeAddOn* addon, const BMessage* config);
	GlNewImage(const GlNewImage& o);

	virtual GlNode*				Clone() const;

protected:
	virtual bool				IsDirty() const;
	virtual void				CacheImage();

private:
	typedef GlImageNode			inherited;
	BPoint						mLastSize;
};

// GL-NEW-IMAGE-ADD-ON
class GlNewImageAddOn : public GlNodeAddOn
{
public:
	GlNewImageAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO; }

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
};


#endif
