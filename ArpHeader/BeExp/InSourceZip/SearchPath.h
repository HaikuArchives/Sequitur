/***************************************************************************
//
//	File:			SearchPath.h
//
//	Description:	A set of directories containing related files.
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SEARCH_PATH_H
#define _SEARCH_PATH_H

#ifndef _ENTRY_LIST_H
#include <EntryList.h>
#endif

#ifndef _DIRECTORY_H
#include <Directory.h>
#endif

#ifndef _FIND_DIRECTORY_H
#include <FindDirectory.h>
#endif

#ifndef _LIST_H
#include <List.h>
#endif

#ifndef _MESSENGER_H
#include <Messenger.h>
#endif

namespace BPrivate {
struct node_entry_ref;
}

class BSearchPath : public BEntryList {
private:
	typedef BEntryList inherited;

public:

	BSearchPath(void);
	~BSearchPath();
	
	void Unset();
	
	// Add directories to the set that are to be traversed.
	// If 'leaf' is non-NULL, each given directory is taken
	// as a base and 'leaf' concatenated to them to determine
	// the final directory to traverse.
	// These first four add a single directory to the set.
	status_t AddDirectory(const BEntry* dir);
	status_t AddDirectory(const entry_ref* dir);
	status_t AddDirectory(const char* dir, const char* leaf=NULL);
	status_t AddDirectory(directory_which which, const char* leaf=NULL);

	// These two methods add a group of directories in a
	// search path to the set.  A search path is a colon
	// separated list of directories, e.g.
	// "/boot/beos:/boot/home".  The special character %A
	// is replaced with the path to the directory containing
	// the application's executable, so that things like
	// the ADDON_PATH variable can be used directly.
	// The second version retrieves the given variable from
	// the program's environment and passes its value on to
	// AddSearchPath().
	status_t AddSearchPath(const char* path, const char* leaf=NULL);
	status_t AddEnvVar(const char* name, const char* leaf=NULL,
					   const char* defEnvVal=NULL);
	
	// Retrieve information about the directories in this search path.
	int32 CountDirectories() const;
	node_ref DirectoryAt(int32 index, entry_ref* outEntry=NULL) const;
	
	status_t FindDirectoryByNode(const node_ref* node,
								 entry_ref* outEntry=NULL) const;
	status_t FindDirectoryByEntry(const entry_ref* entry,
								  node_ref* outNode=NULL) const;
	
	// -------- The EntryList Interface --------
	
	virtual status_t GetNextEntry(BEntry* entry,
	                              bool traverse=false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32    GetNextDirents(struct dirent *buf, 
						   	size_t length, int32 count = INT_MAX);
	
	virtual status_t Rewind();
	virtual int32    CountEntries();
	
	// -------- Live Watching of the Search Path --------
	
	// This is for doing all the watching yourself -- you will just
	// be sent node monitor messages to the given target for all of
	// the directories in the search path, but it is your responsibility
	// to parse the messages and keep track of the files in the path.
	status_t StartWatching(BMessenger target);
	bool IsLive() const;
	
	// This is to let the BSearchPath class do all the dirty work for
	// you.  To use it, you should override the virtual functions to
	// watch what happens.
	status_t StartHandling(BMessenger target);
	bool IsHandling() const;
	
	status_t HandleNodeMessage(const BMessage* msg);
	
	// Stop watching or handling.
	void Stop();

protected:
	virtual void EntryCreated(const node_ref* node, const entry_ref* entry);
	virtual void EntryMoved(const node_ref* node, const entry_ref* entry,
							const entry_ref* oldEntry);
	virtual void EntryRemoved(const node_ref* node, const entry_ref* entry);

private:
		/* FBC */
virtual	void		_TouchMySearchPath1();
virtual	void		_TouchMySearchPath2();
virtual	void		_TouchMySearchPath3();
virtual	void		_TouchMySearchPath4();
virtual	void		_TouchMySearchPath5();
virtual	void		_TouchMySearchPath6();
virtual	void		_TouchMySearchPath7();
		uint32		_touchMyData[8];

	status_t select_dir(const BPrivate::node_entry_ref* which);
	status_t select_dir(int32 which);
	status_t next_dir(void);
	
	status_t next_action(void);
	
	status_t exec_next_action(void);
	
	status_t exec_entry_created(const node_ref* node, const entry_ref* entry);
	status_t exec_entry_moved(int32 index, const entry_ref* newEntry);
	status_t exec_entry_removed(int32 index);
	
	BList fDirs;			// a list of node_entry_ref structs
	BDirectory fCurDir;
	int32 fCurDirsIndex;
	
	// Node monitoring state.
	BMessenger fTarget;		// who is watching
	bool fLive;
	bool fHandling;
	enum action_state { no_action, remove_action, add_action };
	action_state fAction;
	BList fActiveNodes;		// a list of node_entry_ref structs
	BList fRemovedNodes;	// a list of node_entry_ref structs
	BList fRemovedDirs;		// a list of node_entry_ref structs
	int32 fAddedDirIndex;
};

#endif
