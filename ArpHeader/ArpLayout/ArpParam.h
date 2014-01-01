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
 * ArpParam.h
 *
 * The intention here is to be able to create a template class that can
 * be used just like a raw data type (int32, float, BString, etc.), but
 * will also be able to automatically put and get itself from BMessage
 * objects.
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
 *	• 8/30/1998:
 *	  Created this file.
 *
 */

#ifndef ARPLAYOUT_ARPPARAM_H
#define ARPLAYOUT_ARPPARAM_H

#ifndef _MESSAGE_H
#include <be/app/Message.h>
#endif

#ifndef _PROPERTY_INFO_H
#include <be/app/PropertyInfo.h>
#endif

#ifndef __BSTRING__
#include <be/support/String.h>
#endif

// Forward reference parameter and global sets.
class ArpGlobalSetI;
class ArpParamSet;

enum {
	// This is our own custom type.  It indicates that the value for this field
	// is stored in some other "global" message, under the given name.  The value is
	// actually stored as a \0-terminated string.
	// the meaning of what a "global" message is and where to find it is undefined.
	ARP_INDIRECT_TYPE = 'INDr'
};

enum param_value_type
{
	ARP_ENUM_VALUE,
	ARP_MASK_VALUE
};

struct param_value_item
{
	const char*			name;
	param_value_type	type;
	uint64				value;
};

struct parameter_info
{
	size_t						length;
	property_info*				property;
	uint32						local_change_flags;
	uint32						global_change_flags;
	const param_value_item*		values;
};

class ArpParamI
{
public:
	ArpParamI()												{ }
	virtual ~ArpParamI()									{ }
	
	status_t Init(ArpParamSet* set, const char* name);
	
	struct std_arg {
		ArpParamSet*			set;
		ArpGlobalSetI*			globals;
		const parameter_info*	info;
	};
	
	virtual	const char* Name(std_arg& sarg) const			{ return sarg.info->property->name; }
	virtual type_code Type(std_arg& sarg) const				{ return sarg.info->property->types[0]; }

	// Return reference to global value:
	// * NULL if this parameter can not reference a global.
	// * "" if parameter is not currently referencing a global.
	// * or the name of the global it is referencing.
	virtual const char* GlobalRef(std_arg&) const			{ return 0; }
	
	virtual status_t ArchiveParam(std_arg& sarg,
								  BMessage* msg,
								  const char* value = 0,
								  const char* global = 0) const		= 0;
	virtual status_t InstantiateParam(std_arg& std,
									  const BMessage* msg,
									  const char* value = 0,
									  const char* global = 0)		= 0;
	
	virtual status_t GetParam(std_arg& sarg,
							  BMessage* msg,
							  const char* name,
							  const char* value=0) const	= 0;
	virtual status_t SetParam(std_arg& sarg,
							  const BMessage* msg,
							  const char* name,
							  const char* value)			= 0;

protected:
	void MakeGlobalName(std_arg& sarg, BString* name) const;
	void MakeValueName(std_arg& sarg, BString* name) const;
	
private:
	// no copy or assignment.
	ArpParamI(const ArpParamI& other);
	ArpParamI& operator=(const ArpParamI& other);
};

class ArpGlobalParamI : public ArpParamI
{
public:
	ArpGlobalParamI();
	ArpGlobalParamI(ArpParamSet* set, const char* name, const char* initGlobal=0);
	virtual ~ArpGlobalParamI();
	
	status_t Init(ArpParamSet* set, const char* name, const char* initGlobal=0);
	
	virtual const char* GlobalRef(std_arg& std) const;
	void SetGlobalRef(const char* name);
	bool HasGlobalRef() const;
	
	virtual status_t ArchiveParam(std_arg& std, BMessage* msg,
								  const char* value = 0,
								  const char* global = 0) const;
	virtual status_t InstantiateParam(std_arg& std, const BMessage* msg,
									  const char* value = 0,
									  const char* global = 0);
	
	virtual status_t GetParam(std_arg& std, BMessage* msg,
							  const char* name,
							  const char* global=0) const;
	virtual status_t SetParam(std_arg& std, const BMessage* msg,
							  const char* name,
							  const char* global);

protected:
	virtual status_t SetValue(std_arg& std,
							  const BMessage* msg,
							  const char* name)				= 0;
	virtual status_t GetValue(std_arg& std,
							  BMessage* msg,
							  const char* name) const		= 0;
	
	status_t ApplyGlobal(std_arg& std);
	
private:
	// no copy or assignment.
	ArpGlobalParamI(const ArpGlobalParamI& other);
	ArpGlobalParamI& operator=(const ArpGlobalParamI& other);
	
	BString					mGlobal;
};

#endif
