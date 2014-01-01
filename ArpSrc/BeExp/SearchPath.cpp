#include <SearchPath.h>

#ifndef _PATH_H
#include <Path.h>
#endif

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _MESSENGER_H
#include <Messenger.h>
#endif

#ifndef _NODE_MONITOR_H
#include <NodeMonitor.h>
#endif

#ifndef _ROSTER_H
#include <Roster.h>
#endif

#if B_BEOS_VERSION_DANO
#ifndef _STREAM_IO_H
#include <StreamIO.h>
#endif
#endif

#ifndef __BSTRING__
#include <String.h>
#endif

#include <Debug.h>

#include <string.h>
#include <stdlib.h>

static const char* _dot_ = ".";
static const char* _dotdot_ = "..";

namespace BPrivate {
struct node_entry_ref {
	node_entry_ref()
	{
	}
	
	node_entry_ref(const node_entry_ref &ref)
		: entry(ref.entry), node(ref.node)
	{
	}

	bool				operator==(const node_entry_ref &ref) const
	{
		return (entry == ref.entry) && (node == ref.node);
	}
	bool				operator!=(const node_entry_ref &ref) const
	{
		return (entry != ref.entry) || (node != ref.node);
	}
	node_entry_ref &	operator=(const node_entry_ref &ref)
	{
		entry = ref.entry;
		node = ref.node;
		return *this;
	}
	
	bool				operator==(const node_ref &ref) const
	{
		return (node == ref);
	}
	bool				operator!=(const node_ref &ref) const
	{
		return (node != ref);
	}
	
	bool				operator==(const entry_ref &ref) const
	{
		return (entry == ref);
	}
	bool				operator!=(const entry_ref &ref) const
	{
		return (entry != ref);
	}
	
	int32				compare(const node_entry_ref &ref) const
	{
		if (node.node >= 0 && ref.node.node >= 0) {
			if (node.node < ref.node.node) return -1;
			if (node.node > ref.node.node) return 1;
		}
		if (node.device >= 0 && ref.node.device >= 0) {
			if (node.device != ref.node.device) {
				if (node.device < ref.node.device) return -1;
				if (node.device > ref.node.device) return 1;
			}
		}
		return 0;
	}
	
	int32				compare_all(const node_entry_ref &ref) const
	{
		const int32 cmp = compare(ref);
		if (cmp != 0) return cmp;
		if (entry.directory >= 0 && ref.entry.directory >= 0) {
			if (entry.directory != ref.entry.directory) {
				if (entry.directory < ref.entry.directory) return -1;
				if (entry.directory > ref.entry.directory) return 1;
			}
		}
		if (entry.name && ref.entry.name) {
			return strcmp(entry.name, ref.entry.name);
		}
		return 0;
	}
	
	entry_ref entry;
	node_ref node;
};

static
node_entry_ref* find_ref_in_list(BList& list, const node_entry_ref* look,
								 int32* outIndex)
{
	const int32 N = list.CountItems();
	if (N <= 0) {
		if (outIndex) *outIndex = 0;
		return NULL;
	}
	
	int32 lower = 0;
	int32 upper = N-1;
	
	// Look for ref with binary search.
	while( lower <= upper ) {
		int32 middle = lower + (upper-lower+1)/2;
		node_entry_ref* ref = (node_entry_ref*)(list.ItemAt(middle));
		const int32 cmp = ref ? ref->compare(*look) : 1;
		if( cmp < 0 ) upper = middle-1;
		else if( cmp > 0 ) lower = middle+1;
		else {
			// This is the one we are looking for.
			if (outIndex) *outIndex = middle;
			return ref;
		}
	}
	
	if (outIndex) {
		// At this point, 'upper' and 'lower' are around the last checked ref.
		// Arbitrarily use 'upper' and determine the position where the
		// given ref would theoretically appear in the index.
		if( upper < 0 ) upper = 0;
		else if( upper < N ) {
			const node_entry_ref* ref = (const node_entry_ref*)(list.ItemAt(upper));
			if( !ref || ref->compare(*look) > 0 ) {
				upper++;
			}
		}
		
		if (upper >= N)
			*outIndex = N;					// Place at end.
		else
			*outIndex = upper;				// Place right here.
	}
	
	return NULL;
}

static
node_entry_ref* add_ref_to_list(BList& list, node_entry_ref* ref, bool delete_dup)
{
	if (!ref) return NULL;
	
	int32 index;
	node_entry_ref* found = find_ref_in_list(list, ref, &index);
	if (found) {
		if (delete_dup) delete ref;
		return found;
	}
	
	if (!list.AddItem(ref, index)) {
		if (delete_dup) delete ref;
		return NULL;
	}
	
	return ref;
}

static
node_entry_ref* remove_ref_from_list(BList& list, const node_entry_ref* ref)
{
	if (!ref) return NULL;
	
	int32 index;
	node_entry_ref* found = find_ref_in_list(list, ref, &index);
	if (found && list.RemoveItem(index)) return found;
	
	return NULL;
}
}

