/* AmFileRosters.cpp
*/

#define _BUILDING_AmKernel 1
#include <stdio.h>
#include <string.h>
#include <app/Application.h>
#include <StorageKit.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmMotion.h"
#include "AmKernel/AmPropertiesTool.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolBar.h"

#include "BeExp/SearchPath.h"

static const char*	APP_NAME		= "Sequitur";

//#define AM_TRACE_ENTRY_LOCK		(1)

AmFileRosterEntryI::~AmFileRosterEntryI() {}
bool AmFileRosterEntryI::IsImmutable() const
{
	return false;
}
static status_t verify_directory(const BString& pathStr)
{
	BPath		path( pathStr.String() );
	BPath		parent;
	status_t	err = path.InitCheck();
	if (err != B_OK) return err;
	if ((err = path.GetParent(&parent)) != B_OK) return err;
	BDirectory	dir( parent.Path() );
	if ((err = dir.InitCheck()) != B_OK) return err;
	if (dir.Contains(path.Leaf(), B_DIRECTORY_NODE) == true) return B_OK;
	return dir.CreateDirectory(path.Leaf(), 0);
}

/***************************************************************************
 * AM-FILE-ROSTER-ENTRY-I function
 ***************************************************************************/
BString convert_to_filename(const BString& str)
{
	BString		newStr;
	const char*	s = str.String();
	size_t		len = strlen(s);
	for (size_t k = 0; k < len; k++) {
		if (s[k] == '_') newStr << "_0";
		else if (s[k] == '/') newStr << "_1";
		else if (s[k] == '\\') newStr << "_2";
		else if (s[k] == ':') newStr << "_3";
		else if (s[k] == '.') newStr << "_4";
		else newStr << s[k];
	}
	return BString(newStr);
}

/***************************************************************************
 * _AM-ROSTER-SEARCH-PATH
 ***************************************************************************/
class _AmRosterSearchPath : public BLooper, public BSearchPath
{
public:
	_AmRosterSearchPath(const char* name, AmFileRoster* roster, bool readOnly)
			: BLooper(name),
			  mRoster(roster), mReadOnly(readOnly)
	{
	}

	virtual thread_id Run()
	{
		thread_id id = BLooper::Run();
		if (id < B_OK) return id;
		
		status_t err = StartHandling(BMessenger(this));
		if (err != B_OK) return err;
		
		return id;
	}

	virtual	void MessageReceived(BMessage *msg)
	{
		if (HandleNodeMessage(msg) == B_OK) {
			return;
		}
		BLooper::MessageReceived(msg);
		return;
	}
	
	virtual void Quit()
	{
		Stop();
		BLooper::Quit();
	}

protected:
	virtual void EntryCreated(const node_ref* node, const entry_ref* entry)
	{
		ArpASSERT(mRoster);
#if 0
		printf("Entry created");
		if (mReadOnly) printf(", read only\n"); else printf("\n");
#endif
		if (!mRoster) return;
		BPath		path(entry);
		if (path.InitCheck() != B_OK) return;
		BMessage	msg;
		if (ReadFile(entry, msg) != B_OK) return;
		if (mRoster->EntryCreated(msg, mReadOnly, path.Path()) == B_OK) {
			mRoster->SendChangedNotice();
		}
	}
	
	virtual void EntryMoved(const node_ref* node, const entry_ref* entry,
							const entry_ref* oldEntry)
	{
#if 0
		printf("Entry moved");
		if (mReadOnly) printf(", read only\n"); else printf("\n");
#endif
		if (!mRoster) return;
		mRoster->SendChangedNotice();
	}
	
	virtual void EntryRemoved(const node_ref* node, const entry_ref* entry)
	{
#if 0
		printf("Entry removed");
		if (mReadOnly) printf(", read only\n"); else printf("\n");
#endif
		if (!mRoster) return;
		BPath		path(entry);
		if (path.InitCheck() != B_OK) return;
		mRoster->EntryRemoved(path.Path());		
		mRoster->SendChangedNotice();
	}

private:
	AmFileRoster*	mRoster;
	bool			mReadOnly;

	/* This is a bit of a hack brought on by the general architecture:  When
	 * a new file-based object gets created, first the file is created, then
	 * the object is flattened into it.  The problem is that I (generally)
	 * receive an EntryCreated() message before the object has been flattened,
	 * so here I try and wait until there is actual data.  NOTE that you MUST
	 * create the file in the loop -- if the file is created outside of the
	 * loop, then the file will be locked, preventing the thread that's
	 * writing out the data from writing to the file until I've finished
	 * looping, which is of course too late for me.
	 */

	status_t ReadFile(const entry_ref* entry, BMessage& msg) const
	{
		for (uint32 k = 0; k < 100; k++) {
			status_t	err;
			BFile		file(entry, B_READ_ONLY);
			if ((err=file.InitCheck()) == B_OK) {
				if (msg.Unflatten(&file) == B_OK) return B_OK;
			}
			snooze(1000);
		}
		return B_ERROR;
	}
};

// #pragma mark -

/***************************************************************************
 * AM-FILE-ROSTER
 ***************************************************************************/
AmFileRoster::AmFileRoster(const char* name)
		: mSearchPath(NULL), mReadOnlySearchPath(NULL), mName(name)
{
	mSearchPath = new _AmRosterSearchPath("file search path", this, false);
	if (mSearchPath) mSearchPath->Run();
	mReadOnlySearchPath = new _AmRosterSearchPath("read-only file search path", this, true);
	if (mReadOnlySearchPath) mReadOnlySearchPath->Run();
}

AmFileRoster::~AmFileRoster()
{
	if (mSearchPath) mSearchPath->PostMessage(B_QUIT_REQUESTED);
	mSearchPath = NULL;
	if (mReadOnlySearchPath) mReadOnlySearchPath->PostMessage(B_QUIT_REQUESTED);
	mReadOnlySearchPath = NULL;
}

BString AmFileRoster::Name() const
{
	return mName;
}

