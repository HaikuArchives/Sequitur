/*
	
	ArpCoreTerminal.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPTERMINAL_ARPCORETERMINAL_H
#include "ArpTerminal/ArpCoreTerminal.h"
#endif

#ifndef ARPTERMINAL_ARPTERMINALMSG_H
#include "ArpTerminal/ArpTerminalMsg.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _BEEP_H
#include <support/Beep.h>
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef _SCROLLBAR_H
#include <interface/ScrollBar.h>
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _UTF8_H
#include <support/UTF8.h>
#endif

#include <cstring>

ArpMOD();

/* ----------------------------------------------------------
   ArpCoreTerminal::RowInfo class
   ---------------------------------------------------------- */
   
ArpCoreTerminal::RowInfo::RowInfo(int32 size=0)
	: text(NULL), style(NULL), cols(0), usedcols(0), refs(0),
	  div(false), dirty(true), conn(false)
{
	SetColumns(size);
}

ArpCoreTerminal::RowInfo::~RowInfo()
{
	delete text;
	delete style;
}

int32 ArpCoreTerminal::RowInfo::Deref()
{
	if( (--refs) <= 0 ) {
		delete this; return 0;
	}
	return refs;
}

void ArpCoreTerminal::RowInfo::SetColumns(int32 num)
{
	ichar * newtext = NULL;
	style_t * newstyle = NULL;
	ArpD(cdb << ADH << "RowInfo " << this << ": SetColums("
			<< num << ")" << std::endl);
	if( num > 0 ) {
		newtext = new ichar[num];
		newstyle = new style_t[num];
	}
	int32 pos = 0;
	if( text && newtext && style && newstyle ) {
		while( pos < cols && pos < num ) {
			newtext[pos] = text[pos];
			newstyle[pos] = style[pos];
			pos++;
		}
	}
	if( newtext && newstyle ) {
		while( pos < num ) {
			newtext[pos] = ' ';
			newstyle[pos] = TERM_STYLEPLAIN;
			pos++;
		}
	}
	delete text;
	delete style;
	text = newtext;
	style = newstyle;
	cols = num;
	if( usedcols > num ) usedcols = num;
	Dirty(true);
}

void ArpCoreTerminal::RowInfo::OpenColumns(int32 min)
{
	if( min > cols ) SetColumns(min);
}

void ArpCoreTerminal::RowInfo::Dirty(bool state)
{
	if( (dirty=state) != 0 ) { 
		ldirty=0;
		rdirty=cols-1;
	} else {
		ldirty=INT_MAX;
		rdirty=-1;
	}
}

void ArpCoreTerminal::RowInfo::Dirty(int32 left, int32 right)
{
	dirty=true;
	if( left < ldirty ) ldirty=left;
	if( right > rdirty ) rdirty=right;
}

#define VERIFY_ROWS 1

#ifdef VERIFY_ROWS
#define VERROWS(x) x
#else
#define VERROWS(x) ;
#endif

enum {
	TERM_NUMCOLORS = (ArpTerminalInterface::TERM_NUMTEXTCOLORS
					  - ArpTerminalInterface::TERM_CURSORCOLOR)
};
	
static const rgb_color pri_foregrounds[TERM_NUMCOLORS] = {
	{ 0x80, 0x80, 0x80, 0xff },				// cursor
	{ 0x00, 0x00, 0x00, 0xff },				// highlight
	{ 0x80, 0x80, 0x80, 0xff },				// the default foreground
	{ 0x00, 0x00, 0x00, 0xff },				// black
	{ 0xff, 0x00, 0x00, 0xff },				// red
	{ 0x00, 0xff, 0x00, 0xff },				// green
	{ 0xff, 0xff, 0x00, 0xff },				// yellow
	{ 0x00, 0x00, 0xff, 0xff },				// blue
	{ 0xff, 0x00, 0xff, 0xff },				// magenta
	{ 0x00, 0xff, 0xff, 0xff },				// cyan
	{ 0xff, 0xff, 0xff, 0xff },				// white
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff }				// extra
};

static const float scl_foregrounds[TERM_NUMCOLORS] = {
	-10,									// cursor
	0,										// highlight
	1.4,									// the default background
	1,										// black
	1,										// red
	1,										// green
	1,										// yellow
	1,										// blue
	1,										// magenta
	1,										// cyan
	1,										// white
	1,										// extra
	1,										// extra
	1,										// extra
	1,										// extra
	1,										// extra
	1,										// extra
	1										// extra
};

static const rgb_color pri_backgrounds[TERM_NUMCOLORS] = {
	{ 0x00, 0x00, 0xff, 0xff },				// cursor
	{ 0xff, 0xff, 0x00, 0xff },				// highlight
	{ 0x00, 0x00, 0x00, 0xff },				// the default background
	{ 0x00, 0x00, 0x00, 0xff },				// black
	{ 0xff, 0x00, 0x00, 0xff },				// red
	{ 0x00, 0xff, 0x00, 0xff },				// green
	{ 0xff, 0xff, 0x00, 0xff },				// yellow
	{ 0x00, 0x00, 0xff, 0xff },				// blue
	{ 0xff, 0x00, 0xff, 0xff },				// magenta
	{ 0x00, 0xff, 0xff, 0xff },				// cyan
	{ 0xff, 0xff, 0xff, 0xff },				// white
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff },				// extra
	{ 0x80, 0x80, 0x80, 0xff }				// extra
};

static const float scl_backgrounds[TERM_NUMCOLORS] = {
	-.5,									// cursor
	0,										// highlight
	1.4,									// the default background
	1,										// black
	1,										// red
	1,										// green
	1,										// yellow
	1,										// blue
	1,										// magenta
	1,										// cyan
	1,										// white
	1,										// extra
	1,										// extra
	1,										// extra
	1,										// extra
	1,										// extra
	1,										// extra
	1										// extra
};

const ulong ArpCoreTerminal::init_foregrounds[NUM_INITCOLORS] = {
	((0x00)<<16) | ((0x00)<<8) | 0x00,      // the default foreground
	((0x40)<<16) | ((0x40)<<8) | 0x40,      // black
	((0x40)<<16) | ((0x00)<<8) | 0x00,      // red
	((0x00)<<16) | ((0x40)<<8) | 0x00,      // green
	((0x40)<<16) | ((0x40)<<8) | 0x00,      // yellow
	((0x00)<<16) | ((0x00)<<8) | 0x40,      // blue
	((0x40)<<16) | ((0x00)<<8) | 0x40,      // magenta
	((0x00)<<16) | ((0x40)<<8) | 0x40,      // cyan
	((0x00)<<16) | ((0x00)<<8) | 0x00       // white
};

const ulong ArpCoreTerminal::init_backgrounds[NUM_INITCOLORS] = {
#if 1 // this is good for 8-bit
	((0xe0)<<16) | ((0xe0)<<8) | 0xe0,      // the default foreground
	((0xe0)<<16) | ((0xe0)<<8) | 0xe0,      // black
	((0xe0)<<16) | ((0xa0)<<8) | 0xa0,      // red
	((0xa0)<<16) | ((0xe0)<<8) | 0xa0,      // green
	((0xe0)<<16) | ((0xe0)<<8) | 0xa0,      // yellow
	((0xa0)<<16) | ((0xa0)<<8) | 0xe0,      // blue
	((0xe0)<<16) | ((0xa0)<<8) | 0xe0,      // magenta
	((0xa0)<<16) | ((0xe0)<<8) | 0xe0,      // cyan
	((0xff)<<16) | ((0xff)<<8) | 0xff       // white
#else // if 24-bit is being used, this is better...
	((0xe0)<<16) | ((0xe0)<<8) | 0xe0,      // the default foreground
	((0xe0)<<16) | ((0xe0)<<8) | 0xe0,      // black
	((0xff)<<16) | ((0xe0)<<8) | 0xe0,      // red
	((0xe0)<<16) | ((0xff)<<8) | 0xe0,      // green
	((0xff)<<16) | ((0xff)<<8) | 0xe0,      // yellow
	((0xe0)<<16) | ((0xe0)<<8) | 0xff,      // blue
	((0xff)<<16) | ((0xe0)<<8) | 0xff,      // magenta
	((0xe0)<<16) | ((0xff)<<8) | 0xff,      // cyan
	((0xff)<<16) | ((0xff)<<8) | 0xff       // white
#endif
};

ichar ArpCoreTerminal::defaultTranslation[translationLength];

#ifdef DEBUG
const bool ArpCoreTerminal::outputecho = true;
#endif

ArpCoreTerminal::ArpCoreTerminal(BRect frame, const char* name, ulong resizeMode, ulong flags)
	: BView(frame, name, resizeMode,
			  flags|B_WILL_DRAW|B_FRAME_EVENTS),
	  curMode(0),
	  lfChars("\n"), crChars("\r"), enterStream("\n"),
	  curStyle(TERM_STYLEPLAIN),
	  sizeRows(-1), sizeCols(-1),
	  winRows(0), winCols(0),
	  numRows(2), numCols(2),
	  lastNumRows(0), lastNumCols(0),
	  curRow(0), curCol(0),
	  overChar(false),
	  scrollAmount(0), scrollTop(0), scrollBottom(0),
	  scrollMode(AUTOSCROLL_INPUT),
	  historyUse(HISTORY_AGGRESSIVE),
	  updating(false), has_output(false),
	  history_len(0), history_last_len(0),
	  horiz_len(0), horiz_last_len(-1),
	  lastRow(0), lastCol(0),
	  dirtyTop(-1), dirtyBottom(-1),
	  cur_font_style(TERM_STYLEPLAIN),
	  hShown(false), encodingConv(-1)
{
	ArpD(cdb << ADH << "ArpCoreTerminal: Constructor." << std::endl);
	
	for( int32 i=0; i<0x100; i++ ) defaultTranslation[i] = (ichar)i;
	//for( int32 i=0x80; i<0x100; i++ ) defaultTranslation[i] = '?';
	//for( int32 i=0; i<0x100; i++ ) defaultTranslation[i] = (ichar)i;
	//for( int32 i=0; i<0x20; i++ ) defaultTranslation[i] = '?';
	//for( int32 i=0x7f; i<0xa0; i++ ) defaultTranslation[i] = '?';
	//defaultTranslation[0xff] = '?';
	TermSetTranslation(NULL);
	ArpD(cdb << ADH << "ArpCoreTerminal: Initializing colors." << std::endl);
	SetTintForeground(1.65);
	SetTintBackground(.35);
	ArpD(cdb << ADH << "ArpCoreTerminal: Initializing history." << std::endl);
	SetViewColor(ArpColor::Transparent);
	SetFont(be_fixed_font);
	TermSetEncoding(B_ISO1_CONVERSION);
	TermSetHighlight(TERM_HIGHLIGHTOFF,0,0,0,0);
	ClearDirty();
	SetHistorySize(500);
}

ArpCoreTerminal::~ArpCoreTerminal()
{
}

void ArpCoreTerminal::SetTintForeground(float level)
{
	for( int32 i=TERM_CURSORCOLOR; i<TERM_NUMTEXTCOLORS; i++ ) {
		int32 ci = i-TERM_CURSORCOLOR;
		float s = scl_foregrounds[ci];
		float l = level;
		if( s < 0 ) {
			l = 2.0-l;
			s = -s;
		}
		l = 1.0 + (l-1.0)*s;
		if( l < 0 ) l = 0;
		if( l > 2 ) l = 2;
		TermSetTextForeground(tint_color(pri_foregrounds[ci], l), i);
	}
}

void ArpCoreTerminal::SetTintBackground(float level)
{
	for( int32 i=TERM_CURSORCOLOR; i<TERM_NUMTEXTCOLORS; i++ ) {
		int32 ci = i-TERM_CURSORCOLOR;
		float s = scl_backgrounds[ci];
		float l = level;
		if( s < 0 ) {
			l = 2.0-l;
			s = -s;
		}
		l = 1.0 + (l-1.0)*s;
		if( l < 0 ) l = 0;
		if( l > 2 ) l = 2;
		TermSetTextBackground(tint_color(pri_backgrounds[ci], l), i);
	}
}