using namespace BPrivate;

BSearchPath::BSearchPath(void)
	: fCurDirsIndex(-1),
	  fLive(false), fHandling(false), fAction(no_action), fAddedDirIndex(0)
{
}

static bool free_node_entry_func(void* item)
{
	PRINT(("Deleting directory node_entry_ref %p\n", item));
	delete reinterpret_cast<node_entry_ref*>(item);
	return false;
}

BSearchPath::~BSearchPath()
{
	Unset();
}

void BSearchPath::Unset()
{
	Stop();
	
	fDirs.DoForEach(free_node_entry_func);
	fDirs.MakeEmpty();
	fCurDirsIndex = -1;
}

// ---------------------------------------------------------------------------

status_t BSearchPath::AddDirectory(const BEntry* dir)
{
	if (dir->InitCheck() != B_OK) return dir->InitCheck();
	
	struct stat st;
	status_t err = dir->GetStat(&st);
	if (err != B_OK) return err;
	if (!S_ISDIR(st.st_mode)) return B_NOT_A_DIRECTORY;
	
	node_entry_ref* item = new node_entry_ref;
	if (!item) return B_NO_MEMORY;
	
	item->node.device = st.st_dev;
	item->node.node = st.st_ino;
	err = dir->GetRef(&(item->entry));
	
	if (err == B_OK) {
		// See if this directory is already included.  We compare
		// only based on their nodes, and don't use add_ref_to_list()
		// because we want to keep them in the order the caller
		// supplied them.
		for (int32 i=0; i<fDirs.CountItems(); i++) {
			node_entry_ref* existing = (node_entry_ref*)(fDirs.ItemAt(i));
			if (existing && (*existing) == item->node) {
				delete item;
				item = NULL;
			}
		}
		if (item && !fDirs.AddItem(item)) err = B_ERROR;
	}
	
	if (err == B_OK) {
		if (IsLive()) watch_node(&(item->node), B_WATCH_DIRECTORY, fTarget);
		if (IsHandling()) while (exec_next_action() == B_OK) ;
	} else {
		delete item;
	}
	
	return err;
}

status_t BSearchPath::AddDirectory(const entry_ref* dir)
{
	// Make an entry to this ref, traversing any symbolic links.
	// We need to be working with the actual directory entry, so
	// that a node monitor will watch the correct thing.
	BEntry entry(dir, true);
	if (entry.InitCheck() != B_OK) return entry.InitCheck();
	
	return AddDirectory(&entry);
}

status_t BSearchPath::AddDirectory(const char* dir, const char* leaf)
{
	BPath path(dir,leaf);
	if (path.InitCheck() != B_OK) return path.InitCheck();
	
	DEBUG_ONLY(BErr << "Adding path: " << path.Path() << endl);
	
	// Make an entry to this path, traversing any symbolic links.
	// We need to be working with the actual directory entry, so
	// that a node monitor will watch the correct thing.
	BEntry entry(path.Path(), true);
	if (entry.InitCheck() != B_OK) return entry.InitCheck();
	
	return AddDirectory(&entry);
}

status_t BSearchPath::AddDirectory(directory_which which, const char* leaf)
{
	BPath path;
	status_t ret = find_directory(which,&path);
	if (ret != B_OK) return ret;
	return AddDirectory(path.Path(),leaf);
}

// ---------------------------------------------------------------------------

