#include <AddOnManager.h>

#include <Autolock.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Node.h>
#include <Path.h>
#include <Volume.h>
#include <String.h>
#include <DataIO.h>

#include <fs_attr.h>

#include <Debug.h>

#include <Gehnaphore.h>

#include <string.h>
#include <time.h>
#include <stdlib.h>

#define DEBUG_IMAGE_OPEN DEBUG
#define DEBUG_IMAGE_LOAD 1

// --------------------------- BAddOnHandle ---------------------------

enum {
	// Returned by ReadIdentifiers() to indicate that the attributes
	// specify this is not an addon for us.
	IMAGE_NOT_ADDON		= 1
};

BAddOnHandle::BAddOnHandle(const entry_ref* entry, const node_ref* node)
	: fRefCount(0),
	  fNode(node ? *node : node_ref()),
	  fEntry(entry ? *entry : entry_ref()),
	  fImage(B_ERROR), fImageErr(B_OK),
	  fUsers(0), fLastUsed(0),
	  fHasIdentifiers(false), fStaticImage(false)
{
	init_gehnaphore(&fAccess);
	PRINT(("Creating BAddOnHandle %p (%s)\n", this, ref.name));
}

BAddOnHandle::~BAddOnHandle()
{
#if DEBUG_IMAGE_OPEN
	printf("Deleting: %p (%s), image=%ld, users=%ld\n",
			this, fEntry.name, fImage, fUsers);
#endif
	do_flush(B_FLUSH_KEEP_LOADED|B_FLUSH_DYNAMIC);
}

int32 BAddOnHandle::Acquire() const
{
	return atomic_add(&fRefCount, 1);
}

int32 BAddOnHandle::Release() const
{
	int32 last = atomic_add(&fRefCount, -1);
	if (last == 1) delete this;
	return last;
}

node_ref BAddOnHandle::NodeRef() const
{
	return fNode;
}

entry_ref BAddOnHandle::EntryRef() const
{
	return fEntry;
}

status_t BAddOnHandle::SetEntryRef(const entry_ref* ref)
{
	lock_gehnaphore(&fAccess);
	fEntry = *ref;
	unlock_gehnaphore(&fAccess);
	return B_OK;
}
	
image_id BAddOnHandle::Open()
{
	lock_gehnaphore(&fAccess);
	image_id ret = do_open();
	unlock_gehnaphore(&fAccess);
	return ret;
}

image_id BAddOnHandle::do_open()
{
#if DEBUG_IMAGE_OPEN
	printf("Open(): %x (%s)\n", this, fEntry.name);
#endif
	
	// Update the time this add-on was last opened.
	fLastUsed = (time_t)(system_time()/1000000);
	
	if (fImage >= B_OK) {
		atomic_add(&fUsers, 1);
		Acquire();
#if DEBUG_IMAGE_OPEN
		printf("Already loaded, user count is now %ld\n", fUsers);
#endif
		return fImage;
	}
	
	if (fImageErr < B_OK) return fImageErr;
	
	BPath path;
	
	if (fEntry != entry_ref()) {
		// Load image, if there is an entry_ref for it.
		BEntry entry(&fEntry);
		if ((fImageErr=entry.InitCheck()) != B_OK) {
			printf("\"%s\" can't make entry: %s\n", fEntry.name, strerror(fImageErr));
			return fImageErr;
		}
		
		if ((fImageErr=path.SetTo(&entry)) != B_OK) {
			printf("\"%s\" can't make path: %s\n", fEntry.name, strerror(fImageErr));
			return fImageErr;
		}
		
		fImage = load_add_on(path.Path());
		fStaticImage = false;
#if DEBUG_IMAGE_LOAD
		printf("Loaded image %ld (%s) (addon %p), err=%s\n",
				fImage, fEntry.name, this, strerror(fImage < B_OK ? fImage : B_OK));
#endif

	} else {
		// Mark this as a static add-on.
		fImage = B_OK;
		fStaticImage = true;
	}
	
	if (fImage < B_OK) {
		printf("\"%s\" was not an image: %s\n", path.Path(), strerror(fImage));
		fImageErr = fImage;
	} else {
		atomic_add(&fUsers, 1);
		Acquire();
		ImageLoaded(fImage);
	}
	
#if DEBUG_IMAGE_OPEN
	printf("Loaded, user count is now %ld\n", fUsers);
#endif
	
	return fImage;
}

