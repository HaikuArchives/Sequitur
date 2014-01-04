#include <ArpCore/StlVector.h>
#include <support/Errors.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlDegreeLine.h>
#include <GlPublic/GlLineFiller.h>

class GlLineFillerData
{
public:
	vector<GlFillingLine*>	mLines;
	
	GlLineFillerData()		{ }
	~GlLineFillerData()
	{
		for (uint32 k = 0; k < mLines.size(); k++) delete mLines[k];
	}
};

/***************************************************************************
 * GL-LINE-FILLER
 ****************************************************************************/
GlLineFiller::GlLineFiller()
		: mL(-1), mT(-1), mR(-1), mB(-1), mData(0)
{
	mData = new GlLineFillerData();
}

GlLineFiller::~GlLineFiller()
{
	delete mData;
}

status_t GlLineFiller::AddLine(GlFillingLine* line)
{
	if (!mData) return B_NO_MEMORY;
	mData->mLines.push_back(line);
	return B_OK;
}

status_t GlLineFiller::InitLines(uint32 count)
{
	if (!mData) return B_NO_MEMORY;
	for (uint32 k = 0; k < mData->mLines.size(); k++) {
		status_t	err = mData->mLines[k]->Resize(count);
		if (err != B_OK) return err;
	}
	return B_OK;
}

void GlLineFiller::InitLineVal(uint32 size, gl_filler_cell* cells)
{
}

void GlLineFiller::OffsetBounds(int32 changeX, int32 changeY)
{
	mL += changeX;
	mT += changeY;
	mR += changeX;
	mB += changeY;
}

bool GlLineFiller::FrameInBounds(int32 w, int32 h)
{
	return (!(w < mL || h < mT || 0 > mR || 0 > mB));
}

void GlLineFiller::InitFrame(	int32 w, int32 h, uint32 size,
								gl_filler_cell* cells)
{
	if (size < 1) return;

	mL = mR = cells[0].x;
	mT = mB = cells[0].y;
	for (uint32 k = 1; k < size; k++) {
		if (cells[k].x < mL)		mL = cells[k].x;
		else if (cells[k].x > mR)	mR = cells[k].x;

		if (cells[k].y < mT)		mT = cells[k].y;
		else if (cells[k].y > mB)	mB = cells[k].y;
	}
}

void GlLineFiller::GetFrame(int32* l, int32* t, int32* r, int32* b) const
{
	if (l) *l = mL;
	if (t) *t = mT;
	if (r) *r = mR;
	if (b) *b = mB;
}

void GlLineFiller::Print(int32 start, int32 end) const
{
	printf("GlLineFiller %ld line(s)\n", (mData) ? (mData->mLines.size()) : 0);
	if (!mData) return;
	for (uint32 k = 0; k < mData->mLines.size(); k++)
		mData->mLines[k]->Print(start, end, 1);
}

// #pragma mark -

/***************************************************************************
 * _GL-STD-FILLING-LINE
 ****************************************************************************/
class _GlStdFillingLine : public GlFillingLine
{
public:
	gl_filler_cell*			cells;
	uint32					size;
	uint32					start, end;			// The cells that are in bounds
												// (between 0,0 and mW, mH) after
												// an offset.
	
	_GlStdFillingLine() : cells(0), size(0), start(0), end(0), mW(-1), mH(-1)	{ }
	virtual ~_GlStdFillingLine()												{ Free(); }
	
	virtual status_t		Resize(uint32 newSize)
	{
		if (size == newSize) return B_OK;
		Free();
		if (newSize < 1) return B_OK;
		cells = new gl_filler_cell[newSize];
		if (!cells) return B_NO_MEMORY;
		size = newSize;
		return B_OK;
	}

	virtual void			Offset(int32 byX, int32 byY)
	{
		ArpASSERT(mW > 0 && mH > 0);
		start = end = 0;
		bool	first = false;
		for (uint32 k = 0; k < size; k++) {
			cells[k].x += byX;
			cells[k].y += byY;
			if (GL_IN_BOUNDS(cells[k].x, cells[k].y, mW, mH)) {
				if (!first) start = k;
				first = true;
				end = k + 1;
			}
		}
	}

	void					Init(int32 w, int32 h)
	{
		mW = w;
		mH = h;
		start = end = 0;
	}
	
private:
	int32					mW, mH;	// Cached during process
	
