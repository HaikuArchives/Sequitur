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
 * ArpTerminalInterface.h
 *
 * The ArpTerminalInterface class describes an abstract interface
 * to some terminal-like device.  It is the interface through
 * which particular emulator implementations may manipulate
 * the terminal they are connected to: it defines various
 * functions for manipulating the terminal screen and
 * getting/setting general state information.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• I am not currently trying to maintain binary compatibility with
 *	  new versions of this interface.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 8/18/1997:   *** THIS BREAKS BINARY COMPATIBILITY ***
 *	• Added version identification to interface.
 *	• Much clean-up and commentization.
 *
 * 0.1: Created this file from the WebTerm source.
 *
 */

#ifndef ARPTERMINAL_ARPTERMINALINTERFACE_H
#define ARPTERMINAL_ARPTERMINALINTERFACE_H

// This is only to get the ichar definition
#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef _GRAPHICS_DEFS_H
#include <interface/GraphicsDefs.h>
#endif

class ArpTerminalInterface : public ArpConfigurableI {

public:

	// --------------------------------------------------------
	// STANDARD ANSI CONTROL CODES.
	// --------------------------------------------------------
	
	enum {
		ANSI_NUL = 0x00,
		ANSI_SOH = 0x01,
		ANSI_STX = 0x02,
		ANSI_ETX = 0x03,
		ANSI_EOT = 0x04,
		ANSI_ENQ = 0x05,
		ANSI_ACK = 0x06,
		ANSI_BEL = 0x07,
		ANSI_BS  = 0x08,
		ANSI_HT  = 0x09,
		ANSI_LF  = 0x0a,
		ANSI_VT  = 0x0b,
		ANSI_FF  = 0x0c,
		ANSI_CR  = 0x0d,
		ANSI_SO  = 0x0e,
		ANSI_SI  = 0x0f,
		ANSI_DLE = 0x10,
		ANSI_DC1 = 0x11,
		ANSI_DC2 = 0x12,
		ANSI_DC3 = 0x13,
		ANSI_DC4 = 0x14,
		ANSI_NAK = 0x15,
		ANSI_SYN = 0x16,
		ANSI_ETB = 0x17,
		ANSI_CAN = 0x18,
		ANSI_EM  = 0x19,
		ANSI_SUB = 0x1a,
		ANSI_ESC = 0x1b,
		ANSI_FS  = 0x1c,
		ANSI_GS  = 0x1d,
		ANSI_RS  = 0x1e,
		ANSI_US  = 0x1f,
		ANSI_DEL = 0x7f,
		ANSI_IND = 0x84,
		ANSI_NEL = 0x85,
		ANSI_SSA = 0x86,
		ANSI_ESA = 0x87,
		ANSI_HTS = 0x88,
		ANSI_HTJ = 0x89,
		ANSI_VTS = 0x8a,
		ANSI_PLD = 0x8b,
		ANSI_PLU = 0x8c,
		ANSI_RI  = 0x8d,
		ANSI_SS2 = 0x8e,
		ANSI_SS3 = 0x8f,
		ANSI_DCS = 0x90,
		ANSI_PU1 = 0x91,
		ANSI_PU2 = 0x92,
		ANSI_STS = 0x93,
		ANSI_CCH = 0x94,
		ANSI_MW  = 0x95,
		ANSI_SPA = 0x96,
		ANSI_EPA = 0x97,
		ANSI_CSI = 0x9b,
		ANSI_ST  = 0x9c,
		ANSI_OSC = 0x9d,
		ANSI_PM  = 0x9e,
		ANSI_APC = 0x9f,
	};
	
	/* Virtual destructor, so that you can get rid of one of these
	 * once you have it.
	 */
	virtual ~ArpTerminalInterface() { }
	
	// --------------------------------------------------------
	// INTERFACE VERSIONING.
	// --------------------------------------------------------
	