void BAddOnHandle::Close()
{
	lock_gehnaphore(&fAccess);
	if (fImage >= B_OK) do_close();
	unlock_gehnaphore(&fAccess);
	
	Release();
}

void BAddOnHandle::do_close()
{
	atomic_add(&fUsers, -1);
#if DEBUG_IMAGE_OPEN
	printf("Close(): %x (%s), users now=%ld\n", this, fEntry.name, fUsers);
#endif
}

bool BAddOnHandle::Flush(uint32 flags)
{
	lock_gehnaphore(&fAccess);
	bool res = do_flush(flags);
	unlock_gehnaphore(&fAccess);
	return res;
}

bool BAddOnHandle::do_flush(uint32 flags)
{
#if DEBUG_IMAGE_OPEN
	printf("Flush(): %x (%s), image=%ld, users=%ld\n",
		   this, fEntry.name, fImage, fUsers);
#endif

	if (((flags&B_FLUSH_KEEP_LOADED) == 0 && KeepLoaded()) ||
			((flags&B_FLUSH_DYNAMIC) == 0 && IsDynamic())) {
		PRINT(("*** Add-on wants to stay loaded.\n"));
		return false;
	}
	
	if (((flags&B_FLUSH_REFERENCED) != 0 || fUsers <= 0) && fImage >= B_OK) {
		ImageUnloading(fImage);
		if (!fStaticImage) {
#if DEBUG_IMAGE_LOAD
			printf("Unloading image %ld (%s) (addon %p)\n",
					fImage, fEntry.name, this);
#endif
			unload_add_on(fImage);
		}
		fImage = B_ERROR;
		fStaticImage = false;
		return true;
	}
	
	return false;
}

bool BAddOnHandle::IsLoaded() const
{
	return fImage >= B_OK ? true : false;
}

bool BAddOnHandle::IsStatic() const
{
	return fStaticImage;
}

bool BAddOnHandle::KeepLoaded() const
{
	return fStaticImage;
}

bool BAddOnHandle::IsDynamic() const
{
	return false;
}

size_t BAddOnHandle::GetMemoryUsage() const
{
	if (fImage < B_OK || fStaticImage) return 0;
	
	image_info ii;
	if (get_image_info(fImage, &ii) != B_OK) return 0;
	
	// NOTE: This doesn't take into account any shared libraries
	// the image may have dragged along with it.
	return ii.text_size + ii.data_size;
}

status_t BAddOnHandle::GetIdentifiers(BMessage* into, bool quick) const
{
	lock_gehnaphore(&fAccess);
	status_t ret = const_cast<BAddOnHandle*>(this)->do_get_identifiers(into, quick);
	unlock_gehnaphore(&fAccess);
	return ret;
}

status_t BAddOnHandle::do_get_identifiers(BMessage* into, bool quick)
{
	if (fHasIdentifiers) {
		if (into) *into = fIdentifiers;
		return B_OK;
	}
	
	if (fImageErr != B_OK) return fImageErr;
	
	status_t err = B_OK;
	BNode node;
	
	// If this handle has an entry_ref, first try to retrieve its
	// identifiers from its attributes.
	if (fEntry != entry_ref()) {
		
		PRINT(("Retrieving identifers for %s\n", fRef.name));
		
		err = node.SetTo(&fEntry);
		
		if (err == B_OK) {
			PRINT(("Trying to read attributes...\n"));
			err = do_read_identifiers(&fIdentifiers, &node);
			if (err == B_OK) {
				#if DEBUG
				PRINT(("Found :")); fIdentifiers.PrintToStream();
				#endif
				fHasIdentifiers = true;
				if (into) *into = fIdentifiers;
				return B_OK;
			} else if (err == IMAGE_NOT_ADDON) {
				PRINT(("*** This is a bad add-on.\n"));
				fImageErr = B_BAD_IMAGE_ID;
				return fImageErr;
			}
		}
	
	}
	
	// If quick is requested and the add-on lives in a file,
	// then bail here rather than loading it in.
	if (quick && fEntry != entry_ref()) return err;
	
	// Finally try to load in add-on image and retrieve
	// attributes from it.  If this add-on does not have an
	// entry_ref, it will just call LoadIdentifiers() without
	// actually opening an image.	
	PRINT(("Loading attributes from image.\n"));
	image_id image = do_open();
	if (image < B_OK) return image;
	
	err = LoadIdentifiers(&fIdentifiers, image);
	do_close();
	
	// And now, if this add-on lives in a file and is not
	// dynamic, write the newly found identifiers into its
	// attribute.
	if (!IsDynamic()) {
		if (err == B_OK) {
			fHasIdentifiers = true;
			if (into) *into = fIdentifiers;
			if (!fIdentifiers.IsEmpty() && fEntry != entry_ref()) {
				do_write_identifiers(&node, &fIdentifiers);
			}
		} else if (fEntry != entry_ref()) {
			do_write_identifiers(&node, 0);
		}
	}
	
	return err;
}