status_t AmFileRoster::AddObserver(BHandler* handler)
{
	return mNotifier.AddObserver(handler, AM_FILE_ROSTER_CHANGED);
}

status_t AmFileRoster::RemoveObserver(BHandler* handler)
{
	return mNotifier.RemoveObserverAll(handler);
}

void AmFileRoster::SendChangedNotice()
{
	BMessage	msg(AM_FILE_ROSTER_CHANGED);
	mNotifier.ReportMsgChange(&msg, BMessenger());
}

file_entry_id AmFileRoster::EntryIdAt(uint32 index) const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmFileRoster::EntryIdAt() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock	l(&mEntryLock);
	AmFileRosterEntryI*	entry = EntryAt(index);
	if (!entry) return 0;
	return entry->Id();
}

bool AmFileRoster::KeyExists(const char* key) const
{
	ArpVALIDATE(key, return false);
	uint32		count = CountEntries();
	for (uint32 k = 0; k < count; k++) {
		AmFileRosterEntryI*	entry = EntryAt(k);
		if (entry && entry->Key() == key) return true;
	}
	return false;
}

status_t AmFileRoster::CreateEntry(const AmFileRosterEntryI* entry)
{
	ArpASSERT(entry);
	if (!entry) return B_ERROR;
	if (entry->IsImmutable()) return B_ERROR;
	BString				fileName = entry->LocalFileName();
	ArpASSERT(fileName.Length() >= 1);
	if (fileName.Length() < 1) return B_ERROR;

	BMessage			msg;
	status_t			err = entry->WriteTo(msg);
	if (err != B_OK) return err;

	/* If this entry is based on another file, delete it.
	 * However -- and this is a hack -- if there is a file
	 * to delete, then for the purpose of my clients this is
	 * simply a modification, and so the delete notification
	 * should suppressed in certain instances.
	 */
	if (entry->LocalFilePath().Length() > 0) {
#if 0
		if (entry->Key().Length() > 0) {
			BAutolock	l(&mModificationLock);
			mModificationKeys.push_back(entry->Key() );
		}
#endif
		DeleteFile(entry->LocalFilePath().String() );
	}
	
	return CreateEntry(fileName, msg);
}

status_t AmFileRoster::DuplicateEntry(const BString& key, const char* path)
{
	BString			fileName;
	BMessage		msg;
	{
		/* Limit the scoping of the lock -- there's no
		 * reason to hold it once I hit the CreateEntry()
		 * func at the bottom.
		 */
		#ifdef AM_TRACE_ENTRY_LOCK
		printf("AmFileRoster::DuplicateEntry() Acquire mEntryLock\n"); fflush(stdout);
		#endif
		BAutolock	l(mEntryLock);
		AmFileRosterEntryI*		entry = EntryAt(key, path);
		if (!entry) return B_ERROR;
		if (entry->IsImmutable()) return B_ERROR;
		BString		uniqueKey = UniqueKeyFrom(entry->Key() );
		entry = UniqueDuplicate(entry);
		if (!entry) return B_NO_MEMORY;
		fileName = UniqueFileNameFrom(entry->LocalFileName() );
		status_t	err = entry->WriteTo(msg);
		delete entry;
		if (err != B_OK) return err;
	}
	return CreateEntry(fileName, msg);
}

status_t AmFileRoster::DeleteEntry(const BString& key)
{
	BString			filePath;
	{
		/* Limit the scoping of the lock -- there's no
		 * reason to hold it once I hit the DeleteFile()
		 * func at the bottom.
		 */
		#ifdef AM_TRACE_ENTRY_LOCK
		printf("AmFileRoster::DeleteEntry() Acquire mEntryLock\n"); fflush(stdout);
		#endif
		BAutolock	l(&mEntryLock);
		AmFileRosterEntryI*		entry = EntryAt(key);
		if (!entry) return B_ERROR;
		if (entry->IsImmutable()) return B_ERROR;
		filePath = entry->LocalFilePath();
	}
	return DeleteFile(filePath.String());
}

status_t AmFileRoster::CreateEntry(const BString& fileName, const BMessage& msg)
{
	if (mCreationDirectory.Length() < 1) return B_NO_INIT;
	if (fileName.Length() < 1) return B_ERROR;
	BString			filePath(mCreationDirectory);
	filePath << "/" << fileName;
	/* Rosters do not receive notice when files are changed, only when they
	 * are added and removed, so I deliberately wipe out any existing file
	 * in order to receive notification.
	 */
	BEntry		entry(filePath.String());
	status_t	err = entry.InitCheck();
	if (err == B_OK) entry.Remove();
	/* Write the file.
	 */
	BFile			file(filePath.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ((err = file.InitCheck()) != B_OK) return err;
	AmFlatten(msg, &file);
	BNodeInfo		nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) nodeInfo.SetType(MimeType() );
	return B_OK;
}

status_t AmFileRoster::DeleteFile(const char* filePath)
{
	BEntry		entry(filePath);
	status_t	err = entry.InitCheck();
	if (err != B_OK) return err;
	return entry.Remove();
}

void AmFileRoster::SetCreationDirectory(const char* path)
{
	mCreationDirectory = path;
}

BString AmFileRoster::UniqueLabelFrom(const BString& origLabel) const
{
	/* FIX:  It'd be nice if I did an intelligent name, like 'copy 2'.
	 */
	BString		newLabel(origLabel);
	while (LabelExists(newLabel) == true) newLabel << " copy";
	return newLabel;
}

BString AmFileRoster::UniqueKeyFrom(const BString& origKey) const
{
	/* FIX:  It'd be nice if I did an intelligent name, like 'copy 2'.
	 */
	BString		newKey(origKey);
	while (KeyExists(newKey.String() ) == true) newKey << " copy";
	return newKey;
}

bool AmFileRoster::LabelExists(const BString& label) const
{
	AmFileRosterEntryI*	entry = NULL;
	for (uint32 k = 0; (entry = EntryAt(k)) != NULL; k++)
		if (label == entry->Label() ) return true;
	return false;
}

