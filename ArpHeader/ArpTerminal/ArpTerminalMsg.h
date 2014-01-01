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
 * ArpTerminalMsg.h
 *
 * This file defines the message protocol between the terminal
 * and a remote device.  It is based on the BMessage: the two
 * objects talk with each other through BMessenger handles.
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
 * 0.1: Created this file.
 *
 */

#ifndef ARPTERMINAL_TERMINALMSG_H
#define ARPTERMINAL_TERMINALMSG_H

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

// The messages we send or receive

enum {
	/**	Text going from or coming to the terminal.  Data fields are:
	 **
	 **	"text" (B_ASCII_TYPE): The ASCII text being transmitted;
	 **	there may be multiple text data items in an array.
	 **	Note that it is perfectly acceptable for this text to
	 **	contain null ('\0') characters, so you CAN NOT use standard
	 **	C string manipulation functions on it!
	 **/
	TERM_XFER_TEXT_MSG = 'tXFR',
	
	/**	Perform a reset of the terminal.  Data fields are:
	 **
	 **	"hard" (B_BOOL_TYPE): If true, a full hard reset will
	 ** be performed.  Otherwise, only a soft reset will be
	 ** performed.
	 **/
	TERM_RESET_MSG = 'tRST',
	
	/**	Change window title associated with the terminal.  This
	 ** is sent by the terminal to its owner.  Data fields are:
	 **
	 **	"title" (B_STRING_TYPE): The new title to display.
	 **/
	TERM_SET_TITLE_MSG = 'tSTL',
	
	/**	Inform remote device that terminal has attached to it.
	 **
	 ** This message is sent to the remote device, when it is
	 ** attached to the terminal by calling SetRemote().  It is
	 ** primarily intended to supply the remote with the
	 ** terminal's BMessenger, so that full communication can
	 ** proceed.  Data fields are:
	 **
	 ** "terminal" (B_MESSENGER_TYPE): The messenger used for
	 ** sending messages to the terminal.
	 **/
	TERM_ATTACH_MSG = 'tATC',
	
	/**	Inform the terminal that a session is start.
	 **
	 ** This message is sent to the terminal when a new
	 ** "session" is starting.  Usually it is sent by a remote
	 ** device as part of its Connect() sequence.
	 **/
	TERM_START_SESSION_MSG = 'tSSN',
	
	/**	Inform the terminal that a session is ending.
	 **
	 ** This message is sent to the terminal when the current
	 ** "session" is ending.  Usually it is sent by a remote
	 ** device as part of its Disconnect() sequence.  Data fields are:
	 **
	 ** "remoteclosed" (B_BOOL_TYPE): Set to true if this session
	 ** was closed by the remote device; else false if it was initiated
	 ** by the local program.
	 **/
	TERM_END_SESSION_MSG = 'tESN',
	
	/**	Request terminal status.
	 **
	 **	This command is sent to the terminal; it asks for the terminal
	 **	to send messages back containing its state information.
	 **	Currently, this means sending back the following messages:
	 **	
	 **		TERM_WINDOW_SIZE_MSG
	 **		TERM_EMULATORS_MSG
	 **	
	 ** There are no defined data fields for this message.
	 **/
	TERM_REQUEST_STATUS_MSG = 'tSTS',

	/**	Transmit terminal size.
	 **
	 **	This command is sent by the terminal to the data sink,
	 **	describing the current terminal dimensions.  It is trasmited
	 **	whenever these dimensions change, or in response to a
	 **	TERM_REQUEST_STATUS_MSG.
	 **
	 ** You can also send this message to the terminal to change
	 ** its dimensions; these dimensions are the same as the
	 ** TermSetFixedSize command, that is -1 one in a coordinate
	 ** specify that coordinate should follow the window size.
	 **
	 **	Data fields are:
	 **	
	 **	"columns" (B_INT32_TYPE) the number of columns in characters.
	 **	"rows" (B_INT32_TYPE) the number of rows in characters.
	 **/
	TERM_WINDOW_SIZE_MSG = 'tWSZ',
	
	/**	Transmit terminal emulators.
	 **
	 **	This command is sent by the terminal to the data sink,
	 **	describing the current terminal emulator types available.
	 ** It is transmited when the terminal attaches to the device
	 ** or in response to a TERM_REQUEST_STATUS_MSG.
	 **
	 ** Data fields are:
	 **
	 ** "name" (B_STRING_TYPE) an array of the emulator names
	 ** available; these are the same names described in RFCxxxx.
	 **
	 ** "longname" (B_STRING_TYPE) array of more descriptive names,
	 ** corresponding to the names in the above array.  These are
	 ** the names that should be used when displaying text to the
	 ** user.
	 **/
	TERM_EMULATORS_MSG = 'tEMU',
	
	/**	Select terminal emulation.
	 **
	 **	This command is sent to the terminal to switch to
	 ** a particular terminal emulation.  As soon as the terminal
	 ** receives this message, it creates a new emulator of the
	 ** given type (if one can be found) and starts using it,
	 ** deallocating any emulator that it had previously been
	 ** using.
	 **
	 ** This message will also be sent by the terminal to
	 ** the remote when its emulation changes.
	 **
	 ** Data fields are:
	 **
	 ** "name" (B_STRING_TYPE) is the name of the new emulator.
	 **/
	TERM_SELECT_EMULATION_MSG = 'tSMU',
	
	/**	Select remote device.
	 **
	 **	This is a suggested command that can be sent to the
	 ** application to change the current remote device attached
	 ** to the terminal.
	 **
	 ** Data fields (from ArpRemoteManager) are:
	 **
	 ** "url" (B_STRING_TYPE) the URL protocol name of the
	 ** desired remote device.  Examples are "telnet" and
	 ** "shell".
	 **/
	TERM_SELECT_REMOTE_MSG = 'tSRD',
	
	/** Connect remote device.
	 **
	 ** This command is sent to the ArpRemoteInterface::GetDevice()
	 ** messenger, and requests that the remote device initiate
	 ** a connection.
	 **
	 ** Data fields are:
	 **
	 ** "url" (B_STRING_TYPE) a url string for the remote to connect
	 ** to, e.g. "telnet://enteract.com/".  If not supplied, the
	 ** device will do its default connect behavior.
	 **/
	TERM_CONNECT_MSG = 'tCON',
	
	/** Disconnect remote device.
	 **
	 ** This command is sent to the ArpRemoteInterface::GetDevice()
	 ** messenger, and requests that the remote device clear any
	 ** active connection it may have.
	 **
	 ** There are no data fields for this command.
	 **/
	TERM_DISCONNECT_MSG = 'tDCN',
	
	// Synonyms for temporary old source compatiblity.
	TERM_ADD_WATCHER_MSG = ARP_ADD_WATCHER_MSG,
	TERM_REM_WATCHER_MSG = ARP_REM_WATCHER_MSG
};

#endif
