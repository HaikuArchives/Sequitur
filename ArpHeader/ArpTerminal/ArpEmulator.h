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
 * ArpEmulator.h
 *
 * The ArpEmulator class is a base-class that defines a uniform
 * terminal emulator interface.  Actual emulator implementations
 * (e.g., VT100, ANSI, etc.) will inherit from this class and
 * override certain functions to perform their emulation.
 *
 * This class can also be used by itself, in which case it operates
 * as the prototypical dumb terminal.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 0.1: Created this file from the WebTerm source.
 *
 */

#ifndef ARPTERMINAL_ARPEMULATOR_H
#define ARPTERMINAL_ARPEMULATOR_H

#ifndef ARPTERMINAL_ARPTERMINALINTERFACE_H
#include <ArpTerminal/ArpTerminalInterface.h>
#endif

#ifndef ARPTERMINAL_ARPEMULATORADDON_H
#include <ArpTerminal/ArpEmulatorAddon.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef _LOCKER_H
#include <support/Locker.h>
#endif

#ifndef _POINT_H
#include <interface/Point.h>
#endif

class ArpEmulator : public ArpEmulatorInterface {
private:
	typedef ArpEmulatorInterface inherited;

public:

	// Constructor and destructor.  The ArpEmulator must be
	// attached to a terminal for its entire lifetime.
	ArpEmulator(ArpTerminalInterface& myterm);
	virtual ~ArpEmulator();

	static ArpEmulatorAddon AddonInterface;
	
	virtual status_t GetConfiguration(BMessage* values) const	{ return B_OK; }
	virtual status_t PutConfiguration(const BMessage* values)	{ return B_OK; }
	virtual status_t Configure(ArpVectorI<BView*>& views)		{ return B_OK; }
	
	virtual void AttachTerminal(ArpTerminalInterface& terminal);
	
	// Reset all emulation state to its start-up settings, which
	// should be some sane user-friendly values.  Subclasses that
	// override this must call the superclass's function, which
	// will also make sure the terminal is reset.
	virtual void Reset(bool hard);

	virtual const ArpEmulationType* EmulationType(void) const;
	
	/* ------------------------------------------------------------
	   FUNCTIONS FOR MOVING DATA THROUGH THE EMULATION
	   ------------------------------------------------------------ */

	// Transform a character stream into the terminal operations
	// needed to display the text to the user.  By default, these
	// let your emulator handle the data stream by calling
	// EmuRemoteNextSpecial() and EmuRemoteChar(); you should
	// very rarely need to override these two functions yourself.
	virtual void EmulateToTTY(const ichar* d, size_t len);

	// Transform user input to a character stream, which will
	// be sent to the remote device that is attached to this
	// terminal.  The first two normally send the text passed
	// in directly to the terminal's TermSendRemote() function;
	// you probably won't ordinarily need to override them,
	// but you can if you want.  The last one sends the message
	// off to EmuTTYMessage(), which dispatches it appropriately.
	virtual void EmulateToRemote(const ichar* d, size_t len);
	bool EmulateToRemote(BMessage* msg);

	// A couple convenience functions
	void EmulateToTTY(const ArpString& str)
		{ EmulateToTTY((const ichar*)str,str.Length()); }
	void EmulateToRemote(const ArpString& str)
		{ EmulateToRemote((const ichar*)str,str.Length()); }

	/* ------------------------------------------------------------
	   HELPER FUNCTIONS FOR DISPLAYING STRINGS
	   ------------------------------------------------------------ */

	static ArpString charToString(ichar c);
	static ArpString sequenceToString(const ArpString& seq);

protected:

	/* ------------------------------------------------------------
	   EMULATION IMPLEMENTOR INTERFACE
	   ------------------------------------------------------------ */

