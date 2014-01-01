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

#ifndef ARPKERNEL_ARPPARAM_H
#define ARPKERNEL_ARPPARAM_H

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _TYPE_CONSTANTS_H
#include <support/TypeConstants.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpCommon/ArpString.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpCommon/ArpMessage.h>
#endif

class ArpBaseParam {

public:
	
	ArpBaseParam(ArpString name, type_code type);
	virtual ~ArpBaseParam();

	ArpString Name(void) const { return mName; }
	type_code Type(void) const { return mType; }

	virtual static_t MessageToParam(BMessage* msg) = 0;
	virtual static_t ParamToMessage(BMessage* msg) = 0;
	
private:

	ArpString mName;
	type_code mType;
};

// Given a pointer to some type, return its corresponding type_code.
// Some things worth noting:
//	• There are two string types:
//	  B_STRING_TYPE is mapped to the C type "const char*", because
//	  this type_code is defined to be null-terminated.
//	  B_ASCII_TYPE is mapped to ArpString, which can contain embedded
//	  \0 characters.
inline type_code ArpTypeCodeOf(const ArpString* )	{ return B_ASCII_TYPE; }
inline type_code ArpTypeCodeOf(const bool* )		{ return B_BOOL_TYPE; }
inline type_code ArpTypeCodeOf(const char* )		{ return B_CHAR_TYPE; }
inline type_code ArpTypeCodeOf(const double* )		{ return B_DOUBLE_TYPE; }
inline type_code ArpTypeCodeOf(const float* )		{ return B_FLOAT_TYPE; }
inline type_code ArpTypeCodeOf(const int64* )		{ return B_INT64_TYPE; }
inline type_code ArpTypeCodeOf(const int32* )		{ return B_INT32_TYPE; }
inline type_code ArpTypeCodeOf(const int16* )		{ return B_INT16_TYPE; }
inline type_code ArpTypeCodeOf(const int8* )		{ return B_INT8_TYPE; }
inline type_code ArpTypeCodeOf(const BMessage* )	{ return B_MESSAGE_TYPE; }
inline type_code ArpTypeCodeOf(const BMessenger* )	{ return B_MESSENGER_TYPE; }
inline type_code ArpTypeCodeOf(const off_t* )		{ return B_OFF_T_TYPE; }
inline type_code ArpTypeCodeOf(const void** )		{ return B_POINTER_TYPE; }
inline type_code ArpTypeCodeOf(const BPoint* )		{ return B_POINT_TYPE; }
inline type_code ArpTypeCodeOf(const BRect* )		{ return B_RECT_TYPE; }
inline type_code ArpTypeCodeOf(const entry_ref* )	{ return B_REF_TYPE; }
inline type_code ArpTypeCodeOf(const rgb_color* )	{ return B_RGB_COLOR_TYPE; }
inline type_code ArpTypeCodeOf(const size_t* )		{ return B_SIZE_T_TYPE; }
inline type_code ArpTypeCodeOf(const ssize_t* )		{ return B_SSIZE_T_TYPE; }
inline type_code ArpTypeCodeOf(const char** )		{ return B_STRING_TYPE; }
inline type_code ArpTypeCodeOf(const time_t* )		{ return B_TIME_TYPE; }
inline type_code ArpTypeCodeOf(const uint64* )		{ return B_UINT64_TYPE; }
inline type_code ArpTypeCodeOf(const uint32* )		{ return B_UINT32_TYPE; }
inline type_code ArpTypeCodeOf(const uint16* )		{ return B_UINT16_TYPE; }
inline type_code ArpTypeCodeOf(const uint8* )		{ return B_UINT8_TYPE; }

inline type_code ArpTypeCodeOf(const BFont* )		{ return FFont::B_FONT_TYPE; }

//inline type_code ArpTypeCodeOf(const * )		{ return B__TYPE; }

struct ArpGetParamInfo {
	const BMessage* message;
	const char* name;
	type_code type;
	int32 index;
	void* data;
	ssize_t numBytes;
};

status_t ArpGetParam(const ArpGetParamInfo& info, ArpString* param)
{ (*param).Set((const ichar*)info.data, info.numBytes); }
status_t ArpGetParam(const ArpGetParamInfo& info, bool* param)
{ (*param) = *(bool*)info.data; }
status_t ArpGetParam(const ArpGetParamInfo& info, char* param)
{ (*param) = *(char*)info.data; }
status_t ArpGetParam(const ArpGetParamInfo& info, * param)
{ (*param) = info.data; }

status_t ArpSetParam(const BMessage& message, const char* name, type_code type,
                     ArpString* param)
{ message.AddData(name, type, (const char*)(*param), param->Length()+1, false); }

template<class t>
class ArpParam<T> : public ArpBaseParam
{
public:
	ArpParam(ArpString name)
		: ArpBaseParam(name, ArpTypeCodeOf(&mValue)) { }
	ArpParam(ArpString name, T initValue);
		: ArpBaseParam(name, ArpTypeCodeOf(&mValue)), mValue(initValue) { }
	ArpParam(ArpString name, type_code type);
		: ArpBaseParam(name, type) { }
	ArpParam(ArpString name, T initValue, type_code type);
		: ArpBaseParam(name, type), mValue(initValue) { }

	ArpParam(const ArpParam<T>& other)
		: ArpBaseParam(other), mValue(other.mValue) { }
	
	ArpParam& operator=(const ArpParam<T>& other) { mValue = other.mValue; }
	ArpParam& operator=(const T& other) { mValue = other; }
	
	inline T operator T() const { return mValue; }
	
private:
	T mValue;
}

#endif
