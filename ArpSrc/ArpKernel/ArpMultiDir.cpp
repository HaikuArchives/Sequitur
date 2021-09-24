/*
	
	ArpMultiDir.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef APRKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef APRKERNEL_ARPMULTIDIR_H
#include "ArpKernel/ArpMultiDir.h"
#endif

#ifndef _PATH_H
#include <storage/Path.h>
#endif

#ifndef _APPLICATION_H
#include <app/Application.h>
#endif

#ifndef _ROSTER_H
#include <app/Roster.h>
#endif

#include <cstring>
#include <cstdlib>

ArpMOD();

ArpMultiDir::ArpMultiDir(void)
	: cur_dirs_index(-1), dir_ended(true)
{
}

ArpMultiDir::~ArpMultiDir()
{
	BPath* path = NULL;
	while( (path=(BPath*)(dirs.RemoveItem((int32)0))) != NULL ) {
		delete path;
	}
}

status_t ArpMultiDir::AddDirectory(const char* dir, const char* leaf)
{
	BPath* path =  new BPath(dir,leaf,true);
	if( path ) {
		if( path->InitCheck() != B_OK ) return path->InitCheck();
		ArpD(cdb << ADH << "Adding directory: " << path->Path() << std::endl);
		for( int32 i=0; i<dirs.CountItems(); i++ ) {
			BPath* existing = (BPath*)(dirs.ItemAt(i));
			if( existing && (*existing) == (*path) ) {
				ArpD(cdb << ADH << "Already have it!" << std::endl);
				delete path;
				return B_OK;
			}
		}
		if( !dirs.AddItem(path) ) return B_ERROR;
		return B_OK;
	}
	return B_NO_MEMORY;
}

status_t ArpMultiDir::AddDirectory(directory_which which, const char* leaf)
{
	BPath path;
	status_t ret = find_directory(which,&path);
	if( ret != B_OK ) return ret;
	return AddDirectory(path.Path(),leaf);
}

static status_t get_app_path(BPath* setpath)
{
	status_t ret = B_NO_ERROR;
	if( be_app ) {
		if( be_app->Lock() ) {
			app_info ainfo;
			if( (ret=be_app->GetAppInfo(&ainfo)) == B_NO_ERROR ) {
				BEntry entry(&ainfo.ref,true);
				if( (ret=entry.InitCheck()) == B_NO_ERROR ) {
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
	while( dir && *dir ) {
		if( *dir == '%' ) {
			dir++;
			switch( *dir ) {
				case '%':
					break;
				case 'A': {
					BPath path;
					if( get_app_path(&path) == B_NO_ERROR ) {
						if( path.GetParent(&path) == B_NO_ERROR ) {
							const char* dir = path.Path();
							if( dir ) {
								ArpD(cdb << ADH << "App dir = " << dir << std::endl);
								int len = strlen(dir);
								size += len;
								if( buffer ) {
									memcpy(buffer,dir,len);
									buffer += len;
								}
							}
						}
					}
					dir++;
				} break;
				case 0:
					if( buffer ) *buffer = 0;
					return size+1;
				default:
					dir++;
					break;
			}
		}
		if( buffer ) *(buffer++) = *dir;
		dir++;
		size++;
	}
	if( buffer ) *buffer = 0;
	return size+1;
}

status_t ArpMultiDir::AddSearchPath(const char* path, const char* leaf)
{
	if( !path ) return B_OK;
	
	char* mypath = strdup(path);
	if( !mypath ) return B_NO_MEMORY;
	
	char* base = mypath;
	status_t ret = B_OK;
	while( *base ) {
		bool need_repl = false;
		char* buffer;
		while( *mypath != ':' && *mypath != 0 ) {
			if( *mypath == '%' ) need_repl = true;
			mypath++;
		}
		char endc = *mypath;
		*mypath = 0;
		
		if( need_repl ) {
			int32 size = expand_dir(NULL,base);
			buffer = (char*)malloc(size);
			if( buffer ) {
				expand_dir(buffer,base);
				base = buffer;
			} else ret = B_NO_MEMORY;
		} else buffer = NULL;
		
		if( ret == B_OK ) ret = AddDirectory(base, leaf);
		else AddDirectory(base, leaf);
		
		*mypath = endc;
		if( *mypath != 0 ) mypath++;
		base = mypath;
		
		if( buffer ) free(buffer);
	}
	return ret;
}

#if __POWERPC__
extern char** environ;
#endif

status_t ArpMultiDir::AddEnvVar(const char* name, const char* leaf)
{
	if( !name ) return B_OK;
	
	int nlen = strlen(name);
	char** e = environ;
	while( e && *e ) {
		ArpD(cdb << ADH << "Looking at var: " << *e << std::endl);
		if( strncmp(*e,name,nlen) == 0 && (*e)[nlen] == '=' ) {
			ArpD(cdb << ADH << "Found env var: " << *e << std::endl);
			return AddSearchPath(&((*e)[nlen+1]), leaf);
		}
		e++;
	}
	return B_OK;
}

status_t ArpMultiDir::next_dir(void)
{
	cur_dirs_index++;
	ArpD(cdb << ADH << "Moving to next dir #" << cur_dirs_index << std::endl);
	if( cur_dirs_index >= dirs.CountItems() ) {
		ArpD(cdb << ADH << "At end of this directory!" << std::endl);
		inherited::Unset();
		return ENOENT;
	}
	
	BPath* path = (BPath*)(dirs.ItemAt(cur_dirs_index));
	if( path == NULL ) {
		ArpD(cdb << ADH << "Reached last path, it's all over." << std::endl);
		inherited::Unset();
		return ENOENT;
	}
	
	ArpD(cdb << ADH << "Moving to new path " << path << std::endl);
	return inherited::SetTo(path->Path());
}

status_t ArpMultiDir::Rewind()
{
	ArpD(cdb << ADH << "Rewinding ArpMultiDir." << std::endl);
	cur_dirs_index = -1;
	inherited::Unset();
	return B_OK;
}

int32 ArpMultiDir::CountEntries()
{
	int32 cnt=0;
	for( int32 i=0; i<dirs.CountItems(); i++ ) {
		BPath* path = (BPath*)(dirs.ItemAt(i));
		if( path ) {
			status_t ret = inherited::SetTo(path->Path());
			if( ret != B_NO_ERROR ) {
				cnt += inherited::CountEntries();
			}
		}
	}
	Rewind();
	return cnt;
}

status_t ArpMultiDir::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t ret = B_OK;
	if( cur_dirs_index < 0 ) ret = next_dir();
	if( ret != B_OK ) return ret;
	
	do {
		if( ret == ENOENT ) ret = next_dir();
		if( ret != B_NO_ERROR ) return ret;
		ret = inherited::GetNextEntry(entry,traverse);
	} while( ret == ENOENT && inherited::InitCheck() == B_NO_ERROR );
	
	return ret;
}

status_t ArpMultiDir::GetNextRef(entry_ref* ref)
{
	status_t ret = B_OK;
	if( cur_dirs_index < 0 ) ret = next_dir();
	if( ret != B_OK ) return ret;
	
	do {
		if( ret == ENOENT ) ret = next_dir();
		if( ret != B_NO_ERROR ) return ret;
		ret = inherited::GetNextRef(ref);
	} while( ret == ENOENT && inherited::InitCheck() == B_NO_ERROR );
	
	return ret;
}

int32 ArpMultiDir::GetNextDirents(struct dirent *buf, 
						   	size_t length, int32 count)
{
	int32 outcount = 0;
	status_t ret = B_NO_ERROR;
	if( cur_dirs_index < 0 ) if( next_dir() != B_OK ) return 0;
		
	do {
		if( ret == ENOENT ) ret = next_dir();
		if( ret != B_NO_ERROR ) return outcount;
		outcount = inherited::GetNextDirents(buf,length,count);
	} while( outcount == 0 && inherited::InitCheck() == B_NO_ERROR );
	
	return outcount;
}