	enum {
		KEY_CURSOR_UP = 0x10000,
		KEY_CURSOR_DOWN,
		KEY_CURSOR_LEFT,
		KEY_CURSOR_RIGHT,
		KEY_HOME,
		KEY_END,
		KEY_PAGE_UP,
		KEY_PAGE_DOWN,
		KEY_INSERT,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,
		KEY_PRINT,
		KEY_PAUSE,
		KEY_SCROLL,
		KEY_BREAK,
		KEY_SYSREQ,
		KEY_RETURN,    // alpha-numeric LF key
		KEY_ENTER,     // number pad LF key
		KEY_DELETE,
		KEY_BACKSPACE,
		KEY_TAB,
		KEY_SPACE,
		KEY_ESCAPE,
	} KeyCode;
	
	// Physical character codes
	enum {
		CTRL_A = (ichar)1,
		CTRL_B = (ichar)2,
		CTRL_C = (ichar)3,
		CTRL_D = (ichar)4,
		CTRL_E = (ichar)5,
		CTRL_F = (ichar)6,
		CTRL_G = (ichar)7,
		CTRL_H = (ichar)8,
		CTRL_I = (ichar)9,
		CTRL_J = (ichar)10,
		CTRL_K = (ichar)11,
		CTRL_L = (ichar)12,
		CTRL_M = (ichar)13,
		CTRL_N = (ichar)14,
		CTRL_O = (ichar)15,
		CTRL_P = (ichar)16,
		CTRL_Q = (ichar)17,
		CTRL_R = (ichar)18,
		CTRL_S = (ichar)19,
		CTRL_T = (ichar)20,
		CTRL_U = (ichar)21,
		CTRL_V = (ichar)22,
		CTRL_W = (ichar)23,
		CTRL_X = (ichar)24,
		CTRL_Y = (ichar)25,
		CTRL_Z = (ichar)26,
	};

	// Logical character codes
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

	/* ------------------------------------------------------------
	   VIRTUAL FUNCTIONS THAT IMPLEMENT TTY -> REMOTE EMULATION
	   ------------------------------------------------------------ */

	/* Override these methods to handle data going from the terminal
	   TTY (i.e., the user's input) to the remote device.  These
	   appear as low-level messages as they arrived at the terminal.
	   The return value indicates whether the given message has
	   been used by the emulator -- return TRUE if it has.  This
	   tells the terminal not to use the message itself, for
	   example not to start highlighting from a button press.
	   Any messages you don't use MUST be handed off to the
	   superclass. */

	// This is the top-level message entry point; all UI messages
	// originally start here, and this dispatches them to the
	// following functions.
	virtual bool EmuTTYMessage(BMessage* msg);

	// These are entry points for specific messages.
	virtual bool EmuTTYKeyPress(BMessage* msg, int32 key,
									int32 mods);
	virtual bool EmuTTYMouseDown(BMessage* msg, BPoint pos,
									int32 buttons);
	virtual bool EmuTTYMouseMove(BMessage* msg, BPoint pos,
									int32 buttons);
	virtual bool EmuTTYMouseUp(BMessage* msg, BPoint pos,
									int32 buttons);
	virtual bool EmuTTYResize(int32 cols, int32 rows);

	/* ------------------------------------------------------------
	   VIRTUAL FUNCTIONS THAT IMPLEMENT REMOTE -> TTY EMULATION
	   ------------------------------------------------------------ */

	// Override this method to implement a particular emulator.
	// Return true to pass the given character on to the terminal
	// untouched, or false if doing your own processing of it.
	virtual bool EmuRemoteChar(ichar c);

	// This method is called by the root emulator to check if its
	// subclass needs to do its own processing on a character -- it
	// should return the offset of the next character it needs to
	// process itself; all characters up to that will be sent
	// directly to the terminal.  If there are no special
	// characters in the string, return the position past the
	// last character, i.e. (str+len).
	// While it is not strictly necessary, overriding this method
	// can dramatically increase the performance of the emulation.
	virtual const ichar* EmuRemoteNextSpecial(const ichar* str,
												size_t len);
	
	/* ------------------------------------------------------------
	   ALL THE STATE WE DEFINE
	   ------------------------------------------------------------ */

	ArpTerminalInterface* terminal;

private:
	enum { KEYMAP_SIZE = 0x80 };
	static const long keymap[KEYMAP_SIZE];

	static const char* codes_00_20[];
	static const char* codes_7f_9f[];
};

#endif
