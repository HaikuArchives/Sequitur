/*
 * Copyright (c)1997-98 by Dianne Hackborn.
 * All rights reserved.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpTelnet.h
 *
 * This class implements a complete TELNET client.  It is
 * designed as a remote device that can be attached to an
 * ArpRemoteTerminal, and automagically understand about
 * terminal sizes, emulation types, etc., etc.
 *
 * WARNING:
 * This is a quick one day hack port from Java, so beware.
 * It's ugly, undocumented, and has many major broken pieces.
 * But it does work, to some degree...
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• There are surely lots and lots of bugs that aren't
 *	  known...  but soon will be.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 8/16/1996:
 *	•	Disconnect() is now much more robust: it keeps nudging
 *		the reader thread until it goes away, and can disconnect
 *		during the initial host lookup and socket connection.
 *
 * 7/26/1997:
 *	•	Fixed Disconnect() bug.
 *
 * 7/24/1997:
 *	• Created this file from the WebTerm source.
 *
 */

#pragma once

#ifndef ARPNETWORK_ARPTELNET_H
#define ARPNETWORK_ARPTELNET_H

#ifndef _LOCKER_H
#include <Locker.h>
#endif

#ifndef _LOOPER_H
#include <Looper.h>
#endif

#ifndef _MESSENGER_H
#include <Messenger.h>
#endif

#ifndef ARPTERMINAL_ARPREMOTEADDON_H
#include <ArpTerminal/ArpRemoteAddon.h>
#endif

#ifndef ARPTERMINAL_ARPRTERMINALMSG_H
#include <ArpTerminal/ArpTerminalMsg.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef ARPNETWORK_ARPHOSTNAME_H
#include <ArpNetwork/ArpHostName.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREWATCH_H
#include <ArpKernel/ArpConfigureWatch.h>
#endif

#include <ArpNetwork/BTSSocket.h>

class PromptInfo;

enum {
	ARPTELNET_CONNECT_MSG = TERM_CONNECT_MSG,
	ARPTELNET_DISCONNECT_MSG = TERM_DISCONNECT_MSG,
	
	// These aren't implemented.
	ARPTELNET_LOOKUPHOST_MSG = 'lhst',
	ARPTELNET_WAITREPLY_MSG = 'wrpy',
};

class ArpTelnet : public BLooper, public ArpRemoteInterface {
private:
	typedef BLooper inherited;

public:

	static const ArpRemoteAddon AddonInterface;
	
	/* ------------------------------------------------------------
	   CONTRUCTORS AND GLOBAL CONTROL METHODS
	   ------------------------------------------------------------ */

	ArpTelnet(const BMessenger& term, const char* url);
	ArpTelnet(const char* url = NULL);
	