bool BAddOnHandle::MatchIdentifier(const char* name, const char* value,
									bool quick) const
{
	lock_gehnaphore(&fAccess);
	if (const_cast<BAddOnHandle*>(this)->do_get_identifiers(0, quick) != B_OK) {
		unlock_gehnaphore(&fAccess);
		return false;
	}
	
	const char* cur;
	for (int32 i=0; fIdentifiers.FindString(name, i, &cur) == B_OK; i++) {
		if (strcasecmp(cur, value) == 0) {
			unlock_gehnaphore(&fAccess);
			return true;
		}
	}
	
	unlock_gehnaphore(&fAccess);
	
	return false;
}

time_t BAddOnHandle::SecondsSinceOpen() const
{
	lock_gehnaphore(&fAccess);
	time_t now = (time_t)(system_time()/1000000);
	time_t ret = now - fLastUsed;
	unlock_gehnaphore(&fAccess);
	return ret;
}

void BAddOnHandle::ImageLoaded(image_id image)
{
	(void)image;
}

status_t BAddOnHandle::LoadIdentifiers(BMessage* into, image_id from)
{
	(void)into;
	(void)from;
	return B_OK;
}

void BAddOnHandle::ImageUnloading(image_id image)
{
	(void)image;
}

const char* BAddOnHandle::AttrBaseName() const
{
	return "be:addon";
}

const BMessage* BAddOnHandle::LockIdentifiers(bool quick) const
{
	lock_gehnaphore(&fAccess);
	status_t ret = const_cast<BAddOnHandle*>(this)->do_get_identifiers(NULL, quick);
	if (ret != B_OK) {
		unlock_gehnaphore(&fAccess);
		return NULL;
	}
	
	return &fIdentifiers;
}

void BAddOnHandle::UnlockIdentifiers(const BMessage* ident) const
{
	if (!ident) return;
	if (ident != &fIdentifiers) {
		debugger("Bad identifier BMessage returned");
		return;
	}
	
	unlock_gehnaphore(&fAccess);
}

void BAddOnHandle::Lock() const
{
	lock_gehnaphore(&fAccess);
}

void BAddOnHandle::Unlock() const
{
	unlock_gehnaphore(&fAccess);
}

