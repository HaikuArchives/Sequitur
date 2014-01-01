/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpMultiDir.h
 *
 * A BEntryList for traversing through multiple directories.
 * Later will also support live watching of the directories.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 7/22/97:
 *	Created this file.
 *
 */

#ifndef ARPKERNEL_ARPMULTIDIR_H
#define ARPKERNEL_ARPMULTIDIR_H

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

class ArpMultiDir : public BDirectory {
private:
	typedef BDirectory inherited;

public:

	ArpMultiDir(void);
	~ArpMultiDir();
	
	/* Add directories to the set that are to be traversed.
	   If 'leaf' is non-NULL, each given directory is taken
	   as a base and 'leaf' catenated to them to determine
	   the final directory to traverse.
	   These first two add a single directory to the set.
	*/
	status_t AddDirectory(const char* dir,
	                       const char* leaf = NULL);
	status_t AddDirectory(directory_which which,
	                       const char* leaf = NULL);

	/* These second two add a group of directories in a
	   search path to the set.  A search path is a colon
	   separated list of directories, e.g.
	   "/boot/beos:/boot/home".  The special character %A
	   is replaced with the path to the directory containing
	   the application's executable, so that things like
	   the ADDON_PATH variable can be used directly.
	   The second version retrieves the given variable from
	   the program's environment and passes its value on to
	   AddSearchPath().
	*/
	status_t AddSearchPath(const char* path,
	                       const char* leaf = NULL);
	status_t AddEnvVar(const char* name,
	                       const char* leaf = NULL);
	
	// -------- The EntryList Interface --------
	
	virtual status_t GetNextEntry(BEntry* entry,
	                              bool traverse=false);
	virtual status_t GetNextRef(entry_ref* ref);
	virtual int32    GetNextDirents(struct dirent *buf, 
						   	size_t length, int32 count = INT_MAX);
	
	virtual status_t Rewind();
	virtual int32    CountEntries();
	
private:
	status_t next_dir(void);
	
	BList dirs;
	
	int32 cur_dirs_index;
	bool dir_ended;
};

#endif