	/* Return the version of the terminal interface that this
	 * object is implementing.  Terminals should define this
	 * to always return TERM_VERSION_CURRENT; Emulators can use
	 * this to check what terminal interface they were given
	 * by comparing the returned value with the TERM_VERSION_*
	 * constants defined below.
	 */

	enum {
		TERM_VERSION_1 = 19970817,	// First version
		
		TERM_VERSION_CURRENT = TERM_VERSION_1
	};
	typedef uint32 TermVersionType;
	
	virtual TermVersionType TermVersion(void) { return TERM_VERSION_CURRENT; }
	
	// --------------------------------------------------------
	// GLOBAL STATE CONTROL.
	// --------------------------------------------------------
	
	/* Reset the terminal.  This sets the terminal's internal
	 * state -- including its mode flags -- to their
	 * default condition.  Setting the 'hard' argument to true
	 * will additionally cause the terminal to clear its screen
	 * and history buffer, and place the cursor at its (0,0)
	 * position.
	 */
	   
	virtual void TermReset(bool hard) = 0;

	/* Set and get the terminal's standard mode flags.  These
	 * flags are defined by the TERM_MODE* constants below; they
	 * are global operatings modes of the terminal.  The initial
	 * state is that all modes are off.
	 */

	enum {
		TERM_MODESMOOTH = 1<<0,    // Smooth scroll display?
								   // 1<<1 OLD -- was TERM_MODENEWLINE
		TERM_MODENOWRAP = 1<<2,    // Wrap at right of screen?
		TERM_MODEINVERSE = 1<<3,   // Inverse video display?
		TERM_MODEHIDECURSOR = 1<<4,// Make the cursor invisible?
		TERM_MODESWAPBSDEL = 1<<5, // Swap backspace and delete keys?
								   // 1<<6 OLD -- was TERM_MODEENTERISLF
	};
	typedef uint32 TermModeType;
	
	virtual void TermSetMode(TermModeType mode) = 0;
	virtual TermModeType TermMode(void) const = 0;

	/* Set and get the characters that cause line feed / carriage
	 * return operations, and the character stream that us sent
	 * when the ENTER key is pressed.
	 */
	 
	virtual void TermSetLFChars(const char* which) = 0;
	virtual const char* TermLFChars() const = 0;
	virtual void TermSetCRChars(const char* which) = 0;
	virtual const char* TermCRChars() const = 0;
	
	virtual void TermSetEnterStream(const char* str) = 0;
	virtual const char* TermEnterStream() const = 0;
	
	// The translation table interface needs to be reworked
	// (or simply removed) for DR9's UTF-8 stuff.
	virtual void TermSetTranslation(const ichar* t) = 0;
	virtual const ichar* TermTranslation(void) const = 0;

	// These functions get and set the character encoding used
	// by the terminal, as per BFont::SetEncoding().  All characters
	// that pass through the terminal interface will be in this
	// encoding.
	// Note that UTF-8 encoding is not yet supported, and changing
	// the encoding will change the display of all characters
	// currently in the terminal.
	virtual void TermSetEncoding(int32 enc) = 0;
	virtual int32 TermEncoding() const = 0;
	
	// Change the terminal window's title.  This is kind-of a
	// quick-and-dirt hack, but hey it works...
	virtual void TermSetTitle(const char* title) = 0;
	
	// --------------------------------------------------------
	// COLOR CONTROL.
	// --------------------------------------------------------
	
	/* Set and get the terminal's rendering colors.  Colors
	 * are described by the standard rgb_color structure; the
	 * colors available for change are defined by the
	 * TERM_COLOR* constants below.
	 */