status_t BAddOnHandle::do_read_identifiers(BMessage* into, BNode* from)
{
	const char* name = AttrBaseName();
	
	void* data = 0;
	size_t size = (size_t)-1;
	
	ssize_t err = B_OK;
	
	attr_info ai;
	ai.size = 0;
	while( err >= B_OK && ai.size != size) {
		err = from->GetAttrInfo(name, &ai);
	
		if (err >= B_OK && ai.type != B_RAW_TYPE) {
			err = B_BAD_VALUE;
			break;
		}
		
		if (err >= B_OK) {
			size = ai.size;
			if (!data) data = malloc(size);
			else data = realloc(data, size);
			
			if (!data) err = B_NO_MEMORY;
		}
		
		if (err >= B_OK) err = from->ReadAttr(name, B_RAW_TYPE, 0, data, size);
		ai.size = err;
		
		PRINT(("Read attribute %s, size=%ld (err=%s)\n",
				name, err, strerror(err)));
				
		if (err >= B_OK && err == (ssize_t)size) {
			err = from->GetAttrInfo(name, &ai);
		}
		
		PRINT(("Initial attr size was %ld, new size is %ld\n",
				size, ai.size));
	}
	
	if (err < B_OK) {
		free(data);
		return err;
	}
	
	BMemoryIO io(data, size);
	
	time_t when;
	err = io.Read(&when, sizeof(when));
	
	// Determine if the cached identifier is out of date.  The logic is:
	// * If the cached "last modification time" is 0, then the identifier
	//   is never out of date.
	// * If the volume of this add-on is read-only, then the identifier is
	//   never out of date.
	// * Otherwise, compare the file's current modification time with the
	//   cached modification time to determine if it is out of date.
	if (err >= B_OK && when != 0) {
		struct stat st;
		err = from->GetStat(&st);
		
		// By default, assume that the cached time is valid.
		time_t cwhen = when;
		
		// If volume of this add-on isn't read-only, use the file's
		// modification time.
		if (err >= B_OK) {
			BVolume volume(st.st_dev);
			if (volume.InitCheck() == B_OK) {
				if (!volume.IsReadOnly()) {
					cwhen = st.st_mtime;
				}
			}
		}
		
		// No error if cached identifier is not out of date; otherwise,
		// return error code so that the add-on will be loaded.
		if (cwhen <= when) {
			err = B_OK;
		} else {
			PRINT(("Add-on attributes are out of date: %ld is now %ld\n",
					when, cwhen));
			err = B_BAD_VALUE;
		}
	}
	
	PRINT(("Ready to unflatten message; position=%ld, size=%ld\n",
			(int32)io.Position(), size));
			
	if (err >= B_OK) {
		// If nothing else in the attribute, it is not a valid add-on.
		if (io.Position() >= size) {
			PRINT(("*** This is not a valid add-on\n"));
			err = IMAGE_NOT_ADDON;
		} else {
			err = B_OK;
		}
	}
	
	if (err == B_OK) err = into->Unflatten(&io);
	free(data);
	
	PRINT(("Result from reading identifier from attribute: %s\n",
			strerror(err)));
	
	return err;
}

status_t BAddOnHandle::do_write_identifiers(BNode* into, const BMessage* from)
{
	const char* name = AttrBaseName();
	ssize_t err = B_OK;

	BMallocIO io;
	
	time_t cwhen;
	err = into->GetModificationTime(&cwhen);
	if (err >= B_OK) err = io.Write(&cwhen, sizeof(cwhen));
	
	if (err >= B_OK && from) err = from->Flatten(&io);
	
	if (err >= B_OK) {
		err = into->WriteAttr(name, B_RAW_TYPE, 0, io.Buffer(), io.BufferLength());
		if (err != sizeof(io.BufferLength())) err = B_DEVICE_FULL;
	}
	
	if (err >= B_OK) into->SetModificationTime(cwhen);
	
	return err;
}

void		BAddOnHandle::_HoldTheAddOnHandle1() {}
void		BAddOnHandle::_HoldTheAddOnHandle2() {}
void		BAddOnHandle::_HoldTheAddOnHandle3() {}
void		BAddOnHandle::_HoldTheAddOnHandle4() {}
void		BAddOnHandle::_HoldTheAddOnHandle5() {}
void		BAddOnHandle::_HoldTheAddOnHandle6() {}
void		BAddOnHandle::_HoldTheAddOnHandle7() {}
void		BAddOnHandle::_HoldTheAddOnHandle8() {}
void		BAddOnHandle::_HoldTheAddOnHandle9() {}
void		BAddOnHandle::_HoldTheAddOnHandle10() {}
void		BAddOnHandle::_HoldTheAddOnHandle11() {}
void		BAddOnHandle::_HoldTheAddOnHandle12() {}
void		BAddOnHandle::_HoldTheAddOnHandle13() {}
void		BAddOnHandle::_HoldTheAddOnHandle14() {}
void		BAddOnHandle::_HoldTheAddOnHandle15() {}
void		BAddOnHandle::_HoldTheAddOnHandle16() {}

// --------------------------- AddOnSearchPath ---------------------------

namespace BPrivate {
class AddOnSearchPath : public BLooper, public BSearchPath
{
public:
	AddOnSearchPath(BAddOnManager& manager, const char* name)
		: BLooper(name),
		  fManager(manager)
	{
	}
	virtual ~AddOnSearchPath()
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
			fManager.send_notices();
			return;
		}
		
