/*
	
	ArpEmulatorManager.cpp
	
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

#ifndef ARPTERMINAL_ARPEMULATORMANAGER_H
#include "ArpTerminal/ArpEmulatorManager.h"
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

ArpEmulatorManager::EmulatorAddon::EmulatorAddon(BEntry* entry)
	: BasicAddon(entry), interface(NULL)
{
	if( inherited::InitCheck() == B_NO_ERROR ) {
		ArpD(cdb << ADH << "Opening: " << Path().Path() << std::endl);
		image_id image = Open();
		if( image >= 0 ) {
			ArpD(cdb << ADH << "Image = " << image << std::endl);
			ArpEmulatorAddonFunc* func;
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
			if( get_image_symbol(image,"GetEmulatorAddon",
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

ArpEmulatorManager::EmulatorAddon::EmulatorAddon(const ArpEmulatorAddon* emulator)
	: BasicAddon((const char*)NULL), interface(emulator)
{
}

ArpEmulatorManager::EmulatorAddon::~EmulatorAddon()
{
	if( interface ) Close();
}

status_t ArpEmulatorManager::EmulatorAddon::InitCheck(void) const
{
	if( !interface ) return B_ERROR;
	return B_NO_ERROR;
	//return inherited::InitCheck();
}

const ArpEmulatorAddon* ArpEmulatorManager::EmulatorAddon::GetEmulator(void) const
{
	return interface;
}

void ArpEmulatorManager::AddStaticEmulator(const ArpEmulatorAddon* emulator)
{
	BAutolock(Lock());
	if( emulator ) {
		EmulatorAddon* emuadd = new EmulatorAddon(emulator);
		if( emuadd ) AddAddon(emuadd);
	}
}

ArpEmulatorManager::BasicAddon* ArpEmulatorManager::AllocAddon(BEntry* entry)
{
	if( entry ) {
		EmulatorAddon* addon = new EmulatorAddon(entry);
		if( addon ) {
			if( addon->InitCheck() == B_NO_ERROR ) return addon;
			delete addon;
		}
	}
	return NULL;
}

int32 ArpEmulatorManager::CreateAddonMenu(BMenu* inmenu, BMessage* tmpl) const
{
	if( !inmenu ) return -1;
	
	BAutolock(Lock());
	int32 num=0;
	for( int i=0; i<CountAddons(); i++ ) {
		const EmulatorAddon* adimage = (EmulatorAddon*)AddonAt(i);
		const ArpEmulatorAddon* addon = adimage->GetEmulator();
		if( addon ) {
			for( int j=0; j<addon->CountEmulations(); j++ ) {
				const ArpEmulationType* type = addon->EmulationType(j);
				if( type && !type->Synonym ) {
					BMessage* msg = NULL;
					if( tmpl ) msg = new BMessage(*tmpl);
					else msg = new BMessage(TERM_SELECT_EMULATION_MSG);
					if( msg && !msg->AddString("name",type->Name) ) {
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

const ArpEmulatorAddon* ArpEmulatorManager::EmulatorAddonForType(
													const char* name) const
{
	if( !name ) return NULL;
	
	BAutolock(Lock());
	for( int i=0; i<CountAddons(); i++ ) {
		const EmulatorAddon* adimage = (EmulatorAddon*)AddonAt(i);
		const ArpEmulatorAddon* addon = adimage->GetEmulator();
		if( addon ) {
			for( int j=0; j<addon->CountEmulations(); j++ ) {
				const ArpEmulationType* type = addon->EmulationType(j);
				if( strcmp(name,type->Name) == 0 ) return addon;
			}
		}
	}
	return NULL;
}

ArpEmulatorInterface* ArpEmulatorManager::AllocEmulator(const char* name,
									ArpTerminalInterface* terminal) const
{
	BAutolock(Lock());
	const ArpEmulatorAddon* addon = EmulatorAddonForType(name);
	if( addon ) {
		return addon->AllocEmulator(name,*terminal);
	}
	
	return NULL;
}