	enum {
		// These are colors that can applied to each individual
		// character on the display.
		TERM_NORMALCOLOR = 0,
		TERM_COLOR_1 = 1,		// By default, these are ANSI colors
		TERM_COLOR_2 = 2,
		TERM_COLOR_3 = 3,
		TERM_COLOR_4 = 4,
		TERM_COLOR_5 = 5,
		TERM_COLOR_6 = 6,
		TERM_COLOR_7 = 7,
		TERM_COLOR_8 = 8,
		TERM_COLOR_A = 9,		// Extra colors, initially undefined
		TERM_COLOR_B = 10,
		TERM_COLOR_C = 11,
		TERM_COLOR_D = 12,
		TERM_COLOR_E = 13,
		TERM_COLOR_F = 14,
		TERM_COLOR_G = 15,
		
		// These are colors that apply to the entire terminal.
		TERM_HIGHLIGHTCOLOR = -1,
		TERM_CURSORCOLOR = -2,
		
		// This should probably be turned into a function that
		// queries the terminal.
		TERM_NUMTEXTCOLORS = 16,
	};
	typedef int32 TermColorID;

	virtual void TermSetTextBackground(rgb_color bg, TermColorID num) = 0;
	virtual void TermSetTextForeground(rgb_color fg, TermColorID num) = 0;
	virtual rgb_color TermTextBackground(TermColorID num) const = 0;
	virtual rgb_color TermTextForeground(TermColorID num) const = 0;

	// --------------------------------------------------------
	// TERMINAL DIMENSIONS.
	// --------------------------------------------------------
	
	/* Terminal size information.  These functions are used to
	 * find out about various interesting terminal dimensions.
	 * 
	 * Fixed Size: (get/set)  Used to request a specific number
	 * of rows and columns; e.g., a VT100 terminal is normally
	 * fixed at 80 columns and 24 rows.  Setting a fixed
	 * dimension to -1 allows to terminal to automatically adjust
	 * the usable size to fit the screen.
	 * 
	 * View Size: (get only)  The actual number of rows and
	 * columns that are visible in the terminal.  This says
	 * nothing about what you can actually use, but only how
	 * much the user can (currently) see on-screen at any given
	 * time.
	 * 
	 * Size: (get only)  The actual number of usable rows and
	 * columns.  This is either the fixed dimensions, or the
	 * view dimension for any fixed dimension that has been set
	 * to -1.
	 */
	virtual bool TermSetFixedSize(int32 rows, int32 cols) = 0;
	virtual void TermGetFixedSize(int32* rows, int32* cols) const = 0;
	
	virtual void TermGetViewSize(int32* rows, int32* cols) const = 0;
	
	virtual void TermGetSize(int32* rows, int32* cols) const = 0;
	
	/* History size.  This function retrieves the number of rows
	 * currently in history.  NOTE: This is expressed in terminal
	 * coordinates, that is, starting at -1 for one line, -2 for
	 * two lines, etc.
	 */
	virtual int32 TermHistoryTop(void) const = 0;
	
	// --------------------------------------------------------
	// BASIC MANIPULATION.
	// --------------------------------------------------------
	
	/* Cursor position.  Get and set the current position.
	 * cursor coordinates run from [0...RowSize-1] and
	 * [0...ColSize-1], with (0,0) being the top-left character
	 * on the screen.
	 */
	virtual void TermSetCursorPos(int32 row, int32 col) = 0;
	virtual void TermGetCursorPos(int32* row, int32* col) const = 0;

    /* Convert a pixel coordinate in the terminal screen to its
     * character row and column.  This is used, for example, to
     * convert the point that a mouse-down event occurs out to
     * the character location that was hit.
     */
	virtual int32 TermXToCol(float x) const = 0;
	virtual int32 TermYToRow(float y) const = 0;