void ArpCoreTerminal::SetOwner(const BMessenger& hand)
{
	mOwner = hand;
}

BMessenger ArpCoreTerminal::Owner(void) const
{
	if( mOwner.IsValid() ) return mOwner;
	else if( Window() ) return BMessenger(Window());
	return mOwner;
}

/*	------------------------------------------------------------
	HISTORY MANIPULATION METHODS.
	------------------------------------------------------------ */

void ArpCoreTerminal::SetHistorySize(int32 size)
{
	if( size < numRows ) size = numRows;
	page.SetSize(size);
	if( history_len > size ) history_len = size;
	history_last_len = -1;
	UpdateBounds();
}

int32 ArpCoreTerminal::HistorySize(void) const
{
	return page.Size();
}

void ArpCoreTerminal::SetHistoryUse(HistoryUseType use)
{
	historyUse = use;
}

ArpCoreTerminal::HistoryUseType ArpCoreTerminal::HistoryUse(void) const
{
	return historyUse;
}

/*	------------------------------------------------------------
	SCREEN LAYOUT METHODS.
	------------------------------------------------------------ */

void ArpCoreTerminal::SetAutoScrollMode(AutoScrollType scroll)
{
	scrollMode = scroll;
}

ArpCoreTerminal::AutoScrollType ArpCoreTerminal::AutoScrollMode(void) const
{
	return scrollMode;
}

void ArpCoreTerminal::AutoScrollTo(int32* row, int32* col)
{
	if( !row && !col ) return;
	
	BRect bnd(Bounds());
	float ypos = bnd.top;
	float xpos = bnd.left;

	ArpD(cdb << ADH << "ScrollTo: row="
			<< (row ? *row : -10000) << ", col="
			<< (col ? *col : -10000) << ", y=" << ypos
			<< ", x=" << xpos << std::endl);
	if( row ) {
		ypos = RowToY(*row);
		if( ypos + bnd.Height() > (DataHeight()*CharHeight()) ) {
			ypos = (DataHeight()*CharHeight()) - bnd.Height() - 1;
		}
		if( ypos < 0 ) ypos = 0;
	}
	if( col ) {
		xpos = ColToX(*col);
		if( xpos + bnd.Width() > (horiz_len*CharWidth()) ) {
			xpos = (horiz_len*CharWidth()) - bnd.Width() - 1;
		}
		if( xpos < 0 ) xpos = 0;
	}
	
	ArpD(cdb << ADH << "ScrollTo final: y=" << ypos
			<< ", x=" << xpos << std::endl);

	#if 0
	BScrollBar* bar = ScrollBar(B_VERTICAL);
	if( bar ) bar->SetValue(ypos);
	bar = ScrollBar(B_HORIZONTAL);
	if( bar ) bar->SetValue(xpos);
	#endif
	
	ScrollTo(BPoint(xpos,ypos));
}

void ArpCoreTerminal::MakePosVisible(int32 row, int32 col)
{
	int32* dorow = NULL;
	int32* docol = NULL;
	
	int32 visrows=0, viscols=0;
	TermGetViewSize(&visrows,&viscols);
	
	ArpD(cdb << ADH << "MakeVis: row=" << row << ", col=" << col
			<< ", visrows=" << visrows << ", viscols=" << viscols << std::endl);
			
	if( row <= (TopVisibleRow()+1) ) {
		row-=(visrows/4)+1;
		if( row < 0 ) row = 0;
		if( row < TopVisibleRow() ) dorow = &row;
	}
	if( row >= (BottomVisibleRow()-2) ) {
		row+=(visrows/4)+1;
		if( row >= (numRows-1) ) row = numRows-1;
		if( row > (BottomVisibleRow()-1) ) {
			row -= visrows-1;
			dorow = &row;
		}
	}

	if( col <= (LeftVisibleCol()+1) ) {
		col-=(viscols/4)+1;
		if( col < 0 ) col = 0;
		if( col < LeftVisibleCol() ) docol = &col;
	}
	if( col >= (RightVisibleCol()-2) ) {
		col+=(viscols/4)+1;
		if( col >= (horiz_len-1) ) col = horiz_len-1;
		if( col > (RightVisibleCol()-1) ) {
			col -= viscols-1;
			docol = &col;
		}
	}

#if 0
	if( col <= LeftVisibleCol() && col > 0 ) {
		if( col > 0 ) {
			col--;
			docol = &col;
		} else if( col < LeftVisibleCol() ) docol = &col;
	}
	if( col >= (RightVisibleCol()-1) && col < (horiz_len-1) ) {
		col = col-horiz_len+2;
		docol = &col;
	}
#endif
	
	ArpD(cdb << ADH << "VisTo: dorow=" << dorow << ", docol=" << docol
			<< ", row=" << row << ", col=" << col << std::endl);
	ArpD(cdb << ADH << "VisTo: top=" << TopVisibleRow()
			<< ", bottom=" << BottomVisibleRow()
			<< ", left=" << LeftVisibleCol()
			<< ", right=" << RightVisibleCol() << std::endl);
	
	if( dorow || docol ) AutoScrollTo(dorow,docol);
}

void ArpCoreTerminal::InitTerminal()
{
	ArpD(cdb << ADH << "ArpCoreTerminal: Initializing screen..." << std::endl);

	charWidth = fontPlain.StringWidth("M");
	font_height height;
	fontPlain.GetHeight(&height);
	charHeight = ceil(height.ascent+height.descent+height.leading);
	fontAscent = height.ascent;
	fontDescent = height.descent;

	TermSetFixedSize(sizeRows,sizeCols);
	UpdateBounds();
}

bool ArpCoreTerminal::ResizeTerminal(int32 rows, int32 cols)
{
	if( cols < 2 ) cols = 2;
	if( rows < 2 ) rows = 2;

	numRows = rows;
	numCols = cols;
	
	ArpD(cdb << ADH << "Frame=" << Frame() << std::endl);
	ArpD(cdb << ADH << "Columns = " << cols << ", Rows = " << rows << std::endl);

	if( lastNumRows == rows && lastNumCols == cols ) return false;

#if 0
	printf("Resize: lastNumRows=%ld lastNumCols=%ld numRows=%ld numCols=%ld\n",
				lastNumRows, lastNumCols, numRows, numCols);
	printf("Resize: lastRow=%ld lastCol=%ld curRow=%ld curCol=%ld\n",
				lastRow, lastCol, curRow, curCol);
#endif
	int32 lastMode = TermMode();
	TermSetMode(lastMode|TERM_MODEHIDECURSOR);
	drawRowCol(RowToPage(lastRow+numRows-lastNumRows),lastCol);
	//SetDirty(curRow,curRow,curCol,curCol);
	TermSetMode(lastMode);
	
	// Move the cursor row so that it follows the view bottom.
	//if( curRow >= lastNumRows-1 ) curRow += rows-lastNumRows;
	curRow += rows-lastNumRows;
	
	// Move the last cursor row so that it follows the view bottom.
	//lastRow += rows-numRows;

	// Move the top/bottom region so that it follows the view, unless the
	// regionTop is at the top edge of the view, in which case
	// only the bottom is moved.  (Got that??  It's about the
	// best guess we can make...)
	//if( regionTop > 0 ) regionTop += rows-lastNumRows;
	if( regionBottom >= lastNumRows-1 ) regionBottom = numRows-1;

	// Move the right region so that it follows the view, if and only if
	// it is currently set to the far right column.
	if( regionRight >= lastNumCols-1 ) regionRight = numCols-1;
	
	lastNumRows = rows;
	lastNumCols = cols;
	if( curCol >= cols ) curCol = cols-1;
	if( curCol < 0 ) curCol = 0;
	if( curRow >= rows ) curRow = rows-1;
	if( curRow < 0 ) curRow = 0;
	TermSetRegion(regionTop, regionBottom, regionLeft, regionRight);

	if( page.Size() < (numRows+1) ) {
		page.SetSize(numRows+1);
	}

	ArpD(cdb << ADH << "ArpCoreTerminal: Allocating rows" << std::endl);
	for( int32 r = 0; r < numRows; r++ ) {
		RowInfo* ri = page.GetElem(r);
		// We make this one column bigger than needed, to make it easy to
		// display a cut-off character on the far right.
		if( ri == NULL ) {
			page.SetElem(r,new RowInfo(numCols));
		} else {
			ri->OpenColumns(numCols);
		}
	}

	ArpD(cdb << ADH << "ArpCoreTerminal: Initializing history" << std::endl);
	if( history_len < numRows ) history_len = numRows;
	
	if( !updating ) {
		SetDirty(0,numRows-1);
		history_last_len = -1;

		VERROWS(for( int32 j=0; j<numRows; j++ ) assert(GetRowInfo(j) != NULL));

		UpdateBounds();
	} else {
		drawRowCol(RowToPage(curRow),curCol);
		//SetDirty(curRow,curRow,curCol,curCol);
	}
	
	TermSizeChanged(numRows,numCols);
	
	return true;
}

void ArpCoreTerminal::TermReset(bool hard)
{
	ArpD(cdb << ADH << "Doing reset of terminal." << std::endl);
	curStyle = TERM_STYLEPLAIN;
	TermSetTranslation(NULL);
	if( hard ) {
		curMode = 0;
		TermSetHighlight(TERM_HIGHLIGHTOFF,0,0,0,0);
		numCols--;
		InitTerminal();
		for( int32 r = 0; r < numRows; r++ ) {
			page.SetElem(r,new RowInfo(numCols));
		}
		curCol = 0;
		curRow = 0;
	}
	TermSetRegion();
	SetDirty(0,numRows-1);
	SetDirty(TopVisibleRow(),BottomVisibleRow());
}

bool ArpCoreTerminal::TermSetFixedSize(int32 rows, int32 cols)
{
	sizeRows = rows;
	sizeCols = cols;
	if( rows < 0 ) rows = winRows;
	if( cols < 0 ) cols = winCols;
	if( rows < 2 ) rows = 2;
	if( cols < 2 ) cols = 2;
	//printf("Resize terminal to: %ld x %ld\n",rows,cols);
	return ResizeTerminal(rows,cols);
}

void ArpCoreTerminal::TermGetFixedSize(int32* rows, int32* cols) const
{
	if( rows ) *rows = sizeRows;
	if( cols ) *cols = sizeCols;
}

void ArpCoreTerminal::TermGetViewSize(int32* rows, int32* cols) const
{
	if( rows ) *rows = winRows;
	if( cols ) *cols = winCols;
}

void ArpCoreTerminal::TermGetSize(int32* rows, int32* cols) const
{
	if( rows ) *rows = numRows;
	if( cols ) *cols = numCols;
}

int32 ArpCoreTerminal::TermHistoryTop(void) const
{
	return -(history_len-numRows+1);
}

/* ------------------------------------------------------------
	 SIMPLE MANIPULATION METHODS.
	 ------------------------------------------------------------ */

void ArpCoreTerminal::TermSetCursorPos(int32 row, int32 col)
{
	if( row < 0 ) row = 0;
	if( row >= numRows ) row = numRows-1;
	if( col < 0 ) col = 0;
	if( col >= numCols ) col = numCols-1;
	curRow = row;
	curCol = col;
	overChar = false;
}

void ArpCoreTerminal::TermGetCursorPos(int32* row, int32* col) const
{
	if( row ) *row = curRow;
	if( col ) *col = curCol;
}

int32 ArpCoreTerminal::TermXToCol(float x) const
{
	int32 col = int32((x)/charWidth);
	if( col < 0 ) col = 0;
	if( col >= numCols ) col = numCols-1;
	return col;
}

int32 ArpCoreTerminal::TermYToRow(float y) const
{
	int32 row = IRawYToRow(y);
	if( row < 0 ) row = 0;
	if( row >= numRows ) row = numRows-1;
	return row;
}

