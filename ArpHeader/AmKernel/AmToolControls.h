/* AmToolControls.h
 * Copyright (c)2001 by Eric Hackborn.
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.04.04		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMKERNEL_AMTOOLCONTROLS_H
#define AMKERNEL_AMTOOLCONTROLS_H

#include <vector>
#include <interface/View.h>
#include "AmPublic/AmToolRef.h"
class AmToolControlList;
class AmToolControlOp;
class AmToolControlVis;

/***************************************************************************
 * AM-TOOL-CONTROL
 ****************************************************************************/
class _AmTcTarget;
typedef std::vector<_AmTcTarget>		tctarget_vec;

class AmToolControl
{
public:
	AmToolControl();
	AmToolControl(const BMessage& flattened);
	AmToolControl(const AmToolControl& o);
	virtual ~AmToolControl();

	AmToolControl&	operator=(const AmToolControl &o);

	BRect			Frame() const;
	void			SetTool(const AmTool* tool);
	void			SetVisual(AmToolControlVis* visual);

	status_t		GetTarget(	uint32 index, int32* outPipeline,
								int32* outFilter, BString* outName) const;
	void			AddTarget(	int32 pipeline, int32 filter,
								const BString& name);
	status_t		RemoveTarget(uint32 index);
	
	void			SetFrame(BView* view, BRect containingFrame, BPoint where);
	void			DrawOn(BView* view, BRect clip);
	BRect			MouseMoved(BView* view, BPoint where);

	status_t		WriteTo(BMessage& flattened) const;

	enum {
		NO_TYPE			= 0,
		BOOL_TYPE		= 'bool',
		INT32_TYPE		= 'in32',
		RANGE32_TYPE	= 'rn32'
	};
	uint32			Type() const;

	void			Print(uint32 tabs = 0) const;

protected:
	uint32			mType;
	AmToolRef		mToolRef;
	uint8			mMaxAlpha;
	uint8			mMinAlpha;
	uint8			mAlpha;

private:
	BPoint			mScaledPt;
	bool			mActive;
	bool			mMouseHit;

	AmToolControlVis*	mVisual;
	
	BRect			DoHit();
	uint8			AlphaForPoint(BPoint where, BRect bounds) const;

	tctarget_vec	mTargets;
	status_t		SendValueToTargets();
	
const BBitmap*	mOnImage;
const BBitmap*	mOffImage;
const BBitmap*	mCurImage;
};

/***************************************************************************
 * AM-TOOL-CONTROL-LIST
 * This class is the container for all tool controls.  Possibly it's
 * unnecessary, and I'll just end up using a vector.
 ****************************************************************************/
typedef std::vector<AmToolControl*>	toolctrl_vec;

class AmToolControlList
{
public:
	AmToolControlList();
	AmToolControlList(const BMessage& flattened);
	AmToolControlList(const AmToolControlList& o);
	virtual ~AmToolControlList();

	void			SetTool(const AmTool* tool);

	status_t		AddControl(AmToolControl* ctrl);
	AmToolControl*	RemoveControl(uint32 index);
	status_t		GetControl(uint32 index, AmToolControl** control) const;
	status_t		GetControl(void* toolControlId, AmToolControl** control) const;

	void			DrawOn(BView* view, BRect clip);
	void			MouseMoved(BView* view, BPoint where);
	
	void			Prepare(BView* view, BRect containingFrame, BPoint where);
	/* The session has came to an end, if I have any
	 * controls they need to be invalidated.
	 */
	void			MouseCleanup(BView* view);

	AmToolControlList* Copy() const;
	status_t		WriteTo(BMessage& flattened) const;

	void			Print(uint32 tabs = 0) const;
		
private:
	toolctrl_vec	mControls;
};

/***************************************************************************
 * AM-TOOL-CONTROL-VIS
 * Abstract interface for the visual representation of a tool control.
 ****************************************************************************/
class AmToolControlVis
{
public:
	virtual ~AmToolControlVis()	{ }

	static	AmToolControlVis* Instantiate(BMessage& flattened);

	virtual BRect		Frame() const;
	virtual void		SetActive(bool active);
	virtual void		SetFrame(BView* view, BRect containingFrame, BPoint position) = 0;
	virtual void		DrawOn(BView* view, BRect clip, uint8 alpha) = 0;

	virtual AmToolControlVis* Copy() const = 0;
	virtual status_t	WriteTo(BMessage& flattened) const = 0;

protected:
	AmToolControlVis();
	AmToolControlVis(const AmToolControlVis& o);

	BRect			mFrame;
	bool			mActive;
};

// Display the control as simple text
class AmToolControlTextVis : public AmToolControlVis
{
public:
	AmToolControlTextVis(const char* onText, const char* offText);
	AmToolControlTextVis(const AmToolControlTextVis& o);

	virtual void		SetFrame(BView* view, BRect containingFrame, BPoint position);
	virtual void		DrawOn(BView* view, BRect clip, uint8 alpha);

	virtual AmToolControlVis* Copy() const;
	virtual status_t	WriteTo(BMessage& flattened) const;

private:
	typedef AmToolControlVis inherited;
	BString			mOnText;
	BString			mOffText;
};

// Display the control as a bitmap
class AmToolControlBitmapVis : public AmToolControlVis
{
public:
	AmToolControlBitmapVis(const BBitmap* activeBm, const BBitmap* inactiveBm);
	AmToolControlBitmapVis(const BMessage& flattened);
	AmToolControlBitmapVis(const AmToolControlBitmapVis& o);
	virtual ~AmToolControlBitmapVis();
	
	virtual void		SetFrame(BView* view, BRect containingFrame, BPoint position);
	virtual void		DrawOn(BView* view, BRect clip, uint8 alpha);

	virtual AmToolControlVis* Copy() const;
	virtual status_t	WriteTo(BMessage& flattened) const;

private:
	typedef AmToolControlVis inherited;
	BBitmap*	mActiveBm;
	BBitmap*	mInactiveBm;
	BRect		mBounds;
};


#endif
