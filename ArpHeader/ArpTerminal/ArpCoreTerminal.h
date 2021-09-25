/*
 * Copyright (c)1999 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * ----------------------------------------------------------------------
 *
 * ArpCoreTerminal.h
 *
 * The ArpCoreTerminal class is the core implementation of an
 * ArpTerminalInterface.  It knows how to draw and control the
 * terminal screen, and provides all of the low-level support
 * needed to create a fully usable terminal widget.  It does
 * not, however, implement any higher functionality for
 * responding to input from the user or interacting with the
 * ArpEmulator class -- that is left to a subclass, namely
 * ArpTerminal.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• The view is not correctly redrawn when it is resized,
 *	  causing ugly garbage to often appear in the newly
 *	  exposed area.  This problem can be induced by quickly
 *	  resizing the window height smaller and larger.
 *	• The code for highlighting regions with the mouse and
 *	  copying them into the clipboard should probably be moved
 *	  to this class, and out of ArpTerminal.
 *	• When the BView is resized due to a jump in window size
 *	  (that is, when its window's ResizeTo() function is called),
 *	  the terminal is not correctly redrawn: it only draws the
 *	  newly visible parts, and doesn't scroll down the previous
 *	  view.  This could well have the same cause as the first
 *	  bug mentioned here...
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 8/28/1998:
 *	• Fixed a major memory leak -- RowInfo objects were never
 *	  being deallocated.  OOPS!!
 *
 * 8/16/1997:
 *	• Added modes to automatically make the cursor visible on
 *	  terminal input or output.  The core terminal only
 *	  implements this for output, as it can't know what input
 *	  is appropriate for a scroll; it is up to the subclass
 *	  to handle input autoscrolling.
 *
 * 7/31/1997:
 *	• TermXToCol() and TermYToRow() were incorrect.
 *
 * 7/25/1997:
 *	• Added history use control.
 *	• Fixed a bug in regions: it was a little too paranoid about
 *	  placing them close to each other, forcing them farther
 *	  apart than needed.  This problem showed up most obviously
 *	  in scrolls causing screen corruption because the scroll
 *	  region is too large.
 *	• Added some experimental stuff to set the horizontal data
 *	  area to only cover the area actually containing text.
 *
 * 7/17/1997:
 *	• Split this file into ArpCoreTerminal and ArpTerminal.
 *	• Added highlight support and control over the cursor color.
 *	• Wasn't correctly updating terminal state when font changed.
 *
 * 7/9/1997:
 *	• Fixed a bug that was allowing the perceived total data
 *	  height to be larger than the actual number of history
 *	  rows that are being kept.
 *	• Fixed numerous bugs related to redrawing the screen after
 *	  scrolling a region that is not the entire terminal area,
 *	  and generally cleaned up the class definition.
 *
 * 0.1: Created this file from the WebTerm source.
 *
 */

#ifndef ARPTERMINAL_ARPCORETERMINAL_H
#define ARPTERMINAL_ARPCORETERMINAL_H

#ifndef _MESSENGER_H
#include <app/Messenger.h>
#endif

#ifndef _LOCKER_H
#include <support/Locker.h>
#endif

#ifndef ARPKERNEL_ARPREFARRAY_H
#include <ArpKernel/ArpRefArray.h>
#endif

#ifndef ARPTERMINAL_ARPTERMINALINTERFACE_H
#include <ArpTerminal/ArpTerminalInterface.h>
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

#include <String.h>

// forward ref
class ArpMessage;

