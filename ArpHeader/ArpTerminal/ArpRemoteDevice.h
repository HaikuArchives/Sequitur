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
 * ArpRemoteInterface.h
 *
 * This class implements the generic foundation for the device-side
 * of the ArpTerminal's BMessage protocol.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• This doesn't yet work.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 8/26/1998:
 *	•	Created from the ArpTelnet source.
 *
 */

#ifndef ARPTERMINAL_ARPREMOTEDEVICE_H
#define ARPTERMINAL_ARPREMOTEDEVICE_H

#ifndef ARPTERMINAL_ARPREMOTEINTERFACE_H
#include <ArpTerminal/ArpRemoteInterface.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

class ArpRemoteDevice : public BLooper, public ArpRemoteInterface {
private:
	typedef BLooper inherited;

public:

	/* ------------------------------------------------------------
	   CONTRUCTORS AND GLOBAL CONTROL METHODS
	   ------------------------------------------------------------ */

	ArpRemoteDevice(const BMessenger& term);
	ArpRemoteDevice();
	virtual ~ArpRemoteInterface();
	
	/* ------------------------------------------------------------
	   IMPLEMENT THE ARP-REMOTE-INTERFACE
	   ------------------------------------------------------------ */

	virtual BMessenger GetDevice(void) { return BMessenger(this); }
	virtual void SetTerminal(const BMessenger& term) { terminal = term; }
	virtual const BMessenger& GetTerminal(void) { return terminal; }
	virtual status_t Connect();
	virtual status_t Connect(const char* url);
	virtual void Disconnect();
	
protected:

	// NOTE: None of the protected methods lock the BLooper;
	// you must do so yourself, if needed.
	
	/* ------------------------------------------------------------
	   MESSAGE HANDLING
	   ------------------------------------------------------------ */

	void initialize();
	
	void MessageReceived(BMessage* message);
	
	// Send received text to terminal
	void receive(const char* str, int32 len=-1);
	
	void changeEmulator(const char* name);
	
	// Prompts aren't implemented.
	void clearPrompts();

	/* ------------------------------------------------------------
	   DATA SENDING METHODS
	   ------------------------------------------------------------ */

	void setWindowSize(int32 width, int32 height);
	void setEmulator(const char* name);
	void write(char c);
	void write(const char* b, int32 len);
	void write(const ArpString& str);
	
	/* ------------------------------------------------------------
	   INPUT THREAD METHODS
	   ------------------------------------------------------------ */

	/* Entry point and main loop.
	   Note well: we must be careful that any methods that read
	   from the stream (either directly with in.read(), or indirectly
	   with next_char()) do not have the looper locked.  Otherwise,
	   they may block and leave other threads unable to access the
	   object and write more data to the socket. */

	static int32 readThreadEntry(void* arg);
	int32 startReader(void);
	int32 runReader(void);
	
	/* Input instance data.  This may only be accessed by the input
	   thread. */

	/* ------------------------------------------------------------
	   MISC SUPPORT METHODS
	   ------------------------------------------------------------ */

	/* Parse escapes in a parameter string. */
	ArpString parseString(const ArpString& in);

	/* ------------------------------------------------------------
	   INSTANCE DATA SECTION
	   ------------------------------------------------------------ */

	BMessenger terminal;
	
	/* Common instance data, used by all threads.  Methods that
	   access it must be protected by locking this locker.
	   NOTE: To avoid deadlocks, you -must- -not- lock the
	   BLooper after locking the 'globals' semaphore.
	*/

	BLocker globals;
	
	int ptyfd;                       // Communication with shell
	
	bool doEcho;                     // Echo typed characters back to terminal?
	
	thread_id readThread;            // Thread for reading socket.
	status_t socketError;            // Last socket read error.
	thread_id shellThread;           // Thread that is running shell.
	bool connected;                  // TRUE if reader is active.

	BMessage emuTypes;               // Available emulation types
	int32 curEmuType;
	ArpString curEmuName;
	
	int32 curWidth, curHeight;       // Current window dimensions.
};

#endif