BString AmFileRoster::UniqueFileNameFrom(const BString& fileName)
{
	/* FIX:  It'd be nice if I did an intelligent name, like 'copy 2'.
	 */
	BString		fn(fileName);
	while (FileNameExists(fn) == true) fn << " copy";
	return fn;
}

bool AmFileRoster::FileNameExists(const BString& fileName)
{
	if (mSearchPath) {
		mSearchPath->Rewind();
		entry_ref		ref;
		while (mSearchPath->GetNextRef(&ref) == B_OK) {
			BPath		path(&ref);
			if (path.InitCheck() == B_OK && fileName == path.Leaf() )
				return true;
		}
	}
	return false;
}

// #pragma mark -

/***************************************************************************
 * AM-DEVICE-ROSTER
 ***************************************************************************/
static int32			gDeviceRosterCreated = 0;
static AmDeviceRoster*	gDeviceRoster = NULL;

class _AmDeviceObserver : public BLooper
{
public:
	_AmDeviceObserver()  : BLooper("ARP Device Observer")
	{
		Run();
	}
	
	~_AmDeviceObserver()
	{
		/* This is done the way it is because the holder
		 * might be deleted here -- which causes the filter
		 * to be set to NULL, which causes it to try and remove
		 * itself again.
		 */
		uint32						k;
		vector<AmFilterHolderI*>	f;
		for (k = 0; k < mFilters.size(); k++) f.push_back(mFilters[k]);
		mFilters.resize(0);
		for (k = 0; k < f.size(); k++) {
			if (f[k]) f[k]->DecRefs();
		}
	}

	status_t AddObserver(AmFilterHolderI* holder)
	{
		ArpVALIDATE(holder, return B_ERROR);
		holder->IncRefs();
		mFilters.push_back(holder);
		return B_OK;
	}
	
	status_t RemoveObserver(AmFilterHolderI* holder)
	{
		ArpVALIDATE(holder, return B_ERROR);
		for (uint32 k = 0; k < mFilters.size(); k++) {
			if (mFilters[k] && mFilters[k] == holder) {
				holder->DecRefs();
				mFilters.erase(mFilters.begin() + k);
				return B_OK;
			}
		}
		return B_ERROR;
	}

	virtual	void MessageReceived(BMessage* msg)
	{
		if (msg->what != 'isnd') {
			BLooper::MessageReceived(msg);
			return;
		}
		for (uint32 k = 0; k < mFilters.size(); k++) {
			if (mFilters[k] && mFilters[k]->Filter())
				mFilters[k]->Filter()->DeviceChanged();
		}
	}

private:
	vector<AmFilterHolderI*>	mFilters;
};

AmDeviceRoster* AmDeviceRoster::Default()
{
	if (atomic_or(&gDeviceRosterCreated, 1) == 0) {
		gDeviceRoster = new AmDeviceRoster();
	} else {
		while (!gDeviceRoster) sleep(20000);
	}
	return gDeviceRoster;
}

void AmDeviceRoster::ShutdownDefault(bool force_unload)
{
	if (gDeviceRoster) {
		delete gDeviceRoster;
		gDeviceRoster = NULL;
	}
}

status_t AmDeviceRoster::AddFilterObserver(AmFilterHolderI* holder)
{
	BAutolock	l(mObserverLock);
	if (!mObserver) mObserver = new _AmDeviceObserver();
	if (!mObserver) return B_NO_MEMORY;
	if (mObserver->Lock() != true) return B_ERROR;
	status_t		err = mObserver->AddObserver(holder);
	mObserver->Unlock();
	return err;
}

status_t AmDeviceRoster::RemoveFilterObserver(AmFilterHolderI* holder)
{
	BAutolock	l(mObserverLock);
	if (!mObserver) return B_ERROR;
	if (mObserver->Lock() != true) return B_ERROR;
	status_t		err = mObserver->RemoveObserver(holder);
	mObserver->Unlock();
	return err;
}

/* Don't know how necessary either the observer lock or the safe send
 * delivery is -- there were added trying to fix an unrelated bug.
 */
#include "ArpKernel/ArpSafeDelivery.h"

void AmDeviceRoster::SendChangedNotice()
{
	inherited::SendChangedNotice();
	BAutolock	l(mObserverLock);
	if (mObserver) {
		BMessenger	m(mObserver);
		BMessage	msg('isnd');
		SafeSendMessage(m, &msg);
//		mObserver->PostMessage('isnd');
	}
}

AmDeviceRoster::AmDeviceRoster()
		: inherited("Devices"), mObserver(0)
{
	BPath		path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() != B_OK) return;
	BString		dir(path.Path());
	dir << "/AngryRedPlanet/" << APP_NAME << "/Devices";
	status_t	err = verify_directory(dir);
	if (err != B_OK) printf("AmDeviceRoster::AmDeviceRoster() error: %s\n", strerror(err));
	SetCreationDirectory(dir.String());
	
	if (mSearchPath) mSearchPath->AddDirectory(dir.String());
	if (mReadOnlySearchPath) mReadOnlySearchPath->AddSearchPath("%A/Devices");
}

AmDeviceRoster::~AmDeviceRoster()
{
	{
		BAutolock		l(mObserverLock);
		if (mObserver) mObserver->PostMessage(B_QUIT_REQUESTED);
	}
	mDevices.resize(0);
}