	void Free() { delete[] cells; cells = 0; size = 0; }

public:
	virtual void			Print(int32 start = 0, int32 end = -1, uint32 tabs = 0) const
	{
		uint32				tab;
		for (tab = 0; tab < tabs; tab++) printf("\t");
		printf("_GlStdFillingLine %ld cell(s)", size);
		if (size > 0) printf(" origin (%ld, %ld)", cells[0].x, cells[0].y);
		printf(" (%p)\n", this);
		
		if (start < 0) start = 0;
		if (end < 0 || end > int32(size)) end = size;
		for (int32 k = start; k < end; k++) {
			for (tab = 0; tab < tabs + 1; tab++) printf("\t");
			printf("%ld: (%ld, %ld) %f\n", k, cells[k].x, cells[k].y, cells[k].val);
		}
	}

	void					PrintInBounds(int32 w, int32 h, uint32 tabs = 0) const
	{
		uint32				tab;
		for (tab = 0; tab < tabs; tab++) printf("\t");
		printf("_GlStdFillingLine %ld cell(s)", size);
		if (size > 0) printf(" origin (%ld, %ld)", cells[0].x, cells[0].y);
		printf(" (%p)\n", this);
		
		for (uint32 k = 0; k < size; k++) {
			if (GL_IN_BOUNDS(cells[k].x, cells[k].y, w, h)) {
				for (tab = 0; tab < tabs + 1; tab++) printf("\t");
				printf("%ld: (%ld, %ld) %f\n", k, cells[k].x, cells[k].y, cells[k].val);
			}
		}
	}
};

/***************************************************************************
 * GL-LINE-SLICER
 ****************************************************************************/
GlLineSlicer::GlLineSlicer()
		: mLine(new _GlStdFillingLine())
{
	if (mLine) AddLine(mLine);
}

status_t GlLineSlicer::Perform(int32 w, int32 h, float angle)
{
	if (!mLine) return B_NO_MEMORY;
	ArpVALIDATE(w > 0 && h > 0, return B_ERROR);
	ArpVALIDATE(angle >= 0 && angle < 360, return B_ERROR);
	mLine->Init(w, h);

	status_t				err = B_ERROR;
	if (angle < 45)			err = HalfQuad(w, h, angle, -1,  1,	w-1, h-1,	0, -1);
	else if (angle < 90)	err = HalfQuad(w, h, angle, -1,  1,	0, 0,		1, 0);
	else if (angle < 135)	err = HalfQuad(w, h, angle, 1,   1,	w-1, 0,		-1, 0);
	else if (angle < 180)	err = HalfQuad(w, h, angle, 1,   1,	0, h-1,		0, -1);
	else if (angle < 225)	err = HalfQuad(w, h, angle, 1,  -1,	0, 0,		0, 1);
	else if (angle < 270)	err = HalfQuad(w, h, angle, 1,  -1,	w-1, h-1,	-1, 0);
	else if (angle < 315)	err = HalfQuad(w, h, angle, -1, -1,	0, h-1,		1, 0);
	else if (angle < 360)	err = HalfQuad(w, h, angle, -1, -1,	w-1, 0,		0, 1);
	return err;
}

