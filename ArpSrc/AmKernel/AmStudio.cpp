/* AmStudio.cpp
*/
#define _BUILDING_AmKernel 1

#include <cstdio>
#include <cstdlib>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmStudio.h"

static const char*	ENTRY_MSG				= "entry";
static const char*	ENDPOINT_NAME_STR		= "endoint_name";
static const char*	TYPE_STR				= "type";
static const char*	ID_STR					= "id";
static const char*	CHANNEL_STR				= "channel";
static const char*	LABEL_STR				= "label";
static const char*	DEV_MFG_STR				= "dev_mfg";
static const char*	DEV_NAME_STR			= "dev_name";

/***************************************************************************
 * AM-STUDIO
 ****************************************************************************/
AmStudio::AmStudio()
{
}

AmStudio::~AmStudio()
{
}

status_t AmStudio::Associate(am_studio_endpoint& endpoint, ArpCRef<AmDeviceI> device)
{
	ArpASSERT(device);
	if (!device) return B_ERROR;
	BAutolock l(mEntryLock);
	_AmStudioEntry*		entry = EntryFor(endpoint);
	if (!entry) {
		mEntries.push_back( _AmStudioEntry(endpoint, device) );
	} else {
		entry->mDevMfg = device->Manufacturer();
		entry->mDevName = device->Product();
	}
	return B_OK;
}

status_t AmStudio::SetLabel(am_studio_endpoint& endpoint, const char* label)
{
	BAutolock l(mEntryLock);
	_AmStudioEntry*		entry = EntryFor(endpoint);
	if (entry) entry->mLabel = label;
	else {
		mEntries.push_back( _AmStudioEntry(endpoint, label, NULL, NULL) );
	}
	return B_OK;
}

status_t AmStudio::SetDevice(	am_studio_endpoint& endpoint,
								const char* devMfg, const char* devName)
{
	BAutolock l(mEntryLock);
	_AmStudioEntry*		entry = EntryFor(endpoint);
	if (entry) {
		entry->mDevMfg = devMfg;
		entry->mDevName = devName;
	} else {
		mEntries.push_back( _AmStudioEntry(endpoint, NULL, devMfg, devName) );
	}
	return B_OK;
}

ArpCRef<AmDeviceI> AmStudio::DeviceFor(am_studio_endpoint& endpoint) const
{
	BString		mfg, name;
	{
		BAutolock l(mEntryLock);
		_AmStudioEntry*		entry = EntryFor(endpoint);
		if (!entry) return NULL;
		mfg = entry->mDevMfg;
		name = entry->mDevName;
	}
	return AmGlobals().DeviceNamed(mfg.String(), name.String() );
}

status_t AmStudio::GetDeviceInfo(	const am_studio_endpoint& endpoint,
									BString& label, BString& devMfg, BString& devName) const
{
	BAutolock l(mEntryLock);
	_AmStudioEntry*		entry = EntryFor(endpoint);
	if (!entry) return B_ERROR;
	label = entry->mLabel;
	devMfg = entry->mDevMfg;
	devName = entry->mDevName;
	return B_OK;
}

status_t AmStudio::GetInfoAt(	uint32 index, am_studio_endpoint& outEndpoint,
								BString* outLabel, BString* outDevMfg,
								BString* outDevName) const
{
	BAutolock l(mEntryLock);
	if (index >= mEntries.size() ) return B_ERROR;
	outEndpoint = mEntries[index].mEndpoint;
	if (outLabel) *outLabel = mEntries[index].mLabel;
	if (outDevMfg) *outDevMfg = mEntries[index].mDevMfg;
	if (outDevName) *outDevName = mEntries[index].mDevName;
	return B_OK;
}

static bool scrub_one_entry(studio_vec& entries,
							const am_studio_endpoint& endpoint)
{
	for (uint32 k = 0; k < entries.size(); k++) {
		if (entries[k].mEndpoint.name == endpoint.name
				&& entries[k].mEndpoint.type == endpoint.type) {
			entries.erase(entries.begin() + k);
			return true;
		}
	}
	return false;
}

