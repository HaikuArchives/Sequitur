/* GlAlgo2d.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2003.02.11			hackborn@angryredplanet.com
 * Expanded to include paths
 *
 * 2002.08.31			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLALGO2D_H
#define GLPUBLIC_GLALGO2D_H

#include <GlPublic/GlDefs.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlActions.h>
class GlCache2d;
class GlImage;
class GlPath;
class GlPlanes;
class GlPlanes;
class GlProcessStatus;

/***************************************************************************
 * GL-ALGO-2D
 * I represent an unbounded, generated area.  Subclasses fill the area as
 * appropriate, which can either be one (a path) or two (a mask) dimensional.
 ****************************************************************************/
class GlAlgo2d : public GlAlgo
{
public:
	virtual ~GlAlgo2d();

	/* ACCESSING
	   ---------- */
	virtual uint32			Io() const		{ return GL_2D_IO; }

	GlFillType				FillType() const;

	/* CONVERTING
	   --------- */
	virtual GlAlgo2d*		As2d();
	virtual const GlAlgo2d*	As2d() const;

	/* PROCESSING
	   ---------- */
	/* Subclasses implement this to write data to the planes.  They can
	 * either implement the first variant and overwrite all the planes
	 * simultaneously, or they can implement the second variant and have
	 * a single method that performs the same thing on each plane.  The
	 * second variant is easier for clients and more generalized; the first
	 * is generally for optimizations, when the operation is slow.  The src
	 * pixels are optional; sometimes the algo is based on an image, and
	 * some algos will pull info from that image.
	 * NOTE that in certain instances, the src and dest are the same
	 * object -- so if you need to read src data after writing dest data,
	 * either know exactly what you're doing or copy the src.
	 */
	virtual status_t		Process(const GlPlanes* src, GlPlanes& dest,
									GlProcessStatus* status = 0);
	virtual status_t		Process(const GlPlanes* src, uint8* dest, int32 w, int32 h,
									GlProcessStatus* status = 0);
	/* Subclasses implement this to do write data to the path.  img must
	 * exist at least for w, h, even though it might not have any planes.
	 */
	virtual status_t		Process(const GlPlanes& src, GlPath& path);

protected:
	GlAlgo2d(	gl_node_id nid, uint32 targets,
				GlFillType fillType = GL_FILL_BLACK,
				int32 token = GL_NO_TOKEN);
	GlAlgo2d(const GlAlgo2d& o);
	
	uint32					mTargets;

	virtual status_t		Perform(GlNodeDataList& list, const gl_process_args* args);
	virtual status_t		PerformImage(GlImage* img, const gl_process_args* args);

private:
	typedef GlAlgo		inherited;
	GlFillType			mFillType;

public:
	status_t			ProcessAll(	const GlPlanes* src, GlPlanes& dest,
									GlProcessStatus* status = 0);
//	status_t			ProcessAll(const GlPlanes* src, GlImage& dest);
	status_t			ProcessAll(const GlPlanes& src, GlPath& path);
// OBSOLETE:  WEED OUT
	status_t			ProcessAll(const GlPlanes* pixels, uint8* mask, int32 w, int32 h);
	/* Convenience utility -- allocate a new mask, fill it out,
	 * run myself on it, and answer the mask.  The pixels are
	 * there for the dimensions; they might not have any planes.
	 */
	uint8*				NewMask(const GlPlanes& pixels);
};

/*******************************************************
 * GL-ALGO-2D-WRAP
 * Perform operations on algo 2d chains.  I DO NOT own
 * the algo I'm assigned -- I do cache it, BUT I DON'T
 * PERFORM ANY SORT OF LOCK, SO IT MUST EXIST AS LONG
 * AS I DO.
 *******************************************************/
class GlAlgo2dWrap
{
public:
	GlAlgo2d**			cache;
	uint32				size;

	GlAlgo2dWrap(GlAlgo* algo = 0);
	~GlAlgo2dWrap();

	status_t			InitCheck() const;
	status_t			SetAlgo(GlAlgo* algo);		

	status_t			Process(const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status = 0);

	/* Process my algo on the supplied cache.  If there are pixels,
	 * then its plane dimensions will overwrite the cache's dimensions.
	 * Answer the cache data or 0.
	 */
	uint8*				Cache(	GlCache2d& cache, const GlPlanes* pixels = 0,
								GlProcessStatus* status = 0);

private:
	status_t			Cache(GlAlgo* algo);
	void				Free();
};

#endif