		fManager.send_notices();
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
		BAutolock l(fManager.Locker());
		fManager.InstallAddOn(entry, node);
	}
	
	virtual void EntryMoved(const node_ref* node, const entry_ref* entry,
							const entry_ref* /*oldEntry*/)
	{
		BAutolock l(fManager.Locker());
		BAddOnHandle* addon = fManager.FindAddOn(node);
		if (addon) {
			addon->SetEntryRef(entry);
			addon->Release();
			fManager.mark_changed(false);
		}
	}
	
	virtual void EntryRemoved(const node_ref* node, const entry_ref* /*entry*/)
	{
		BAutolock l(fManager.Locker());
		BAddOnHandle* addon = fManager.FindAddOn(node);
		if (addon) {
			fManager.RemoveAddOn(addon);
			addon->Release();
		}
	}

private:
	BAddOnManager& fManager;
};
}

using namespace BPrivate;

// --------------------------- BAddOnManager ---------------------------

BAddOnManager::BAddOnManager(const char* name)
	: fLock(name), fSearchPath(NULL), fRunning(false)
{
	BString looperName(name);
	looperName += "_looper";
	fSearchPath = new AddOnSearchPath(*this, looperName.String());
}

BAddOnManager::~BAddOnManager()
{
	Shutdown();
}

status_t BAddOnManager::AddDirectory(const BEntry* dir)
{
	BAutolock l(Locker());
	return fSearchPath->AddDirectory(dir);
}

status_t BAddOnManager::AddDirectory(const entry_ref* dir)
{
	BAutolock l(Locker());
	return fSearchPath->AddDirectory(dir);
}

status_t BAddOnManager::AddDirectory(const char* dir, const char* leaf)
{
	BAutolock l(Locker());
	return fSearchPath->AddDirectory(dir, leaf);
}

status_t BAddOnManager::AddDirectory(directory_which which, const char* leaf)
{
	BAutolock l(Locker());
	return fSearchPath->AddDirectory(which, leaf);
}

status_t BAddOnManager::AddSearchPath(const char* path, const char* leaf)
{
	BAutolock l(Locker());
	return fSearchPath->AddSearchPath(path, leaf);
}

status_t BAddOnManager::AddEnvVar(const char* name, const char* leaf,
				   const char* defEnvVal)
{
	BAutolock l(Locker());
	return fSearchPath->AddEnvVar(name, leaf, defEnvVal);
}

status_t BAddOnManager::Run()
{
	if (IsRunning()) return B_OK;
	
	BAutolock l(Locker());
	if (IsRunning()) return B_OK;
	
	PRINT(("Starting addon manager\n"));
	if (!fSearchPath) return B_NO_MEMORY;
	
	status_t res = fSearchPath->Run();
	if (res < B_OK) return res;
	
	fRunning = fSearchPath->Thread() >= B_OK ? true : false;
	
	return B_OK;
}

bool BAddOnManager::IsRunning() const
{
	return fRunning;
}

static bool unload_addon_func(void* item)
{
	PRINT(("Unloading addon %p\n", item));
	reinterpret_cast<BAddOnHandle*>(item)->Flush();
	return false;
}

static bool force_unload_addon_func(void* item)
{
	PRINT(("Unloading addon %p\n", item));
	reinterpret_cast<BAddOnHandle*>(item)->Flush(
		B_FLUSH_KEEP_LOADED|B_FLUSH_DYNAMIC|B_FLUSH_REFERENCED);
	return false;
}

static bool free_addon_func(void* item)
{
	PRINT(("Deleting addon %p\n", item));
	reinterpret_cast<BAddOnHandle*>(item)->Release();
	return false;
}

void BAddOnManager::Shutdown(bool force_unload)
{
	PRINT(("Shutting down BAddOnManager, with %ld add-ons\n",
			fAddOns.CountItems()));
	Locker()->Lock();
	AddOnSearchPath* sp = fSearchPath;
	fSearchPath = NULL;
	Locker()->Unlock();
	
	// NOTE WELL: Need to stop the looper WITHOUT acquiring the lock
	// for the add on manager, to avoid deadlocks.
	if (sp && sp->Lock()) {
		sp->Quit();
		sp = NULL;
	}
	
	{
		BAutolock l(Locker());
		BList addons = fAddOns;
		fAddOns.MakeEmpty();
		if (force_unload)
			addons.DoForEach(force_unload_addon_func);
		else
			addons.DoForEach(unload_addon_func);
		addons.DoForEach(free_addon_func);
	}
}