void ArpCoreTerminal::TermSetRegion()
{
	regionTop = 0;
	regionBottom = numRows-1;
	regionLeft = 0;
	regionRight = numCols-1;
}

void ArpCoreTerminal::TermSetRegion(int32 top, int32 bottom)
{
	TermSetRegion(top,bottom,0,numCols-1);
}

void ArpCoreTerminal::TermSetRegion(int32 top, int32 bottom, int32 left, int32 right)
{
	if( top < 0 ) top = 0;
	if( top >= numRows ) top = numRows-1;
	if( bottom < 0 ) bottom = 0;
	if( bottom >= numRows ) bottom = numRows-1;
	if( top >= bottom ) {
		if( top > 1 ) top = 0;
		else bottom++;
	}
	if( left < 0 ) left = 0;
	if( left >= numCols ) left = numCols-1;
	if( right < 0 ) right = 0;
	if( right >= numCols ) right = numCols-1;
	if( left >= right-2 ) {
		if( left > 2 ) left = 0;
		else right = numCols-1;
	}
	//if( !(bottom != 1 || bottom > regionBottom) ) {
	//	cdb << "Uh oh!" << std::endl;
	//}
	regionTop = top;
	regionBottom = bottom;
	regionLeft = left;
	regionRight = right;
	ArpD(cdb << ADH << "Region set to: top=" << regionTop
			<< ", bottom=" << regionBottom
			<< ", left=" << regionLeft << ", right=" << regionRight << std::endl);
}

void ArpCoreTerminal::TermGetRegion(int32* top, int32* bottom,
									int32* left, int32* right) const
{
	if( top ) *top = regionTop;
	if( bottom ) *bottom = regionBottom;
	if( left ) *left = regionLeft;
	if( right ) *right = regionRight;
}

void ArpCoreTerminal::ConformHighlight(TermHighlightType type,
									int32& top, int32& tcol,
									int32& bottom, int32& bcol)
{
	if( type == TERM_HIGHLIGHTLINE ) {
		tcol = -1;
		bcol = 32768;
	} else if( type == TERM_HIGHLIGHTWORD ) {
		RowInfo* ri = NULL;
		if( (ri = GetRowInfo(top)) != NULL ) {
			ichar* text = ri->Text();
			while( (tcol-1) >= 0 && tcol < ri->UsedCols()
				&& text[tcol-1] != ' ') tcol--;
		}
		if( (ri = GetRowInfo(bottom)) != NULL ) {
			ichar* text = ri->Text();
			while( bcol >= 0 && (bcol+1) < ri->UsedCols()
				&& text[bcol+1] != ' ') bcol++;
		}
	}
}

void ArpCoreTerminal::TermSetHighlight(TermHighlightType type,
									int32 row1, int32 col1,
									int32 row2, int32 col2)
{
	if( type == TERM_HIGHLIGHTOFF ) {
		if( hShown ) {
			SetDirty(PageToRow(hTopRow),PageToRow(hBotRow));
			hShown = false;
		}
		hTopRow = hTopCol = -1;
		hBotRow = hBotCol = 1<<31;

	} else {
		hShown = true;
		row1 = RowToPage(row1);
		row2 = RowToPage(row2);
		if( row2 > row1 || (row2 == row1 && col2 < col1) ) {
			// swap so that row/col 1 is on top
			row1 ^= row2;
			row2 ^= row1;
			row1 ^= row2;
			col1 ^= col2;
			col2 ^= col1;
			col1 ^= col2;
		}
		row1 = PageToRow(row1);
		row2 = PageToRow(row2);
		ConformHighlight(type,row1,col1,row2,col2);
		row1 = RowToPage(row1);
		row2 = RowToPage(row2);
		
		if( hMode == TERM_HIGHLIGHTOFF ) {
			SetDirty(PageToRow(row1),PageToRow(row2));

		} else {
			if( hTopRow > row1 )
				SetDirty(PageToRow(hTopRow),PageToRow(row1));
			else if( hTopRow < row1 )
				SetDirty(PageToRow(row1),PageToRow(hTopRow));
			else if( hTopCol < col1 )
				SetDirty(PageToRow(row1),PageToRow(row1),
						hTopCol, col1);
			else if( hTopCol > col1 )
				SetDirty(PageToRow(row1),PageToRow(row1),
						col1, hTopCol);
			if( hBotRow > row2 )
				SetDirty(PageToRow(hBotRow),PageToRow(row2));
			else if( hBotRow < row2 )
				SetDirty(PageToRow(row2),PageToRow(hBotRow));				
			else if( hBotCol < col2 )
				SetDirty(PageToRow(row2),PageToRow(row2),
						hBotCol, col2);
			else if( hBotCol > col2 )
				SetDirty(PageToRow(row2),PageToRow(row2),
						col2, hBotCol);
		}
		hTopRow = row1;
		hTopCol = col1;
		hBotRow = row2;
		hBotCol = col2;
	}
	
	TermClean();
	hMode = type;
}

bool ArpCoreTerminal::TermInHighlight(int32 row, int32 col) const
{
	// If highlight is off, can't be in any highlight region.
	if( hMode == TERM_HIGHLIGHTOFF ) return false;
	
	// If there is no text under this location with which to display
	// a highlight, never identify it as being within a highlight region.
	const RowInfo* ri = GetRowInfo(row);
	if( !ri || col > ri->UsedCols() ) return false;
	
	// Convert the location row into page coordinates for comparison
	// with current highlight state.
	row = RowToPage(row);
	
	// Single line highlight...
	if( row == hTopRow && row == hBotRow ) {
		if( col >= hTopCol && col <= hBotCol ) {
			return true;
		}
		return false;
	}
	
	// Multi-line highlight...
	if( row == hTopRow && col >= hTopCol ) return true;
	if( row < hTopRow && row > hBotRow ) return true;
	if( row == hBotRow && col <= hBotCol ) return true;
	
	return false;
}

void ArpCoreTerminal::TermGetHighlight(TermHighlightType* type,
									int32* row1, int32* col1,
									int32* row2, int32* col2) const
{
	if( hShown ) {
		if( type ) *type = hMode;
		if( row1 ) *row1 = PageToRow(hTopRow);
		if( col1 ) *col1 = hTopCol;
		if( row2 ) *row2 = PageToRow(hBotRow);
		if( col2 ) *col2 = hBotCol;
	} else {
		if( type ) *type = TERM_HIGHLIGHTOFF;
		if( row1 ) *row1 = 0;
		if( col1 ) *col1 = 0;
		if( row2 ) *row2 = 0;
		if( col2 ) *col2 = 0;
	}
}

void ArpCoreTerminal::TermGetTextRange(int32 row1, int32 col1,
									int32 row2, int32 col2,
									size_t* array_size,
									ichar* text, style_t* style) const
{
	row1 = RowToPage(row1);
	row2 = RowToPage(row2);
	ArpD(cdb << ADH << "ArpCoreTerminal GetRange: row1=" << row1
			<< ", col1=" << col1 << ", row2=" << row2
			<< ", col2=" << col2);
	if( row2 > row1 || (row2 == row1 && col2 < col1) ) {
		ArpD(cdb << ADH << "; swap");
		// swap so that row/col 1 is on top
		row1 ^= row2;
		row2 ^= row1;
		row1 ^= row2;
		col1 ^= col2;
		col2 ^= col1;
		col1 ^= col2;
	}
	ArpD(cdb << ADH << "." << std::endl);
	if( row1 < 0 ) row1 = 0;
	if( row1 >= history_len ) row1 = history_len-1;
	if( row2 < 0 ) row2 = 0;
	if( row2 >= history_len ) row2 = history_len-1;
	
	size_t avail = 0;
	if( array_size ) {
		avail = *array_size;
		*array_size = 0;
	}
	
	int32 leftlimit = col1;
	int32 rightlimit = 32767;
	style_t last_style = TERM_STYLEPLAIN;
	
	while( row1 >= row2 && leftlimit <= rightlimit ) {
		if( row1 == row2 ) rightlimit = col2;
		ArpD(cdb << ADH << "Region row " << row1
					<< ": left=" << leftlimit
					<< ", right=" << rightlimit << std::endl);
		const RowInfo* ri = page[row1];
		if( ri && leftlimit <= ri->UsedCols() && rightlimit >= 0 ) {
			int32 left = leftlimit;
			int32 right = rightlimit;
			if( left < 0 ) left = 0;
			int32 amount = right-left+1;
			if( right >= ri->UsedCols() ) {
				amount = ri->UsedCols()-left;
			}
			ArpD(cdb << ADH << amount << " chars selected with conn="
						<< ri->Connected() << std::endl);
			if( amount > 0 ) {
				if( left < ri->UsedCols() ) {
					size_t can_use = (int32)avail < amount ? avail:amount;
					ArpD(cdb << ADH << ", can use " << can_use << std::endl);
					if( text && can_use > 0 ) {
						memcpy(text,&(ri->Text()[left]),sizeof(ichar)*can_use);
						text += can_use;
					}
					if( style && can_use > 0 ) {
						memcpy(style,&(ri->Style()[left]),sizeof(style_t)*can_use);
						style += can_use;
						last_style = *(style-1);
					}
					if( array_size ) *array_size += amount;
					avail -= can_use;
				}
			}
			if( (amount > 0 || left == ri->UsedCols())
				&& right >= ri->UsedCols() && !ri->Connected() ) {
				ArpD(cdb << ADH << ", has newline");
				const char* p = TermEnterStream();
				if( array_size ) (*array_size) += p ? strlen(p) : 0;
				while( avail > 0 && p && *p ) {
					if( text ) *(text++) = *(p++);
					if( style ) *(style++) = last_style;
					avail--;
				}
			}
			ArpD(cdb << ADH << "." << std::endl);
		}
		leftlimit = 0;
		row1--;
	}
}

void ArpCoreTerminal::TermSetStyle(int32 style)
{
	curStyle = (style&TERM_STYLEMASK) | (curStyle&~TERM_STYLEMASK);
}

void ArpCoreTerminal::TermSetForeColor(TermColorID col)
{
	if( col < 0 ) col = 0;
	else if( col >= TERM_NUMTEXTCOLORS ) col = TERM_NUMTEXTCOLORS-1;
	curStyle = ((col&TERM_STYLECOLOR_MASK)<<TERM_STYLEFOREGROUND_POS)
		| (curStyle&~(TERM_STYLECOLOR_MASK<<TERM_STYLEFOREGROUND_POS));
}

void ArpCoreTerminal::TermSetBackColor(TermColorID col)
{
	if( col < 0 ) col = 0;
	else if( col >= TERM_NUMTEXTCOLORS ) col = TERM_NUMTEXTCOLORS-1;
	curStyle = ((col&TERM_STYLECOLOR_MASK)<<TERM_STYLEBACKGROUND_POS)
		| (curStyle&~(TERM_STYLECOLOR_MASK<<TERM_STYLEBACKGROUND_POS));
}

int32 ArpCoreTerminal::TermStyle(void) const
{
	return curStyle&TERM_STYLEMASK;
}

ArpTerminalInterface::TermColorID ArpCoreTerminal::TermForeColor(void) const
{
	return (curStyle>>TERM_STYLEFOREGROUND_POS) & TERM_STYLECOLOR_MASK;
}

ArpTerminalInterface::TermColorID ArpCoreTerminal::TermBackColor(void) const
{
	return (curStyle>>TERM_STYLEBACKGROUND_POS) & TERM_STYLECOLOR_MASK;
}

void ArpCoreTerminal::TermSetMode(TermModeType mode)
{
	// don't allow old mode flags
	mode &= ~(TermModeType)((1<<1) | (1<<6));
	if( (curMode&TERM_MODEINVERSE) != (int32)(mode&TERM_MODEINVERSE) ) {
		SetDirty(TopVisibleRow(),BottomVisibleRow());
	} else if( (curMode&TERM_MODEHIDECURSOR) != (int32)(mode&TERM_MODEHIDECURSOR) ) {
		SetDirty(curRow,curRow,curCol,curCol);
	}
	curMode = mode;
}

