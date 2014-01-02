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
 * The ArpTerminalInterface class describes an abstract interface
 * to some a remote device that is attached to an ArpTerminal.
 * Communication with this devices is through a BMessage protocal
 * device -- the remote device is itself a BLooper, which communicates
 * with an ArpRemoteTerminal's BMessenger.
 *
 * This file mostly describes a C++ interface for the surrounding
 * application to handle the remote device, and makes no attempt
 * to implement the actual BMessage protocol.  See ArpRemoteDevice
 * for a basic implementation of the ArpRemoteTerminal BMessage
 * protocol.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• This is a preliminary design, and subject to significant change.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• First release, corresponding to ArpTelnet v2.0.
 *
 */

#ifndef ARPTERMINAL_ARPREMOTEINTERFACE_H
#define ARPTERMINAL_ARPREMOTEINTERFACE_H

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef	_ARCHIVABLE_H
#include <support/Archivable.h>
#endif

#ifndef	_LOOPER_H
#include <app/Looper.h>
#endif

#ifndef	_MESSENGER_H
#include <app/Messenger.h>
#endif

struct ArpRemoteType {
	int32 StructLength;			// sizeof(ArpRemoteType)
	const char* URL;
	const char* LongName;
	const char* Synonym;
};

class ArpRemoteInterface : public ArpConfigurableI {

public:

	// --------------------------------------------------------
	// INTERFACE VERSIONING.
	// --------------------------------------------------------
	
	/* Return the version of the remote interface that this
	 * object is implementing.  Devices should define this
	 * to always return REMOTE_VERSION_CURRENT.
	 */

	enum {
		REMOTE_VERSION_1 = 19980826,	// First version
		
		REMOTE_VERSION_CURRENT = REMOTE_VERSION_1
	};
	typedef uint32 RemoteVersionType;
	
	virtual RemoteVersionType RemoteVersion(void)
		{ return REMOTE_VERSION_CURRENT; }
	
	// --------------------------------------------------------
	// TERMINAL COMMUNICATION.
	// --------------------------------------------------------
	
	// Retrieve the type information about this remote object.
	virtual const ArpRemoteType* RemoteType(void) const = 0;
	
	/* Return a BMessenger that can be used to send messages
	 * to the remote device.  You attach a device to a terminal
	 * by handling this BMessenger to the ArpRemoteTerminal.
	 */
	virtual BMessenger GetDevice(void) = 0;
	
	/* Get and set the BMessenger used to communication with
	 * the terminal.  You do not normally need to call these,
	 * as the terminal will handshake with its remote device
	 * we this is attached to it.
	 */
	virtual void SetTerminal(const BMessenger& term) = 0;
	virtual BMessenger GetTerminal(void) = 0;
	
	/* Connect and disconnect this object from its physical
	 * device.  Note that while these functions return immediately,
	 * they may not have an immediate effect -- for example a
	 * TELNET device will need to bring up a window prompting
	 * the user for a machine name.
	 */
	virtual status_t Connect(BWindow* owner = NULL) = 0;
	virtual void Disconnect() = 0;
	
	/* Make an immediate connection that is specified by a URL.
	 * If no URL is specified, either:
	 *   (1) Make a connection to the current device; or, if there
	 *       is no current device,
	 *   (2) Do Connect() to bring up a dialog window.
	 */
	virtual status_t Connect(const char* url, BWindow* owner = NULL) = 0;

	/* Return current status of connection:
	 * IsConnected() means that there is some kind of connection open.
	 * IsInUse() means that there is a connection and the user has done
	 * something with it.  (I.e., the shell will not return true for
	 * IsInUse() until the user presses a key, so that the surrounding
	 * application won't verifying closing that connection until
	 * something actually happens with it.)
	 */
	virtual bool IsConnected() const = 0;
	virtual bool IsInUse() const = 0;
	 
	/* Call this function to get rid of one of these objects. */
	virtual void Delete() = 0;
	
protected:
	/* Note that you are not allowed to directly delete one of these
	 * objects, so the destructor is protected. */
	virtual ~ArpRemoteInterface() { };
	
};

#endif
