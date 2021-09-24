/*
	
	ArpRemoteManager.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef APRKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef APRKERNEL_ARPSTRING_H
#include "ArpKernel/ArpString.h"
#endif

#ifndef ARPTERMINAL_ARPREMOTEMANAGER_H
#include "ArpTerminal/ArpRemoteManager.h"
#endif

#ifndef ARPTERMINAL_ARPTERMINALMSG_H
#include "ArpTerminal/ArpTerminalMsg.h"
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef _MENUITEM_H
#include <interface/MenuItem.h>
#endif

#include <cstring>

ArpMOD();

ArpRemoteManager::RemoteAddon::RemoteAddon(BEntry* entry)
	: BasicAddon(entry), interface(NULL)
{
	if( inherited::InitCheck() == B_NO_ERROR ) {
		ArpD(cdb << ADH << "Opening: " << Path().Path() << std::endl);
		image_id image = Open();
		if( image >= 0 ) {
			ArpD(cdb << ADH << "Image = " << image << std::endl);
			ArpRemoteAddonFunc* func;
#ifdef DEBUG
			char name[1024];
			int32 len = sizeof(name);
			int32 type = 0;
			for( int32 i=0; get_nth_image_symbol(image,i,&name[0],
								&len,&type,(void**)&func) == B_NO_ERROR; i++ ) {
				ArpD(cdb << ADH << "Sym " << i << ": name=" << &name[0]
								<< ", type=" << type << ", func=" << (void*)func << std::endl);
				len = sizeof(name);
			}
#endif
			if( get_image_symbol(image,"GetRemoteAddon",
								B_SYMBOL_TYPE_TEXT,(void**)&func) == B_NO_ERROR ) {
				ArpD(cdb << ADH << "Function = " << (void*)func << std::endl);
				interface = (*func)();
			}
			if( interface == NULL ) Close();
		}
	}
	ArpD(cdb << ADH << "Addon " << Path().Path() << ": interface="
				<< (void*)interface << std::endl);
}

ArpRemoteManager::RemoteAddon::RemoteAddon(const ArpRemoteAddon* remote)
	: BasicAddon((const char*)NULL), interface(remote)
{
}

ArpRemoteManager::RemoteAddon::~RemoteAddon()
{
	if( interface ) Close();
}

status_t ArpRemoteManager::RemoteAddon::InitCheck(void) const
{
	if( !interface ) return B_ERROR;
	return B_NO_ERROR;
	//return inherited::InitCheck();
}

const ArpRemoteAddon* ArpRemoteManager::RemoteAddon::GetRemote(void) const
{
	return interface;
}

void ArpRemoteManager::AddStaticRemote(const ArpRemoteAddon* remote)
{
	BAutolock(Lock());
	if( remote ) {
		RemoteAddon* emuadd = new RemoteAddon(remote);
		if( emuadd ) AddAddon(emuadd);
	}
}

ArpRemoteManager::BasicAddon* ArpRemoteManager::AllocAddon(BEntry* entry)
{
	if( entry ) {
		RemoteAddon* addon = new RemoteAddon(entry);
		if( addon ) {
			if( addon->InitCheck() == B_NO_ERROR ) return addon;
			delete addon;
		}
	}
	return NULL;
}

int32 ArpRemoteManager::CreateAddonMenu(BMenu* inmenu, BMessage* tmpl) const
{
	if( !inmenu ) return -1;
	
	BAutolock(Lock());
	int32 num=0;
	for( int i=0; i<CountAddons(); i++ ) {
		const RemoteAddon* adimage = (RemoteAddon*)AddonAt(i);
		const ArpRemoteAddon* addon = adimage->GetRemote();
		if( addon ) {
			for( int j=0; j<addon->CountDevices(); j++ ) {
				const ArpRemoteType* type = addon->DeviceType(j);
				if( type && !type->Synonym ) {
					BMessage* msg = NULL;
					if( tmpl ) msg = new BMessage(*tmpl);
					else msg = new BMessage(TERM_SELECT_REMOTE_MSG);
					if( msg && !msg->AddString("url",type->URL) ) {
						BMenuItem *item
							= new BMenuItem(type->LongName,msg);
						if( item ) {
							inmenu->AddItem(item);
							num++;
						} else delete msg;
					} else delete msg;
				}
			}
		}
	}
	
	return num;
}

const ArpRemoteAddon* ArpRemoteManager::RemoteAddonForURL(
													const char* name) const
{
	if( !name ) return 0;
	
	ArpD(cdb << ADH << "Finding addon for remote " << name << std::endl);
	
	BAutolock(Lock());
	for( int i=0; i<CountAddons(); i++ ) {
		const RemoteAddon* adimage = (RemoteAddon*)AddonAt(i);
		const ArpRemoteAddon* addon = adimage->GetRemote();
		if( addon ) {
			for( int j=0; j<addon->CountDevices(); j++ ) {
				const ArpRemoteType* type = addon->DeviceType(j);
				if( strcmp(name,type->URL) == 0 ) return addon;
			}
		}
	}
	return NULL;
}

ArpRemoteInterface* ArpRemoteManager::AllocRemote(const char* name) const
{
	BAutolock(Lock());
	const ArpRemoteAddon* addon = RemoteAddonForURL(name);
	if( addon ) {
		return addon->AllocRemote(name);
	}
	
	return NULL;
}