ArpTerminalInterface::TermModeType ArpCoreTerminal::TermMode(void) const
{
	return curMode;
}

void ArpCoreTerminal::TermSetLFChars(const char* which)
{
	lfChars = which;
}

const char* ArpCoreTerminal::TermLFChars() const
{
	return lfChars.String();
}

void ArpCoreTerminal::TermSetCRChars(const char* which)
{
	crChars = which;
}

const char* ArpCoreTerminal::TermCRChars() const
{
	return crChars.String();
}

void ArpCoreTerminal::TermSetEnterStream(const char* str)
{
	enterStream = str;
}

const char* ArpCoreTerminal::TermEnterStream() const
{
	return enterStream.String();
}
	
void ArpCoreTerminal::TermSetTranslation(const ichar* t)
{
	if( t == NULL ) t = &defaultTranslation[0];
	for( int32 i=0; i<translationLength; i++ ) {
		curTranslation[i] = t[i];
	}
}

const ichar* ArpCoreTerminal::TermTranslation(void) const
{
	return curTranslation;
}

void ArpCoreTerminal::TermSetEncoding(int32 enc)
{
	if( encodingConv == enc ) return;
	
	encodingConv = enc;
	switch( encodingConv ) {
		case -1:						encoding = B_UNICODE_UTF8;		break;
		case B_ISO1_CONVERSION:			encoding = B_ISO_8859_1;		break;
		case B_ISO2_CONVERSION:			encoding = B_ISO_8859_2;		break;
		case B_ISO3_CONVERSION:			encoding = B_ISO_8859_3;		break;
		case B_ISO4_CONVERSION:			encoding = B_ISO_8859_4;		break;
		case B_ISO5_CONVERSION:			encoding = B_ISO_8859_5;		break;
		case B_ISO6_CONVERSION:			encoding = B_ISO_8859_6;		break;
		case B_ISO7_CONVERSION:			encoding = B_ISO_8859_7;		break;
		case B_ISO8_CONVERSION:			encoding = B_ISO_8859_8;		break;
		case B_ISO9_CONVERSION:			encoding = B_ISO_8859_9;		break;
		case B_ISO10_CONVERSION:		encoding = B_ISO_8859_10;		break;
		case B_MAC_ROMAN_CONVERSION:	encoding = B_MACINTOSH_ROMAN;	break;
		default:						encoding = B_UNICODE_UTF8;		break;
		#if 0
		case B_UNICODE_UTF8:	encodingConv = -1;						break;
		case B_ISO_8859_1:		encodingConv = B_ISO1_CONVERSION;		break;
		case B_ISO_8859_2:		encodingConv = B_ISO2_CONVERSION;		break;
		case B_ISO_8859_3:		encodingConv = B_ISO3_CONVERSION;		break;
		case B_ISO_8859_4:		encodingConv = B_ISO4_CONVERSION;		break;
		case B_ISO_8859_5:		encodingConv = B_ISO5_CONVERSION;		break;
		case B_ISO_8859_6:		encodingConv = B_ISO6_CONVERSION;		break;
		case B_ISO_8859_7:		encodingConv = B_ISO7_CONVERSION;		break;
		case B_ISO_8859_8:		encodingConv = B_ISO8_CONVERSION;		break;
		case B_ISO_8859_9:		encodingConv = B_ISO9_CONVERSION;		break;
		case B_ISO_8859_10:		encodingConv = B_ISO10_CONVERSION;		break;
		case B_MACINTOSH_ROMAN:	encodingConv = B_MAC_ROMAN_CONVERSION;	break;
		default:
			encoding = B_ISO_8859_1;
			encodingConv = B_ISO1_CONVERSION;
			break;
		#endif
	}
	fontPlain.SetEncoding(encoding);
	fontBold.SetEncoding(encoding);
	fontItalic.SetEncoding(encoding);
	fontBoldItalic.SetEncoding(encoding);
	BView::SetFont(&fontPlain,B_FONT_ENCODING);
	Invalidate();
}

int32 ArpCoreTerminal::TermEncoding() const {
	return encodingConv;
}

void ArpCoreTerminal::TermSetTitle(const char* title) {
	BMessage titleMsg(TERM_SET_TITLE_MSG);
	if( title ) titleMsg.AddString("title", title);
	Owner().SendMessage(&titleMsg);
}

void ArpCoreTerminal::TermSetTextBackground(rgb_color bg, TermColorID num)
{
	const ArpColor mybg(bg);
	bool changed = false;
	
	if( num >= 0 && num < TERM_NUMTEXTCOLORS ) {
		if( textBackground[num] != mybg ) changed = true;
		textBackground[num] = mybg;
	} else if( num == TERM_HIGHLIGHTCOLOR ) {
		if( highBackground != mybg ) changed = true;
		highBackground = mybg;
	} else if( num == TERM_CURSORCOLOR ) {
		if( cursorBackground != mybg ) changed = true;
		cursorBackground = mybg;
	}
	
	if( changed ) SetDirty(TopVisibleRow(),BottomVisibleRow());
}

void ArpCoreTerminal::TermSetTextForeground(rgb_color fg, TermColorID num)
{
	const ArpColor myfg(fg);
	bool changed = false;
	
	if( num >= 0 && num < TERM_NUMTEXTCOLORS ) {
		if( textForeground[num] != myfg ) changed = true;
		textForeground[num] = myfg;
	} else if( num == TERM_HIGHLIGHTCOLOR ) {
		if( highForeground != myfg ) changed = true;
		highForeground = myfg;
	} else if( num == TERM_CURSORCOLOR ) {
		if( cursorForeground != myfg ) changed = true;
		cursorForeground = myfg;
	}
	
	if( changed ) SetDirty(TopVisibleRow(),BottomVisibleRow());
}

rgb_color ArpCoreTerminal::TermTextBackground(TermColorID num) const
{
	if( num >= 0 && num < TERM_NUMTEXTCOLORS ) return textBackground[num];
	else if( num == TERM_HIGHLIGHTCOLOR ) return highBackground;
	else if( num == TERM_CURSORCOLOR ) return cursorBackground;
	return textBackground[0];
}

rgb_color ArpCoreTerminal::TermTextForeground(TermColorID num) const
{
	if( num >= 0 && num < TERM_NUMTEXTCOLORS ) return textForeground[num];
	else if( num == TERM_HIGHLIGHTCOLOR ) return highForeground;
	else if( num == TERM_CURSORCOLOR ) return cursorForeground;
	return textForeground[0];
}

// Does not go through translation table!
void ArpCoreTerminal::TermSetChar(ichar c)
{
	RowInfo* ri=GetRowInfo(curRow);
	if( ri ) {
		OpenToColumn(curRow,curCol,ri);
		ri->Text()[curCol] = c;
		ri->Style()[curCol] = curStyle;
	}
}

// Does not go through translation table!
void ArpCoreTerminal::TermSetChar(ichar c, int32 row, int32 col)
{
	RowInfo* ri=GetRowInfo(row);
	if( ri ) {
		OpenToColumn(row,col,ri);
		ri->Text()[col] = c;
		ri->Style()[col] = curStyle;
	}
}

ichar ArpCoreTerminal::TermGetChar() const
{
	const RowInfo* ri=GetRowInfo(curRow);
	return (ri && curCol >= 0 && curCol < ri->Cols())
			? ri->Text()[curCol] : ' ';
}

ichar ArpCoreTerminal::TermGetChar(int32 row, int32 col) const
{
	const RowInfo* ri=GetRowInfo(row);
	return (ri && col >= 0 && col < ri->Cols())
			? ri->Text()[col] : ' ';
}

ArpTerminalInterface::style_t
ArpCoreTerminal::TermGetCharStyle(void) const
{
	const RowInfo* ri=GetRowInfo(curRow);
	return (ri && curCol >= 0 && curCol < ri->Cols())
			? ri->Style()[curCol] : TERM_STYLEPLAIN;
}

ArpTerminalInterface::style_t
ArpCoreTerminal::TermGetCharStyle(int32 row, int32 col) const
{
	const RowInfo* ri=GetRowInfo(row);
	return (ri && col >= 0 && col < ri->Cols())
			? ri->Style()[col] : TERM_STYLEPLAIN;
}

/* ------------------------------------------------------------
	 HIGH-LEVEL MANIPULATION METHODS.
	 ------------------------------------------------------------ */

int32 ArpCoreTerminal::ScrollRow(int32 row, int32 amount,
							int32 top, int32 bot, bool constrain)
{
	if( amount < 0 ) {
		if( row >= numRows || top >= (numRows-1) ) {
			row -= amount;
			if( row >= history_len ) {
				row = history_len-1;
				if( constrain ) {
					SetDirty(PageToRow(row),PageToRow(row));
				}
			}
		} else if( row <= top && row >= bot ) {
			row -= amount;
			if( row > top ) {
				if( constrain ) {
					SetDirty(PageToRow(row),PageToRow(top));
					row = top;
				} else {
					row = numRows + (top-row-1);
				}
			}
		}
	} else if( amount > 0 ) {
		if( row <= top && row >= bot ) {
			row -= amount;
			if( row < bot ) {
				if( constrain ) {
					SetDirty(PageToRow(bot),PageToRow(row));
				}
				row = bot;
			}
		}
	}
	return row;
}