status_t AmStudio::DeleteEndpoint(const am_studio_endpoint& endpoint)
{
	BAutolock l(mEntryLock);
	while (scrub_one_entry(mEntries, endpoint) ) ;
	return B_OK;
}

status_t AmStudio::Write(BDataIO* data) const
{
	BMessage	msg;
	BMessage	entryMsg;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].WriteTo(&entryMsg) == B_OK)
			msg.AddMessage(ENTRY_MSG, &entryMsg);
		entryMsg.MakeEmpty();
	}
	AmFlatten(msg, data);
	return B_OK;
}

status_t AmStudio::Read(BDataIO* data)
{
	BMessage	msg;
	status_t	err = msg.Unflatten(data);
	if (err != B_OK) return err;
	mEntries.resize(0);
	BMessage	entryMsg;
	for (int32 k = 0; msg.FindMessage(ENTRY_MSG, k, &entryMsg) == B_OK; k++) {
		mEntries.push_back( _AmStudioEntry(&entryMsg) );
	}
	return B_OK;
}

_AmStudioEntry* AmStudio::EntryFor(const am_studio_endpoint& endpoint) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].Matches(endpoint))
			return const_cast<_AmStudioEntry*>( &mEntries[k] );
	}
	return NULL;
}

/**********************************************************************
 * _AM-STUDIO-ENTRY
 **********************************************************************/
_AmStudioEntry::_AmStudioEntry()
{
}

_AmStudioEntry::_AmStudioEntry(const BMessage* msg)
{
	const char*		str;
	int32			i;
	if (msg->FindString(ENDPOINT_NAME_STR, &str) == B_OK) mEndpoint.name = str;
	if (msg->FindInt32(TYPE_STR, &i) == B_OK) mEndpoint.type = (AmEndpointType)i;
	if (msg->FindInt32(ID_STR, &i) == B_OK) mEndpoint.id = i;
	if (msg->FindInt32(CHANNEL_STR, &i) == B_OK) mEndpoint.channel = i;
	if (msg->FindString(LABEL_STR, &str) == B_OK) mLabel = str;
	if (msg->FindString(DEV_MFG_STR, &str) == B_OK) mDevMfg = str;
	if (msg->FindString(DEV_NAME_STR, &str) == B_OK) mDevName = str;
}

_AmStudioEntry::_AmStudioEntry(	am_studio_endpoint& endpoint,
								ArpCRef<AmDeviceI> device)
{
	ArpASSERT(device);
	mEndpoint = endpoint;
	mDevMfg = device->Manufacturer();
	mDevName = device->Product();
}

_AmStudioEntry::_AmStudioEntry(	am_studio_endpoint& endpoint, const char* label,
								const char* devMfg, const char* devName)
		: mLabel(label), mDevMfg(devMfg), mDevName(devName)
{
	mEndpoint = endpoint;
}

_AmStudioEntry& _AmStudioEntry::operator=(const _AmStudioEntry &e)
{
	mEndpoint = e.mEndpoint;
	mLabel = e.mLabel;
	mDevMfg = e.mDevMfg;
	mDevName = e.mDevName;
	return *this;
};

bool _AmStudioEntry::Matches(const am_studio_endpoint& endpoint) const
{
	return mEndpoint == endpoint;
}

status_t _AmStudioEntry::WriteTo(BMessage* msg) const
{
	if (mEndpoint.name.Length() > 0) msg->AddString( ENDPOINT_NAME_STR, mEndpoint.name.String() );
	msg->AddInt32(TYPE_STR, mEndpoint.type);
	msg->AddInt32(ID_STR, mEndpoint.id);
	msg->AddInt32(CHANNEL_STR, mEndpoint.channel);
	if (mLabel.Length() > 0) msg->AddString( LABEL_STR, mLabel.String() );
	if (mDevMfg.Length() > 0) msg->AddString( DEV_MFG_STR, mDevMfg.String() );
	if (mDevName.Length() > 0) msg->AddString( DEV_NAME_STR, mDevName.String() );
	return B_OK;
}
