/*
	
	ArpParam.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#include <ArpLayout/ArpGlobalSet.h>
#endif

#ifndef ARPLAYOUT_ARPPARAMSET_H
#include <ArpLayout/ArpParamSet.h>
#endif

#ifndef ARPLAYOUT_ARPPARAM_H
#include <ArpLayout/ArpParam.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

// --------------------------------------------------------------------------
//                                ArpParamI
// --------------------------------------------------------------------------

status_t
ArpParamI::Init(ArpParamSet* set, const char* name)
{
	return set->AttachParameter(name, this) ? B_OK : B_ERROR;
}

void
ArpParamI::MakeGlobalName(std_arg& std, BString* name) const
{
	const char* base = Name(std);
	const int32 len = strlen(base) + 5;
	char* buf = name->LockBuffer(len);
	strcpy(buf, "glb:");
	strcat(buf, base);
	name->UnlockBuffer(len);
}

void
ArpParamI::MakeValueName(std_arg& std, BString* name) const
{
	const char* base = Name(std);
	const int32 len = strlen(base) + 5;
	char* buf = name->LockBuffer(len);
	strcpy(buf, "val:");
	strcat(buf, base);
	name->UnlockBuffer(len);
}

// --------------------------------------------------------------------------
//                                ArpGlobalParamI
// --------------------------------------------------------------------------

ArpGlobalParamI::ArpGlobalParamI()
{
}

ArpGlobalParamI::ArpGlobalParamI(ArpParamSet* set, const char* name, const char* initGlobal=0)
{
	Init(set, name, initGlobal);
}

ArpGlobalParamI::~ArpGlobalParamI()
{
	// Note -- can't automatically detach because we don't have
	// a reference back to the parameter set.
	//if( mParamSet ) mParamSet->DetachParameter(this);
}

status_t
ArpGlobalParamI::Init(ArpParamSet* set, const char* name, const char* initGlobal=0)
{
	status_t err = ArpParamI::Init(set, name);
	mGlobal = initGlobal;
	return err;
}

const char*
ArpGlobalParamI::GlobalRef(std_arg& std) const
{
	return mGlobal.String();
}

void
ArpGlobalParamI::SetGlobalRef(const char* name)
{
	ArpD(cdb << ADH << "Setting global of " << this
			<< " to " << (name ? name : "<NULL>") << endl);
	mGlobal = name;
}

bool
ArpGlobalParamI::HasGlobalRef() const
{
	return mGlobal.Length() > 0;
}

status_t
ArpGlobalParamI::ArchiveParam(std_arg& std, BMessage* msg,
							  const char* value,
							  const char* global) const
{
	BString name;
	if( !value ) {
		MakeValueName(std, &name);
		value = name.String();
	}
	ArpD(cdb << ADH << "Archiving " << Name(std)
			<< ": value name is " << value << endl);
	status_t err = GetValue(std, msg, value);
	
	if( !err && HasGlobalRef() ) {
		if( !global ) {
			MakeGlobalName(std, &name);
			global = name.String();
		}
		ArpD(cdb << ADH << "Storing global " << global
				<< ": " << GlobalRef(std) << endl);
		err = msg->AddData(global, ARP_INDIRECT_TYPE,
						   GlobalRef(std), strlen(GlobalRef(std))+1);
	}
	
	return err;
}

status_t
ArpGlobalParamI::InstantiateParam(std_arg& std, const BMessage* msg,
								  const char* value,
								  const char* global)
{
	BString name;
	if( !value ) {
		MakeValueName(std, &name);
		value = name.String();
	}
	status_t err = SetValue(std, msg, value);
	ArpD(cdb << ADH << "Instantiating " << Name(std)
			<< ": value name is " << value << endl);
			
	if( !global ) {
		MakeGlobalName(std, &name);
		global = name.String();
	}
	const char* globName=0;
	ssize_t globLen=0;
	err = msg->FindData(global, ARP_INDIRECT_TYPE,
						(const void**)&globName, &globLen);
	if( globName ) {
		SetGlobalRef(globName);
		ArpD(cdb << ADH << "Retrieving global " << global
				<< ": " << GlobalRef(std) << endl);
		err = ApplyGlobal(std);
	}
	ArpD(cdb << ADH << "Restored global " << global
			<< ": " << GlobalRef(std)
			<< " (err=" << err << ")" << endl);
	return err;
}

status_t
ArpGlobalParamI::GetParam(std_arg& std, BMessage* msg,
						  const char* name,
						  const char* value=0) const
{
	status_t err = B_OK;
	if( HasGlobalRef() ) {
		if( name ) {
			err = msg->AddData(name, ARP_INDIRECT_TYPE,
							   GlobalRef(std), strlen(GlobalRef(std))+1);
		}
		if( !err && value ) err = GetValue(std, msg, value);
	} else if( name ) {
		err = GetValue(std, msg, name);
	}
	return err;
}

status_t
ArpGlobalParamI::SetParam(std_arg& std, const BMessage* msg,
						  const char* name,
						  const char* value)
{
	status_t err = B_NAME_NOT_FOUND;
	
	if( msg && name ) {
		ArpD(cdb << ADH << "Looking for param " << name << endl);
		if( msg->HasData(name,Type(std)) ) {
			err = SetValue(std, msg, name);
			if( !err ) SetGlobalRef((const char*)0);
			return err;
		} else {
			ArpD(cdb << ADH << "Not found." << endl);
			const char* globName=0;
			ssize_t globLen=0;
			err = msg->FindData(name, ARP_INDIRECT_TYPE,
								(const void**)&globName, &globLen);
			if( globName ) {
				SetGlobalRef(globName);
				err = ApplyGlobal(std);
				ArpD(cdb << ADH << "Found global reference: "
								<< GlobalRef(std) << endl);
			}
		}
		
	} else {
		err = ApplyGlobal(std);
	}
	
	return err;
}

status_t
ArpGlobalParamI::ApplyGlobal(std_arg& std)
{
	status_t err = B_NAME_NOT_FOUND;
	
	if( HasGlobalRef() && std.globals ) {
		ArpD(cdb << ADH << "Retrieving global " << GlobalRef(std)
					<< " for " << Name(std) << endl);
		if( std.globals->GlobalValues()->HasData(GlobalRef(std),Type(std)) ) {
			err = SetValue(std, std.globals->GlobalValues(), GlobalRef(std));
			ArpD(cdb << ADH << "Result is: " << err << endl);
			
		// If the global wasn't found, and we are being updated from the
		// complete global set, add our current value to the global params.
		} else if( !std.globals->IsGlobalUpdate() ) {
			ArpD(cdb << ADH << "Global not found; adding." << endl);
			BMessage newGlob;
			err = GetValue(std, &newGlob, GlobalRef(std));
			err = std.globals->UpdateGlobals(&newGlob);
			return err;
			
		} else {
			ArpD(cdb << ADH << "Global value not found." << endl);
		}
	}
	return err;
}
