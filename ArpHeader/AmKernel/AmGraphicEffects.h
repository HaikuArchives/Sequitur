/* AmGraphicEffects.h
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
 * 2001.02.19		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMKERNEL_AMGRAPHICEFFECTS_H
#define AMKERNEL_AMGRAPHICEFFECTS_H

#include <be/app/MessageFilter.h>
#include <be/app/MessageRunner.h>
#include <be/interface/View.h>
#include "AmPublic/AmGraphicEffect.h"
#include "AmPublic/AmViewFactory.h"

class _AmLineSegment
{
public:
	_AmLineSegment(BPoint From, BPoint To, rgb_color C);

	_AmLineSegment&	operator=(const _AmLineSegment &o);
	void			Fade(uint8 step, rgb_color fadeC);
	void			AlphaFade(uint8 step);
	
	BPoint		from, to;
	rgb_color	c;
};

/***************************************************************************
 * AM-FADING-LINE-EFFECT
 * This is a "line" (actually it's a series of line segments) that fades
 * away from view.
 ***************************************************************************/
class AmFadingLineEffect : public AmGraphicEffect,
						   public BMessageFilter
{
public:
	AmFadingLineEffect(tool_id ownerId, uint32 maxLines = 128);
	virtual ~AmFadingLineEffect();

	virtual bool	IsFinished() const;
	virtual void	Begin(BView* target, BPoint pt);
	virtual void	MouseMoved(BPoint where, uint32 code);
	virtual void	DrawOn(BView* view, BRect clip);

	virtual	filter_result Filter(BMessage *message, BHandler **target);

	void		AddLine(BPoint from, BPoint to, rgb_color c);
	/* These two methods are mutually exclusive.  If you know the view
	 * you're drawing on allows alpha, use the alpha fade, otherwise use
	 * the normal fade.
	 */
	void		Fade(uint8 step = 30);
	void		AlphaFade(uint8 step = 30);

protected:
	void		SetFadeColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	void		SetFadeColor(rgb_color c);
	void		SetLineColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	void		SetLineColor(rgb_color c);

private:
	typedef AmGraphicEffect inherited;
	
	BView*		mTarget;
	uint32		mMaxLines;
	rgb_color	mFadeColor;
	rgb_color	mLineColor;
	bool		mHasLastPoint;
	BPoint		mLastPoint;
	BMessageRunner* mFadeRunner;
	
	vector<_AmLineSegment> mLines;

	void		CleanUp();
};

#endif 