// ** THIS IS GROSS.  DON'T LOOK HERE! **

// (stolen from Handler.cpp)
static const uint32 kStartObserving = 'OBST';
static const uint32 kStopObserving = 'OBSP';

#define kObserveTarget "be:observe_target"

status_t BAddOnManager::StartWatching(BMessenger receiver)
{
	if (!IsRunning()) return B_NO_INIT;

	BMessage message(kStartObserving);
	message.AddMessenger(kObserveTarget, receiver);
	message.AddInt32(B_OBSERVE_WHAT_CHANGE, B_ADD_ONS_CHANGED);
	return BMessenger(fSearchPath).SendMessage(&message, (BHandler *)NULL, 1000000);
}

status_t BAddOnManager::StopWatching(BMessenger receiver)
{
	if (!IsRunning()) return B_NO_INIT;

	BMessage message(kStopObserving);
	message.AddMessenger(kObserveTarget, receiver);
	message.AddInt32(B_OBSERVE_WHAT_CHANGE, B_ADD_ONS_CHANGED);
	return BMessenger(fSearchPath).SendMessage(&message, (BHandler *)NULL, 1000000);
}

void BAddOnManager::AddHandler(BHandler *handler)
{
	if (fLock.LockingThread() == find_thread(NULL)) {
		debugger("Don't call AddHandler() with the manager locked.");
	}
	
	BAutolock _l(fSearchPath);
	fSearchPath->AddHandler(handler);
}

bool BAddOnManager::RemoveHandler(BHandler *handler)
{
	if (fLock.LockingThread() == find_thread(NULL)) {
		debugger("Don't call AddHandler() with the manager locked.");
	}
	
	BAutolock _l(fSearchPath);
	if (fSearchPath)
		return fSearchPath->RemoveHandler(handler);
	return false;
}

int32 BAddOnManager::CountAddOns() const
{
	return fAddOns.CountItems();
}

BAddOnHandle* BAddOnManager::AddOnAt(int32 i) const
{
	BAutolock l(Locker());
	BAddOnHandle* h = reinterpret_cast<BAddOnHandle*>(fAddOns.ItemAt(i));
	return h;
}

BAddOnHandle* BAddOnManager::FindAddOn(const node_ref* node) const
{
	if (!node) return NULL;
	
	BAutolock l(Locker());
	
	for (int32 i=0; i<fAddOns.CountItems(); i++) {
		BAddOnHandle* h = AddOnAt(i);
		if (h && h->NodeRef() == *node) {
			return h;
		}
	}
	
	return NULL;
}

BAddOnHandle* BAddOnManager::InstallAddOn(const entry_ref* entry,
										  const node_ref* node)
{
	BAddOnHandle* addon = InstantiateHandle(entry, node);
	if (addon) InstallAddOn(addon);
	return addon;
}

void BAddOnManager::InstallAddOn(BAddOnHandle* addon)
{
	BAutolock l(Locker());
	if (addon) {
		addon->Acquire();
		fAddOns.AddItem(addon);
		mark_changed();
	}
}

bool BAddOnManager::RemoveAddOn(BAddOnHandle* addon)
{
	BAutolock l(Locker());
	if (addon) {
		bool res = fAddOns.RemoveItem(addon);
		if (res) {
			addon->Release();
			mark_changed();
		}
	}
	return false;
}

BLocker* BAddOnManager::Locker() const
{
	return &fLock;
}

size_t BAddOnManager::GetMemoryUsage() const
{
	BAutolock l(const_cast<BAddOnManager*>(this)->Locker());
	
	size_t amount = 0;
	for (int32 i=fAddOns.CountItems()-1; i>=0; i--) {
		BAddOnHandle* h = const_cast<BAddOnManager*>(this)->AddOnAt(i);
		if (h) amount += h->GetMemoryUsage();
	}
	
	return amount;
}

void BAddOnManager::UsingAddOn(int32 i)
{
	// NOTE: Since we are doing the easy thing and using a BList
	// to manage the add-ons, this is pretty brain-dead.  But for
	// a small set of add-ons, it shouldn't be a problem.
	// In the future, maybe the add-on handlers should be stored in
	// a doubly-linked list.
	if (i > 0) {
		BAutolock l(Locker());
		fAddOns.MoveItem(i, 0);
	}
}