status_t AmDeviceRoster::GetDeviceInfo(	uint32 index,
										BString& outMfg,
										BString& outName,
										BString& outKey,
										bool* outReadOnly,
										bool* outIsValid,
										BString* outDescription,
										BString* outAuthor,
										BString* outEmail,
										BBitmap** outIcon,
										BString* outFilePath) const
{
	BAutolock	l(mEntryLock);
	if (index >= mDevices.size() ) return B_ERROR;
	outMfg = mDevices[index]->Manufacturer();
	outName = mDevices[index]->Product();
	outKey = mDevices[index]->Key();
	if (outReadOnly) *outReadOnly = mDevices[index]->IsReadOnly();
	if (outIsValid) *outIsValid = mDevices[index]->IsValid();
	if (outDescription) *outDescription = mDevices[index]->ShortDescription();
	if (outAuthor) *outAuthor = mDevices[index]->Author();
	if (outEmail) *outEmail = mDevices[index]->Email();
	if (outIcon) {
		*outIcon = NULL;
		if (mDevices[index]->Icon(BPoint(20, 20)) ) *outIcon = new BBitmap(mDevices[index]->Icon(BPoint(20, 20)) );
	}
	if (outFilePath) *outFilePath = mDevices[index]->LocalFilePath().String();
	return B_OK;
}

AmDevice* AmDeviceRoster::NewDevice(const BString& key, const char* path) const
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mDevices.size(); k++) {
		if (key == mDevices[k]->Key() ) {
			if (!path || (path && mDevices[k]->LocalFilePath() == path) )
				return mDevices[k]->Copy();
		}
	}
	return NULL;
}

ArpCRef<AmDeviceI> AmDeviceRoster::DeviceAt(uint32 index) const
{
	BAutolock	l(mEntryLock);
	if (index >= mDevices.size() ) return NULL;
	return ArpCRef<AmDeviceI>(mDevices[index]);
}

ArpCRef<AmDeviceI> AmDeviceRoster::DeviceNamed(const char* mfg, const char* name) const
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mDevices.size(); k++) {
		if (mDevices[k]->Manufacturer() == mfg
				&& mDevices[k]->Product() == name)
			return ArpCRef<AmDeviceI>(mDevices[k]);
	}
	return NULL;
}

uint32 AmDeviceRoster::CountEntries() const
{
	BAutolock	l(mEntryLock);
	return mDevices.size();
}

BString AmDeviceRoster::KeyAt(uint32 index) const
{
	BAutolock	l(mEntryLock);
	if (index >= mDevices.size() ) return BString();
	return mDevices[index]->Key();
}

status_t AmDeviceRoster::EntryCreated(	const BMessage& msg,
										bool readOnly,
										const char* filePath)
{
	AmDevice*	d = new AmDevice(msg, readOnly, filePath);
	if (!d) return B_NO_MEMORY;
	BString		key = d->Key();
	BAutolock	l(mEntryLock);
	/* Find if there is already a valid device with
	 * this key.  If so, I'm a conflict, and not useable.
	 */
	for (uint32 k = 0; k < mDevices.size(); k++) {
		if (mDevices[k]->IsValid() && mDevices[k]->Key() == key) {
			d->SetIsValid(false);
			break;
		}
	}
	mDevices.push_back(d);
	return B_OK;
}

status_t AmDeviceRoster::EntryRemoved(const char* filePath)
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mDevices.size(); k++) {
		if (mDevices[k]->LocalFilePath() == filePath) {
			mDevices.erase(mDevices.begin() + k);
			return B_OK;
		}
	}
	return B_ERROR;
}

AmFileRosterEntryI* AmDeviceRoster::EntryAt(const BString& key, const char* path) const
{
	for (uint32 k = 0; k < mDevices.size(); k++) {
		if (key == mDevices[k]->Key() ) {
			if (!path || (path && mDevices[k]->LocalFilePath() == path) )
				return mDevices[k];
		}
	}
	return 0;
}

AmFileRosterEntryI* AmDeviceRoster::EntryAt(uint32 index) const
{
	if (index >= mDevices.size() ) return NULL;
	return mDevices[index];
}

AmFileRosterEntryI* AmDeviceRoster::UniqueDuplicate(AmFileRosterEntryI* entry) const
{
	AmDevice*		d = dynamic_cast<AmDevice*>(entry);
	if (!d) return NULL;
	d = d->Copy();
	if (!d) return NULL;
	BString			name = UniqueLabelFrom(d->Product() );
	d->SetName(name.String() );
	return d;	
}

const char* AmDeviceRoster::MimeType() const
{
	return "application/x-vnd.Arp-sequitur-device";
}

// #pragma mark -

/***************************************************************************
 * AM-MOTION-ROSTER
 ***************************************************************************/
static int32			gMotionRosterCreated = 0;
static AmMotionRoster*	gMotionRoster = NULL;

AmMotionRoster* AmMotionRoster::Default()
{
	if (atomic_or(&gMotionRosterCreated, 1) == 0) {
		gMotionRoster = new AmMotionRoster();
	} else {
		while (!gMotionRoster) sleep(20000);
	}
	return gMotionRoster;
}

void AmMotionRoster::ShutdownDefault(bool force_unload)
{
	if (gMotionRoster) {
		delete gMotionRoster;
		gMotionRoster = NULL;
	}
}

AmMotionRoster::AmMotionRoster()
		: inherited("Motions")
{
	BPath		path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() != B_OK) return;
	BString		dir(path.Path());
	dir << "/AngryRedPlanet/" << APP_NAME << "/Motions";
	status_t	err = verify_directory(dir);
	if (err != B_OK) printf("AmMotionRoster::AmMotionRoster() error: %s\n", strerror(err));
	SetCreationDirectory(dir.String());
	
	if (mSearchPath) mSearchPath->AddDirectory(dir.String());
	if (mReadOnlySearchPath) mReadOnlySearchPath->AddSearchPath("%A/Motions");
}

AmMotionRoster::~AmMotionRoster()
{
	for (uint32 k = 0; k < mMotions.size(); k++) {
		delete mMotions[k];
	}
	mMotions.resize(0);
}