	virtual void Delete() {
		Lock();
		Quit();
	}
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& views);
	
	virtual const ArpRemoteType* RemoteType(void) const;
	
	virtual BMessenger GetDevice(void) { return BMessenger(this); }
	
	// You don't actually need to call these, as the terminal
	// will handshake with its remote device when the device
	// is attached to it.
	void SetTerminal(const BMessenger& term) { terminal = term; }
	BMessenger GetTerminal(void) { return terminal; }
	
	const char* Host();
	int Port();

	// Note: these functions return immediately; the object will
	// still be in the process of connecting or disconnecting when
	// control returns to the caller.
	status_t Connect(BWindow* owner = NULL);
	status_t Connect(const char* url, BWindow* owner = NULL);
	void Disconnect();

	virtual bool IsConnected() const		{ return readThread >= 0; }
	virtual bool IsInUse() const 			{ return mInUse; }
	
	/* ------------------------------------------------------------
	   DATA IN CONFIGURATION MESSAGES.
	   ------------------------------------------------------------ */

	static const char* HostConfigName;
	static const char* PortConfigName;
	
	/* ------------------------------------------------------------
	   CONSTANT VALUES SECTION
	   ------------------------------------------------------------ */

	/* TELNET protocol commands.  A command consists of a 0xFF byte
	   followed by one of the following codes. */

	enum {
		IAC		= 255,	/* Interpret next byte as a command;
						   IAC IAC is a 255 data byte. */

		// The following five are followed by a byte indicating the option.

		DONT	= 254,	/* Ask the remote system to stop
						   performing an option, or confirm
						   we no longer expect it to perform
						   the option. */
		DO		= 253,	/* Ask the remote system to start
						   performing an option, or confirm
						   we now expect it to perform
						   the option. */
		WONT	= 252,	/* Refuse to perform or continue
						   performing an option. */
		WILL	= 251,	/* Indicate desire to begin performing
						   an option or confirm that it is
						   now being performed. */
		SB		= 250, 	/* Perform subnegotiation of the option. */

		GA		= 249,	/* Go Ahead */
		EL		= 248,	/* Erase Line */
		EC		= 247,	/* Erase Character */
		AYT		= 246,	/* Are You There */
		AO		= 245,	/* Abort Output */
		IP		= 244,	/* Interrupt Process */
		BREAK	= 243,	/* Break */
		DM		= 242,	/* Data Mark:
						   Data stream portion of Sync. */
		NOP		= 241,	/* No Operation */
		SE		= 240,	/* End of subnegotiation */
		EOR		= 239,
		ABORT	= 238,
		SUSP	= 237,
		xEOF	= 236,

		SYNC	= 242,
	};
	
	enum {
		TELCMD_FIRST	= xEOF,
		TELCMD_LAST		= IAC,
		TELCMD_NUM		= TELCMD_LAST-TELCMD_FIRST+1,
	};
	
	/* TELNET protocol options.  Most of these are not implementented;
	   the commented ones are implemented.

	   XXX We really need to be better about respecting things like
	   BINARY, LINEMODE, etc. */

	enum {
		TELOPT_BINARY         =  0,

		// We ask for the remote to do so, but will echo back
		// to the terminal if the remote won't.
		TELOPT_ECHO           =  1,

		TELOPT_RCP            =  2,

		// Suppress Go Ahead: we ask for the remote to do so.
		TELOPT_SGA            =  3,

		TELOPT_NAMS           =  4,
		TELOPT_STATUS         =  5,
		TELOPT_TM             =  6,
		TELOPT_RCTE           =  7,
		TELOPT_NAOL           =  8,
		TELOPT_NAOP           =  9,
		TELOPT_NAOCRD         = 10,
		TELOPT_NAOHTS         = 11,
		
		// Negotiate about output horiontal tab disposition:
		// we ask the remote to let us take care of tabs.
		TELOPT_NAOHTD         = 12,
		
		TELOPT_NAOFFD         = 13,
		TELOPT_NAOVTS         = 14,
		TELOPT_NAOVTD         = 15,
		TELOPT_NAOLFD         = 16,
		TELOPT_XASCII         = 17,
		TELOPT_LOGOUT         = 18,
		TELOPT_BM             = 19,
		TELOPT_DET            = 20,
		TELOPT_SUPDUP         = 21,
		TELOPT_SUPDUPOUTPUT   = 22,
		TELOPT_SNDLOC         = 23,

		// Negotiate terminal type: we negotiate using the
		// available emulators the terminal gives us.
		TELOPT_TTYPE          = 24,

		TELOPT_EOR            = 25,
		TELOPT_TUID           = 26,
		TELOPT_OUTMRK         = 27,
		TELOPT_TTYLOC         = 28,
		TELOPT_3270REGIME     = 29,
		TELOPT_X3PAD          = 30,

		// Negotiate about window size.
		TELOPT_NAWS           = 31,

		// Connection speed: we just tell the remote we are
		// running at 52,000 in and 52,000 out bps.
		TELOPT_TSPEED         = 32,
		
		TELOPT_LFLOW          = 33,
		TELOPT_LINEMODE       = 34,
		TELOPT_XDISPLOC       = 35,
		TELOPT_OLD_ENVIRON    = 36,
		TELOPT_AUTHENTICATION = 37,
		TELOPT_ENCRYPT        = 38,
		TELOPT_NEW_ENVIRON    = 39,
		TELOPT_EXOPL         = 255,

		TELOPT_FIRST = TELOPT_BINARY,
		TELOPT_LAST  = TELOPT_NEW_ENVIRON,
		TELOPT_NUM   = TELOPT_LAST-TELOPT_FIRST+1,
	};
	
