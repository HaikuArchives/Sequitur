/*
	
	ArpAddonManager.cpp
	
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

#ifndef APRKERNEL_ARPADDONMANAGER_H
#include "ArpKernel/ArpAddonManager.h"
#endif

#ifndef _MENU_H
#include <be/interface/Menu.h>
#endif

#ifndef _MENUITEM_H
#include <be/interface/MenuItem.h>
#endif

#ifndef _ENTRY_H
#include <be/storage/Entry.h>
#endif

#include <support/Autolock.h>
#include <string.h>

ArpMOD();

ArpAddonManager::BasicAddon::BasicAddon(BEntry* entry)
	: image(B_ERROR), ref_count(0)
{
	ArpD(cdb << ADH << "Setting addon path: entry=" << (void*)entry
			<< " (init=" << entry->InitCheck()
			<< " exists=" << entry->Exists() <<")" << endl);
	if( entry ) entry->GetPath(&where);
	ArpD(cdb << ADH << "Creating new addon for: " << where << endl);
}

ArpAddonManager::BasicAddon::BasicAddon(const char* path)
	: image(B_ERROR), ref_count(0)
{
	ArpD(cdb << ADH << "Setting addon path: path=" << (void*)path
				<< " (" << (path ? path : "<null>") << ")" << endl);
	if( path ) where.SetTo(path);
	ArpD(cdb << ADH << "Creating new addon for: " << where << endl);
}

ArpAddonManager::BasicAddon::~BasicAddon()
{
	if( image >= 0 ) unload_add_on(image);
}

status_t ArpAddonManager::BasicAddon::InitCheck(void) const
{
	return where.InitCheck();
}

const BPath& ArpAddonManager::BasicAddon::Path(void) const
{
	return where;
}

const char* ArpAddonManager::BasicAddon::Name(void) const
{
	return where.Leaf();
}

const char* ArpAddonManager::BasicAddon::LongName(void) const
{
	return where.Leaf();
}

image_id ArpAddonManager::BasicAddon::Open(void)
{
	if( image < 0 ) {
		image = load_add_on(Path().Path());
	}
	if( image >= 0 ) ref_count++;
	return image;
}

void ArpAddonManager::BasicAddon::Close(void)
{
	if( --ref_count <= 0 ) {
		if( image >= 0 ) unload_add_on(image);
		image = B_ERROR;
		ref_count = 0;
	}
}

void ArpAddonManager::Start(void)
{
	ArpD(cdb << ADH << "Starting addon manager" << endl);
	BAutolock(Lock());
	ArpD(cdb << ADH << "Locked myself." << endl);
	BEntry entry;
	Rewind();
	while( GetNextEntry(&entry,true) == B_NO_ERROR ) {
		if( entry.InitCheck() == B_NO_ERROR ) {
			ArpD(cdb << ADH << "Trying addon for: " << (void*)&entry << endl);
			BasicAddon* addon = AllocAddon(&entry);
			if( addon ) {
				if( addon->InitCheck() == B_NO_ERROR ) AddAddon(addon);
				else delete addon;
			}
		}
	}
}

ArpAddonManager::BasicAddon* ArpAddonManager::FindAddon(const char* name) const
{
	BAutolock(Lock());
	//int32 num=0;
	for( int i=0; i<CountAddons(); i++ ) {
		ArpAddonManager::BasicAddon* addon = AddonAt(i);
		if( addon ) {
			const char* pname = addon->Name();
			if( pname && strcmp(pname, name) == 0 ) return addon;
		}
	}
	return NULL;
}
	
int32 ArpAddonManager::CountAddons(void) const
{
	BAutolock(Lock());
	return addons.CountItems();
}

ArpAddonManager::BasicAddon* ArpAddonManager::AddonAt(int32 index) const
{
	BAutolock(Lock());
	return (ArpAddonManager::BasicAddon*)addons.ItemAt(index);
}
	
int32 ArpAddonManager::CreateAddonMenu(BMenu* inmenu, BMessage* tmpl) const
{
	if( !inmenu ) return -1;
	
	BAutolock(Lock());
	int32 num=0;
	for( int i=0; i<CountAddons(); i++ ) {
		const ArpAddonManager::BasicAddon* addon = AddonAt(i);
		if( addon && addon->LongName() ) {
			BMessage* msg = NULL;
			if( tmpl ) {
				msg = new BMessage(*tmpl);
				if( msg ) {
					const char* pname = addon->Name();
					if( pname ) msg->AddString("addonname",pname);
					pname = addon->Path().Path();
					if( pname ) msg->AddString("addonfile",pname);
				}
			}
			BMenuItem *item = new BMenuItem(addon->LongName(),msg);
			if( item ) {
				inmenu->AddItem(item);
				num++;
			} else delete msg;
		}
	}
	
	return num;
}

ArpAddonManager::BasicAddon* ArpAddonManager::AllocAddon(BEntry* entry)
{
	if( entry ) {
		BasicAddon* addon = new BasicAddon(entry);
		if( addon ) {
			if( addon->InitCheck() == B_NO_ERROR ) return addon;
			delete addon;
		}
	}
	return NULL;
}

void ArpAddonManager::AddAddon(ArpAddonManager::BasicAddon* addon)
{
	BAutolock(Lock());
	if( addon ) addons.AddItem(addon);
}

bool ArpAddonManager::RemoveAddon(ArpAddonManager::BasicAddon* addon)
{
	BAutolock(Lock());
	if( addon ) return addons.RemoveItem(addon);
	return false;
}

ArpAddonManager::BasicAddon* ArpAddonManager::RemoveAddon(int32 index)
{
	BAutolock(Lock());
	return (ArpAddonManager::BasicAddon*)addons.RemoveItem(index);
}

bool ArpAddonManager::RemoveAddons(int32 index, int32 count)
{
	BAutolock(Lock());
	return addons.RemoveItems(index,count);
}