ssize_t BAddOnManager::PruneAddOnMemory(ssize_t memory_needed)
{
	BAutolock l(Locker());
	
	PRINT(("Attempting to prune %.1fk of addon memory.\n", (float) memory_needed / 1024));
	for (int32 i=fAddOns.CountItems()-1; i>=0; i--) {
		BAddOnHandle* h = AddOnAt(i);
		if (h && h->IsLoaded() && !h->KeepLoaded() && !h->IsDynamic()) {
			const ssize_t amount = h->GetMemoryUsage();
			const bool flushed = h->Flush();
			PRINT(("Attempted to prune %p (%s): amount=%.1fk, flushed=%ld\n",
					h, h->EntryRef().name, (float) amount / 1024, (int32)flushed));
					
			if (flushed) {
				printf("Pruned add-on %p (%s), using %.1fk\n",
						h, h->EntryRef().name, (float) amount / 1024);
				if (memory_needed != -1) {
					if (amount > memory_needed) return 0;
					memory_needed -= amount;
				}
			}
			
		}
	}
	
	return memory_needed;
}

int32 BAddOnManager::PruneAddOnTime(time_t min_seconds, int32 growth)
{
	BAutolock l(Locker());
	
	const time_t base_seconds = min_seconds;
	int32 count=0;
	
	PRINT(("Attempting to prune add-ons at least %ld secs old.\n", min_seconds));
	for (int32 i=fAddOns.CountItems()-1; i>=0; i--) {
		BAddOnHandle* h = AddOnAt(i);
		if (h && h->IsLoaded() && !h->KeepLoaded() && !h->IsDynamic()) {
			const time_t age = h->SecondsSinceOpen();
			const bool flushed = age >= min_seconds ? h->Flush() : false;
			PRINT(("Thought about pruning %p (%s): age=%ld, min_seconds=%ld, flushed=%ld\n",
					h, h->EntryRef().name, age, min_seconds, (int32)flushed));
			
			if (flushed) {
				printf("Pruned add-on %p (%s) at index %ld with age %ld (min=%ld)\n",
						h, h->EntryRef().name, i, age, min_seconds);
				count++;
			}
			
			min_seconds += (base_seconds*growth)/100;
		}
	}
	
	PRINT(("Pruned %ld add-ons\n", count));
	
	return count;
}

BAddOnHandle* BAddOnManager::InstantiateHandle(const entry_ref* entry,
											   const node_ref* node)
{
	return new BAddOnHandle(entry, node);
}

void BAddOnManager::mark_changed(bool needWakeup)
{
	if (atomic_or(&fChanged, 1) == 0 && IsRunning() && needWakeup) {
		BMessenger(fSearchPath).SendMessage(B_ADD_ONS_CHANGED);
	}
}

void BAddOnManager::send_notices()
{
	if (atomic_and(&fChanged, 0) != 0) {
		BAutolock _l(fSearchPath);
		fSearchPath->SendNotices(B_ADD_ONS_CHANGED);
	}
}

void		BAddOnManager::_AmyTheAddOnManager1() {}
void		BAddOnManager::_AmyTheAddOnManager2() {}
void		BAddOnManager::_AmyTheAddOnManager3() {}
void		BAddOnManager::_AmyTheAddOnManager4() {}
void		BAddOnManager::_AmyTheAddOnManager5() {}
void		BAddOnManager::_AmyTheAddOnManager6() {}
void		BAddOnManager::_AmyTheAddOnManager7() {}
void		BAddOnManager::_AmyTheAddOnManager8() {}
void		BAddOnManager::_AmyTheAddOnManager9() {}
void		BAddOnManager::_AmyTheAddOnManager10() {}
void		BAddOnManager::_AmyTheAddOnManager11() {}
void		BAddOnManager::_AmyTheAddOnManager12() {}
void		BAddOnManager::_AmyTheAddOnManager13() {}
void		BAddOnManager::_AmyTheAddOnManager14() {}
void		BAddOnManager::_AmyTheAddOnManager15() {}
void		BAddOnManager::_AmyTheAddOnManager16() {}