protected:

	// NOTE: None of the protected methods lock the BLooper;
	// you must do so yourself, if needed.
	
	/* ------------------------------------------------------------
	   CONSTANT DATA SECTION
	   ------------------------------------------------------------ */

	// ASCII names of the telnet command codes.
	static const char* telcmds[TELCMD_NUM+1];
	
	// ASCII names of the telnet options.
	static const char* telopts[TELOPT_NUM+1];

	// The local options that we have implemented
	static const bool locopts_impl[TELOPT_NUM+1];

	// The remote options that we have implemented
	static const bool remopts_impl[TELOPT_NUM+1];

	/* ------------------------------------------------------------
	   MESSAGE HANDLING
	   ------------------------------------------------------------ */

	void initialize(const char* url);
	
	void MessageReceived(BMessage* message);
	
	// Send received text to terminal
	void receive(const char* str, int32 len=-1);
	void receive(ArpString str) { receive((const char*)str, str.Length()); }
	
	void changeEmulator(const char* name);
	
	// Prompts aren't implemented.
	void clearPrompts();

	status_t DoConnect(const char* host = NULL, int32 port = -1);
	void DoDisconnect();
	
	void CloseConnectWin();
	
	virtual ~ArpTelnet();
	
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

    /* The methods in this section are the only ones that run without
       having 'globals' locked.  They need to do this because they will
       block while reading the socket -- if the globals were locked
       during this, our entire class would hang. */
    
	/* Entry point and main loop. */
	
	static int32 readThreadEntry(void* arg);
	int32 startSession(void);
	int32 startReader(void);
	int32 runReader(void);

	/* Process some data in the input buffer.  Returns the next position
	   in the buffer. */
	   
	const uint8* process_buffer(const uint8* buffer, size_t len);
	
	enum InputState {
		InputData = -1,		// Reading normal data.
		InputIAC = IAC,		// Read an IAC -- next character is a command.
		
		InputDONT = DONT,	// Read a DONT command -- next char is option.
		InputDO = DO,		// Read a DO command -- next char is option.
		InputWILL = WILL,	// Read a WILL command -- next char is option.
		InputWONT = WONT,	// Read a WONT command -- next char is option.
		
		InputSB = SB,		// Reading a subnegotiation request code.
		InputSBData = -2,	// Reading subnegotiation data.
		InputSBIAC = -3		// Reading IAC sequence in subnegotiation data.
	};
	
	/* Input instance data.  These may only be accessed by the input
	   thread. */

	InputState mInputState;      // Current reader state
	int32 mInSBCode;             // Subnegotiation code that has been received
	ArpString mInSBData;         // Subnegotiation data that has been received
	
	/* ------------------------------------------------------------
	   CHARACTER DATA INPUT METHODS
	   ------------------------------------------------------------ */

	/* Methods for handling standard data.  These lock the globals
	   because they call handlePrompts(), which accesses the common
	   doPrompts and prompts variables. */

	void process_data(uint8 c);
	void process_data(const uint8* c, size_t len);

	/* Change state of all prompts with the next character.  Methods that
	   call this must have locked the globals, because this method manipulates
	   shared variables. */

	void handlePrompts(uint8 c);

	/* ------------------------------------------------------------
	   TELNET OPTION CONTROL METHODS
	   ------------------------------------------------------------ */

	/* Send an option command to the remote server. */
	void send_opt(int32 cmd, int32 opt, bool force);

	/* Take action on a process comment received from the server. */
	void process_opt(int32 cmd, int32 opt);

	/* Process a subnegotiation sequence. */
	void process_sb(int32 opt, ArpString data);

	/* Used for constructing messages. */
	int32 put_byte(uint8* b, int32 pos, uint8 val);

	/* Send a window size negotiation. */
	void send_naws(int width, int height);

	/* Send a terminal type negotiation. */
	void send_ttype();

	/* Negotiate output horizontal tab disposition: let us do it all. */
	void send_naohtd();

	/* Negotiate about terminal speed: fast as possible! */
	void send_tspeed();

	/* ------------------------------------------------------------
	   MISC SUPPORT METHODS
	   ------------------------------------------------------------ */

	/* Parse escapes in a parameter string. */
	ArpString parseString(const ArpString& in);

	void print_cmd(const ArpString& label, int32 cmd);

	void print_opt(const ArpString& label, int32 opt);

	bool telcmd_ok(int32 cmd);
	bool locopt_ok(int32 opt);
	bool remopt_ok(int32 opt);

	ArpString telcmd(int32 cmd);
	ArpString telopt(int32 opt);
	
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
	
	ArpConfigureWatch mWatch;		// Standard configurable object implementation
	
	BMessenger cnwin;				// The currently open connect window.
	ArpString hostName;
	int32 hostPort;
	ArpHostName host;				// Host address to connect with.
	BTSSocket sock;					// Network socket.
	bool connected;					// TRUE if socket is active.
	bool mInUse;					// TRUE if user has done something.
	thread_id readThread;			// Thread for reading socket.
	status_t socketError;			// Last socket read error.
	bool mDisconnectRequested;		// TRUE if disconnect was requested by user.
	
	bool isTelnet;					// An actual TELNET session?
	bool delayedTTYPE;				// Didn't have data for TTYPE?
	
	BMessage emuTypes;				// Available emulation types
	int32 curEmuType;
	ArpString curEmuName;
	
	int32 curWidth, curHeight;		// Current window dimensions.

	bool doPrompts;					// Are we processing prompts?
	BList prompts;					// Currently defined prompts.

	// Current local and remote option status.
	bool loc_options[TELOPT_NUM];
	bool rem_options[TELOPT_NUM];

};

#endif
