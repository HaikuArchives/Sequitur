/*
	
	ArpConfigurePanel.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREFILE_H
#include <ArpKernel/ArpConfigureFile.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPCOLLECTIONS_ARPPTRVECTOR_H
#include <ArpCollections/ArpPtrVector.h>
#endif

#ifndef ARPCOLLECTIONS_ARPSTLVECTOR_H
#include <ArpCollections/ArpSTLVector.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _DIRECTORY_H
#include <storage/Directory.h>
#endif

#ifndef _FILE_H
#include <storage/File.h>
#endif

#ifndef _NODEINFO_H
#include <storage/NodeInfo.h>
#endif

#ifndef _NODE_MONITOR_H
#include <storage/NodeMonitor.h>
#endif

#include <stdlib.h>

ArpMOD();

ArpConfigureFile::ArpConfigureFile()
	: mConfigs(0), mNames(0), mWatchMessage(0)
{
	Initialize();
}

ArpConfigureFile::ArpConfigureFile(const ArpConfigureFile& o)
	: mConfigs(0), mNames(0), mWatchMessage(0)
{
	Initialize();
	(*this) = o;
}

ArpConfigureFile::~ArpConfigureFile()
{
	StopWatcher();
	FreeMemory();
}

ArpConfigureFile& ArpConfigureFile::operator=(const ArpConfigureFile& o)
{
	StopWatcher();
	
	size_t i;
	mConfigs->resize(0);
	mConfigs->reserve(o.mConfigs->size());
	for( i=0; i<o.mConfigs->size(); i++ ) mConfigs->push_back(o.mConfigs->at(i));
	mNames->resize(0);
	mNames->reserve(o.mNames->size());
	for( i=0; i<o.mNames->size(); i++ ) mNames->push_back(o.mNames->at(i));
	mFile = o.mFile;
	mMimeType = o.mMimeType;
	
#if 0	// Don't copy the watcher info
	mWatcher = o.mWatcher;
	
	delete mWatchMessage;
	mWatchMessage = 0;
	if( o.mWatchMessage ) {
		mWatchMessage = new BMessage(o.mWatchMessage);
	}
#endif

	StartWatcher();
	
	ArpD(cdb << ADH << "Copied ArpConfigureFile:" << endl);
	ArpD(cdb << ADH << "  src num configs = " << o.mConfigs->size()
					<< ", names = " << o.mNames->size() << endl);
	ArpD(cdb << ADH << "  new num configs = " << mConfigs->size()
					<< ", names = " << mNames->size() << endl);
	return *this;
}

status_t ArpConfigureFile::AddConfig(ArpConfigurableI* obj, const char* nm)
{
	ArpD(cdb << ADH << "Adding configurable: obj=" << obj
				<< ", name=" << (nm ? nm : "<NULL>") << endl);
	mConfigs->push_back(obj);
	mNames->push_back(BString(nm));
	return B_OK;
}

const ArpVectorI<ArpConfigurableI*>& ArpConfigureFile::Configs() const
{
	return *mConfigs;
}
	
const ArpVectorI<BString>& ArpConfigureFile::Names() const
{
	return *mNames;
}

BMessenger ArpConfigureFile::Watcher() const
{
	return mWatcher;
}

status_t ArpConfigureFile::StartWatching(const BMessenger& destination,
										 BMessage* message)
{
	StopWatching();
	mWatcher = destination;
	mWatchMessage = message;
	return StartWatcher();
}

void ArpConfigureFile::StopWatching()
{
	delete mWatchMessage;
	mWatchMessage = 0;
	mWatcher = BMessenger();
}

entry_ref ArpConfigureFile::EntryRef() const
{
	return mFile;
}

status_t ArpConfigureFile::SetFile(const entry_ref& file)
{
	StopWatcher();
	mFile = file;
	return StartWatcher();
}

BPath ArpConfigureFile::Path() const
{
	BPath path;
	if( mFile != entry_ref() ) {
		BEntry entry(&mFile);
		if( entry.InitCheck() == B_OK ) entry.GetPath(&path);
	}
	return path;
}

status_t ArpConfigureFile::SetFile(const BPath& file)
{
	BEntry entry(file.Path());
	status_t err = entry.InitCheck();
	if( err != B_OK ) return err;
	
	entry_ref ref;
	err = entry.GetRef(&ref);
	if( err != B_OK ) return err;
	return SetFile(ref);
}

status_t ArpConfigureFile::SetFile(const char* path, const char* name,
								   directory_which base)
{
	status_t err = B_OK;
	
	// convert the given file location into a BPath
	BPath filename;
	err = find_directory(base, &filename, true);
	if( err != B_OK ) return err;
	
	err = filename.InitCheck();
	if( err != B_OK ) return err;
	
	ArpD(cdb << ADH << "ArpConfigureFile: base path = "
					<< filename.Path() << endl);
	if( path ) {
		err = filename.Append(path);
		if( err != B_OK ) return err;
		err = create_directory(filename.Path(), 0777);
		if( err != B_OK ) return err;
		ArpD(cdb << ADH << "ArpConfigureFile: added path = "
						<< filename.Path() << endl);
	}
	
	err = filename.Append(name);
	if( err != B_OK ) return err;
	
	ArpD(cdb << ADH << "ArpConfigureFile: full path = "
					<< filename.Path() << endl);
	BEntry entry(filename.Path());
	err = entry.InitCheck();
	if( err != B_OK ) return err;
	
	entry_ref ref;
	err = entry.GetRef(&ref);
	if( err != B_OK ) return err;
	
	return SetFile(ref);
}

status_t ArpConfigureFile::SetFile(const char* name)
{
	ArpD(cdb << ADH << "ArpConfigureFile: setting file from path = "
					<< name << endl);
					
	BEntry entry(&mFile);
	if( entry.InitCheck() != B_OK ) return entry.InitCheck();
	
	BPath path;
	status_t err = entry.GetPath(&path);
	if( err != B_OK ) return err;
	
	ArpD(cdb << ADH << "ArpConfigureFile: orig path = "
					<< path << endl);
	
	err = path.InitCheck();
	if( err == B_OK ) err = path.GetParent(&path);
	if( err == B_OK ) err = path.Append(name);
	ArpD(cdb << ADH << "ArpConfigureFile: renamed path = "
					<< path.Path() << endl);
	
	if( err != B_OK ) return err;
	
	entry.SetTo(path.Path());
	err = entry.InitCheck();
	if( err != B_OK ) return err;
	
	entry_ref ref;
	err = entry.GetRef(&ref);
	if( err != B_OK ) return err;
	
	return SetFile(ref);
}

bool ArpConfigureFile::HasFile() const
{
	return mFile == entry_ref() ? false : true;
}

void ArpConfigureFile::SetMimeType(const char* mimetype)
{
	mMimeType = mimetype;
}

const char* ArpConfigureFile::MimeType() const
{
	return mMimeType.String();
}

status_t ArpConfigureFile::MakeSettings(BMessage* in) const
{
	if( in == 0 ) return B_BAD_VALUE;
	
	const ArpVectorI<ArpConfigurableI*>& configs = Configs();
	const ArpVectorI<BString>& names = Names();
	
	status_t err = B_OK;
	
	for( size_t i=0; err == B_OK && i<configs.size(); i++ ) {
		if( configs[i] ) {
			if( i < names.size() && names[i] != "" ) {
				BMessage msg;
				err = configs[i]->GetConfiguration(&msg);
				if( err == B_OK ) {
					err = in->AddMessage(names[i].String(), &msg);
				}
			} else {
				err = configs[i]->GetConfiguration(in);
			}
		}
	}
	
	return err;
}

status_t ArpConfigureFile::WriteSettings(const BMessage* from) const
{
	if( from == 0 ) return B_BAD_VALUE;
	
	ArpD(cdb << ADH << "ArpConfigureFile: Writing settings with BMessage: "
					<< *from << endl);
	
	if( mFile == entry_ref() ) return B_NO_INIT;
	
	status_t err;
	
	// We seem to need to force any symbolic link to be resolved --
	// otherwise, the BFile.Flatten() fails.
	BEntry entry(&mFile, true);
	
	BFile file(&entry, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	err = file.InitCheck();
	ArpD(cdb << ADH << "Result from creating file: " << err << endl);
	if( err ) return err;
	
	if( (err=from->Flatten(&file)) ) return err;
	
	ArpD(cdb << ADH << "Result from writing settings: " << err << endl);
	
	if( mMimeType != "" ) {
		ArpD(cdb << ADH << "Writing MIME type: " << MimeType() << endl);
		BNodeInfo info(&file);
		if( (err=info.InitCheck()) ) return err;
		err = info.SetType(mMimeType.String());
		ArpD(cdb << ADH << "Result from writing mime type: " << err << endl);
		if( err ) return err;
	}
	
	return err;
}

status_t ArpConfigureFile::WriteSettings() const
{
	BMessage settings;
	
	ArpD(cdb << ADH << "ArpConfigureFile: Writing settings..." << endl);
	
	status_t err;
	if( (err=MakeSettings(&settings)) ) return err;
	
	ArpD(cdb << ADH << "ArpConfigureFile: Retrieved settings: "
					<< settings << endl);
	
	return WriteSettings(&settings);
}

status_t ArpConfigureFile::ApplySettings(const BMessage* from)
{
	if( from == 0 ) return B_BAD_VALUE;
	
	const ArpVectorI<ArpConfigurableI*>& configs = Configs();
	const ArpVectorI<BString>& names = Names();
	
	status_t err = B_OK;
	
	for( size_t i=0; err == B_OK && i<configs.size(); i++ ) {
		if( configs[i] ) {
			if( i < names.size() && names[i] != "" ) {
				BMessage msg;
				err = from->FindMessage(names[i].String(), &msg);
				if( err == B_OK ) {
					err = configs[i]->PutConfiguration(&msg);
				}
			} else {
				err = configs[i]->PutConfiguration(from);
			}
		}
	}
	
	return err;
}

status_t ArpConfigureFile::ReadSettings(BMessage* to) const
{
	if( to == 0 ) return B_BAD_VALUE;
	
	if( mFile == entry_ref() ) return B_NO_INIT;
	
	status_t err;
	BFile file(&mFile, B_READ_ONLY);
	if( (err=file.InitCheck()) ) return err;
	
	if( (err=to->Unflatten(&file)) ) return err;
	
	return err;
}

status_t ArpConfigureFile::ReadSettings()
{
	BMessage settings;
	
	status_t err;
	if( (err=ReadSettings(&settings)) ) return err;
	
	if( (err=ApplySettings(&settings)) ) return err;
	
	return err;
}

void ArpConfigureFile::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	ArpD(cdb << ADH << "ArpConfigureFile received message: "
					<< *message << endl);
	
	if( message->what == B_NODE_MONITOR && mWatcher.IsValid() ) {
	
		int32 opcode;
		if( message->FindInt32("opcode", &opcode) == B_OK
				&& opcode == B_ENTRY_MOVED ) {
			entry_ref ref;
			const char* name;
			if( message->FindInt32("device", &ref.device) == B_OK
				&& message->FindInt64("to directory", &ref.directory) == B_OK
				&& message->FindString("name", &name) == B_OK ) {
				
				ref.set_name(name);
				mFile = ref;
			}
		}
		
		if( mWatchMessage ) {
			BMessage newMsg(*mWatchMessage);
			ArpUpdateMessage(newMsg, *message);
			mWatcher.SendMessage(&newMsg);
		} else {
			mWatcher.SendMessage(message);
		}
		
		return;
	}
	
	inherited::MessageReceived(message);
}

void ArpConfigureFile::Initialize(void)
{
	try {
		mConfigs = new ArpPtrVector<ArpConfigurableI*>(0);
		mNames = new ArpSTLVector<BString>(0);
	} catch(...) {
		FreeMemory();
		throw;
	}
}

void ArpConfigureFile::FreeMemory(void)
{
	delete mWatchMessage;
	mWatchMessage = 0;
	delete mNames;
	mNames = 0;
	delete mConfigs;
	mConfigs = 0;
}

class ArpHandlerLock {
public:
	ArpHandlerLock(BHandler* handler) {
		mHandler = handler;
		mLocked = mHandler->LockLooper();
	}
	~ArpHandlerLock() {
		if( mHandler && mLocked ) mHandler->UnlockLooper();
	}
	
	bool IsLocked() const {
		return mLocked;
	}
	
private:
	BHandler* mHandler;
	bool mLocked;
};

status_t ArpConfigureFile::StartWatcher(void)
{
	StopWatcher();
	
	ArpD(cdb << ADH << "Starting watcher..." << endl);
	if( mFile != entry_ref() && mWatcher.IsValid() ) {
		ArpD(cdb << ADH << "On file: " << Path().Path() << endl);
		BEntry entry(&mFile);
		if( entry.InitCheck() != B_OK ) return entry.InitCheck();
		status_t err = entry.GetNodeRef(&mNode);
		if( err != B_OK ) return err;
		
		BLooper* looper=0;
		mWatcher.Target(&looper);
		ArpD(cdb << ADH << "Target is: " << looper << endl);
		if( looper == 0 ) {
			mNode = node_ref();
			return B_ERROR;
		}
		
		looper->AddHandler(this);
		
		err = watch_node(&mNode, B_WATCH_ALL, this);
		ArpD(cdb << ADH << "Result from watch_node(): " << err << endl);
		if( err != B_OK ) {
			looper->RemoveHandler(this);
			mNode = node_ref();
			return err;
		}
	}
	
	return B_OK;
}

status_t ArpConfigureFile::StopWatcher(void)
{
	ArpD(cdb << ADH << "Stopping watcher..." << endl);
	status_t err = B_NO_ERROR;
	if( mNode != node_ref() ) {
		err = watch_node(&mNode, B_STOP_WATCHING, this);
	}
	if( Looper() ) Looper()->RemoveHandler(this);
	
	return err;
}
