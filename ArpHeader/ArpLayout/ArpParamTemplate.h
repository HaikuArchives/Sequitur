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

#ifndef ARPLAYOUT_ARPPARAMTEMPLATE_H
#define ARPLAYOUT_ARPPARAMTEMPLATE_H

#ifndef ARPLAYOUT_ARPPARAM_H
#include <ArpLayout/ArpParam.h>
#endif

#ifndef _FFONT_H
#include <FFont/FFont.h>
#endif

#ifndef __BSTRING__
#include <support/String.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

// --------------------- Getting Parameters ---------------------

template<class T> status_t ArpFindParam(T* value, const BMessage* msg,
										const char* name, type_code type,
										int32 index)
{
	T* get = 0;
	ssize_t len = 0;
	status_t err = msg->FindData(name, type, index, (const void**)&get, &len);
	if( err ) {
		ArpD(cdb << ADH << "Unable to find data: " << name
				<< " at index " << index << std::endl);
		return err;
	}
	ArpVALIDATE(len == sizeof(T), return B_MISMATCHED_VALUES);
	*value = *get;
	return err;
}

inline
status_t ArpFindParam(BFont* value, const BMessage* msg,
								 const char* name, type_code type,
								 int32 index)
{
	ArpVALIDATE(type == FFont::FONT_TYPE, return B_BAD_TYPE);
	return FindMessageFont(msg, name, index, value);
}

inline
status_t ArpFindParam(BString* value, const BMessage* msg,
								 const char* name, type_code type,
								 int32 index)
{
	const char* get = 0;
	ssize_t len = 0;
	status_t err = msg->FindData(name, type, index, (const void**)&get, &len);
	if( err ) {
		ArpD(cdb << ADH << "Unable to find data: " << name
				<< " at index " << index << std::endl);
		return err;
	}
	if( get[len] == 0 ) len--;
	value->SetTo(get, len);
	return err;
}

// --------------------- Setting Parameters ---------------------

template<class T> status_t ArpAddParam(const T* value, BMessage* msg,
									   const char* name, type_code type)
{
	return msg->AddData(name, type, value, sizeof(T));
}

inline
status_t ArpAddParam(const BFont* value, BMessage* msg,
								const char* name, type_code type)
{
	ArpVALIDATE(type == FFont::FONT_TYPE, return B_BAD_TYPE);
	return AddMessageFont(msg, name, value);
}

inline
status_t ArpAddParam(const BString* value, BMessage* msg,
								const char* name, type_code type)
{
	return msg->AddData(name, type, value->String(), value->Length()+1);
}

// --------------------- Templatized Standard Parameter ---------------------

template<class T>
class ArpParam : public ArpParamI
{
public:
	ArpParam()
	{
	}
	ArpParam(ArpParamSet* set, const char* name, const T& initValue)
		: ArpParamI(set, name),
		  mValue(initValue)
	{
	}
	virtual ~ArpParam()
	{
	}
	
	status_t Init(ArpParamSet* set, const char* name, const T& initValue)
	{
		status_t err = ArpParamI::Init(set, name);
		mValue = initValue;
		return err;
	}
	
	inline ArpParam& operator=(const T& other)				{ mValue = other; return *this; }
	
	inline operator T() const								{ return mValue; }
	
	inline const T* operator&() const						{ return &mValue; }
	inline T* operator&()									{ return &mValue; }
	
	inline const T& Value() const							{ return mValue; }
	inline T& Value()										{ return mValue; }

	virtual status_t ArchiveParam(std_arg& sarg,
								  BMessage* msg,
								  const char* value = 0,
								  const char* global = 0) const
	{
		BString name;
		if( !value ) {
			MakeValueName(sarg, &name);
			value = name.String();
		}
		ArpD(cdb << ADH << "Archiving " << Name(sarg)
				<< ": value name is " << value << std::endl);
		return ArpAddParam(&mValue, msg, value, Type(sarg));
	}
	
	virtual status_t InstantiateParam(std_arg& sarg,
									  const BMessage* msg,
									  const char* value = 0,
									  const char* global = 0)
	{
		BString name;
		if( !value ) {
			MakeValueName(sarg, &name);
			value = name.String();
		}
		ArpD(cdb << ADH << "Instantiating " << Name(sarg)
				<< ": value name is " << value << std::endl);
		return ArpFindParam(&mValue, msg, value, Type(sarg), 0);
	}
	
	virtual status_t GetParam(std_arg& sarg,
							  BMessage* msg,
							  const char* name,
							  const char* value=0) const
	{
		if( !name ) return B_OK;
		return ArpAddParam(&mValue, msg, name, Type(sarg));
	}
	
	virtual status_t SetParam(std_arg& sarg,
							  const BMessage* msg,
							  const char* name,
							  const char* value)
	{
		status_t err = B_NAME_NOT_FOUND;
		if( name ) err = ArpFindParam(&mValue, msg, name, Type(sarg), 0);
		if( err && value ) {
			err = ArpFindParam(&mValue, msg, value, Type(sarg), 0);
		}
		return err;
	}
	
private:
	// no copy or assignment.
	ArpParam(const ArpParam<T>& other);
	ArpParam& operator=(const ArpParam<T>& other);
	
	T mValue;
};

// --------------------- Templatized Global Parameter ---------------------

template<class T>
class ArpGlobalParam : public ArpGlobalParamI
{
public:
	ArpGlobalParam()
	{
	}
	ArpGlobalParam(ArpParamSet* set, const char* name,
			 const T& initValue, const char* initGlobal=0)
		: ArpGlobalParamI(set, name, initGlobal),
		  mValue(initValue)
	{
	}
	virtual ~ArpGlobalParam()
	{
	}
	
	status_t Init(ArpParamSet* set, const char* name,
				  const T& initValue, const char* initGlobal=0)
	{
		status_t err = ArpGlobalParamI::Init(set, name, initGlobal);
		mValue = initValue;
		return err;
	}
	
	inline ArpGlobalParam& operator=(const T& other)		{ mValue = other; return *this; }
	
	inline operator T() const								{ return mValue; }
	
	inline const T* operator&() const						{ return &mValue; }
	inline T* operator&()									{ return &mValue; }
	
	inline const T& Value() const							{ return mValue; }
	inline T& Value()										{ return mValue; }

protected:
	virtual status_t SetValue(std_arg& sarg,
							  const BMessage* msg,
							  const char* name)
	{
		return ArpFindParam(&mValue, msg, name, Type(sarg), 0);
	}
	
	virtual status_t GetValue(std_arg& sarg,
							  BMessage* msg,
							  const char* name) const
	{
		return ArpAddParam(&mValue, msg, name, Type(sarg));
	}

private:
	// no copy or assignment.
	ArpGlobalParam(const ArpGlobalParam<T>& other);
	ArpGlobalParam& operator=(const ArpGlobalParam<T>& other);
	
	T mValue;
};

#endif