status_t GlLineSlicer::HalfQuad(int32 w, int32 h, float angle,
								int8 xSign, int8 ySign,
								int32 originX, int32 originY,
								int32 changeX, int32 changeY)
{
	/* Init the lines */
	uint32					count = (w > h) ? w : h;
	ArpVALIDATE(count > 0, return B_ERROR);
	status_t				err = InitLines(count);
	if (err != B_OK) return err;
	/* Fill the coords of each cell */
	int32					x = originX, y = originY;
	GlDegreeLine			line(angle, float(x), float(y), xSign, ySign);
	for (uint32 k = 0; k < count; k++) {
		mLine->cells[k].x = x;
		mLine->cells[k].y = y;
		line.GetNext(&x, &y);
	}
	mLine->Offset(0, 0);
	InitLineVal(mLine->size, mLine->cells);
	/* Init the frame */
	InitFrame(w, h, count, mLine->cells);

	ArpASSERT(false);	// FIX:  Who calls this?  Need to figure out the steps
#if 0
	/* Find the number of steps */
	int32				ox = originX, oy = originY;
	uint32				c = 0;
	while (GL_IN_BOUNDS(ox, oy, w, h)) {
		ox += changeX;
		oy += changeY;
		c++;
	}
uint32	c2 = 0;
#endif
	/* Render each slice */
	while (FrameInBounds(w, h)) {
		FillLine(w, h, mLine->size, mLine->cells);
		mLine->Offset(changeX, changeY);
		OffsetBounds(changeX, changeY);
//c2++;
	}
//printf("c %ld real %ld\n", c, c2);
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNING-GAP-LINE
 * A line for filling in the gaps.  All I am is a series of indexes that
 * reference cells in the front line -- the Scan() method is told whether
 * the gaps are above, below, left or right of the referenced cell.
 ****************************************************************************/
class _GlScanningGapLine : public GlFillingLine
{
public:
	int32*					indexes;
	gl_filler_cell*			cells;
	uint32					size;
	
	_GlScanningGapLine() : indexes(0), cells(0), size(0)	{ }
	virtual ~_GlScanningGapLine()							{ Free(); }
	
	virtual status_t		Resize(uint32 newSize)
	{
		if (size == newSize) return B_OK;
		Free();
		if (newSize < 1) return B_OK;
		indexes = new int32[newSize];
		cells = new gl_filler_cell[newSize];
		if (!indexes || !cells) {
			Free();
			return B_NO_MEMORY;
		}
		size = newSize;
		return B_OK;
	}
	
private:
	void Free() { delete[] indexes; indexes = 0; delete[] cells; cells = 0; size = 0; }

public:
	virtual void			Print(int32 start = 0, int32 end = -1, uint32 tabs = 0) const
	{
		uint32				tab;
		for (tab = 0; tab < tabs; tab++) printf("\t");
		printf("_GlScanningGapLine %ld cell(s)\n", size);
		
		if (start < 0) start = 0;
		if (end < 0 || end > int32(size)) end = size;
		for (int32 k = start; k < end; k++) {
			if (indexes[k] < 0) return;
			for (tab = 0; tab < tabs + 1; tab++) printf("\t");
			printf("%ld: index %ld (%ld, %ld) val %f\n", k, indexes[k],
					cells[k].x, cells[k].y, cells[k].val);
		}
	}
};

/***************************************************************************
 * GL-LINE-SCANNER
 ****************************************************************************/
GlLineScanner::GlLineScanner()
		: mLine1(new _GlStdFillingLine()), mLine2(new _GlStdFillingLine()),
		  mGapLine(new _GlScanningGapLine())
{
	if (mLine1) AddLine(mLine1);
	if (mLine2) AddLine(mLine2);
	if (mGapLine) AddLine(mGapLine);
}

status_t GlLineScanner::Perform(int32 w, int32 h, float angle)
{
	if (!mLine1 || !mLine2 || !mGapLine) return B_NO_MEMORY;
	ArpVALIDATE(w > 0 && h > 0, return B_ERROR);
	ArpVALIDATE(angle >= 0 && angle < 360, return B_ERROR);
	mLine1->Init(w, h);
	mLine2->Init(w, h);
	status_t		err = B_ERROR;

	//												sign	perp	count	anchor	anchor x/y		gap offset	gap x/y
	if (angle < 45) err = Scan(w, h, angle,			-1, 1,	1, 1,	w + h,	w - 1,	w - 1, 0,		1,			1, 0);
	else if (angle < 90) err = Scan(w, h, angle,	-1, 1,	1, 1,	w + h,	w - 1,	w - 1, 0,		-1,			0, -1);
	else if (angle < 135) err = Scan(w, h, angle,	1, 1,	1, -1,	w + h,	h - 1,	0, 0,			1,			0, -1);
	else if (angle < 180) err = Scan(w, h, angle,	1, 1,	1, -1,	h + w,	h - 1,	0, 0,			-1,			-1, 0);
	else if (angle < 225) err = Scan(w, h, angle,	1, -1,	1, 1,	h + w,	h - 1,	0, h - 1,		-1,			-1, 0);
	else if (angle < 270) err = Scan(w, h, angle,	1, -1,	1, 1,	h + w,	h - 1,	0, h - 1,		1,			0, 1);
	else if (angle < 315) err = Scan(w, h, angle,	-1, -1,	1, -1,	h + w,	w - 1,	w - 1, h - 1,	-1,			0, 1);
	else if (angle < 360) err = Scan(w, h, angle,	-1, -1,	1, -1,	h + w,	w - 1,	w - 1, h - 1,	1,			1, 0);
	return err;
}

status_t GlLineScanner::Scan(	int32 w, int32 h, float angle,
								int8 signX, int8 signY, int8 perpSignX, int8 perpSignY,
								uint32 count, uint32 anchor, int32 anchorX, int32 anchorY,
								int32 gapOffset, int32 gapX, int32 gapY)
{
	if (count < 1) return B_ERROR;

	float		perpAngle = angle + 90;
	if (perpAngle > 359) perpAngle -= 360;

	/* Initialize cells */
	status_t	err = Init(w, h, count, perpAngle, perpSignX, perpSignY);
	if (err != B_OK) return err;
	/* Offset cells to anchor */
	int32		offsetX = anchorX - mLine1->cells[anchor].x,
				offsetY = anchorY - mLine1->cells[anchor].y;
	for (uint32 k = 0; k < count; k++) {
		mLine1->cells[k].x += offsetX;
		mLine1->cells[k].y += offsetY;

		mLine2->cells[k].x = mLine1->cells[k].x;
		mLine2->cells[k].y = mLine1->cells[k].y;

		mGapLine->indexes[k] = -1;
	}
	InitFrame(w, h, mLine1->size, mLine1->cells);
	mLine1->Offset(0, 0);

	/* Perform the scanning */
	return ScanLoop(w, h, angle, signX, signY, anchorX, anchorY, gapOffset, gapX, gapY);
}

status_t GlLineScanner::ScanLoop(	int32 w, int32 h, float angle, int8 signX, int8 signY,
									int32 anchorX, int32 anchorY,
									int32 gapOffset, int32 gapX, int32 gapY)
{
	ArpASSERT(mLine1 && mLine2 && mGapLine);
	mFoundGaps = false;
	
	_GlStdFillingLine*	frontLine = mLine1;
	_GlStdFillingLine*	backLine = mLine2;
	int32				x, y, prevX = anchorX, prevY = anchorY;
	GlDegreeLine		line(angle, float(anchorX), float(anchorY), signX, signY);
	int32				prevOffsetX = 0, prevOffsetY = 0;

	/* Count how many steps this will be.
	 */
	uint32				frame = 0, count = CountSteps(line, w, h, prevX, prevY);

	InitLineVal(frontLine->size, frontLine->cells);

#ifndef NDEBUG
uint32		c2 = 0;
#endif
	while (FrameInBounds(w, h)) {
#ifndef NDEBUG
c2++;
#endif
		float			f = frame / float(count);
		frame++;

		FillLine(w, h, frontLine->cells, 0, frontLine->size, f);
		DrawLine(w, h, frontLine->cells, 0, frontLine->size, f);
		/* Deal with any gaps created by a shift along both axes.
		 */
		if (prevOffsetX != 0 && prevOffsetY != 0) HandleGaps(	w, h, gapOffset, gapX, gapY, f,
																frontLine, backLine);
		/* After any gaps have been filled, fill the back line in
		 * with the front line data, in preparation for it becoming
		 * the new front line.
		 */
		for (uint32 k = 0; k < frontLine->size; k++) backLine->cells[k].val = frontLine->cells[k].val;

		line.GetNext(&x, &y);
		int32				offsetX = x - prevX, offsetY = y - prevY;
		OffsetBounds(offsetX, offsetY);
		backLine->Offset(prevOffsetX + offsetX, prevOffsetY + offsetY);

		/* leapfrog lines */
		_GlStdFillingLine*	tmpLine = frontLine;
		frontLine = backLine;
		backLine = tmpLine;

		prevX = x;
		prevY = y;
		prevOffsetX = offsetX;
		prevOffsetY = offsetY;
	}
#ifndef NDEBUG
ArpASSERT(count == c2);
//printf("c1 %ld c2 %ld\n", count, c2);
#endif
	return B_OK;
}

static bool _dist_1(int32 x1, int32 y1, int32 x2, int32 y2)
{
	int32		distX = ARP_ABS(x1 - x2), distY = ARP_ABS(y1 - y2);
	bool		ans = false;
	if (distX == 0 && distY == 1) ans = true;
	else if (distX == 1 && distY == 0) ans = true;

	if (!ans) printf("**_dist_1 (%ld, %ld) (%ld, %ld) dist x %ld dist y %ld\n",
			x1, y1, x2, y1, distX, distY);
	return ans;
}

void GlLineScanner::HandleGaps(	int32 w, int32 h, int32 gapOffset,
								int32 gapX, int32 gapY, float frame,
								_GlStdFillingLine* frontLine,
								_GlStdFillingLine* backLine)
{
	if (!mFoundGaps) FindGaps(w, h, gapOffset, gapX, gapY, frontLine, backLine);
	if (mGapLine->size < 1 || mGapLine->indexes[0] < 0) return;

	uint32			gapSize = 0;
	for (uint32 k = 0; k < mGapLine->size; k++) {
		if (mGapLine->indexes[k] < 0) break;
		int32			i = mGapLine->indexes[k];
		mGapLine->cells[k].x = frontLine->cells[i].x + gapX;
		mGapLine->cells[k].y = frontLine->cells[i].y + gapY;

		if (GL_IN_BOUNDS(mGapLine->cells[k].x, mGapLine->cells[k].y, w, h)) {
			int32		iPlusGap = i + gapOffset;
			int32		flStart = int32(frontLine->start), flEnd = int32(frontLine->end),
						blStart = int32(backLine->start), blEnd = int32(backLine->end);
			float		sum = 0, count = 0;
			if (i >= flStart && i < flEnd) {
				sum += frontLine->cells[i].val;
				count++;
			}
			if (iPlusGap >= blStart && iPlusGap < blEnd) {
				sum += backLine->cells[iPlusGap].val;
				count++;
			}
			ArpASSERT(_dist_1(mGapLine->cells[k].x, mGapLine->cells[k].y, frontLine->cells[i].x, frontLine->cells[i].y));
			ArpASSERT(_dist_1(mGapLine->cells[k].x, mGapLine->cells[k].y, backLine->cells[iPlusGap].x, backLine->cells[iPlusGap].y));

			if (iPlusGap >= flStart && iPlusGap < flEnd) {
				sum += frontLine->cells[iPlusGap].val;
				count++;
				ArpASSERT(_dist_1(mGapLine->cells[k].x, mGapLine->cells[k].y, frontLine->cells[iPlusGap].x, frontLine->cells[iPlusGap].y));
			}
			if (i >= blStart && i < blEnd) {
				sum += backLine->cells[i].val;
				count++;
				ArpASSERT(_dist_1(mGapLine->cells[k].x, mGapLine->cells[k].y, backLine->cells[i].x, backLine->cells[i].y));
			}
			mGapLine->cells[k].val = sum / count;
		}
		gapSize++;
	}
	DrawLine(w, h, mGapLine->cells, 0, gapSize, frame);
}

void GlLineScanner::FindGaps(	int32 w, int32 h, int32 gapOffset,
								int32 gapX, int32 gapY,
								_GlStdFillingLine* frontLine,
								_GlStdFillingLine* backLine)
{
	uint32			gaps = 0;
	for (int32 frontK = 0; frontK < int32(frontLine->size); frontK++) {
		int32		backK = frontK + gapOffset;
		if (backK >= 0 && backK < int32(backLine->size)) {
			int32	x = frontLine->cells[frontK].x + gapX,
					y = frontLine->cells[frontK].y + gapY;
			if (x != backLine->cells[backK].x || y != backLine->cells[backK].y) {
				mGapLine->indexes[gaps] = frontK;
				gaps++;
			}
		}
	}
	mFoundGaps = true;
}

status_t GlLineScanner::Init(	int32 w, int32 h, uint32 count, float perpAngle,
								int8 perpSignX, int8 perpSignY)
{
	status_t		err = InitLines(count);
	if (err != B_OK) return err;

	int32			x = 0, y = 0;
	GlDegreeLine	line(perpAngle, float(x), float(y), perpSignX, perpSignY);
	for (uint32 k = 0; k < count; k++) {
		mLine1->cells[k].x = x;
		mLine1->cells[k].y = y;
		line.GetNext(&x, &y);
	}
	return B_OK;
}

uint32 GlLineScanner::CountSteps(	GlDegreeLine line, int32 w, int32 h,
									int32 prevX, int32 prevY) const
{
	int32			l, t, r, b, x, y;
	uint32			c = 0;
	GetFrame(&l, &t, &r, &b);
	while (!(w < l || h < t || 0 > r || 0 > b)) {
		c++;
		line.GetNext(&x, &y);
		int32			offsetX = x - prevX, offsetY = y - prevY;

		l += offsetX;
		t += offsetY;
		r += offsetX;
		b += offsetY;
//printf("\t1: w %ld h %ld x %ld y %ld  (%ld, %ld, %ld, %ld)\n", w, h, cx, cy, cl, ct, cr, cb);

		prevX = x;
		prevY = y;
	}
	return c;
}