static status_t get_app_path(BPath* setpath)
{
	status_t ret = B_NO_ERROR;
	if (be_app) {
		if (be_app->Lock()) {
			app_info ainfo;
			if ((ret=be_app->GetAppInfo(&ainfo)) == B_NO_ERROR) {
				BEntry entry(&ainfo.ref,true);
				if ((ret=entry.InitCheck()) == B_NO_ERROR) {
					be_app->Unlock();
					return entry.GetPath(setpath);
				}
			}
			be_app->Unlock();
		} else ret = B_ERROR;
	} else ret = B_ERROR;
	return ret;
}

static int32 expand_dir(char* buffer, const char* dir)
{
	int32 size=0;
	while (dir && *dir) {
		if (*dir == '%') {
			dir++;
			switch (*dir) {
				case '%':
					break;
				case 'A': {
					BPath path;
					if (get_app_path(&path) == B_NO_ERROR) {
						if (path.GetParent(&path) == B_NO_ERROR) {
							const char* dir = path.Path();
							if (dir) {
								PRINT(("App dir = %s\n", dir));
								int len = strlen(dir);
								size += len;
								if (buffer) {
									memcpy(buffer,dir,len);
									buffer += len;
								}
							}
						}
					}
					dir++;
				} break;
				case 0:
					if (buffer) *buffer = 0;
					return size+1;
				default:
					dir++;
					break;
			}
		}
		if (buffer) *(buffer++) = *dir;
		dir++;
		size++;
	}
	if (buffer) *buffer = 0;
	return size+1;
}

status_t BSearchPath::AddSearchPath(const char* path, const char* leaf)
{
	if (!path) return B_OK;
	
	DEBUG_ONLY(BErr << "Adding path: " << path << ", leaf: " << leaf << endl);
	
	char* mypath = strdup(path);
	if (!mypath) return B_NO_MEMORY;
	
	char* base = mypath;
	char* pos = base;
	status_t ret = B_OK;
	while (*base) {
		bool need_repl = false;
		char* buffer;
		while (*pos != ':' && *pos != 0) {
			if (*pos == '%') need_repl = true;
			pos++;
		}
		const char endc = *pos;
		*pos = 0;
		
		if (need_repl) {
			int32 size = expand_dir(NULL,base);
			buffer = (char*)malloc(size);
			if (buffer) {
				expand_dir(buffer,base);
				base = buffer;
			} else ret = B_NO_MEMORY;
		} else buffer = NULL;
		
		if (ret == B_OK) ret = AddDirectory(base, leaf);
		else AddDirectory(base, leaf);
		
		if (endc != 0) pos++;
		base = pos;
		
		if (buffer) free(buffer);
	}
	
	free(mypath);
	
	return ret;
}

#if __POWERPC__
extern char** environ;
#endif

status_t BSearchPath::AddEnvVar(const char* name, const char* leaf,
							  const char* def_env_val)
{
	if (!name) return B_OK;
	
	const char* val = getenv(name);
	if (val == 0) val = def_env_val;
	if (val == 0) return B_BAD_VALUE;
	
	return AddSearchPath(val, leaf);
}

// ---------------------------------------------------------------------------

int32 BSearchPath::CountDirectories() const
{
	return fDirs.CountItems();
}

node_ref BSearchPath::DirectoryAt(int32 index, entry_ref* outEntry) const
{
	const node_entry_ref* ref = NULL;
	if (index >= 0 && index < fDirs.CountItems()) {
		ref = (const node_entry_ref*)(fDirs.ItemAt(index));
	}
	
	if (ref == NULL) {
		if (outEntry) *outEntry = entry_ref();
		return node_ref();
	}
	
	if (outEntry) *outEntry = ref->entry;
	return ref->node;
}

