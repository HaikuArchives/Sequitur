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
 * ArpEmulatorInterface.h
 *
 * The ArpEmulator class defines an abstract interface to a
 * terminal emulation.  It is used by the ArpTerminal class
 * to transform incoming text/control characters into their
 * appropriate terminal operations, and transform user input
 * into a proper outgoing text/control character stream.
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
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 7/19/97:
 *	Created file from ArpEmulator.h
 *
 */

#ifndef ARPTERMINAL_ARPEMULATORINTERFACE_H
#define ARPTERMINAL_ARPEMULATORINTERFACE_H

#ifndef ARPTERMINAL_ARPTERMINALINTERFACE_H
#include <ArpTerminal/ArpTerminalInterface.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef	_ARCHIVABLE_H
#include <be/support/Archivable.h>
#endif

struct ArpEmulationType {
	int32 StructLength;			// sizeof(ArpEmulationType)
	const char* Name;
	const char* LongName;
	const char* Synonym;
};

class ArpEmulatorInterface : public ArpConfigurableI {

public:

	/* Virtual destructor, so that you can get rid of one of these
	 * once you have it.
	 */
	virtual ~ArpEmulatorInterface() { }
	
	// --------------------------------------------------------
	// INTERFACE VERSIONING.
	// --------------------------------------------------------
	
	/* Return the version of the remote interface that this
	 * object is implementing.  Devices should define this
	 * to always return REMOTE_VERSION_CURRENT.
	 */

	enum {
		EMULATOR_VERSION_1 = 19980826,	// First version
		
		EMULATOR_VERSION_CURRENT = EMULATOR_VERSION_1
	};
	typedef uint32 EmulatorVersionType;
	
	virtual EmulatorVersionType EmulatorVersion(void)
		{ return EMULATOR_VERSION_CURRENT; }
	
	virtual void AttachTerminal(ArpTerminalInterface& terminal) = 0;
	
	/* ------------------------------------------------------------
	   GLOBAL CONTROL METHODS
	   ------------------------------------------------------------ */

	// Reset all emulation state to its start-up settings, which
	// should be some sane user-friendly values.  The emulator
	// should also call the terminal's TermReset() method as part
	// of this operation.
	virtual void Reset(bool hard) = 0;

	// Retrieve the type information about this emulator object.
	virtual const ArpEmulationType* EmulationType(void) const = 0;
	
	/* ------------------------------------------------------------
	   FUNCTIONS FOR MOVING DATA THROUGH THE EMULATION
	   ------------------------------------------------------------ */

	// Transform a character stream into the terminal operations
	// needed to display the text to the user.  The emulator
	// displays the final text to the user through the terminal's
	// TermSendTTY() and other functions.
	virtual void EmulateToTTY(const ichar* d, size_t len) = 0;

	// Transform user input to a character stream, which should
	// be sent to the remote device that is attached to this
	// terminal.  The emulator sends the final character stream
	// to the remote device through the terminal's
	// TermSendRemote() function.  The second function's return
	// value indicates whether the terminal actually did something
	// with the message -- if you return false, the terminal will
	// be able to do its own processing on the message, for
	// example use a mouse down to start highlighting.
	virtual void EmulateToRemote(const ichar* d, size_t len) = 0;
	virtual bool EmulateToRemote(BMessage* msg) = 0;

};

#endif