status_t AmMotionRoster::GetMotionInfo(	uint32 index,
										BString& outLabel,
										BString& outKey,
										bool* outReadOnly,
										bool* outIsValid,
										BString* outDescription,
										BString* outAuthor,
										BString* outEmail,
										BString* outFilePath) const
{
	BAutolock	l(mEntryLock);
	if (index >= mMotions.size() ) return B_ERROR;
	outLabel = mMotions[index]->Label();
	outKey = mMotions[index]->Label();
	if (outReadOnly) *outReadOnly = mMotions[index]->IsReadOnly();
	if (outIsValid) *outIsValid = mMotions[index]->IsValid();
	if (outDescription) *outDescription = mMotions[index]->ShortDescription();
	if (outAuthor) *outAuthor = mMotions[index]->Author();
	if (outEmail) *outEmail = mMotions[index]->Email();
	if (outFilePath) *outFilePath = mMotions[index]->LocalFilePath();
	return B_OK;
}

AmMotionI* AmMotionRoster::NewMotion(const BString& key, const char* path) const
{
	for (uint32 k = 0; k < mMotions.size(); k++) {
		if (key == mMotions[k]->Label() ) {
			if (!path || (path && mMotions[k]->LocalFilePath() == path) )
				return mMotions[k]->Copy();
		}
	}
	return NULL;
}

uint32 AmMotionRoster::CountEntries() const
{
	BAutolock	l(mEntryLock);
	return mMotions.size();
}

BString AmMotionRoster::KeyAt(uint32 index) const
{
	BAutolock	l(mEntryLock);
	if (index >= mMotions.size() ) return BString();
	return mMotions[index]->Label();
}

status_t AmMotionRoster::EntryCreated(	const BMessage& msg,
										bool readOnly,
										const char* filePath)
{
	AmMotion*	m = new AmMotion(msg, readOnly, filePath);
	if (!m) return B_NO_MEMORY;
	BString		key = m->Key();
	BAutolock	l(&mEntryLock);
	/* Find if there is already a valid tool with
	 * this key.  If so, I'm a conflict, and not useable.
	 */
	for (uint32 k = 0; k < mMotions.size(); k++) {
		if (mMotions[k]->IsValid()
				&& mMotions[k]->Key() == key) {
			m->SetIsValid(false);
			break;
		}
	}

	mMotions.push_back(m);
	return B_OK;
}

status_t AmMotionRoster::EntryRemoved(const char* filePath)
{
	BAutolock	l(&mEntryLock);
	for (uint32 k = 0; k < mMotions.size(); k++) {
		if (mMotions[k]->LocalFilePath() == filePath) {
			delete mMotions[k];
			mMotions.erase(mMotions.begin() + k);
			return B_OK;
		}
	}
	return B_ERROR;
}

AmFileRosterEntryI* AmMotionRoster::EntryAt(const BString& key, const char* path) const
{
	for (uint32 k = 0; k < mMotions.size(); k++) {
		if (key == mMotions[k]->Label() ) {
			if (!path || (path && mMotions[k]->LocalFilePath() == path) )
				return mMotions[k];
		}
	}
	return 0;
}

AmFileRosterEntryI* AmMotionRoster::EntryAt(uint32 index) const
{
	if (index >= mMotions.size() ) return NULL;
	return mMotions[index];
}

AmFileRosterEntryI* AmMotionRoster::UniqueDuplicate(AmFileRosterEntryI* entry) const
{
	AmMotion*		m = dynamic_cast<AmMotion*>(entry);
	if (!m) return NULL;
	m = new AmMotion(*m);
	if (!m) return NULL;
	BString			uniqueLabel = UniqueLabelFrom(m->Label() );
	m->SetLabel(uniqueLabel);
	return m;	
}

const char* AmMotionRoster::MimeType() const
{
	return "application/x-vnd.Arp-sequitur-motion";
}

// #pragma mark -

/***************************************************************************
 * AM-MULTI-FILTER-ROSTER
 ***************************************************************************/
static int32				gMultiFilterRosterCreated = 0;
static AmMultiFilterRoster*	gMultiFilterRoster = NULL;

AmMultiFilterRoster* AmMultiFilterRoster::Default()
{
	if (atomic_or(&gMultiFilterRosterCreated, 1) == 0) {
		gMultiFilterRoster = new AmMultiFilterRoster();
	} else {
		while (!gMultiFilterRoster) sleep(20000);
	}
	return gMultiFilterRoster;
}

void AmMultiFilterRoster::ShutdownDefault(bool force_unload)
{
	if (gMultiFilterRoster) {
		delete gMultiFilterRoster;
		gMultiFilterRoster = NULL;
	}
}

AmMultiFilterRoster::AmMultiFilterRoster()
		: inherited("Multi Filters")
{
	BPath		path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() != B_OK) return;
	BString		dir(path.Path());
	dir << "/AngryRedPlanet/" << APP_NAME << "/MultiFilters";
	status_t	err = verify_directory(dir);
	if (err != B_OK) printf("AmMultiFilterRoster::AmMultiFilterRoster() error: %s\n", strerror(err));
	SetCreationDirectory(dir.String());
	
	if (mSearchPath) mSearchPath->AddDirectory(dir.String());
	if (mReadOnlySearchPath) mReadOnlySearchPath->AddSearchPath("%A/MultiFilters");
}

AmMultiFilterRoster::~AmMultiFilterRoster()
{
	mFilters.resize(0);
}

status_t AmMultiFilterRoster::GetFilterInfo(uint32 index,
											BString& outLabel,
											BString& outKey,
											bool* outReadOnly,
											BString* outAuthor,
											BString* outEmail) const
{
	BAutolock	l(mEntryLock);
	if (index >= mFilters.size() ) return B_ERROR;
	outLabel = mFilters[index]->Label();
	outKey = mFilters[index]->UniqueName();
	if (outReadOnly) *outReadOnly = mFilters[index]->IsReadOnly();
	if (outAuthor) *outAuthor = mFilters[index]->Author();
	if (outEmail) *outEmail = mFilters[index]->Email();
	return B_OK;
}

AmMultiFilterAddOn* AmMultiFilterRoster::NewFilter(const BString& key, const char* path) const
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (key == mFilters[k]->UniqueName() ) {
			if (!path || (path && mFilters[k]->LocalFilePath() == path) )
				return mFilters[k]->Copy();
		}
	}
	return NULL;
}