status_t BSearchPath::FindDirectoryByNode(const node_ref* node,
										  entry_ref* outEntry) const
{
	for (int32 i=0; i<fDirs.CountItems(); i++) {
		const node_entry_ref* dir = (const node_entry_ref*)(fDirs.ItemAt(i));
		if (dir && dir->node == *node) {
			if (outEntry) *outEntry = dir->entry;
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

status_t BSearchPath::FindDirectoryByEntry(const entry_ref* entry,
										   node_ref* outNode) const
{
	for (int32 i=0; i<fDirs.CountItems(); i++) {
		const node_entry_ref* dir = (const node_entry_ref*)(fDirs.ItemAt(i));
		if (dir && dir->entry == *entry) {
			if (outNode) *outNode = dir->node;
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// ---------------------------------------------------------------------------

status_t BSearchPath::Rewind()
{
	PRINT(("Rewinding BSearchPath.\n"));
	fCurDirsIndex = -1;
	fCurDir.Unset();
	return B_OK;
}

int32 BSearchPath::CountEntries()
{
	int32 cnt=0;
	for (int32 i=0; i<fDirs.CountItems(); i++) {
		if (select_dir(i) == B_OK) {
			cnt += fCurDir.CountEntries();
		}
	}
	Rewind();
	return cnt;
}

status_t BSearchPath::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t ret = B_OK;
	if (fCurDirsIndex < 0 ) ret = next_dir();
	
	do {
		while (ret == B_ENTRY_NOT_FOUND ) ret = next_dir();
		if (ret != B_OK) return ret;
		ret = fCurDir.GetNextEntry(entry,traverse);
	} while (ret == B_ENTRY_NOT_FOUND && fCurDir.InitCheck() == B_NO_ERROR );
	
	PRINT(("Returning entry=%x, err=%s\n", entry, strerror(ret)));
	
	return ret;
}

status_t BSearchPath::GetNextRef(entry_ref* ref)
{
	status_t ret = B_OK;
	if (fCurDirsIndex < 0 ) ret = next_dir();
	
	do {
		while (ret == B_ENTRY_NOT_FOUND ) ret = next_dir();
		if (ret != B_OK) return ret;
		ret = fCurDir.GetNextRef(ref);
	} while (ret == B_ENTRY_NOT_FOUND && fCurDir.InitCheck() == B_NO_ERROR );
	
	PRINT(("Returning ref=%s, err=%s\n", ref->name, strerror(ret)));
	
	return ret;
}

int32 BSearchPath::GetNextDirents(struct dirent *buf, 
						   	size_t length, int32 count)
{
	int32 outcount = 0;
	status_t ret = B_NO_ERROR;
	if (fCurDirsIndex < 0 ) ret = next_dir();
		
	do {
		while (ret == B_ENTRY_NOT_FOUND ) ret = next_dir();
		if (ret != B_OK) return outcount;
		outcount = fCurDir.GetNextDirents(buf,length,count);
	} while (outcount == 0 && fCurDir.InitCheck() == B_NO_ERROR );
	
	return outcount;
}

// ---------------------------------------------------------------------------

status_t BSearchPath::StartWatching(BMessenger target)
{
	Stop();
	
	if (!target.IsValid()) return B_BAD_VALUE;
	fTarget = target;
	fLive = true;
	
	for (int32 i=0; i<fDirs.CountItems(); i++) {
		node_entry_ref* ref = (node_entry_ref*)(fDirs.ItemAt(i));
		if (!ref) continue;
		
		watch_node(&(ref->node), B_WATCH_DIRECTORY, fTarget);
	}
	
	return B_OK;
}

bool BSearchPath::IsLive() const
{
	return fLive;
}

// ---------------------------------------------------------------------------

status_t BSearchPath::StartHandling(BMessenger target)
{
	status_t res = StartWatching(target);
	if (res != B_OK) return res;
	
	fHandling = true;
	
	while (exec_next_action() == B_OK) ;
	
	return B_OK;
}

bool BSearchPath::IsHandling() const
{
	return fHandling;
}

status_t BSearchPath::HandleNodeMessage(const BMessage* msg)
{
	if (!IsHandling() || msg->what != B_NODE_MONITOR) return B_ERROR;
	
	int32 opcode = 0;
	
	node_ref node;
	entry_ref entry;
	
	node_entry_ref tmp;
	
	DEBUG_ONLY(BErr << "Handling node message: " << *msg << endl);
	
	status_t res = msg->FindInt32("opcode", &opcode);
	if (res == B_OK) res = msg->FindInt64("node", (int64*)&(node.node));
	if (res == B_OK) res = msg->FindInt32("device", (int32*)&(node.device));
	if (res == B_OK) entry.device = node.device;
	
	tmp.node = node;
	
	int32 nodeIndex = -1;
	node_entry_ref* existing = NULL;
	if (res == B_OK) existing = find_ref_in_list(fActiveNodes, &tmp, &nodeIndex);
	if (nodeIndex < 0 ) res = B_BAD_INDEX;
	
	// Take care of the opcodes that are of interest to us.
	if (res == B_OK) {
		switch (opcode) {
			case B_ENTRY_CREATED: {
				if (!existing) {
					// A new node that isn't in our active list, add it.
					BString name;
					res = msg->FindString("name", &name);
					if (res == B_OK)
						res = entry.set_name(name.String());
					if (res == B_OK)
						res = msg->FindInt64("directory", (int64*)&(entry.directory));
						
				} else {
					// This node is already in the active list, skip it.
					opcode = 0;
					
				}
			} break;
			case B_ENTRY_REMOVED: {
				// NOTE: Don't fail here.  Even if we can't get this,
				// the node still needs to be removed.
				msg->FindInt64("directory", (int64*)&(entry.directory));
			} break;
			case B_ENTRY_MOVED: {
				BString name;
				ino_t fromDir;
				int32 i;
				res = msg->FindString("name", &name);
				if (res == B_OK)
					res = entry.set_name(name.String());
				if (res == B_OK)
					res = msg->FindInt64("from directory", (int64*)&fromDir);
				if (res == B_OK)
					res = msg->FindInt64("to directory", (int64*)&(entry.directory));
				if (res == B_OK) {
					bool fromInPath=false, toInPath=false;
					for (i=0; i<fDirs.CountItems(); i++) {
						node_entry_ref* dir = (node_entry_ref*)(fDirs.ItemAt(i));
						if (dir && dir->node.device == node.device) {
							if (dir->node.node == fromDir) fromInPath = true;
							if (dir->node.node == entry.directory) toInPath = true;
						}
					}
					
					// Okay, now what the hell just happened?
					if (!existing) {
						if (toInPath) {
							// Wherever it came from, it's new to us and in the
							// search path.  Just treat it as a create.
							opcode = B_ENTRY_CREATED;
							
						} else {
							// For everything else -- we don't currently have the
							// node and it's not in our search path, so ignore
							// it.
							opcode = 0;
						}
						
					} else {
						if (!toInPath) {
							// Wherever it's going, we used to have it and now
							// its no longer in the search path.  Make it a remove.
							opcode = B_ENTRY_REMOVED;
						
						} else {
							// For everything else -- we currently have the node
							// and it is still in our search path, so it was
							// a move of some kind.
							opcode = B_ENTRY_MOVED;
						}
					
					}
				}
			} break;
			default: {
				opcode = 0;
			} break;
		}
	}
	
	// Now perform the action if this is something we care about and there
	// were no errors.  We don't report any errors past this point because
	// whatever else happens it was a message we understood so the caller
	// shouldn't use it.
	
	if (res == B_OK) {
		switch (opcode) {
			case B_ENTRY_CREATED:
				// Add this node to the active list.
				exec_entry_created(&node, &entry);
				break;
			case B_ENTRY_REMOVED:
				if (existing) {
					// Remove existing node from active list.
					exec_entry_removed(nodeIndex);
				} else {
					// Add this node to delayed remove list.
					node_entry_ref* ref = new node_entry_ref;
					if (!ref) res = B_NO_MEMORY;
					else {
						DEBUG_ONLY(BErr << "Adding " << entry << " to removed list." << endl);
						ref->node = node;
						ref->entry = entry;
						node_entry_ref* found = add_ref_to_list(fRemovedNodes, ref, true);
						if (found == NULL) res = B_NO_MEMORY;
					}
				}
				break;
			case B_ENTRY_MOVED:
				exec_entry_moved(nodeIndex, &entry);
				break;
			default:
				// Nothing we care about.
				res = B_ERROR;
		}
	}
	
	return res;
}

// ---------------------------------------------------------------------------

void BSearchPath::Stop()
{
	if (IsLive()) {
		for (int32 i=0; i<fDirs.CountItems(); i++) {
			node_entry_ref* ref = (node_entry_ref*)(fDirs.ItemAt(i));
			if (!ref) continue;
			
			watch_node(&(ref->node), B_STOP_WATCHING, fTarget);
		}
		
		fLive = false;
		fHandling = false;
		fAction = no_action;
		fTarget = BMessenger();
		
		fActiveNodes.DoForEach(free_node_entry_func);
		fActiveNodes.MakeEmpty();
		fRemovedNodes.DoForEach(free_node_entry_func);
		fRemovedNodes.MakeEmpty();
		fRemovedDirs.DoForEach(free_node_entry_func);
		fRemovedDirs.MakeEmpty();
	}
}

// ---------------------------------------------------------------------------

void BSearchPath::EntryCreated(const node_ref* /*node*/, const entry_ref* /*entry*/)
{
}

void BSearchPath::EntryMoved(const node_ref* /*node*/, const entry_ref* /*entry*/,
							 const entry_ref* /*oldEntry*/)
{
}

void BSearchPath::EntryRemoved(const node_ref* /*node*/, const entry_ref* /*entry*/)
{
}

// ---------------------------------------------------------------------------

status_t BSearchPath::select_dir(const node_entry_ref* which)
{
	PRINT(("Moving to new entry %x (%s)\n", which, which->entry.name));
	
	return fCurDir.SetTo(&which->node);
}

status_t BSearchPath::select_dir(int32 which)
{
	if (which < 0 || which >= fDirs.CountItems()) return B_BAD_INDEX;
	
	node_entry_ref* ref = (node_entry_ref*)(fDirs.ItemAt(which));
	if (ref == NULL) return B_BAD_VALUE;
	
	return select_dir(ref);
}

status_t BSearchPath::next_dir(void)
{
	fCurDirsIndex++;
	PRINT(("Moving to next dir #%ld\n", fCurDirsIndex));
	if (fCurDirsIndex >= fDirs.CountItems()) {
		PRINT(("At end of last directory!\n"));
		fCurDir.Unset();
		return B_BAD_INDEX;
	}
	
	status_t err = select_dir(fCurDirsIndex);
	if (err) return B_ENTRY_NOT_FOUND;
	return B_OK;
}

status_t BSearchPath::next_action(void)
{
	status_t err = B_CANCELED;
	
	if (!IsHandling()) return err;
	
	// Figure out what to do next.
	while (fRemovedDirs.CountItems() > 0) {
		node_entry_ref* dir = (node_entry_ref*)(fRemovedDirs.RemoveItem((int32)0));
		if (dir) {
			err = select_dir(dir);
			delete dir;
			if (err == B_OK) {
				fAction = remove_action;
				return B_OK;
			}
		}
	}
	
	while (fAddedDirIndex < fDirs.CountItems()) {
		node_entry_ref* dir = (node_entry_ref*)(fDirs.ItemAt(fAddedDirIndex));
		fAddedDirIndex++;
		if (dir) {
			err = select_dir(dir);
			if (err == B_OK) {
				fAction = add_action;
				return B_OK;
			}
		}
	}
	
	fAction = no_action;
	
	return err;
}

status_t BSearchPath::exec_next_action(void)
{
	char			tmp[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
	struct dirent	*d = (struct dirent *) &tmp;
	
	status_t ret = B_OK;
	if (!IsHandling()) return B_ENTRY_NOT_FOUND;
	if (fAction == no_action) ret = next_action();
	
	// The next node's offset in the active list.
	int32 active_index = -1;
	
	do {
		// Find the next file, moving on to the next action if needed.
		while (ret == B_ENTRY_NOT_FOUND ) {
			fAction = no_action;
			ret = next_action();
		}
		if (ret < B_OK) return ret;
		ret = fCurDir.GetNextDirents(d, sizeof(tmp), 1);
		if (ret == 0) ret = B_ENTRY_NOT_FOUND;
		
		// Got another file...
		if (ret >= 1) {
			ret = B_OK;
			
			// If this is a parent directory, skip it.
			if (strcmp(d->d_name, _dot_) == 0 || strcmp(d->d_name, _dotdot_) == 0) {
				ret = 1;
				continue;
			}
			
			// Make a tempory node_entry_ref to lookup by node.
			node_entry_ref tmp;
			tmp.node.device = d->d_dev;
			tmp.node.node = d->d_ino;
			int32 index;
			
			if (fAction == add_action) {
				// If this node is in the removed list, take it out and skip it.
				node_entry_ref* found = find_ref_in_list(fRemovedNodes, &tmp, &index);
				if (found) {
					ret = 1;
					fRemovedNodes.RemoveItem(index);
					delete found;
					continue;
				}
				
				// If this node is already in the active list, skip it.
				found = find_ref_in_list(fActiveNodes, &tmp, &active_index);
				if (found) {
					ret = 1;
					active_index = -1;
					continue;
				}
			
			} else if (fAction == remove_action) {
				// If this node is not in the active list, skip it.
				node_entry_ref* found = find_ref_in_list(fActiveNodes, &tmp, &active_index);
				if (active_index < 0 || !found) {
					ret = 1;
					active_index = -1;
					continue;
				}
			
			} else {
				debugger("Bad action in BSearchPath");
				
			}
			
		}
	} while ((ret == B_ENTRY_NOT_FOUND || ret > 0) && fCurDir.InitCheck() == B_NO_ERROR);
	
	if (ret > 0) {
		ret = fCurDir.InitCheck();
		if (ret == B_OK) ret = B_ERROR;
	}
	
	if (ret != B_OK) return ret;
	
	// If we get here with B_OK, then a file has been added or removed.
	// Execute the action.
	
	switch (fAction) {
		case add_action: {
			node_ref node;
			node.device = d->d_dev;
			node.node = d->d_ino;
			
			entry_ref entry;
			entry.device = d->d_dev;
			entry.directory = d->d_pino;
			ret = entry.set_name(d->d_name);
			if (ret != B_OK) return ret;
			
			return exec_entry_created(&node, &entry);
		}
		case remove_action: {
			return exec_entry_removed(active_index);
		}
		default:				debugger("Bad action in BSearchPath");
	}
	
	return B_OK;
}

// ---------------------------------------------------------------------------

status_t BSearchPath::exec_entry_created(const node_ref* node, const entry_ref* entry)
{
	DEBUG_ONLY(BErr << "Adding " << *entry << " to active list." << endl);
	
	node_entry_ref* ref = new node_entry_ref;
	if (ref == NULL) return B_NO_MEMORY;
	
	ref->node = *node;
	ref->entry = *entry;
	
	node_entry_ref* found = add_ref_to_list(fActiveNodes, ref, true);
	if (found != ref) return B_FILE_EXISTS;
	
	EntryCreated(node, entry);
	return B_OK;
}

status_t BSearchPath::exec_entry_moved(int32 index, const entry_ref* newEntry)
{
	// This is implemented as a remove and then an add to be sure
	// the list is still in the correct order.
	node_entry_ref* it = (node_entry_ref*)(fActiveNodes.RemoveItem(index));
	if (!it) return B_ENTRY_NOT_FOUND;
	
	DEBUG_ONLY(BErr << "Moving " << it->entry << " (#" << index << ")"
		 << " to " << *newEntry << endl);
				
	EntryMoved(&(it->node), newEntry, &(it->entry));
	
	it->entry = *newEntry;
	
	node_entry_ref* found = add_ref_to_list(fActiveNodes, it, true);
	if (found != it) return B_FILE_EXISTS;
	
	return B_OK;
}

status_t BSearchPath::exec_entry_removed(int32 index)
{
	node_entry_ref* ref = (node_entry_ref*)(fActiveNodes.RemoveItem(index));
	if (!ref) return B_ENTRY_NOT_FOUND;
	
	DEBUG_ONLY(BErr << "Removing " << ref->entry << " (#" << index << ")"
		 << " from active list." << endl);
	
	EntryRemoved(&(ref->node), &(ref->entry));
	delete ref;
	
	// And make sure it's also no longer in the removed list.
	node_entry_ref tmp;
	tmp.node = ref->node;
	ref = remove_ref_from_list(fRemovedNodes, &tmp);
	if (ref) delete ref;
	
	return B_OK;
}

void		BSearchPath::_TouchMySearchPath1() {}
void		BSearchPath::_TouchMySearchPath2() {}
void		BSearchPath::_TouchMySearchPath3() {}
void		BSearchPath::_TouchMySearchPath4() {}
void		BSearchPath::_TouchMySearchPath5() {}
void		BSearchPath::_TouchMySearchPath6() {}
void		BSearchPath::_TouchMySearchPath7() {}
