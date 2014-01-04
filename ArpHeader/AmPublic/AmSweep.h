/* AmSweep.h
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
#ifndef AMPUBLIC_AMSWEEP_H
#define AMPUBLIC_AMSWEEP_H

#include <support/SupportDefs.h>
class AmEvent;

/*******************************************************
 * GL-SWEEP
 * A simple object that takes a value of 0 to 1 and
 * returns a (probably different) value of 0 to 1.
 *******************************************************/
class AmSweep
{
public:
	virtual ~AmSweep();

	/* A unique value for the curve subclass.
	 */
	int16				Key() const;
	virtual AmSweep*	Clone() const = 0;
	/* Run v through myself and any subsequent curves and
	 * return the result.  Subsequent curves are subtractive.
	 */
	virtual float		At(float v) const;
	status_t			SetNext(AmSweep* next);
	/* Some clients cache the results of curves for performance.
	 * However, curves with randomness should not be cached.
	 */
	bool				IsRandom() const;
	/* This is a real hack, right now solely needed by the
	 * fractal lines -- any client must tell the curve what
	 * the final integer size of the curve will be.
	 */
	void				SetTargetSize(uint32 size);
	
	/* Events can be used to set properties.  Each sweep
	 * can be configured appropriately.
	 */
	virtual void		HandleEvent(AmEvent* event);
	
protected:
	AmSweep(int16 key);
	AmSweep(const AmSweep& o);
	
	int16				mKey;
	AmSweep*			mNext;
	/* Subclasses must answer 0 to 1.  v will be 0 to 1.
	 */
	virtual float		Process(float v) const = 0;
	/* Any curve that has randomess should answer with true.
	 * Default is false so most curves can ignore this.
	 */
	virtual bool		is_random() const;
	virtual void		set_target_size(uint32 size);

public:
	/* This function just adds the key to a master list of keys,
	 * answering true if successful and false if the key already
	 * exists.  Objects that create new curves should call this
	 * in an assert -- it's just to verify that the same int16
	 * isn't used twice during development, after development it
	 * doesn't matter.
	 */
	static bool			RegisterKey(int16 key);
	void				Print(uint32 tabs = 0) const;
};

#endif