	/* Region control.  Regions allow you to specify small parts
	 * of the display in which modifications are to take place.
	 * Scrolling, clearing, inserting, etc., only affect the
	 * characters that are within the specified region; all other
	 * characters on the screen are left alone.
	 *
	 * A region contains all of the characters covered by the
	 * specified coordinates.  For example, if a region runs from
	 * row 3 to row 5, performing a LF while the cursor is at
	 * row 5 would cause row 4 to be copied into 3, 5 into 4,
	 * and row 5 cleared.  (And, possibly, row 3 may be copied
	 * into the history buffer.)
	 *
	 * If only a top and bottom position are specified, the region
	 * extends horizontal from column 0 to the rightmost column.
	 */
	virtual void TermSetRegion(void) = 0;
	virtual void TermSetRegion(int32 top, int32 bottom) = 0;
	virtual void TermSetRegion(int32 top, int32 bottom, int32 left, int32 right) = 0;
	virtual void TermGetRegion(int32* top, int32* bottom, int32* left, int32* right) const = 0;

	// --------------------------------------------------------
	// CHARACTER STYLE.
	// --------------------------------------------------------
	
	/* Style flags.  Use with TermSetStyle() and TermStyle(); each
	 * character can have its own style, and that style is taken
	 * from the current style setting when it was written to the
	 * terminal.  Note that the bitfields of the terminal's
	 * internal storage of style is being exposed here...  this
	 * is probably not good, but in the interest of efficiency...
	 */

	enum {
		TERM_STYLEPLAIN      = 0,
		TERM_STYLEINVERSE    = 1<<0,
		TERM_STYLEBOLD       = 1<<1,
		TERM_STYLEITALIC     = 1<<2,
		TERM_STYLEUNDERSCORE = 1<<3,
		TERM_STYLEMASK       = 0xff,

		TERM_STYLECOLOR_MASK = 0xf,
		TERM_STYLEFOREGROUND_POS = 8,
		TERM_STYLEBACKGROUND_POS = 12,
		
		// A special flag, selecting whatever the current
		// style is.  This is used as a default value for
		// later functions.
		TERM_STYLECURRENT    = 1<<7,
	};
	typedef uint16 style_t;
	
	/* Get and set the current character style and colors.
	 * Note that TermGetStyle() and TermStyle() operate with
	 * style_t types, but only the lower 8 bits of flags
	 * should be used.
	 */
	virtual void TermSetStyle(int32 style) = 0;
	virtual void TermSetForeColor(TermColorID col) = 0;
	virtual void TermSetBackColor(TermColorID col) = 0;

	virtual int32 TermStyle(void) const = 0;
	virtual TermColorID TermForeColor(void) const = 0;
	virtual TermColorID TermBackColor(void) const = 0;

	// --------------------------------------------------------
	// CHARACTER ACCESS.
	// --------------------------------------------------------
	
	/* These functions provide low-level access to the characters
	 * (and their related style information) that are actually
	 * on the terminal screen.
	 */
	virtual void TermSetChar(ichar c) = 0;
	virtual void TermSetChar(ichar c, int32 row, int32 col) = 0;

	// XXX The 'Get' should be removed from these function names!
	virtual ichar TermGetChar(void) const = 0;
	virtual ichar TermGetChar(int32 row, int32 col) const = 0;
	virtual style_t TermGetCharStyle() const = 0;
	virtual style_t TermGetCharStyle(int32 row, int32 col) const = 0;

	// --------------------------------------------------------
	// TEXT RANGE MANIPULATION.
	// --------------------------------------------------------
	
	/* Unlike all other methods, these allow you to access data
	 * in the terminal's history: data is located at negative
	 * row values, i.e. -1 is the first row of data history above
	 * the actual terminal display.
	 *
	 * In all cases, the row/column coordinates are swapped to
	 * make sure they are correctly ordered from start position
	 * to end position.
	 *
	 * The highlighting modes defined below specify constraints
	 * on the region that is passed into TermSetHighlight():
	 *
	 * TERM_HIGHLIGHTOFF turns off all highlighting, ignoring
	 * the range that is given.
	 *
	 * TERM_HIGHLIGHTCHAR highlights the exact given range.
	 *
	 * TERM_HIGHLIGHTWORD constraints the range to fall on
	 * word boundaries; the highlight start column is moved
	 * left and the end column is moved right to accomplish this.
	 *
	 * TERM_HIGHLIGHTLINE forces the start column to the far
	 * left and the end column to the far right.
	 */
	
