/* GlAlgo1d.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.02.19				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLALGO1D_H
#define GLPUBLIC_GLALGO1D_H

#include <support/SupportDefs.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlRect.h>
class GlAlgo1dAction;
class GlCache1d;
class GlImage;
class GlPlanes;

// Volume2d modes
enum {
	GL_VOLUME_BELOW_MODE		= 'blw_',
	GL_VOLUME_AT_MODE			= 'at__',
	GL_VOLUME_ABOVE_MODE		= 'abv_'
};

/* Different 1ds have different ways of counting
 * counting their steps.  Some can be viewed as
 * envelopes -- that is, they start at 0 and end
 * at 1.  Others can be viewed as LFOs, that is,
 * they start at 0 and end at 1 - (1 / steps).
 * The LFO style is intended for looping shapes --
 * they will generally have a cycle parameter to
 * control how many loops occur.
 */
#define GL_1D_ENV_STEP(step, count)		(((count) < 2) ? (0.5f) : ((step) / float((count) - 1)))
#define GL_1D_LFO_STEP(step, count)		((step) / float(count))

/*******************************************************
 * GL-MAP
 * A simple object that takes a value of 0 to 1 and
 * returns a (probably different) value of 0 to 1.
 *******************************************************/
class GlAlgo1d : public GlAlgo
{
public:
	virtual ~GlAlgo1d();

	/* ACCESSING
	   --------- */
	virtual uint32			Io() const		{ return GL_1D_IO; }

	/* A unique value for the curve subclass.
	 */
	int32					Key() const;
	enum {
		CONSTANT_F			= 0x00000001,	// Always produce the same output no matter the input
		INFINITE_F			= 0x00000002,	// Endlessly create detail with zooming - fractals
		MORPHING_F			= 0x00000004,	// Morph between states (IncFrame() does something)
		RANDOM_F			= 0x00000008,	// Different output with the same input - pink noise
		APPLY_TO_2D_F		= 0x00010000,	// Anyone implementing ApplyTo2d() MUST set this
		LOOPING_F			= 0x10000000	// Purely a cosmetic thing: algos that loop should display
											// themselves from 0 to 1, rather than 0 to 0.999999
	};
	/* Currently the only flag you can supply is SINGLE_ALGO_F if you just
	 * want this algo's properties.
	 */
	uint32					Properties(uint32 flags = 0) const;

	/* CONVERTING
	   --------- */
	virtual GlAlgo1d*		As1d();
	virtual const GlAlgo1d*	As1d() const;

	/* PERFORMING
	   ---------- */
	/* As soon as everyone has moved over to the new model I can
	 * ditch the status_t return.
	 */
	enum {
		INIT_LINE_F			= 0x00000001,
		SINGLE_ALGO_F		= 0x00000002	// Only run on the algo I'm calling,
											// not the whole chain
	};
	status_t				Run(float* line, float* at, int32 size, uint32 flags = INIT_LINE_F) const;

	/* The new-new-new style.  I'm going to try to weed out any single calls to
	 * At() -- I don't think that's ever necessary with the interpolating array.
	 */
	GlCache1d*				NewCache(	uint32 size, uint32 frames = 0,
										uint32 flags = INIT_LINE_F);

	/* This is the new style, just like the 2D.  It's necessary,
	 * anyway, for things like Scale.  The at arg is optional, and may
	 * be NULL.  If it exists, it will be the same size as line and gives
	 * the position at each position, if you see (otherwise the position
	 * can be inferred from the size of the line).  The HEAD_F is important
	 * to be aware of:  Roughly speaking, there are two types of algos, those
	 * that generate and those that transform.  Those that merely transform
	 * won't ever care about the at arg or HEAD_F, they'll just change the
	 * line values as appropriate.  However, those that generate should assume
	 * the line arg is uninitialized if HEAD_F is set.
	 */
	enum {
		ALGO_HEAD_F			= 0x00000001	// This is the first algo in the chain
	};
	virtual status_t		Algo(float* line, float* at, int32 size, uint32 flags) const = 0;

	/* UTILITIES
	   --------- */

	/* Two 1D objects can be promoted into a 2D, where the
	 * receiver becomes the x dimension and the argument the y.
	 * This is done this way because some subclasses might want
	 * to optimize or alter the procedure -- for example, a 2D
	 * Perlin noise is not just two 1D's blended together.  Answer
	 * B_OK if you promoted successfully, otherwise an error.
	 */
	virtual status_t		Promote2d(	uint8* mask, int32 w, int32 h,
										const GlAlgo1d* y1d) const;
	/* This is a form of 2D promotion, but first the 1D's are
	 * promoted into a 3D -- a volume.  Then it's downsampled to
	 * a 2D according to the rules.  This is because, right now,
	 * it doesn't really make sense to have a full 3D volume object,
	 * I think.
	 *
	 * The depth determines the z coordinate that I'm looking at,
	 * the mode how to generate a value at that x, y.
	 *
	 * By default this method does nothing.  Subclasses that
	 * can perform a 3D promotion only the supplied 1d's should
	 * implement it and return B_OK, otherwise return B_ERROR.
	 */
	virtual status_t		Volume2d(	uint8* depth, uint8* mask,
										int32 w, int32 h,
										uint32 mode, uint32 op,
										const GlAlgo1d* y1d,
										const GlAlgo1d* z1d) const;

	/* Anyone implementing this MUST set the APPLY_TO_2D_F property.
	 */
	virtual status_t	ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const;

protected:
	GlAlgo1d(	int32 key, gl_node_id nid, int32 token = GL_NO_TOKEN,
				float initVal = 1.0);
	GlAlgo1d(const GlAlgo1d& o);

	friend class		GlAlgo1dWrap;
	
	int32				mKey;
	float				mInitVal;
	
	/* Answer the properties of any maps (i.e. parse node ChainAt()s)
	 * I contain.
	 */
	virtual uint32		properties() const;
	virtual void		_print() const;

private:
	typedef GlAlgo		inherited;
	
public:
	/* This function just adds the key to a master list of keys,
	 * answering true if successful and false if the key already
	 * exists.  Objects that create new curves should call this
	 * in an assert -- it's just to verify that the same int32
	 * isn't used twice during development, after development it
	 * doesn't matter.
	 */
	static bool			RegisterKey(int32 key);
};

/*******************************************************
 * GL-ALGO-1D-WRAP
 * Perform operations on algo 1D chains.  I DO NOT own
 * the algo I'm assigned -- I do cache it, BUT I DON'T
 * PERFORM ANY SORT OF LOCK, SO IT MUST EXIST AS LONG
 * AS I DO.
 *******************************************************/
class GlAlgo1dWrap
{
public:
	/* If it's really important, clients can access the
	 * cache directly, but in most cases they should go
	 * through the interface.
	 */
	GlAlgo1d**			cache;
	uint32				size;

	GlAlgo1dWrap(GlAlgo* algo = 0);
	~GlAlgo1dWrap();

	status_t			InitCheck() const;
	status_t			SetAlgo(GlAlgo* algo);

	status_t			Process(float* line, float* at, int32 size,
								uint32 flags = GlAlgo1d::INIT_LINE_F) const;

private:
	status_t			Cache(GlAlgo* algo);
	void				Free();
};

#endif