ArpRef<AmMultiFilterAddOn> AmMultiFilterRoster::FilterAt(uint32 index) const
{
	BAutolock	l(mEntryLock);
	if (index >= mFilters.size() ) return NULL;
	return mFilters[index];
//	return ArpRef<AmFilterAddOn>(mFilters[index]);
}

static float check_instantiation(	ArpRef<AmMultiFilterAddOn> addon,
									const BMessage* archive)
{
	// First check if they are of the same class.
	BString		myClass = addon->Key();
	if (AmFilterAddOn::MatchClassArchive(myClass.String(), archive) < 0) {
		return -1;
	}
	
	// Now let the addon do a thorough exam.
	return addon->CheckInstantiation(archive);
}

ArpRef<AmMultiFilterAddOn> AmMultiFilterRoster::FindFilterAddOn(const BMessage* archive) const
{
	BAutolock	l(mEntryLock);

	ArpRef<AmMultiFilterAddOn>	match = NULL;
	float						matchQuality = -1;
	
	for (uint32 i = 0; i < mFilters.size(); i++) {
		if (mFilters[i]) {
			float q = check_instantiation(mFilters[i], archive);
			if (q >= 0 && q > matchQuality) {
				matchQuality = q;
				match = mFilters[i];
			}
		}
	}
	return match;
}

ArpRef<AmMultiFilterAddOn> AmMultiFilterRoster::FindFilterAddOn(const char* uniqueName) const
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (mFilters[k]->UniqueName() == uniqueName)
			return mFilters[k];
	}
	return NULL;
}

#if 0
ArpRef<AmMultiFilterAddOn> AmMultiFilterRoster::FilterNamed(const BString& key) const
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (mFilters[k]->UniqueName() == key)
			return mFilters[k];
//			return ArpRef<AmFilterAddOn>(mFilters[k]);
	}
	return NULL;
}
#endif

uint32 AmMultiFilterRoster::CountEntries() const
{
	BAutolock	l(mEntryLock);
	return mFilters.size();
}

BString AmMultiFilterRoster::KeyAt(uint32 index) const
{
	BAutolock	l(mEntryLock);
	if (index >= mFilters.size() ) return BString();
	return mFilters[index]->UniqueName();
}

status_t AmMultiFilterRoster::EntryCreated(	const BMessage& msg,
											bool readOnly,
											const char* filePath)
{
	AmMultiFilterAddOn*	f = new AmMultiFilterAddOn(msg, readOnly, filePath);
	if (!f) return B_NO_MEMORY;
	BString		key = f->Key();
	BAutolock	l(mEntryLock);
	/* Find if there is already a valid filter with
	 * this key.  If so, I'm a conflict, and not useable.
	 */
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (mFilters[k]->IsValid()
				&& mFilters[k]->Key() == key) {
			f->SetIsValid(false);
			break;
		}
	}
	mFilters.push_back(f);
	return B_OK;
}

status_t AmMultiFilterRoster::EntryRemoved(const char* filePath)
{
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (mFilters[k]->LocalFilePath() == filePath) {
			mFilters.erase(mFilters.begin() + k);
			return B_OK;
		}
	}
	return B_ERROR;
}

AmFileRosterEntryI* AmMultiFilterRoster::EntryAt(const BString& key, const char* path) const
{
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (key == mFilters[k]->UniqueName() ) {
			if (!path || (path && mFilters[k]->LocalFilePath() == path) )
				return mFilters[k];
		}
	}
	return 0;
}

AmFileRosterEntryI* AmMultiFilterRoster::EntryAt(uint32 index) const
{
	if (index >= mFilters.size() ) return NULL;
	return mFilters[index];
}

AmFileRosterEntryI* AmMultiFilterRoster::UniqueDuplicate(AmFileRosterEntryI* entry) const
{
	AmMultiFilterAddOn*		m = dynamic_cast<AmMultiFilterAddOn*>(entry);
	if (!m) return NULL;
	m = new AmMultiFilterAddOn(*m);
	if (!m) return NULL;
	BString			uniqueKey = UniqueKeyFrom(m->Key() );
	m->SetKey(uniqueKey.String() );
	BString			uniqueLabel = UniqueLabelFrom(m->Label() );
	m->SetLabel(uniqueLabel.String() );
	return m;	
}

const char* AmMultiFilterRoster::MimeType() const
{
	return "application/x-vnd.Arp-sequitur-multifilter";
}

// #pragma mark -

/***************************************************************************
 * AM-TOOL-ROSTER
 ***************************************************************************/
static int32			gToolRosterCreated = 0;
static AmToolRoster*	gToolRoster = NULL;

AmToolRoster* AmToolRoster::Default()
{
	if (atomic_or(&gToolRosterCreated, 1) == 0) {
		gToolRoster = new AmToolRoster();
	} else {
		while (!gToolRoster) sleep(20000);
	}
	return gToolRoster;
}

void AmToolRoster::ShutdownDefault(bool force_unload)
{
	if (gToolRoster) {
		delete gToolRoster;
		gToolRoster = NULL;
	}
}

AmToolRoster::AmToolRoster()
		: inherited("Tools")
{
	/* The property tool is currently hardcoded.
	 */
	mToolRefs.push_back(new AmPropertiesTool());

	BPath		path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (path.InitCheck() != B_OK) return;
	BString		dir(path.Path());
	dir << "/AngryRedPlanet/" << APP_NAME << "/Tools";
	status_t	err = verify_directory(dir);
	if (err != B_OK) printf("AmToolRoster::AmToolRoster() error: %s\n", strerror(err));
	SetCreationDirectory(dir.String());
	
	if (mSearchPath) mSearchPath->AddDirectory(dir.String());
	if (mReadOnlySearchPath) mReadOnlySearchPath->AddSearchPath("%A/Tools");

	ResetToolMaps();
}

AmToolRoster::~AmToolRoster()
{
	mToolRefs.resize(0);
}