	typedef enum {
		TERM_HIGHLIGHTOFF  = 0,
		TERM_HIGHLIGHTCHAR = 1,
		TERM_HIGHLIGHTWORD = 2,
		TERM_HIGHLIGHTLINE = 3
	};
	typedef uint32 TermHighlightType;
	
	// Set the highlight region to display.
	virtual void TermSetHighlight(TermHighlightType type,
									int32 row1, int32 col1,
									int32 row2, int32 col2) = 0;
	// Return true if the given position is within the currently
	// displayed highlight region.
	virtual bool TermInHighlight(int32 row, int32 col) const = 0;
	// Return the currently displayed highlight region.
	virtual void TermGetHighlight(TermHighlightType* type,
									int32* row1, int32* col1,
									int32* row2, int32* col2) const = 0;
	
	/* Retrieve a range of text from the terminal.  This function
	 * copies the terminal text (and possibly style information)
	 * into a block of memory.  You should call it first with
	 * 'text' and 'style' NULL and 'array_size' pointing to an
	 * int32 with a value of 0, to retrieve the number of
	 * characters that will be returned.  Then allocate character
	 * style_t arrays of the required length, and call again
	 * with 'array_size' pointing to the size of the array you
	 * allocated and 'text' and 'style' now pointing to the
	 * allocated buffers.
	 */
	virtual void TermGetTextRange(int32 row1, int32 col1,
									int32 row2, int32 col2,
									size_t* array_size,
									ichar* text, style_t* style) const = 0;
									
	// --------------------------------------------------------
	// HIGH-LEVEL MANIPULATION.
	// --------------------------------------------------------
	
	/* Vertically scroll a terminal region up or down.
	 * 'amount' specifies the number of rows to move; if negative,
	 * they are moved up, else they are moved down.  If the top
	 * and bottom rows are not specified, the current region is
	 * used.
	 *
	 * NOTE: Horizontal regions smaller than the terminal width
	 * are not currently respected when scrolling.
	 */
	virtual void TermScrollRegion(int32 top, int32 bottom, int32 amount) = 0;
	virtual void TermScroll(int32 amount) = 0;

