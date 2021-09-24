/* AmStudio.h
 * Copyright (c)2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.02.02		hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef AMKERNEL_AMSTUDIO_H
#define AMKERNEL_AMSTUDIO_H

#include <vector>
#include <app/Message.h>
#include <storage/File.h>
#include <support/DataIO.h>
#include <support/Locker.h>
#include <support/SupportDefs.h>
#include "AmPublic/AmDeviceI.h"

/**********************************************************************
 * _AM-STUDIO-ENTRY
 **********************************************************************/
class _AmStudioEntry
{
public:
	_AmStudioEntry();
	_AmStudioEntry(const BMessage* msg);
	_AmStudioEntry(am_studio_endpoint& endpoint, ArpCRef<AmDeviceI> device);
	_AmStudioEntry(am_studio_endpoint& endpoint, const char* label,
					const char* devMfg, const char* devName);

	_AmStudioEntry&	operator=(const _AmStudioEntry &e);

	bool			Matches(const am_studio_endpoint& endpoint) const;

	status_t		WriteTo(BMessage* msg) const;

	am_studio_endpoint	mEndpoint;
	BString				mLabel;
	BString				mDevMfg;
	BString				mDevName;
};
typedef std::vector<_AmStudioEntry>		studio_vec;

/***************************************************************************
 * AM-STUDIO
 * This simple class holds a mapping between devices and MIDI consumers.
 ***************************************************************************/
class AmStudio
{
public:
	AmStudio();
	virtual ~AmStudio();
	
	status_t			Associate(am_studio_endpoint& endpoint, ArpCRef<AmDeviceI> device);
	status_t			SetLabel(am_studio_endpoint& endpoint, const char* label);
	status_t			SetDevice(am_studio_endpoint& endpoint, const char* devMfg, const char* devName);
	ArpCRef<AmDeviceI>	DeviceFor(am_studio_endpoint& endpoint) const;
	status_t			GetDeviceInfo(	const am_studio_endpoint& endpoint,
										BString& label, BString& devMfg, BString& devName) const;
	/* Answer with the info for the endpoint at the given location.  This
	 * is useful because the studio can store 'dormant' endpoints -- endpoints
	 * that were active at one time but are not currently.
	 */
	status_t			GetInfoAt(	uint32 index, am_studio_endpoint& outEndpoint,
									BString* outLabel = NULL, BString* outDevMfg = NULL,
									BString* outDevName = NULL) const;
	/* Delete all entries with the same name as that supplied in the endpoint.  Currently
	 * I ignore ID, because I'm not sure what value the ID is for dormant endpoints.
	 */
	status_t			DeleteEndpoint(const am_studio_endpoint& endpoint);
	
	status_t			Write(BDataIO* data) const;
	status_t			Read(BDataIO* data);

private:
	mutable BLocker		mEntryLock;
	studio_vec			mEntries;

	_AmStudioEntry*		EntryFor(const am_studio_endpoint& endpoint) const;
};

#endif