void ArpCoreTerminal::TermScrollRegion(int32 top, int32 bottom, int32 amount)
{
	ArpD(cdb << ADH << "ArpCoreTerminal: TermScrollRegion("
			<< top << "," << bottom << "," << amount << ")" << std::endl);
	if( top >= bottom ) return;
	if( bottom >= numRows ) bottom = numRows-1;
	if( bottom < 0 ) bottom = 0;
	if( top >= numRows ) top = numRows-1;
	if( top < 0 ) top = 0;
	int32 lastMode = curMode;
	curMode &= ~TERM_MODEHIDECURSOR;
#if 0
	int32 crow = lastRow, ccol = lastCol;
	lastRow = lastCol = -1;
	drawRowCol(RowToPage(lastRow),lastCol);
#endif
	int32 regsize = bottom-top;
	if( (curMode&TERM_MODESMOOTH) != 0
			|| (scrollAmount+amount) >= regsize
			|| (scrollAmount+amount) <= -regsize
			|| (scrollTop <= scrollBottom &&
				(scrollTop != top || scrollBottom != bottom)) ) {
		TermClean();
		scrollAmount = 0;
	}
	scrollAmount += amount;
	scrollTop = top;
	scrollBottom = bottom;
	//TermClean();
	if( lastRow >= 0 && lastCol >= 0 ) {
		SetDirty(lastRow,lastRow,lastCol,lastCol);
		//drawRowCol(RowToPage(lastRow),lastCol);
		lastRow = lastCol = -1;
	}
	curMode = lastMode;
	dirtyTop = ScrollRow(dirtyTop,amount,
						RowToPage(top),RowToPage(bottom),false);
	dirtyBottom = ScrollRow(dirtyBottom,amount,
						RowToPage(top),RowToPage(bottom),false);
	hTopRow = ScrollRow(hTopRow,amount,
						RowToPage(top),RowToPage(bottom));
	hBotRow = ScrollRow(hBotRow,amount,
						RowToPage(top),RowToPage(bottom));
	if( amount < 0 && historyUse <= HISTORY_NONE
		|| ( historyUse < HISTORY_AGGRESSIVE && top != 0 ) ) {
		amount = -amount;
		if( amount > (bottom-top+1) ) amount=(bottom-top+1);
		int32 i;
		for( i=top; i<=bottom-amount; i++ ) {
			page.SetElem(RowToPage(i),page.GetElem(RowToPage(i+amount)));
		}
		for( i=bottom; i>bottom-amount; i-- ) {
			page.SetElem(RowToPage(i),new RowInfo(numCols));
		}
		SetDirty(top,bottom);
	} else if( amount < 0 ) {
		amount = -amount;
		if( amount > (bottom-top+1) ) amount=(bottom-top+1);
		page.ScrollView(-amount);
		int32 last_len = history_len;
		history_len += amount;
		if( history_len > page.Size() ) history_len = page.Size();
		int32 i;
		// The easy case is when text scrolls off the top.
		if( top == 0 ) {
			// First get rid of excess memory in those put to history.
			for( i=-amount; i<0; i++ ) {
				RowInfo* ri = page.GetElem(RowToPage(i));
				if( ri ) ri->Squeeze();
			}
			// Next copy down any text at the bottom that isn't being scrolled.
			for( i=numRows-1; i>=bottom+1; i-- ) {
				page.SetElem(RowToPage(i),
							page.GetElem(RowToPage(i-amount)));
			}
			// Now paste in new empty lines at bottom.
			for( i=bottom-amount+1; i<=bottom; i++ ) {
				page.SetElem(RowToPage(i),new RowInfo(numCols));
			}
			// Need to dirty everything, if couldn't completely
			// scroll view because history is full.
			if( (history_len-last_len) < amount ) {
				SetDirty(TopVisibleRow(),BottomVisibleRow());
			// Otherwise, just redraw the newly added rows.
			} else {
				SetDirty(bottom-amount,bottom);
			}
		// The harder case: 'top' is not the top of the page, but we still
		// would like to place scrolled-off text into the history.
		} else {
			// First, copy everything down so it looks like nothing has
			// been scrolled up -- we've effectively just made room in
			// the history above the visible page for new text.
			for( i=numRows-1; i>=0; i-- ) {
				page.SetElem(RowToPage(i),page.GetElem(RowToPage(i-amount)));
			}
			// Now place correct lines into history and squeeze 'em.
			for( i=0; i<amount; i++ ) {
				RowInfo* ri = page.GetElem(RowToPage(i+top));
				if( ri ) ri->Squeeze();
				page.SetElem(RowToPage(i-amount),ri);
			}
			// Do the actual scroll in the region between 'top' and 'bottom'.
			for( i=top; i<=bottom-amount; i++ ) {
				page.SetElem(RowToPage(i),page.GetElem(RowToPage(i+amount)));
			}
			// And create the new empty lines.
			for( i=bottom-amount+1; i<=bottom; i++ ) {
				page.SetElem(RowToPage(i),new RowInfo(numCols));
			}
			// If no new data was added to the history, we need
			// to redraw any history showing and the terminal
			// region that was scrolled.
			if( history_len <= last_len ) {
				int32 vtop = TopVisibleRow();
				if( vtop < 0 ) SetDirty(vtop,-1);
				SetDirty(top,bottom);
			// Otherwise, it looks MUCH better to just redraw
			// the whole thing.
			} else {
				SetDirty(TopVisibleRow(),BottomVisibleRow());
			}
			// Redraw the unscrolled and new lines.
			//SetDirty(-amount,top-1);
			//SetDirty(bottom-amount+1,numRows-1);
		}
	} else if( amount > 0 ) {
		if( amount > (bottom-top+1) ) amount=(bottom-top+1);
		int32 i;
		for( i=bottom; i>=top+amount; i-- ) {
			page.SetElem(RowToPage(i),page.GetElem(RowToPage(i-amount)));
		}
		for( i=top; i<top+amount; i++ ) {
			page.SetElem(RowToPage(i),new RowInfo(numCols));
		}
		SetDirty(top,bottom);
	}
	VERROWS(for( int32 j=0; j<numRows; j++ ) assert(GetRowInfo(j) != NULL));
}

void ArpCoreTerminal::TermScroll(int32 amount)
{
	TermScrollRegion(regionTop,regionBottom,amount);
}

void ArpCoreTerminal::TermSetChars(int32 left, int32 right,
								ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);

	//int32 initleft = left;
	
	if( left < regionLeft ) left = regionLeft;
	if( right > regionRight ) right = regionRight;
	ichar* text = ri->Text();
	style_t* style = ri->Style();
	while( left <= right ) {
		text[left] = fillc;
		style[left] = fillstyle;
		left++;
	}
	OpenToColumn(curRow,right,ri);
}

void ArpCoreTerminal::TermDeleteChars(int32 left, int32 right,
									ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);

	int32 initleft = left;
	
	if( left < regionLeft ) left = regionLeft;
	if( right > regionRight ) right = regionRight;
	if( left > right ) return;
	int32 over = right+1;
	ichar* text = ri->Text();
	style_t* style = ri->Style();
	if( ri->UsedCols() > left
		&& ri->UsedCols() <= regionRight+1 ) {
		ri->SetUsedCols(ri->UsedCols()-(right-left+1));
		if( ri->UsedCols() <= left ) ri->SetUsedCols(left+1);
	}
	while( over <= regionRight && left <= regionRight ) {
		text[left] = text[over];
		style[left] = style[over];
		left++;
		over++;
	}
	while( left <= regionRight ) {
		text[left] = fillc;
		style[left] = fillstyle;
		left++;
	}
	SetDirty(curRow,curRow,initleft,regionRight,ri);
}

void ArpCoreTerminal::TermInsertChars(int32 left, int32 right,
									ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);

	if( left < regionLeft ) left = regionLeft;
	if( right > regionRight ) right = regionRight;
	if( left > right ) return;
	int32 inner = regionRight-(right-left)-1;
	if( ri->UsedCols() > left
		&& ri->UsedCols() <= regionRight+1 ) {
		ri->SetUsedCols(ri->UsedCols()+(right-left+1));
		if( ri->UsedCols() > regionRight+1 )
			ri->SetUsedCols(regionRight+1);
	}
	right = regionRight;
	ichar* text = ri->Text();
	style_t* style = ri->Style();
	while( inner >= left && right >= left ) {
		text[right] = text[inner];
		style[right] = style[inner];
		inner--;
		right--;
	}
	while( right >= left ) {
		text[right] = fillc;
		style[right] = fillstyle;
		right--;
	}
	SetDirty(curRow,curRow,left,regionRight,ri);
}

void ArpCoreTerminal::TermClearEOL(ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);

	ichar* text = ri->Text();
	style_t* style = ri->Style();
	if( ri->UsedCols() > curCol
		&& ri->UsedCols() <= regionRight+1 ) {
		ri->SetUsedCols(curCol);
	}
	for( int32 i=curCol; i<=regionRight; i++ ) {
		text[i] = fillc;
		style[i] = fillstyle;
	}
	SetDirty(curRow,curRow,curCol,regionRight,ri);
}

void ArpCoreTerminal::TermClearBOL(ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);

	ichar* text = ri->Text();
	style_t* style = ri->Style();
	if( ri->UsedCols() > regionLeft
		&& ri->UsedCols() <= curCol+1 ) {
		ri->SetUsedCols(regionLeft);
	}
	for( int32 i=regionLeft; i<=curCol; i++ ) {
		text[i] = fillc;
		style[i] = fillstyle;
	}
	SetDirty(curRow,curRow,regionLeft,curCol,ri);
}

void ArpCoreTerminal::TermClearLine(ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);

	ichar* text = ri->Text();
	style_t* style = ri->Style();
	if( ri->UsedCols() > regionLeft
		&& ri->UsedCols() <= regionRight+1 ) {
		ri->SetUsedCols(regionLeft);
	}
	for( int32 i=regionLeft; i<=regionRight; i++ ) {
		text[i] = fillc;
		style[i] = fillstyle;
	}
	SetDirty(curRow,curRow,regionLeft,regionRight,ri);
}

void ArpCoreTerminal::TermClearEOD(ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	int32 left = curCol;
	for( int32 j=curRow; j<=regionBottom; j++ ) {
		RowInfo* ri = GetRowInfo(j);
		assert(ri != NULL);
		ichar* text = ri->Text();
		style_t* style = ri->Style();
		if( ri->UsedCols() > left
			&& ri->UsedCols() <= regionRight+1 ) {
			ri->SetUsedCols(left);
		}
		for( int32 i=left; i<=regionRight; i++ ) {
			text[i] = fillc;
			style[i] = fillstyle;
		}
		left = regionLeft;
	}
	SetDirty(curRow,regionBottom,regionLeft,regionRight);
}

void ArpCoreTerminal::TermClearBOD(ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	for( int32 j=regionTop; j<curRow; j++ ) {
		RowInfo* ri = GetRowInfo(j);
		assert(ri != NULL);
		ichar* text = ri->Text();
		style_t* style = ri->Style();
		if( ri->UsedCols() > regionLeft
			&& ri->UsedCols() <= regionRight+1 ) {
			ri->SetUsedCols(regionLeft);
		}
		for( int32 i=regionLeft; i<=regionRight; i++ ) {
			text[i] = fillc;
			style[i] = fillstyle;
		}
	}
	RowInfo* ri = GetRowInfo(curRow);
	assert(ri != NULL);
	ichar* text = ri->Text();
	style_t* style = ri->Style();
	if( ri->UsedCols() > regionLeft
		&& ri->UsedCols() <= regionRight+1 ) {
		ri->SetUsedCols(regionLeft);
	}
	for( int32 i=regionLeft; i<curCol; i++ ) {
		text[i] = fillc;
		style[i] = fillstyle;
	}
	SetDirty(regionTop,curRow,regionLeft,regionRight);
}

void ArpCoreTerminal::TermClear(ichar fillc, style_t fillstyle)
{
	if( fillstyle == TERM_STYLECURRENT ) fillstyle=curStyle;
	
	if( historyUse >= HISTORY_MODERATE ) {
		TermScroll(-(regionBottom-regionTop+1));
	}
	
	for( int32 j=regionTop; j<=regionBottom; j++ ) {
		RowInfo* ri = GetRowInfo(j);
		assert(ri != NULL);
		ichar* text = ri->Text();
		style_t* style = ri->Style();
		if( ri->UsedCols() > regionLeft
			&& ri->UsedCols() <= regionRight+1 ) {
			ri->SetUsedCols(regionLeft);
		}
		for( int32 i=regionLeft; i<=regionRight; i++ ) {
			text[i] = fillc;
			style[i] = fillstyle;
		}
	}
	SetDirty(regionTop,regionBottom,regionLeft,regionRight);
}

/* ------------------------------------------------------------
	 SCREEN REFRESHING METHODS.
	 ------------------------------------------------------------ */

void ArpCoreTerminal::TermRedraw(int32 top, int32 bot, bool full)
{
	ArpD(cdb << ADH << "ArpCoreTerminal: Redrawing screen: "
			<< top << " to " << bot << ", full=" << full << std::endl);
			
	// convert into page coordinates
	top = RowToPage(top);
	bot = RowToPage(bot);
	
	ArpD(cdb << ADH << "ArpCoreTerminal: cTop=" << clipTop
			<< ", cBot=" << clipBottom << ", dTop="
			<< RowToPage(dirtyTop) << ", dBot="
			<< RowToPage(dirtyBottom) << std::endl);
			
	if( top > clipTop ) top = clipTop;
	if( bot < clipBottom ) bot = clipBottom;
	
	if( !full ) {
		if( top > RowToPage(dirtyTop) ) top = RowToPage(dirtyTop);
		if( bot < RowToPage(dirtyBottom) ) bot = RowToPage(dirtyBottom);
	}
	
	ArpD(cdb << ADH << "ArpCoreTerminal: Redraw clipped page rows: "
				<< top << " to " << bot << std::endl);
			
	if( bot > top ) return;
	
	// Set special style marks where highlight state changes,
	// so rendering code doesn't have to add special checks to
	// check for the appropriate places.
	MarkHighlight(hTopRow,hTopCol,true);
	MarkHighlight(hBotRow,hBotCol,true);
	
	BView::SetFont(&fontPlain,B_FONT_FAMILY_AND_STYLE|B_FONT_SIZE);	
	cur_font_style = TERM_STYLEPLAIN;
	for( int32 i = top; i >= bot; i-- ) {
		drawRow(i,full);
	}

	// Remove highlight marks.
	MarkHighlight(hTopRow,hTopCol,false);
	MarkHighlight(hBotRow,hBotCol,false);
	
	if( RowToPage(curRow) <= top && RowToPage(curRow) >= bot
		&& curCol >= clipLeft && curCol <= clipRight ) {
		drawRowCol(RowToPage(lastRow),lastCol);
		lastRow = curRow;
		lastCol = curCol;
	}
		
	ClearDirty();
	if( full ) Flush();
}