class ArpCoreTerminal :	public BView,
					public ArpTerminalInterface {
private:
	typedef BView inherited;

public:

	/* ------------------------------------------------------------
	   CONSTRUCTOR AND GLOBAL CONTROL METHODS.
	   ------------------------------------------------------------ */

	ArpCoreTerminal(BRect frame, const char* name,
			 uint32 resizeMode = B_FOLLOW_ALL_SIDES,
			 uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE);
	virtual ~ArpCoreTerminal();
	
	void SetTintForeground(float level);
	void SetTintBackground(float level);
	
	// Set destination for terminal reports to its owner.
	// By default, this is simply its BWindow.
	void SetOwner(const BMessenger& hand);
	BMessenger Owner(void) const;
	
	/* ------------------------------------------------------------
	   HISTORY MANIPULATION METHODS.
	   ------------------------------------------------------------ */

	// These set/get the maximum number of history rows that
	// are available; fewer may actually be used.
	void SetHistorySize(int32 size);
	int32 HistorySize(void) const;
	
	// This is used to control how history is used.
	typedef enum {
		HISTORY_NONE = 0,		// Don't scroll anything into history
		HISTORY_MINIMUM,		// Only real scrolls off top into history
		HISTORY_MODERATE,		// Add screen clears to above
		HISTORY_AGGRESSIVE,		// Put everything possible in
	} HistoryUseType;
	
	void SetHistoryUse(HistoryUseType use);
	HistoryUseType HistoryUse(void) const;
	
	/* ------------------------------------------------------------
	   SCREEN LAYOUT METHODS.
	   ------------------------------------------------------------ */

	void InitTerminal();
	
	/* Automatically scroll on terminal input and output.  This
	   class only implements scrolling on terminal output, since
	   it can't know what types of input are appropriate for
	   scrolling the screen.  Subclasses should implement the
	   input scrolling theirselves, as:
	   
	   if( input_scroll_appropriate ) {
			if( (((int)AutoScrollMode())&AUTOSCROLL_INPUT) != 0 ) {
				int32 row=0, col=0;
				TermGetCursorPos(&row,&col);
				MakePosVisible(row,col);
			}
	   }
	*/
	typedef enum {
		AUTOSCROLL_NONE = 0,
		AUTOSCROLL_INPUT = 1,
		AUTOSCROLL_OUTPUT = 2,
		AUTOSCROLL_BOTH = 3,
	} AutoScrollType;
	void SetAutoScrollMode(AutoScrollType scroll);
	AutoScrollType AutoScrollMode(void) const;
	
	// Pass in NULL pointer to not change that position.
	void AutoScrollTo(int32* row, int32* col);
	
	void MakePosVisible(int32 row, int32 col);
	
	void StartSession();
	void EndSession();
	
	/* ------------------------------------------------------------
	   ABSTRACT INTERFACE.
	   ------------------------------------------------------------ */

	// Configuration interface isn't implemented here -- it is
	// implemented by ArpRemoteTerminal.
	virtual status_t GetConfiguration(BMessage* values) const	{ return B_OK; }
	virtual status_t PutConfiguration(const BMessage* values)	{ return B_OK; }
	virtual status_t Configure(ArpVectorI<BView*>& views)		{ return B_OK; }
	
	virtual void TermReset(bool hard);
	
	virtual void TermSetMode(TermModeType mode);
	virtual TermModeType TermMode(void) const;

	virtual void TermSetLFChars(const char* which);
	virtual const char* TermLFChars() const;
	virtual void TermSetCRChars(const char* which);
	virtual const char* TermCRChars() const;
	
	virtual void TermSetEnterStream(const char* str);
	virtual const char* TermEnterStream() const;
	
	virtual void TermSetTranslation(const ichar* t);
	virtual const ichar* TermTranslation(void) const;

	virtual void TermSetEncoding(int32 enc);
	virtual int32 TermEncoding() const;
	
	virtual void TermSetTitle(const char* title);
	
	virtual void TermSetTextBackground(rgb_color bg, TermColorID num);
	virtual void TermSetTextForeground(rgb_color fg, TermColorID num);
	virtual rgb_color TermTextBackground(TermColorID num) const;
	virtual rgb_color TermTextForeground(TermColorID num) const;

	virtual bool TermSetFixedSize(int32 rows, int32 cols);
	virtual void TermGetFixedSize(int32* rows, int32* cols) const;
	virtual void TermGetViewSize(int32* rows, int32* cols) const;
	virtual void TermGetSize(int32* rows, int32* cols) const;
	
	virtual int32 TermHistoryTop(void) const;
	
	virtual void TermSetCursorPos(int32 row, int32 col);
	virtual void TermGetCursorPos(int32* row, int32* col) const;

	virtual int32 TermXToCol(float x) const;
	virtual int32 TermYToRow(float y) const;

	virtual void TermSetRegion(void);
	virtual void TermSetRegion(int32 top, int32 bottom);
	virtual void TermSetRegion(int32 top, int32 bottom, int32 left, int32 right);

	virtual void TermGetRegion(int32* top, int32* bottom, int32* left, int32* right) const;

	virtual void TermSetHighlight(TermHighlightType type,
									int32 row1, int32 col1,
									int32 row2, int32 col2);
	virtual bool TermInHighlight(int32 row, int32 col) const;
	virtual void TermGetHighlight(TermHighlightType* type,
									int32* row1, int32* col1,
									int32* row2, int32* col2) const;
	virtual void TermGetTextRange(int32 row1, int32 col1,
									int32 row2, int32 col2,
									size_t* array_size,
									ichar* text, style_t* style) const;
									
	virtual void TermSetStyle(int32 style);
	virtual void TermSetForeColor(TermColorID col);
	virtual void TermSetBackColor(TermColorID col);

	virtual int32 TermStyle(void) const;
	virtual TermColorID TermForeColor(void) const;
	virtual TermColorID TermBackColor(void) const;

	// These two do not go through the translation table!
	virtual void TermSetChar(ichar c);
	virtual void TermSetChar(ichar c, int32 row, int32 col);

	virtual ichar TermGetChar(void) const;
	virtual ichar TermGetChar(int32 row, int32 col) const;
	virtual style_t TermGetCharStyle() const;
	virtual style_t TermGetCharStyle(int32 row, int32 col) const;

	virtual void TermScrollRegion(int32 top, int32 bottom, int32 amount);
	virtual void TermScroll(int32 amount);

	virtual void TermSetChars(int32 left, int32 right, ichar fillc=' ',
					  style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermDeleteChars(int32 left, int32 right, ichar fillc=' ',
						style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermInsertChars(int32 left, int32 right, ichar fillc=' ',
						 style_t fillstyle=TERM_STYLECURRENT);

	virtual void TermClearEOL(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermClearBOL(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermClearLine(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermClearEOD(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermClearBOD(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT);
	virtual void TermClear(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT);

	virtual void TermRedraw(int32 top, int32 bot, bool full);
	virtual void TermRedraw(void);
	virtual void TermClean(void);

	virtual void TermSendTTY(const ichar * d, size_t len, uint32 flags=0);
	virtual void TermSendRemote(const ichar * d, size_t len);

  /* ------------------------------------------------------------
     BVIEW METHODS WE OVERRIDE.
     ------------------------------------------------------------ */

	virtual void SetFont(const BFont *font, uint32 mask = B_FONT_ALL);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual void GetNeededSize(int32 cols, int32 rows,
								float* width, float* height);
	virtual void TermSizeChanged(int32 rows, int32 cols);

	virtual void AttachedToWindow();
	virtual void FrameResized(float width, float height);
	virtual	void ScrollTo(BPoint where);
	virtual void Draw(BRect rect);
	
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void MouseDown(BPoint point);

	/* ------------------------------------------------------------
	   INHERITANCE SECTION
	   ------------------------------------------------------------ */

protected:

	/* Information about a single row of text.  This is available
	   to subclasses to directly view and change the terminal
	   text -- but do please be reasonable in how you use it. */
	class RowInfo {
	public:
		void OpenColumns(int32 min);
		void Dirty(bool state);
		void Dirty(int32 left, int32 right);
		inline ichar* Text(void) { return text; }
		inline style_t* Style(void) { return style; }
		inline const ichar* Text(void) const { return text; }
		inline const style_t* Style(void) const { return style; }
		inline void SetDivider(bool state) { div = state; }
		inline bool Divider(void) const { return div; }
		inline bool Dirty(void) const { return dirty; }
		inline int32 LDirty(void) const { return ldirty < 0 ? 0:ldirty; }
		inline int32 RDirty(void) const { return rdirty >= cols ? cols-1:rdirty; }
		inline int32 Cols(void) const { return cols; }
		inline void SetUsedCols(int32 num)
			{ usedcols = num; if(usedcols > cols) SetColumns(usedcols); }
		inline int32 UsedCols(void) const { return usedcols; }
		inline void OpenUsedCols(int32 pos)
			{ usedcols = pos > usedcols ? pos : usedcols; }
		inline void SetConnected(bool state) { conn = state; }
		inline bool Connected(void) const { return conn; }
	private:
		friend ArpCoreTerminal;
		friend ArpRefArray<RowInfo>;
		
		// Only the core terminal gets to allocate or free rows.
		RowInfo(int32 size=0);
		~RowInfo();
		int32 Ref() { return ++refs; }
		int32 Deref();
		
		// Don't let anyone else reduce the number of allocated
		// columns.
		void SetColumns(int32 num);
		inline void Squeeze(void) { SetColumns(UsedCols()); }
		
		ichar* text;			// Characters in row
		style_t* style;			// Style of characters
		int32 ldirty,rdirty;	// The left and right-most changes
		int32 cols;				// Number of columns in row
		int32 usedcols;			// # columns of actually placed data
		int32 refs;				// Number of references on this row
		bool div;				// TRUE if this is a session divider
		bool dirty;				// Changed since last redraw?
		bool conn;				// TRUE if connected to next row
	};
	
	/* ------------------------------------------------------------
	   OVERRIDE THESE METHODS TO CHANGE FUNCTIONALITY.
	   ------------------------------------------------------------ */

	// This modifies the highlight regions so that it conforms
	// to the given TermHighlightType mode.  You can do this
	// by modifying the row and column variables passed in;
	// any highlighting being displayed goes through this
	// function, so it will always be able to adjust the highlight
	// range.  The default implementation adjusts "line" and
	// "word" highlighting types as appropriate.
	virtual void ConformHighlight(TermHighlightType type,
									int32& top, int32& tcol,
									int32& bottom, int32& bcol);
	
	// This sends text input to the terminal emulator.  This is called by
	// ConvertInput(), below.
	virtual void EmulateToRemote(const ichar* d, size_t len);
	
	/* ------------------------------------------------------------
	   LOW-LEVEL RENDERING METHODS.
	   ------------------------------------------------------------ */

	inline float RawYToRow(float y) const
		{ return ((y-top_offset)/charHeight)-(history_len-numRows); }
	inline float RawXToCol(float x) const { return x/charWidth; }
		
	inline int32 IRawYToRow(float y) const
		{ return int32(floor(RawYToRow(y))); }
	inline int32 IRawXToCol(float x) const
		{ return int32(floor(RawXToCol(x))); }
	
	inline int32 TopVisibleRow(void) const
		{ return int32(floor((last_bounds.top-top_offset)/charHeight)-(history_len-numRows)); }
	inline int32 BottomVisibleRow(void) const
		{ return int32(ceil((last_bounds.bottom-top_offset)/charHeight)-(history_len-numRows)); }
	inline int32 TopDataRow(void) const { return PageToRow(history_len-1); }
	inline int32 BottomDataRow(void) const { return PageToRow(0); }
	
	inline int32 LeftVisibleCol(void) const
		{ return int32(floor(last_bounds.left/charWidth)); }
	inline int32 RightVisibleCol(void) const
		{ return int32(ceil(last_bounds.right/charWidth)); }
	inline int32 LeftDataCol(void) const { return 0; }
	inline int32 RightDataCol(void) const { return horiz_len-1; }
	
	inline int32 DataHeight(void) const { return history_len; }
	inline int32 DataWidth(void) const { return horiz_len; }
	
	inline float CharHeight(void) const { return charHeight; }
	inline float CharWidth(void) const { return charWidth; }
	
	// Retrieve top-left pixel location on the canvas
	inline float RowToY(int32 row) const {
		return (history_len-RowToPage(row)-1)*charHeight + top_offset; }
	inline float ColToX(int32 col) const { return col*charWidth; }
	
	// Set character clipping region from a view bounds rectangle
	void SetClipRegion(const BRect& rect);
	
	inline RowInfo* GetRowInfo(int32 row)
		{ return (row<numRows && row>=(numRows-history_len))
					? page[numRows-row-1] : NULL; }
	inline const RowInfo* GetRowInfo(int32 row) const
		{ return (row<numRows && row>=(numRows-history_len))
					? page[numRows-row-1] : NULL; }
	void PutChar(ichar c);
	void PutStdChar(ichar c);

	void SetDirty(int32 top, int32 bottom, int32 left=-1, int32 right=-1,
				  RowInfo* inri=NULL);
	inline void ClearDirty(void)
		{ dirtyTop=1; dirtyBottom=0;
		  scrollAmount=0; scrollTop=-1; scrollBottom=-2; }
	inline bool IsDirty(void) const { return dirtyTop <= dirtyBottom; }

	// Expand used characters to given column, setting dirty state
	// as needed to cause new character and any highlighting up
	// to it to be drawn.
	void OpenToColumn(int32 row, int32 col, RowInfo* ri);
	
	// Return the BFont corresponding to a particular char style.   
	const BFont& GetStyledFont(style_t style) const;

	// Take raw input from the OS [in UTF-8 format], convert it to the current
	// character encoding, and send it out to the emulator by calling
	// EmulatorToRemote().
	virtual void ConvertInput(const ichar* d, size_t len);
	
	// Return the conversion code corresponding to TermEncoding().
	// This is -1 if no conversion should be done.  (That is, the encoding
	// is native Unicode UTF-8.)
	int32 GetEncodingConv() { return encodingConv; }
	
	/* ------------------------------------------------------------
	   IMPLEMENTATION SECTION
	   ------------------------------------------------------------ */

private:
	
	inline int32 RowToPage(int32 row) const { return numRows-1-row; }
	inline int32 PageToRow(int32 page) const { return numRows-1-page; }
	
	bool ResizeTerminal(int32 rows, int32 cols);
	// Update scroll bars positions with terminal size/position
	bool UpdateBounds(void);
	
	// Row locations here are in page coordinates
	void drawRowCol(int32 row, int32 col);
	void drawRow(int32 row, int32 force);

	void ChooseFont(style_t style);
	
	// Turn on/off a place where highlight changes; in page coords
	void MarkHighlight(int32 row, int32 col, bool state);
	
	// Move row coordinate position to follow a scroll.  All
	// inputs are in page coordinates.  Negative values scroll up.
	int32 ScrollRow(int32 row, int32 amount, int32 top, int32 bot,
					bool constrain=true);
	
	inline bool InRegion(int32 row, int32 col,
							int32 topRow, int32 topCol,
							int32 botRow, int32 botCol) const
	{
		if( topRow == botRow )
			return (row == topRow && col >= topCol && col <= botCol);
		return ( (row < topRow && row > botRow)
				|| (row == topRow && col >= topCol)
				|| (row == botRow && col <= botCol) );
	}
	
	/* Initial text colors */
	
	enum {
		NUM_INITCOLORS = 9
	};
	static const uint32 init_foregrounds[NUM_INITCOLORS];
	static const uint32 init_backgrounds[NUM_INITCOLORS];

	/* ------------------------------------------------------------
	   INSTANCE DATA SECTION
	   ------------------------------------------------------------ */

	// The terminal informs its container about changes and
	// events by sending them here.
	BMessenger mOwner;
	
	// Global mode flags.
	int32 curMode;

	// Line termination modes.
	BString lfChars, crChars, enterStream;
	
	// The fonts being used for rendering and useful info about them.
	BFont fontPlain, fontBold, fontItalic, fontBoldItalic;
	float charWidth, charHeight, fontAscent, fontDescent;

	style_t curStyle;            // Current character style.
	
	// The current background and foreground colors being used.
	ArpColor textBackground[ArpTerminalInterface::TERM_NUMTEXTCOLORS];
	ArpColor textForeground[ArpTerminalInterface::TERM_NUMTEXTCOLORS];

	// Background and foreground for highlighted text.
	ArpColor highBackground, highForeground;
	
	// Background and foreground of cursor.
	ArpColor cursorBackground, cursorForeground;
	
	/* Translation table for translating written characters to screen codes.
	   This needs to be transformed into something that works
	   for unicode variable-length characters... */
	enum {
		translationLength = 256
	};
	// The standard translation table.
	static ichar defaultTranslation[translationLength];
	// The current table being used.
	ichar curTranslation[translationLength];

	/* Current screen dimension information and cursor location. */

	int32 sizeRows, sizeCols;    // Size requested by user
	int32 winRows, winCols;	     // Current width/height of view
	int32 numRows, numCols;      // Current terminal dimensions
	int32 lastNumRows, lastNumCols;
	
	int32 curRow, curCol;        // Current cursor location.
	bool overChar;               // Flag if at far right of screen.

	int32 regionTop, regionBottom, // Current bounding region.
	    regionLeft, regionRight;

	/* Page scrolling -- keep track of how much has scrolled
	   so that all pages are drawn; and force a redraw if
	   the scroll bounds change so we don't get confused. */
	
	int32 scrollAmount, scrollTop, scrollBottom;
	
	AutoScrollType scrollMode;
	
	/* The following four variables are in page coordinates --
	   that is, the bottom line is 0 and the top line has
	   the value of (history_len-1). */
	   
	int32 clipLeft, clipTop;     // Limit drawing to characters
	int32 clipRight, clipBottom; // over and in this rectangle
	
	/* Full page size information and view on that page. */

	HistoryUseType historyUse;
	
	bool updating;				// Currently changing this info?
	bool has_output;
	
	int32 history_len;			// Total size of page history, incl vis region
	int32 history_last_len;		// Last length of history we displayed
	
	int32 horiz_len;			// Number of characters
	int32 horiz_last_len;		// Last displayed # characters
	
	BRect last_bounds;			// Last used bounding box
	float top_offset;			// If data height < bound height

	/* The actual, complete character data for the page. */
	
	ArpRefArray<RowInfo> page;

	/* Various state used for rendering optimization. */
	
	int32 lastRow, lastCol;		// Last postion cursor was drawn at.
	int32 dirtyTop,				// First and last dirty rows.
	      dirtyBottom;
	
	style_t cur_font_style;
	
	/* Highlighting state information.  Character locations are
	   in page coordinates. */

	int32 hMode;				// Current highlight mode / off
	bool hShown;				// Set if highlight is occurring.
	
	int32 hTopRow, hTopCol;		// Top row&column of highlight
	int32 hBotRow, hBotCol;		// Bottom row&column of highlight

	char convBuffer[512];		// Buffer for doing character encoding conversions
	uint8 encoding;				// Current character encoding
	int32 encodingConv;			// The corresponding conversion. (-1 for no conv)
	
	// This is just for debugging
#ifdef DEBUG
	static const bool outputecho;
#endif
};

#endif
