/* GlLineFiller.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.07.23			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLLINEFILLER_H
#define GLPUBLIC_GLLINEFILLER_H

#include <support/SupportDefs.h>
class GlLineFillerData;
class _GlStdFillingLine;
class _GlScanningGapLine;

struct gl_filler_cell
{
	int32		x, y;
	float		val;
};

class GlFillingLine
{
public:
	virtual status_t		Resize(uint32 size) = 0;
	virtual void			Offset(int32 byX, int32 byY)		{ }
	
public:
	virtual void			Print(int32 start = 0, int32 end = -1, uint32 tabs = 0) const = 0;
};

/***************************************************************************
 * GL-LINE-FILLER
 * I use a line to fill a rectangular area.  What gets filled and how
 * is up to the subclass, all I do is guarantee that every cell in the target
 * is touched.  There are different ways of performing the fill.  See
 * the Slice() and Scan() methods for descriptions.
 ****************************************************************************/
class GlLineFiller
{
public:
	GlLineFiller();
	virtual ~GlLineFiller();

	/* This class does memory management for any filling lines.  I take
	 * ownership of whatever line is supplied.
	 */
	status_t			AddLine(GlFillingLine* line);

protected:
	status_t			InitLines(uint32 count);
	/* A callback for subclasses to initialize the val member of the
	 * base line, if they want.
	 */
	virtual void		InitLineVal(uint32 size, gl_filler_cell* cells);

	void				OffsetBounds(int32 changeX, int32 changeY);

	bool				FrameInBounds(int32 w, int32 h);

	/* Once the cell coords are assigned, the bounds (i.e. how much
	 * of the target rect the cells overlap) is assigned.
	 */
	void				InitFrame(	int32 w, int32 h, uint32 size,
									gl_filler_cell* cells);

	void				GetFrame(int32* l, int32* t, int32* r, int32* b) const;
	
private:
	int32				mL, mT, mR, mB;
	GlLineFillerData*	mData;
	
public:
	void	Print(int32 start = 0, int32 end = -1) const;
};

/***************************************************************************
 * GL-LINE-SLICER
 * I perform a fill by running a slice through the image.  For
 * example, if the angle is 0, then a slice constitutes a line
 * running from the left edge to the right edge.  I run a slice
 * for each Y in the target.  If the angle is 90, a slice is
 * a line running from the top edge to the bottom edge, and I run
 * one for each X in the target.  Etc.
 ****************************************************************************/
class GlLineSlicer : public GlLineFiller
{
public:
	status_t			Perform(int32 w, int32 h, float angle);

protected:
	GlLineSlicer();
	
	virtual void		FillLine(	int32 w, int32 h, uint32 count,
									gl_filler_cell* cells) = 0;

private:
	_GlStdFillingLine*	mLine;
	
	status_t			HalfQuad(	int32 w, int32 h, float angle,
									int8 xSign, int8 ySign,
									int32 originX, int32 originY,
									int32 changeX, int32 changeY);
};

/***************************************************************************
 * GL-LINE-SCANNER
 * I run a scan line through the image.  For example, if the
 * angle is 0, then a scan line runs from top to bottom, and
 * I move this over the image from right to left.  If the
 * angle is 180, the scan line moves from left to right.  If
 * the angle is 90, a scan line runs from left to right, and
 * I fill by moving from top to bottom.  Etc.
 ****************************************************************************/
class GlLineScanner : public GlLineFiller
{
public:
	status_t			Perform(int32 w, int32 h, float angle);

protected:
	GlLineScanner();

	/* The fill method is used to assign values to the val of the
	 * cells.  The draw method is used to assign the val in the cells
	 * to whatever area I'm drawing to.  The end is actually 1 past
	 * the end, so iterations should be   for (k = start; k < end; k++)
	 */
	virtual void		FillLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame) = 0;
	virtual void		DrawLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame) = 0;

private:
	/* The scanner keeps two lines of identical cells, and
	 * plays leapfrog with them.  It does this so it can find
	 * and interpolate the gaps caused when the line moves on
	 * both the x and y.
	 */
	_GlStdFillingLine*	mLine1;
	_GlStdFillingLine*	mLine2;
	_GlScanningGapLine*	mGapLine;

	bool				mFoundGaps;

	status_t			Scan(	int32 w, int32 h, float angle,
								int8 signX, int8 signY, int8 perpSignX, int8 perpSignY,
								uint32 count, uint32 anchor, int32 anchorX, int32 anchorY,
								int32 gapOffset, int32 gapX, int32 gapY);
	status_t			ScanLoop(	int32 w, int32 h, float angle, int8 signX, int8 signY,
									int32 anchorX, int32 anchorY, int32 gapOffset,
									int32 gapX, int32 gapY);

	void				HandleGaps(	int32 w, int32 h, int32 gapOffset,
									int32 gapX, int32 gapY, float frame,
									_GlStdFillingLine* frontLine,
									_GlStdFillingLine* backLine);
	void				FindGaps(	int32 w, int32 h, int32 gapOffset,
									int32 gapX, int32 gapY,
									_GlStdFillingLine* frontLine,
									_GlStdFillingLine* backLine);

	status_t			Init(	int32 w, int32 h, uint32 count, float perpAngle,
								int8 perpSignX, int8 perpSignY);
	uint32				CountSteps(	GlDegreeLine line, int32 w, int32 h,
									int32 prevX, int32 prevY) const;
};

#endif