void ArpCoreTerminal::TermRedraw(void)
{
	TermRedraw(TopVisibleRow(),BottomVisibleRow(),true);
}

void ArpCoreTerminal::TermClean(void)
{
	ArpD(cdb << ADH << "ArpCoreTerminal: Cleaning screen: "
			<< dirtyTop << " to " << dirtyBottom << std::endl);
	if( UpdateBounds() ) {
		if( has_output ) {
			ArpD(cdb << ADH << "Scroll after update: mode="
					<< int(AutoScrollMode()) << std::endl);
			has_output = false;
			if( (((int)AutoScrollMode())&AUTOSCROLL_OUTPUT) != 0 ) {
				int32 row=0, col=0;
				TermGetCursorPos(&row,&col);
				MakePosVisible(row,col);
			}
		}
		return;
	}
	if( IsDirty() ) {
		if( lastRow != curRow || lastCol != curCol ) {
			SetDirty(lastRow,lastRow,lastCol,lastCol);
			SetDirty(curRow,curRow,curCol,curCol);
		}
		ArpD(cdb << ADH << "ArpCoreTerminal::TermClean() drawing dirty rows" << std::endl);
		//printf("Cleaning: %ld to %ld\n",dirtyTop,dirtyBottom);
		TermRedraw(dirtyTop,dirtyBottom,false);
	} else if( lastRow != curRow || lastCol != curCol ) {
		ArpD(cdb << ADH << "ArpCoreTerminal::TermClean() drawing row=" << lastRow
						<< " col=" << lastCol << std::endl);
		drawRowCol(RowToPage(lastRow),lastCol);
		ArpD(cdb << ADH << "ArpCoreTerminal::TermClean() drawing row=" << curRow
						<< " col=" << curCol << std::endl);
		drawRowCol(RowToPage(curRow),curCol);
	}
	lastRow = curRow;
	lastCol = curCol;
	Flush();
	if( has_output ) {
		has_output = false;
		ArpD(cdb << ADH << "Scroll after clean: mode="
				<< int(AutoScrollMode()) << std::endl);
		if( (((int)AutoScrollMode())&AUTOSCROLL_OUTPUT) != 0 ) {
			int32 row=0, col=0;
			TermGetCursorPos(&row,&col);
			MakePosVisible(row,col);
		}
	}
}

/* ------------------------------------------------------------
	 CHARACTER-STREAM METHODS.
	 ------------------------------------------------------------ */

void ArpCoreTerminal::TermSendTTY(const ichar * d, size_t len, uint32 flags)
{
	ArpD(if( outputecho ) cdb << ADH << "ArpCoreTerminal: Write: "
									<< ArpString(d,len) << std::endl);
	if( len ) has_output = true;
	if( len == 1 ) {
		if( (flags&TERM_OUTRAW) != 0 ) PutChar(*d);
		else PutStdChar(*d);
	} else {
		while( len > 0 ) {
			if( (flags&TERM_OUTRAW) != 0 ) PutChar(*d);
			else PutStdChar(*d);
			d++;
			len--;
		}
	}
	if( (flags&TERM_OUTPARTIAL) == 0 ) {
		TermClean();
	}
}

/* ------------------------------------------------------------
	 EVENT HANDLING METHODS.
	 ------------------------------------------------------------ */

void ArpCoreTerminal::TermSendRemote(const ichar * d, size_t len)
{
	TermSendTTY(d,len);
}

void ArpCoreTerminal::ConvertInput(const ichar* d, size_t len)
{
	if( GetEncodingConv() < 0 ) {
		EmulateToRemote(d,len);
		return;
	}
	
	ArpD(cdb << ADH << "Convert input: str=" << (void*)d << ", len=" << len << std::endl);
	while( len > 0 ) {
		ArpD(cdb << ADH << "Converting: ptr=" << (void*)d << ", " << len
					<< " characters left." << std::endl);
		int32 srclen = len;
		int32 destlen = sizeof(convBuffer);
		if( convert_from_utf8(GetEncodingConv(),(const char*)d,&srclen,
								&convBuffer[0],&destlen,NULL) != B_OK ) {
			ArpD(cdb << ADH << "Error converting!  char=" << (char)(*d)
							<< " (" << (int)(*d) << ")" << std::endl);
			destlen = 0;
			srclen = 0;
			while( (d[srclen]&0x80) != 0 && srclen < (int32)len ) srclen++;
			if( srclen < (int32)len ) srclen++;
		}
		if( srclen <= 0 ) {
			ArpD(cdb << ADH << "Huh?!?  Couldn't convert any characters!" << std::endl);
			return;
		}
		ArpD(cdb << ADH << "Eating " << srclen << " chars, writing "
						<< destlen << " chars." << std::endl);
		EmulateToRemote((ichar*)&convBuffer[0],destlen);
		if( srclen >= (int32)len ) return;
		len -= srclen;
		d += srclen;
	}
}

void ArpCoreTerminal::EmulateToRemote(const ichar* d, size_t len)
{
	TermSendRemote(d, len);
}

void ArpCoreTerminal::SetFont(const BFont *infont, uint32 /*mask*/)
{
	if( fontPlain == *infont ) return;
	
	fontPlain = *infont;
	fontPlain.SetSpacing(B_FIXED_SPACING);
	fontPlain.SetEncoding(encoding);
	fontPlain.SetRotation(0);
	fontPlain.SetShear(90);
	
	fontPlain.SetFace(B_REGULAR_FACE);
	fontBold = fontPlain;
	fontBold.SetFace(B_BOLD_FACE);
	fontItalic = fontPlain;
	fontItalic.SetFace(B_ITALIC_FACE);
	fontBoldItalic = fontPlain;
	fontBoldItalic.SetFace(B_BOLD_FACE|B_ITALIC_FACE);
	
	BView::SetFont(&fontPlain);
	cur_font_style = TERM_STYLEPLAIN;
	InitTerminal();
	Invalidate();
}

void ArpCoreTerminal::GetNeededSize(int32 cols, int32 rows,
								float* width, float* height)
{
	if( width ) *width = cols*charWidth;
	if( height ) *height = rows*charHeight;
}

void ArpCoreTerminal::GetPreferredSize(float *width, float *height)
{
	if( width ) {
		if( sizeCols > 0 ) *width = sizeCols*charWidth;
		else *width = 4*charWidth;
	}
	if( height ) {
		if( sizeRows > 0 ) *height = sizeRows*charHeight;
		else *height = 4*charHeight;
	}
}

void ArpCoreTerminal::TermSizeChanged(int32 /*rows*/, int32 /*cols*/)
{
}

void ArpCoreTerminal::AttachedToWindow(void)
{
	ArpD(cdb << ADH << "ArpCoreTerminal: AttachedToWindow()." << std::endl);
	BView::AttachedToWindow();
	InitTerminal();
	Draw(Frame());
	TermClean();
}

void ArpCoreTerminal::FrameResized(float width, float height)
{
	ArpD(cdb << ADH << "FrameResized: " << width
				<< " x " << height << std::endl);
	BView::FrameResized(width,height);
	if( last_bounds != Bounds() ) {
		ArpD(cdb << ADH << "Frame resizing to: " << width
				<< " x " << height << std::endl);
		if( UpdateBounds() ) ClearDirty();
		else TermClean();
	}
}

void ArpCoreTerminal::ScrollTo(BPoint where)
{
	ArpD(cdb << ADH << "ScrollTo: " << where << std::endl);
#if 0
	int32 lastMode = curMode;
	curMode |= TERM_MODEHIDECURSOR;
	drawRowCol(RowToPage(lastRow),lastCol);
	if( lastRow != curRow || lastCol != curCol ) {
		drawRowCol(RowToPage(curRow),curCol);
	}
#endif
	TermClean();
	BView::ScrollTo(where);
	UpdateBounds();
#if 0
	curMode = lastMode;
	lastRow = lastCol = -1;
#endif
	//Window()->UpdateIfNeeded();
}

// This is used to mark the places where highlighting starts/stops,
// so that we don't have to do any special checking for early
// changes of style in drawRow().
enum {
	EXTRA_STYLEMARK = 1<<7
};

void ArpCoreTerminal::MarkHighlight(int32 row, int32 col, bool state)
{
	if( col >= 0 && row >= 0 && row < history_len ) {
		RowInfo* ri = page[row];
		if( ri && col < ri->Cols() ) {
			style_t* style = ri->Style();
			style[col] = (style[col]&(~EXTRA_STYLEMARK))
						| (state ? EXTRA_STYLEMARK : 0);
		}
	}
}

void ArpCoreTerminal::Draw(BRect rect)
{
	// If the user hasn't cleaned up some dirty data before
	// allowing us back into the message loop [thus allowing
	// this function to be called], bail on trying to do any
	// kind of draw optimizations and blast out the whole thing.
	if( IsDirty() && Frame() != rect ) {
		//printf("Draw when dirty: Invalidate me!\n");
		Invalidate();
		ClearDirty();
		return;
	}
	// XXXX This sometimes gets called BEFORE FrameResized() has
	// been called with the new size, resulting in much uglyness.
	// So check for new sizes here, too, and deal with them if
	// needed.
	BRect bnd = Bounds();
	if( last_bounds != bnd ) {
		ArpD(cdb << ADH << "Draw resizing to: " << bnd << std::endl);
		if( UpdateBounds() ) {
			ClearDirty();
			return;
		}
		//TermClean();
	}
	SetDrawingMode(B_OP_COPY);	
	ArpD(cdb << ADH << "Screen pos=" << RawYToRow(rect.top)
			<< ", hist pos=" << RawYToRow(last_bounds.top)
			<< ", hist len=" << history_len << std::endl);
	SetClipRegion(rect);
	ArpD(cdb << ADH << "Term draw region: (" << clipLeft
			<< "," << clipTop << ")-(" << clipRight
			<< "," << clipBottom << ")" << std::endl);
	TermRedraw();
	SetClipRegion(Bounds());
}

void ArpCoreTerminal::KeyDown(const char *bytes, int32 numBytes)
{
	BView::KeyDown(bytes,numBytes);
}

void ArpCoreTerminal::MouseDown(BPoint /*point*/)
{
	if( !IsFocus() ) {
		MakeFocus(true);
		return;
	}
}

/* ------------------------------------------------------------
   ROUTINES AVAILABLE TO INHERITED CLASSES.
   ------------------------------------------------------------ */

void ArpCoreTerminal::SetClipRegion(const BRect& rect)
{
	clipLeft = int32(floor(rect.left/charWidth));
	clipTop = RowToPage(floor(RawYToRow(rect.top)));
	clipRight = int32(ceil(rect.right/charWidth));
	clipBottom = RowToPage(ceil(RawYToRow(rect.bottom)));
}