status_t AmToolRoster::GetToolInfo(	uint32 index,
									BString& outLabel,
									BString& outUniqueName,
									bool* outReadOnly,
									bool* outIsValid,
									BString* outAuthor,
									BString* outEmail,
									BString* outShortDescription,
									BString* outLongDescription,
									BBitmap** outIcon,
									BString* outFilePath) const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::GetToolInfo() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock	l(mEntryLock);
	if (index >= mToolRefs.size() ) return B_ERROR;
	outLabel = mToolRefs[index].mTool->Label();
	outUniqueName = mToolRefs[index].mTool->Key();
	if (outReadOnly) *outReadOnly = mToolRefs[index].mTool->IsReadOnly();
	if (outIsValid) *outIsValid = mToolRefs[index].mTool->IsValid();
	if (outAuthor) *outAuthor = mToolRefs[index].mTool->Author();
	if (outEmail) *outEmail = mToolRefs[index].mTool->Email();
	if (outShortDescription) *outShortDescription = mToolRefs[index].mTool->ShortDescription();
	if (outLongDescription) mToolRefs[index].mTool->LongDescription(*outLongDescription);
	if (outIcon) {
		*outIcon = NULL;
		if (mToolRefs[index].mTool->Icon() ) *outIcon = new BBitmap(mToolRefs[index].mTool->Icon() );
	}
	if (outFilePath) *outFilePath = mToolRefs[index].mTool->LocalFilePath().String();
	return B_OK;
}

AmTool* AmToolRoster::NewTool(const BString& key, const char* path) const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::NewTool() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock	l(mEntryLock);
	for (uint32 k = 0; k < mToolRefs.size(); k++) {
		if (key == mToolRefs[k].mTool->Key() ) {
			if (!path || (path && mToolRefs[k].mTool->LocalFilePath() == path) )
				return mToolRefs[k].mTool->Copy();
		}
	}
	return NULL;
}

AmToolRef AmToolRoster::ToolAt(uint32 index) const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::ToolAt() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock	l(mEntryLock);
	if (index >= mToolRefs.size() ) return AmToolRef();
	return mToolRefs[index];
}

AmToolRef AmToolRoster::Tool(uint32 button, uint32 modifierKeys) const
{
	if (modifierKeys&B_CONTROL_KEY) {
		if (button&B_PRIMARY_MOUSE_BUTTON) return FindTool(mPriCtrlToolName);
		else if (button&B_SECONDARY_MOUSE_BUTTON) return FindTool(mSecCtrlToolName);
		else if (button&B_TERTIARY_MOUSE_BUTTON) return FindTool(mTerCtrlToolName);
	}

	if (button&B_PRIMARY_MOUSE_BUTTON) return FindTool(mPriToolName);
	else if (button&B_SECONDARY_MOUSE_BUTTON) return FindTool(mSecToolName);
	else if (button&B_TERTIARY_MOUSE_BUTTON) return FindTool(mTerToolName);
	return AmToolRef();
}

AmToolRef AmToolRoster::FindTool(const BString& uniqueName) const
{
	ArpASSERT(uniqueName.Length() > 0);
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::FindTool() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock l(mEntryLock);
	for (uint32 k = 0; k < mToolRefs.size(); k++) {
		if (uniqueName == mToolRefs[k].ToolKey() ) {
			return mToolRefs[k];
		}
	}
	return AmToolRef();
}

AmToolRef AmToolRoster::FindTool(int32 key) const
{
	const char* toolKey;
	if (key == B_F1_KEY) toolKey = "arp:Pencil";
	else if (key == B_F2_KEY) toolKey = "arp:Select";
	else if (key == B_F3_KEY) toolKey = "arp:Wand";
	else if (key == B_F4_KEY) toolKey = "arp:Eraser";
	else return AmToolRef();
	return FindTool(toolKey);
}

void AmToolRoster::SetTool(const BString& toolName, uint32 button, uint32 modifierKeys)
{
	ArpASSERT(toolName.Length() > 0);
	ArpASSERT(FindTool(toolName.String()).IsValid() );
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::SetTool() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock l(mEntryLock);
	int32		sentButton = 0;
	if (button&B_PRIMARY_MOUSE_BUTTON) {
		if (modifierKeys&B_CONTROL_KEY) mPriCtrlToolName = toolName;
		else mPriToolName = toolName;
		sentButton = B_PRIMARY_MOUSE_BUTTON;
	} else if (button&B_SECONDARY_MOUSE_BUTTON) {
		if (modifierKeys&B_CONTROL_KEY) mSecCtrlToolName = toolName;
		else mSecToolName = toolName;
		sentButton = B_SECONDARY_MOUSE_BUTTON;
	} else if (button&B_TERTIARY_MOUSE_BUTTON) {
		if (modifierKeys&B_CONTROL_KEY) mTerCtrlToolName = toolName;
		else mTerToolName = toolName;
		sentButton = B_TERTIARY_MOUSE_BUTTON;
	}

	if (sentButton != 0) {
		BMessage	msg(AM_FILE_ROSTER_CHANGED);
		msg.AddString("roster", Name() );
		msg.AddString("tool_name", toolName);
		msg.AddInt32("button", sentButton);
		mNotifier.ReportMsgChange(&msg, BMessenger());
	}
}

#if 0
// Didn't look like this was being used...  If it's put back in, needs
// to include the modifier key.
	status_t				GetToolMapping(const BString& toolName, uint32* button) const;
status_t AmToolRoster::GetToolMapping(	const BString& toolName, uint32* button) const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::GetToolMapping() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock l(mEntryLock);
	if (toolName == mPriToolName) {
		*button = B_PRIMARY_MOUSE_BUTTON;
		return B_OK;
	} else if (toolName == mSecToolName) {
		*button = B_SECONDARY_MOUSE_BUTTON;
		return B_OK;
	} else if (toolName == mTerToolName) {
		*button = B_TERTIARY_MOUSE_BUTTON;
		return B_OK;
	}
	return B_ERROR;
}
#endif