	/* Set/Delete/Insert a range of characters.  The row where
	 * the character is located is the one that is affected.
	 * 'fillc' is the character code to fill empty spaces with
	 * and 'fillstyle' is the style to use; by default the fill
	 * is a space character in the current style.
	 *
	 * TermSetChars() simply replaces the characters from 'left'
	 * to 'right' with the fill character.
	 *
	 * TermDeleteChars() removes the characters from 'left' to
	 * 'right', moving the characters after 'right' over to
	 * fill the vacated space.  Characters after the current
	 * right region are not affected; characters removed from
	 * the far right are replaced with the fill character.
	 *
	 * TermInsertChars() copies the character from 'left' to
	 * 'right' over, so that the character at 'left' now
	 * occupies the position at ('right'+1).  The vacated spaces
	 * are replaced with the fill character, and characters after
	 * the current right region are not affected.
	 */
	virtual void TermSetChars(int32 left, int32 right, ichar fillc=' ',
					  style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermDeleteChars(int32 left, int32 right, ichar fillc=' ',
						style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermInsertChars(int32 left, int32 right, ichar fillc=' ',
						 style_t fillstyle=TERM_STYLECURRENT) = 0;

    /* Clear areas of the terminal screen.  The affected
     * characters are replaced with the selected fill character
     * and style, as described above.  In all cases, characters
     * outside of the current region are not affected.
     *
     * TermClearEOL() clears all characters from the current
     * cursor position to the right region on its line.
     *
     * TermClearBOL() clears all characters from the current
     * cursor position to the left region its line.
     *
     * TermClearLine() clears all characters in the current
     * cursor position's line, from the left to right region.
     *
     * TermClearEOD() clears all characters from the current
     * cursor position to the end of the display; that is, to
     * (RegionRight,RegionLeft).
     *
     * TermClearBOD() clears all characters from the current
     * cursor position to the beginning of the display; that
     * is, to (RegionLeft,RegionTop).
     *
     * TermClear() clears all characters in the current region.
     */
	virtual void TermClearEOL(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermClearBOL(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermClearLine(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermClearEOD(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermClearBOD(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT) = 0;
	virtual void TermClear(ichar fillc=' ', style_t fillstyle=TERM_STYLECURRENT) = 0;

	// --------------------------------------------------------
	// SCREEN REFRESHING.
	// --------------------------------------------------------
	
	/* The terminal display is not redrawn until explicitly
	 * told to do so.  This is done to improve efficiency:
	 * this way, you can perform a large number of simple
	 * operations on the terminal to render what you want, then
	 * the terminal can batch up its screen drawing operations
	 * to display your changes as fast as possible.
	 *
	 * TermRedraw() draws particular lines (or the entire
	 * display), whether or not they have actually changed.
	 * If 'full' is false, it will only draw the regions that
	 * are actually visible to the user.
	 *
	 * TermClean() is the more important function, which
	 * analyzes the changes made to the terminal display since
	 * it was last redrawn, and performs the low-level drawing
	 * operations needed to synchronize what the user sees with
	 * what actually exists in the terminal.  You must call this
	 * to show any changes caused by the previously described
	 * terminal functions.
	 */
	virtual void TermRedraw(int32 top, int32 bot, bool full) = 0;
	virtual void TermRedraw(void) = 0;
	virtual void TermClean(void) = 0;

	// --------------------------------------------------------
	// CHARACTER STREAMS.
	// --------------------------------------------------------
	
	/* These methods implement the RAW character-steam I/O
	 * interface with the terminal.  They are used by emulators
	 * to send raw text to the terminal display (TermSendTTY) or
	 * the remote deveice it is attached to (TermSendRemote).
	 * 
	 * If you are not using an emulator with the terminal, you
	 * may call these yourself to communicate through the raw
	 * data stream.  Otherwise, you probably want to use the
	 * emulator functions EmulateToTTY() and EmulateToRemote(),
	 * which implement the parsing and generation of appropriate
	 * control characters.
	 *
	 * TermSendTTY() allows two optional flags:
	 *
	 * TERM_OUTRAW tells the function to write the raw characters
	 * to the display, without interpreting them in any way.  If
	 * this is not set, the terminal will parse standard ASCII
	 * to perform some basic formatting functionality.  The codes
	 * it understands are:
	 *
	 *	ANSI_NUL:	Ignored
	 *	ANSI_BEL:	Ring the bell (e.g., call beep())
	 *	ANSI_BS:	Move the cursor position left by one column
	 *	ANSI_HT:	Move cursor to next 8-column tabstop
	 *	ANSI_LF:	Move cursor down one row; if TERM_MODENEWLINE
	 *				is on, also move to left of screen
	 *	ANSI_CR:	Move cursor to left of screen
	 *	ANSI_FF:	Clear screen
	 *	ANSI_DEL:	Delete character at cursor
	 *
	 * TERM_OUTPARTIAL tells the terminal that this is not a
	 * complete character stream.  By default the terminal will
	 * call TermClean() to update the display after it has
	 * placed the character stream in the display.  If this
	 * flag is set, TermClean() will not be called and you must
	 * do this yourself.
	 */
	
	enum {
		TERM_OUTRAW     = 1<<0,  // Don't interpret \n \b etc.?
		TERM_OUTPARTIAL = 1<<1,  // Don't update screen?
	};
	typedef uint32 TermOutputType;
	
	virtual void TermSendTTY(const ichar * d, size_t len, uint32 flags=0) = 0;
	virtual void TermSendRemote(const ichar * d, size_t len) = 0;
};

#endif