void ArpCoreTerminal::PutChar(ichar c)
{
	if( c < translationLength ) c = curTranslation[(int)c];
	if( curCol >= regionRight && !overChar ) {
		RowInfo * ri = GetRowInfo(curRow);
		assert(ri != NULL);
		OpenToColumn(curRow,curCol,ri);
		ri->Text()[curCol] = c;
		ri->Style()[curCol] = curStyle;
		curCol = regionRight;
		overChar = true;
		ArpD(cdb << ADH << "ArpCoreTerminal: Hit margin, overChar=" << overChar <<std::endl);
	} else if (curCol >= regionRight && overChar) {
		if( (curMode&TERM_MODENOWRAP) == 0 ) {
			overChar = false;
			PutStdChar('\r');
			PutStdChar('\n');
			RowInfo * ri = GetRowInfo(curRow-1);
			assert(ri != NULL);
			ri->SetConnected(true);
			ri = GetRowInfo(curRow);
			assert(ri != NULL);
			OpenToColumn(curRow,curCol,ri);
			ri->Text()[curCol] = c;
			ri->Style()[curCol] = curStyle;
			curCol++;
			ArpD(cdb << ADH << "ArpCoreTerminal: Past margin, overChar="
							<< overChar <<std::endl);
		} else {
			RowInfo * ri = GetRowInfo(curRow);
			assert(ri != NULL);
			OpenToColumn(curRow,curCol,ri);
			ri->Text()[curCol] = c;
			ri->Style()[curCol] = curStyle;
			ArpD(cdb << ADH << "ArpCoreTerminal: Stay margin, overChar="
							<< overChar << std::endl);
		}
	} else {
		RowInfo * ri = GetRowInfo(curRow);
		assert(ri != NULL);
		OpenToColumn(curRow,curCol,ri);
		ri->Text()[curCol] = c;
		ri->Style()[curCol] = curStyle;
		curCol++;
		overChar = false;
	}
}

void ArpCoreTerminal::PutStdChar(ichar c)
{
	switch( c ) {
	case ANSI_NUL:
		break;
	case ANSI_BEL:     // Bell
		beep();
		break;
	case ANSI_BS:     // Backspace
		curCol--;
		if(curCol < regionLeft) {
			curCol = regionLeft;
		}
		break;
	case ANSI_HT: {   // Tab
		int32 newcol = (curCol&0xfff8) + 8;
		if( newcol > regionRight ) newcol = regionRight;
		curCol = newcol;
		OpenToColumn(curRow,curCol,NULL);
	} break;
	case ANSI_FF: {
		RowInfo * ri = GetRowInfo(curRow);
		assert(ri != NULL);
		ri->SetConnected(false);
		HistoryUseType lastUse = historyUse;
		if( lastUse < HISTORY_MODERATE ) historyUse = HISTORY_NONE;
		TermScroll(-(regionBottom-regionTop+1));
		historyUse = lastUse;
	} break;
	case ANSI_DEL: {
		RowInfo * ri = GetRowInfo(curRow);
		assert(ri != NULL);
		if( ri->UsedCols() > curCol
			&& ri->UsedCols() <= regionRight+1 ) {
			ri->SetUsedCols(ri->UsedCols()-1);
		}
		ichar* text = ri->Text();
		style_t* style = ri->Style();
		for( int32 i=curCol; i<regionRight; i++ ) {
			text[i] = text[i+1];
			style[i] = style[i+1];
		}
		text[regionRight] = ' ';
		style[regionRight] = curStyle;
		SetDirty(curRow,curRow,curCol,regionRight,ri);
	} break;
	default: {
		// special handling of new line and carriage return.
		bool handled = false;
		if( crChars.FindFirst(c) >= 0 ) {
			curCol = regionLeft;
			RowInfo * ri = GetRowInfo(curRow);
			assert(ri != NULL);
			ri->SetConnected(false);
			overChar = false;
			handled = true;
		}
		if( lfChars.FindFirst(c) >= 0 ) {
			RowInfo * ri = GetRowInfo(curRow);
			assert(ri != NULL);
			ri->SetConnected(false);
			if( curRow >= regionBottom ) {
				TermScroll(-1);
				curRow = regionBottom;
				SetDirty(regionBottom,regionBottom);
			} else {
				curRow++;
			}
			handled = true;
		}
		if( !handled ) PutChar(c);
	} break;
	}
}

void ArpCoreTerminal::StartSession()
{
	EndSession();
	if( HistoryUse() > HISTORY_NONE ) {
		int topStuff = curRow-regionTop;
		if( topStuff > 0 ) TermScroll(-topStuff);
		curRow = regionTop;
		curCol = regionLeft;
	}
}

void ArpCoreTerminal::EndSession()
{
	ArpD(cdb << ADH << "Ending session...\n");
	curStyle = TERM_STYLEPLAIN;
	TermSetRegion();
	for( curRow=regionBottom; curRow>=(regionTop-1); curRow-- ) {
		RowInfo* ri = GetRowInfo(curRow);
		if( !ri ) continue;
		
		if( curRow < regionTop
				|| (ri->UsedCols() > 0 && !ri->Divider()) ) {
			ArpD(cdb << ADH << "Inserting divider at " << curRow << std::endl);
			if( curRow >= regionTop ) {
				PutStdChar(ANSI_LF);
				PutStdChar(ANSI_CR);
			} else curRow = regionTop;
			overChar = false;
			ri = GetRowInfo(curRow);
			if( ri) {
				ri->SetDivider(true);
				ri->SetConnected(false);
			}
			SetDirty(curRow, curRow);
		}
		
		if( ri->Divider() ) {
			ArpD(cdb << ADH << "Done -- found divider at " << curRow << std::endl);
			PutStdChar(ANSI_LF);
			PutStdChar(ANSI_CR);
			return;
		}
		
		ArpD(cdb << ADH << "Empty line at " << curRow << std::endl);
	}
	
	if( curRow < 0 ) curRow = 0;
}

void ArpCoreTerminal::SetDirty(int32 top, int32 bottom, int32 left, int32 right, RowInfo* inri)
{
	ArpD(cdb << ADH << "ArpCoreTerminal::SetDirty(" << top << ", " << bottom
				<< ", " << left << ", " << right << ", " << inri << ")" << std::endl);
	if( dirtyTop > dirtyBottom ) {
		dirtyTop = top;
		dirtyBottom = bottom;
	} else {
		if( top < dirtyTop ) dirtyTop = top;
		if( bottom > dirtyBottom ) dirtyBottom = bottom;
	}
	for( int32 i=top; i<=bottom; i++ ) {
		RowInfo * ri = inri ? inri : GetRowInfo(i);
		if( ri ) {
			//assert(ri != NULL);
			if( left >= 0 ) ri->Dirty(left,right);
			else ri->Dirty(true);
		}
	}
}

void ArpCoreTerminal::OpenToColumn(int32 row, int32 col, RowInfo* ri)
{
	if( !ri ) ri=GetRowInfo(curRow);
	if( ri ) {
		if( hShown ) {
			SetDirty(row, row,
					ri->UsedCols() < col ? ri->UsedCols() : col,
					col, ri);
		} else {
			SetDirty(row,row,col,col,ri);
		}
		ri->OpenUsedCols(col+1);
	}
}

/* ------------------------------------------------------------
   PRIVATE IMPLEMENTATION.
   ------------------------------------------------------------ */

bool ArpCoreTerminal::UpdateBounds(void)
{
	if( !Window() ) return false;
	
	if( updating ) return false;	
	updating = 1;
	bool invalidate = false;
	float force_vbar = -1;
	float max_vval = -1;
	
	ArpD(cdb << ADH << "ArpCoreTerminal: Updating boundaries." << std::endl);
	BRect bounds = Bounds();
	bool changed = false;
	if( history_last_len != history_len
		|| last_bounds.top != bounds.top
		|| last_bounds.bottom != bounds.bottom ) {
		winRows = int32(floor( (bounds.Height()+1)/charHeight ));
		BScrollBar* vbar = ScrollBar(B_VERTICAL);
		if( vbar ) {
			float winheight = floor(bounds.Height()+.5);
			float dsize = floor((history_len*charHeight)+.5);
			//float off = 0;
			if( dsize < winheight ) dsize = winheight;
			float maxval = dsize-winheight-1;
			if( maxval < 0 ) maxval = 0;
			float last_value = floor(vbar->Value()+.5);
			float last_min = 0, last_max = 0;
			vbar->GetRange(&last_min, &last_max);
			last_max = floor(last_max+.5);
			ArpD(cdb << ADH << "dsize = " << dsize
					<< ", winheight = " << winheight
					<< ", hview = " << bounds.top << std::endl);
			ArpD(cdb << ADH << "Set range: from " << 0
					<< " to " << maxval << ", bad="
					<< (int)((dsize-winheight-1)<0) << std::endl);
			max_vval = maxval;
			//vbar->SetRange(0,maxval);
			ArpD(cdb << ADH << "Set proportion: " << winheight/dsize << std::endl);
			vbar->SetProportion(winheight/dsize);
			ArpD(cdb << ADH << "Set steps: " << floor(charHeight)
					<< " and " << floor((winRows-2)*charHeight) << std::endl);
			vbar->SetSteps(floor(charHeight),
							floor((winRows-2)*charHeight));
			if( last_value == last_max ) {
				force_vbar = maxval;
				#if 0
				if( last_value < maxval ) {
					Invalidate(BRect(bounds.left,last_value,
									bounds.right,maxval));
				}
				#endif
			}
			bounds = Bounds();
		}
		changed = true;
	}
	
	int32 last = BottomVisibleRow();
	horiz_len = int32(charWidth+.5);
	for( int32 i=TopVisibleRow(); i<last; i++ ) {
		RowInfo* ri = GetRowInfo(i);
		if( ri ) {
			if( ri->UsedCols() > horiz_len ) horiz_len = ri->UsedCols();
		}
	}
	if( horiz_last_len != horiz_len
		|| last_bounds.left != bounds.left
		|| last_bounds.right != bounds.right ) {
		BScrollBar* hbar = ScrollBar(B_HORIZONTAL);
		winCols = int32( (bounds.Width()+1)/charWidth );
		if( hbar ) {
			float winwidth = bounds.Width();
			float dsize = horiz_len*charWidth;
			if( dsize < winwidth ) dsize = winwidth;
			float maxval = dsize-winwidth-1;
			if( maxval < 0 ) maxval = 0;
			ArpD(cdb << ADH << "dsize = " << dsize
					<< ", winwidth = " << winwidth
					<< ", hpos = " << bounds.left << std::endl);
			hbar->SetRange(0,maxval);
			hbar->SetProportion(winwidth/dsize);
			hbar->SetSteps(ceil(charWidth),
							floor((winCols-2)*charWidth));
			bounds = Bounds();
		}
		changed = true;
	}
	
	if( changed ) {
		SetClipRegion(bounds);
		if( sizeRows < 0 || sizeCols < 0 ) {
			TermSetFixedSize(sizeRows,sizeCols);
		}
		last_bounds = bounds;
		history_last_len = history_len;
		horiz_last_len = horiz_len;
		// Update view offset
		float winheight = floor(bounds.Height()+.5);
		float dsize = floor((history_len*charHeight)+.5);
		float off = 0;
		if( dsize < winheight ) off = ceil(winheight-dsize+.5);
		if( off != top_offset ) {
			top_offset = off;
			Invalidate();
			invalidate = true;
		}
	}
	
	updating = 0;
	BScrollBar* vbar = ScrollBar(B_VERTICAL);
	if( vbar ) {
		if( max_vval >= 0 ) {
			vbar->SetRange(0,max_vval);
		}
		if( force_vbar >= 0 ) {
			vbar->SetValue(force_vbar);
		}
	}
	return invalidate;
}
	
const BFont& ArpCoreTerminal::GetStyledFont(style_t style) const
{
	switch( style&(TERM_STYLEBOLD|TERM_STYLEITALIC) ) {
	case TERM_STYLEPLAIN:
		return fontPlain;
	case TERM_STYLEBOLD:
		return fontBold;
	case TERM_STYLEITALIC:
		return fontItalic;
	case TERM_STYLEBOLD|TERM_STYLEITALIC:
		return fontBoldItalic;
	default:
		return fontPlain;
	}
}

void ArpCoreTerminal::ChooseFont(style_t style)
{
	style &= (TERM_STYLEBOLD|TERM_STYLEITALIC);
	if( style != cur_font_style ) {
		BView::SetFont(&GetStyledFont(style),B_FONT_FAMILY_AND_STYLE);
		cur_font_style = style;
	}
}