uint32 AmToolRoster::CountEntries() const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::CountEntries() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock	l(mEntryLock);
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("\tAmToolRoster::CountEntries() Acquired mEntryLock\n"); fflush(stdout);
	#endif
	return mToolRefs.size();
}

BString AmToolRoster::KeyAt(uint32 index) const
{
	#ifdef AM_TRACE_ENTRY_LOCK
	printf("AmToolRoster::KeyAt() Acquire mEntryLock\n"); fflush(stdout);
	#endif
	BAutolock	l(mEntryLock);
	if (index >= mToolRefs.size() ) return BString();
	return mToolRefs[index].ToolKey();
}

bool AmToolRoster::KeyExists(const char* key) const
{
	for (uint32 k = 0; k < mToolRefs.size(); k++) {
		if (mToolRefs[k].mTool->Key() == key) {
			return true;
		}
	}
	return false;
}

status_t AmToolRoster::EntryCreated(	const BMessage& msg,
										bool readOnly,
										const char* filePath)
{
	AmTool*	t = new AmTool(msg, readOnly, filePath);
	if (!t) return B_NO_MEMORY;
	BString		key = t->Key();
	{
		#ifdef AM_TRACE_ENTRY_LOCK
		printf("AmToolRoster::EntryCreated() Acquire mEntryLock\n"); fflush(stdout);
		#endif
		BAutolock	l(mEntryLock);
		/* Find if there is already a valid tool with
		 * this key.  If so, I'm a conflict, and not useable.
		 */
		for (uint32 k = 0; k < mToolRefs.size(); k++) {
			if (mToolRefs[k].mTool->IsValid()
					&& mToolRefs[k].mTool->Key() == key) {
				t->SetIsValid(false);
				break;
			}
		}
		mToolRefs.push_back(t);
		#ifdef AM_TRACE_ENTRY_LOCK
		printf("\tAmToolRoster::EntryCreated() Release mEntryLock\n"); fflush(stdout);
		#endif
	}
	/* Update any toolbars that might already contain this tool.
	 * A toolbar will contain a tool that is just created if the
	 * user just modified an existing tool, which appears as a delete
	 * then add.
	 */
	AmToolBarRef	toolBarRef;
	for (uint32 j = 0; (toolBarRef = AmGlobals().ToolBarAt(j, false)).IsValid(); j++) {
		// WRITE TOOL BAR BLOCK
		AmToolBar*	bar = toolBarRef.WriteLock();
		if (bar) bar->ToolChange(key);
		toolBarRef.WriteUnlock(bar);
		// END WRITE TOOL BAR BLOCK
	}
	return B_OK;
}

status_t AmToolRoster::EntryRemoved(const char* filePath)
{
	status_t		err = B_ERROR;
	BString			key;
	{
		#ifdef AM_TRACE_ENTRY_LOCK
		printf("AmToolRoster::EntryRemoved() Acquire mEntryLock\n"); fflush(stdout);
		#endif
		BAutolock	l(mEntryLock);
		for (uint32 k = 0; k < mToolRefs.size(); k++) {
			if (mToolRefs[k].mTool->LocalFilePath() == filePath) {
				key = mToolRefs[k].ToolKey();
				mToolRefs.erase(mToolRefs.begin() + k);
				err = B_OK;
				break;
			}
		}
	}

	/* Tell the tool bars to refresh -- don't actually remove the
	 * tool from the tool bar, in case the user is just modifying
	 * the tool (which shows up as a delete and add).  Any invalid
	 * tools get scrubbed from the toolbars when the application exists.
	 */
	if (key.Length() > 0) {
		AmToolBarRef	toolBarRef;
		for (uint32 j = 0; (toolBarRef = AmGlobals().ToolBarAt(j, false)).IsValid(); j++) {
			// WRITE TOOL BAR BLOCK
			AmToolBar*	bar = toolBarRef.WriteLock();
			if (bar) bar->ToolChange(key);
			toolBarRef.WriteUnlock(bar);
			// END WRITE TOOL BAR BLOCK
		}
	}
	return err;
}

AmFileRosterEntryI* AmToolRoster::EntryAt(const BString& key, const char* path) const
{
	for (uint32 k = 0; k < mToolRefs.size(); k++) {
		if (key == mToolRefs[k].mTool->Key() ) {
			if (!path || (path && mToolRefs[k].mTool->LocalFilePath() == path) )
				return mToolRefs[k].mTool;
		}
	}
	return 0;
}

AmFileRosterEntryI* AmToolRoster::EntryAt(uint32 index) const
{
	if (index >= mToolRefs.size() ) return NULL;
	return mToolRefs[index].mTool;
}

AmFileRosterEntryI* AmToolRoster::UniqueDuplicate(AmFileRosterEntryI* entry) const
{
	AmTool*		t = dynamic_cast<AmTool*>(entry);
	if (!t) return NULL;
	t = t->Copy();
	if (!t) return NULL;
	BString		uniqueKey = UniqueKeyFrom(t->Key() );
	t->SetKey(uniqueKey.String() );
	BString		uniqueLabel = UniqueLabelFrom(t->Label() );
	t->SetLabel(uniqueLabel.String() );
	return t;	
}

const char* AmToolRoster::MimeType() const
{
	return "application/x-vnd.Arp-sequitur-tool";
}

void AmToolRoster::ResetToolMaps()
{
	mPriToolName = "arp:Pencil";
	mSecToolName = "arp:Properties";
	mTerToolName = "arp:Wand";

	mSecCtrlToolName = "arp:Properties";
	
	if (!FindTool(mPriToolName).IsValid()) {
		if (mToolRefs.size() > 0) mPriToolName = mToolRefs[0].ToolKey();
	}
	if (!FindTool(mTerToolName).IsValid()) {
		if (mToolRefs.size() > 1) mTerToolName = mToolRefs[1].ToolKey();
	}
}

