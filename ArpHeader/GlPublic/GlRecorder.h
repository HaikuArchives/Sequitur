/* GlRecorder.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2004.03.18			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLRECORDER_H
#define GLPUBLIC_GLRECORDER_H

#include <be/app/Messenger.h>
#include <be/support/Locker.h>
#include <GlPublic/GlControlState.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlMidiEvent.h>
class ArpBitmap;
class GlParamWrap;

/***************************************************************************
 * GL-RECORDER
 * I am an object that records data and updates a display.
 ***************************************************************************/
class GlRecorder
{
public:
	GlRecorder();

	void				IncRefs();
	void				DecRefs();

	virtual void		Init();
	/* Subclasses should fill out the state appropriately.
	 */
	virtual void		SetState(GlControlState& s) const;
	/* Views can have multiple bitmap views they display -- the recorder
	 * needs to know the index of the view it wants to take over.  Default
	 * is 0, and in practice probably no one will do otherwise.
	 */
	virtual int32		ViewIndex() const;

	/* Set the current step, which will be 0.0 - 1.0.  This method
	 * has been locked by the caller.  Make sure to either call the
	 * superclass or assign mStep yourself.
	 */
	virtual void		LockedSetStep(float step);
	/* Handle drawing a new image to display.  The method might be
	 * supplied an existing bitmap -- if so, it can draw directly to
	 * it if it's smart and wants to be fast.  Alternatively, it can
	 * set bm to a new bitmap, if it cares more about convenience than
	 * performance.  What it can't do is delete the supplied bitmap.
	 * Also, if it does answer a new bitmap, it has conceded ownership.
	 * Answer with B_OK if you want drawing to take place.
	 */
	virtual status_t	Draw(ArpBitmap** bm) = 0;
	/* Handle param changes.
	 */
	virtual void		ParamEvent(gl_param_key key, const GlParamWrap& wrap) = 0;
	virtual void		MidiEvent(GlMidiEvent event, int32 letter) = 0;

	/* Copy whatever changes I've made into whomever created me.
	 */
	virtual status_t	UpdateSource() = 0;

protected:
	friend class		GlRecorderHolder;
	/* The recorder is held by two objects for two purposes -- it's
	 * held by the view, to generate a new image based on the current
	 * state, and it's held by the recorder holder, which feeds new
	 * values to it.  Any data that both of these functions depend on
	 * must be locked before accessing.  It'd be nice if the lock was
	 * outside of this view, but generally there's a very small part
	 * of each function that needs to be locked, so subclasses are
	 * responsible.
	 */
	mutable BLocker		mAccess;
	/* The step is 0 - 1.  Subclasses need to translate this into
	 * a position in their sequence array, or whatever they're using
	 * to record into.
	 */
	float				mStep;
	
	virtual ~GlRecorder();
	
private:
	int32				mRefs;
};

#endif
