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
 * ArpMessage.h
 *
 * A slightly more convenient interface to constructing and
 * accessing BMessage objects.  This includes two things:
 *
 * New methods for adding and retrieving BFont and rgb_color
 * structures from a BMessage.
 *
 * A simpler set of functions for placing data in a BMessage,
 * useful to avoid creating temporary variables.  It basically
 * works like this:
 *
 * FunctionThatWantsMessage( ArpMessage()
 *                           .SetInt32("SomeValue", 234)
 *                           .SetString("AnotherValue", "A string")
 *                           ... );
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
 * 0.1: Created this file.
 *
 */

#ifndef ARPKERNEL_ARPMESSAGE_H
#define ARPKERNEL_ARPMESSAGE_H

#ifndef _MESSAGE_H
#include <Message.h>
#endif

#ifndef _FFONT_H
#include <FFont/FFont.h>
#endif

BMessage& ArpUpdateMessage(BMessage& to, const BMessage& from);

class ArpMessage : public BMessage {

  public:

	ArpMessage();
	ArpMessage(uint32 what);
	ArpMessage(const BMessage& a_message);
	
	ArpMessage&	operator=(const BMessage& msg);

	ArpMessage& Update(const BMessage& msg);
		
	// Standard types there is no BMessage interface for
	status_t	AddRGBColor(const char *name, const rgb_color *col);
	status_t	FindRGBColor(const char *name, rgb_color *col) const;
	status_t	FindRGBColor(const char *name, int32 index, rgb_color *col) const;
	bool		HasRGBColor(const char* name, int32 index=0) const;
	status_t	ReplaceRGBColor(const char *name, const rgb_color *col);
	status_t	ReplaceRGBColor(const char *name, int32 index, const rgb_color *col);
	
	// My custom types
	status_t	AddFont(const char *name, const BFont *font);
	status_t	FindFont(const char *name, BFont *font) const;
	status_t	FindFont(const char *name, int32 index, BFont *font) const;
	bool		HasFont(const char* name, int32 index=0) const;
	status_t	ReplaceFont(const char *name, const BFont *font);
	status_t	ReplaceFont(const char *name, int32 index, const BFont *font);
	
	status_t	AddIndirect(const char *name, const char* ind);
	status_t	FindIndirect(const char *name, const char** ind) const;
	status_t	FindIndirect(const char *name, int32 index, const char** ind) const;
	bool		HasIndirect(const char* name, int32 index=0) const;
	status_t	ReplaceIndirect(const char *name, const char* ind);
	status_t	ReplaceIndirect(const char *name, int32 index, const char* ind);
	
	ArpMessage&	SetRect(const char *name, const BRect& a_rect);
	ArpMessage&	SetPoint(const char *name, const BPoint& a_point);
	ArpMessage&	SetString(const char *name, const char* a_string);
	ArpMessage&	SetInt8(const char *name, int8 val);
	ArpMessage&	SetInt16(const char *name, int16 val);
	ArpMessage&	SetInt32(const char *name, int32 val);
	ArpMessage&	SetInt64(const char *name, int64 val);
	ArpMessage&	SetBool(const char *name, bool a_boolean);
	ArpMessage&	SetFloat(const char *name, float a_float);
	ArpMessage&	SetDouble(const char *name, double a_double);
	ArpMessage&	SetPointer(const char *name, const void *ptr);
	ArpMessage&	SetMessenger(const char *name, const BMessenger& messenger);
	ArpMessage&	SetRef(const char *name, const entry_ref *ref);
	ArpMessage&	SetMessage(const char *name, const BMessage *msg);
	ArpMessage&	SetRGBColor(const char *name, const rgb_color *col);
	ArpMessage&	SetFont(const char *name, const BFont *font);
	ArpMessage&	SetIndirect(const char *name, const char* indirectField);
	ArpMessage&	SetData(const char *name,
						type_code type,
						const void *data,
						ssize_t numBytes);

	// These were an idea that maybe should just go away.
	const BRect			GetRect(const char *name, const BRect& def, int32 index=0) const;
	const BPoint		GetPoint(const char *name, const BPoint& def, int32 index=0) const;
	const char*			GetString(const char *name, const char* def, int32 index=0) const;
	int8				GetInt8(const char *name, int8 def, int32 index=0) const;
	int16				GetInt16(const char *name, int16 def, int32 index=0) const;
	int32				GetInt32(const char *name, int32 def, int32 index=0) const;
	int64				GetInt64(const char *name, int64 def, int32 index=0) const;
	bool				GetBool(const char *name, bool def, int32 index=0) const;
	float				GetFloat(const char *name, float def, int32 index=0) const;
	double				GetDouble(const char *name, double def, int32 index=0) const;
	void*				GetPointer(const char *name, void* def, int32 index=0) const;
	const BMessenger	GetMessenger(const char *name, const BMessenger& def, int32 index=0) const;
	const entry_ref		GetRef(const char *name, const entry_ref& def, int32 index=0) const;
	const BMessage		GetMessage(const char *name, const BMessage& def, int32 index=0) const;
	const rgb_color		GetRGBColor(const char *name, const rgb_color& def, int32 index=0) const;
	const BFont			GetFont(const char *name, const BFont& def, int32 index=0) const;
	const char*			GetIndirect(const char *name, const char* def, int32 index=0) const;
	const void*			GetData(const char *name, type_code type,
								ssize_t* gotBytes, const void* def,
								int32 index=0) const;

	status_t GetError(void) const { return status; }
	void ClrError(void) { status = B_NO_ERROR; }
	
  private:
	status_t status;
};

/* ------------------------------------------------------------- */

inline ArpMessage::ArpMessage()
	: BMessage(), status(B_NO_ERROR)
{ }

inline ArpMessage::ArpMessage(uint32 whatCode)
	: BMessage(whatCode), status(B_NO_ERROR)
{ }

inline ArpMessage::ArpMessage(const BMessage& msg)
	: BMessage(msg), status(B_NO_ERROR)
{ }

inline ArpMessage&	ArpMessage::operator=(const BMessage& msg)
{
	*(BMessage*)this = msg;
	status = B_NO_ERROR;
	return *this;
}

#endif