void ArpCoreTerminal::drawRowCol(int32 row, int32 col)
{
	if( col < clipLeft || col > clipRight
		|| row < clipBottom || row > clipTop ) return;
	float x = col*charWidth;
	float y = (history_len-row-1)*charHeight + top_offset;
	
	// What to draw if this row/col doesn't exist.
	ichar text = ' ';
	style_t style = TERM_STYLEPLAIN;
	RowInfo* ri = NULL;
	if( col >= 0 && row >= 0 && row < history_len ) {
		ri = page[row];
		if( ri && ri->Divider() ) return;
		if( ri && col < ri->UsedCols() && col < ri->Cols() ) {
			text = ri->Text()[col];
			style = ri->Style()[col];
		}
	}
	ChooseFont(style);
	ArpColor highcol;
	ArpColor lowcol;
	if( !(curMode&TERM_MODEHIDECURSOR)
			&& RowToPage(curRow)==row && curCol==col ) {
		highcol = cursorForeground;
		lowcol = cursorBackground;
	} else if( hShown && ri && col <= ri->UsedCols()
			&& InRegion(row,col,hTopRow,hTopCol,hBotRow,hBotCol) ) {
		if( col == ri->UsedCols() ) {
			if( (curMode&TERM_MODEINVERSE) != 0 ) {
				SetHighColor(textForeground[0]);
			} else {
				SetHighColor(textBackground[0]);
			}
			FillRect(BRect(x+charWidth/2,y,
							x+charWidth-1,y+charHeight-1));
			SetHighColor(highForeground);
			SetLowColor(highBackground);
			FillRect(BRect(x,y,x+charWidth/2-1,y+charHeight-1),
					B_MIXED_COLORS);
			return;
		}
		highcol = highForeground;
		lowcol = highBackground;
	} else if( ( (style&TERM_STYLEINVERSE) == TERM_STYLEINVERSE )
				^ ( (curMode&TERM_MODEINVERSE) != 0 ) ) {
		highcol = textBackground[(style>>TERM_STYLEBACKGROUND_POS)&TERM_STYLECOLOR_MASK];
		lowcol = textForeground[(style>>TERM_STYLEFOREGROUND_POS)&TERM_STYLECOLOR_MASK];
	} else {
		highcol = textForeground[(style>>TERM_STYLEFOREGROUND_POS)&TERM_STYLECOLOR_MASK];
		lowcol = textBackground[(style>>TERM_STYLEBACKGROUND_POS)&TERM_STYLECOLOR_MASK];
	}
	// XXXX
	//printf("drawRowCol: r=%ld c=%ld cr=%ld cc=%ld lr=%ld lc=%ld\n",
	//		PageToRow(row), col, curRow, curCol, lastRow, lastCol);

	SetHighColor(lowcol);
	FillRect(BRect(x,y,x+charWidth-1,y+charHeight-1));
	SetHighColor(highcol);
	SetLowColor(lowcol);
	DrawChar(text,BPoint(x,y+fontAscent));
	if( style&TERM_STYLEUNDERSCORE ) {
		StrokeLine(BPoint(x,y+fontAscent),
					BPoint(x+charWidth-1,y+fontAscent));
	}
}

void ArpCoreTerminal::drawRow(int32 row, int32 force)
{
	float y = (history_len-row-1)*charHeight + top_offset;
	
	//printf("DrawRow: row=%ld, y=%f\n",row,y);
	
	// Computing columns to draw from visible/clipping regions
	int32 col = int32(last_bounds.left/charWidth);
	int32 last_col = int32(ceil(last_bounds.right/charWidth));
	if( col < clipLeft ) col = clipLeft;
	if( last_col > clipRight ) last_col = clipRight;

	// This is the color to draw any empty background in
	if( (curMode&TERM_MODEINVERSE) != 0 ) {
		SetHighColor(textForeground[0]);
	} else {
		SetHighColor(textBackground[0]);
	}
			
	RowInfo* ri = NULL;
	
	// If part being drawn is completely empty, just erase it
	if( row < 0 || row >= history_len || (ri=page[row]) == 0
		|| col >= ri->UsedCols() || last_col < 0 || ri->Divider() == true ) {
			
		// If this is a divider, only draw out special spiffy divider
		// glyph.
		if( ri && ri->Divider() ) {
			ArpColor base( (curMode&TERM_MODEINVERSE) != 0
							? textForeground[0] : textBackground[0] );
			BRect line(last_bounds.left, y, last_bounds.right, y+charHeight-1);
			float extra = floor( (line.Height()-6) / 2 );
			if( extra > 0 ) {
				SetHighColor(base);
				FillRect( BRect(line.left, line.top,
								line.right, line.top+extra-1) );
				FillRect( BRect(line.left, line.bottom-extra+1,
								line.right, line.bottom) );
				line.top += extra;
				line.bottom -= extra;
			}
			
			SetHighColor(tint_color(base, B_DARKEN_MAX_TINT) );
			StrokeLine(BPoint(line.left, line.top),
					   BPoint(line.right, line.top));
			line.top++;
			if( line.top <= line.bottom ) {
				SetHighColor(tint_color(base, B_LIGHTEN_1_TINT) );
				StrokeLine(BPoint(line.left, line.bottom),
						   BPoint(line.right, line.bottom));
				line.bottom--;
			}
			if( line.top <= line.bottom ) {
				SetHighColor(tint_color(base, B_DARKEN_4_TINT) );
				StrokeLine(BPoint(line.left, line.top),
						   BPoint(line.right, line.top));
				line.top++;
			}
			if( line.top <= line.bottom ) {
				SetHighColor(tint_color(base, B_LIGHTEN_2_TINT) );
				StrokeLine(BPoint(line.left, line.bottom),
						   BPoint(line.right, line.bottom));
				line.bottom--;
			}
			if( line.top <= line.bottom ) {
				SetHighColor(tint_color(base, B_DARKEN_3_TINT) );
				StrokeLine(BPoint(line.left, line.top),
						   BPoint(line.right, line.top));
				line.top++;
			}
			if( line.top <= line.bottom ) {
				SetHighColor(tint_color(base, B_LIGHTEN_MAX_TINT) );
				StrokeLine(BPoint(line.left, line.bottom),
						   BPoint(line.right, line.bottom));
				line.bottom--;
			}
			if( line.top <= line.bottom ) {
				SetHighColor(tint_color(base, B_DARKEN_2_TINT) );
				FillRect(BRect(line.left, line.top,
							   line.right, line.bottom));
			}
			ri->Dirty(false);
			return;
		}
	
		float left=col*charWidth;
		float mid=left;
		float right = ((last_col+1)*charWidth)-1;
		if( hShown && (!ri || col == ri->UsedCols()) &&
			InRegion(row,col,hTopRow,hTopCol,hBotRow,hBotCol) ) {
			mid += charWidth/2;
		}
		if( mid < right ) {
			FillRect(BRect(mid,y,right,y+charHeight-1));
		} else mid = right;
		if( left < mid ) {
			SetHighColor(highForeground);
			SetLowColor(highBackground);
			FillRect(BRect(left,y,mid-1,y+charHeight-1),B_MIXED_COLORS);
		}
		if( !(curMode&TERM_MODEHIDECURSOR)
				&& row == RowToPage(curRow) ) {
			ArpD(cdb << ADH << "ArpCoreTerminal: Drawing cursor: row=" << curRow
						<< ", col=" << curCol << std::endl);
			drawRowCol(RowToPage(curRow),curCol);
		}
		if(ri) ri->Dirty(false);	
		return;
	}
	
	if( !force && !ri->Dirty() ) return;
	
	// If left edge is out of available data bounds [should
	// never happen!], erase the left area
	if( col < 0 ) {
		FillRect(BRect(col*charWidth,y,-1,y+charHeight-1));
		col = 0;
	}
	
	// If right edge is out of this row's data bounds, erase
	// the covered area
	if( last_col >= ri->UsedCols()
		&& (force || ri->RDirty()+1 >= ri->UsedCols()) ) {
		float left=ri->UsedCols()*charWidth;
		float mid=left;
		float right = ((last_col+1)*charWidth)-1;
		if( hShown &&
			InRegion(row,ri->UsedCols(),hTopRow,hTopCol,hBotRow,hBotCol) ) {
			mid += charWidth/2;
		}
		if( mid < right ) {
			FillRect(BRect(mid,y,right,y+charHeight-1));
		} else mid = right;
		if( left < mid ) {
			SetHighColor(highForeground);
			SetLowColor(highBackground);
			FillRect(BRect(left,y,mid-1,y+charHeight-1),B_MIXED_COLORS);
		}
		last_col = ri->UsedCols()-1;
	}
	
	// Restrict drawing range to dirty area		
	if( !force ) {
		if( ri->LDirty() < ri->Cols() ) col = ri->LDirty();
		if( ri->RDirty() >= 0 ) last_col = ri->RDirty();
	}
	
	// Expand left and right by one char, to take care of characters
	// that go out of their cell area
	col--;
	last_col++;
	
	// Make sure the above were valid row dimensions
	if( col < 0 ) col = 0;
	if( last_col >= ri->UsedCols() ) last_col = ri->UsedCols()-1;
	
	// Make absolutely sure we won't be accessing invalid data
	if( last_col >= ri->Cols() ) last_col = ri->Cols()-1;
	
	ichar* text = ri->Text();
	style_t* style = &(ri->Style()[col]);
	
	ArpD(cdb << ADH << "ArpCoreTerminal: Drawing row: " << row
				<< ", from " << col << " to " << last_col << std::endl);
#if 0
	std::cerr << "ArpCoreTerminal: Drawing row: " << row
				<< ", from " << col << " to " << last_col
				<< ", numRows=" << numRows << ", Cols()=" << ri->Cols() << std::endl;
#endif

	while( col <= last_col ) {
		int32 first=col;
		style_t cur_style = *style;
		ArpColor highcol;
		ArpColor lowcol;
		if( hShown &&
			InRegion(row,col,hTopRow,hTopCol,hBotRow,hBotCol) ) {
			highcol = highForeground;
			lowcol = highBackground;
		} else if( ( (cur_style&TERM_STYLEINVERSE) != 0 )
		   			^ ( (curMode&TERM_MODEINVERSE) != 0 ) ) {
			highcol = textBackground[(cur_style>>TERM_STYLEBACKGROUND_POS)
							&TERM_STYLECOLOR_MASK];
			lowcol = textForeground[(cur_style>>TERM_STYLEFOREGROUND_POS)
							&TERM_STYLECOLOR_MASK];
		} else {
			highcol = textForeground[(cur_style>>TERM_STYLEFOREGROUND_POS)
							&TERM_STYLECOLOR_MASK];
			lowcol = textBackground[(cur_style>>TERM_STYLEBACKGROUND_POS)
							&TERM_STYLECOLOR_MASK];
		}
		
		while( col <= last_col && cur_style == *style ) {
			col++;
			style++;
		}
		float x = first*charWidth;
		float xr = (col*charWidth)-1;
		ChooseFont(cur_style);
		
#if 0
		ArpD(cdb << ADH << "Font=" << font.Name()
					<< ", Size=" << font.Size() << std::endl);
#endif

		SetHighColor(lowcol);
		FillRect(BRect(x,y,xr,y+charHeight-1));
		SetHighColor(highcol);
		SetLowColor(lowcol);
		DrawString((const char *)&text[first],col-first,BPoint(x,y+fontAscent));

		if( (cur_style&TERM_STYLEUNDERSCORE) != 0 ) {
			StrokeLine(BPoint(x,y+fontAscent),BPoint(xr,y+fontAscent));
					//BPoint(x+((col-first)*charWidth),y+fontAscent));
		}
	}
	if( !(curMode&TERM_MODEHIDECURSOR)
			&& row == RowToPage(curRow) ) {
		ArpD(cdb << ADH << "ArpCoreTerminal: Drawing cursor: row=" << curRow
					<< ", col=" << curCol << std::endl);
		drawRowCol(RowToPage(curRow),curCol);
	}
	
	ri->Dirty(false);	
}
