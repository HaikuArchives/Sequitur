//Temporary fix
#ifndef ARP_CORE_STRING16_H
#define ARP_CORE_STRING16_H

#include <String.h>
#include <stdlib.h>

class BString16 : public BString {
public:
	BString16()
	:
	BString()
	{
		
	}
	const char*
	AsAscii() const
	{
		return String();
	}
	BString16(const BString16* str)
	:
	BString()
	{
		SetTo(str->String());
	}
	BString16(const char* str)
	:
	BString()
	{
		SetTo(str);
	}
	operator const char*()
	{
		return String();
	}
	void	SetTo(const char* str, int32 maxLength)
	{
		BString::SetTo(str, maxLength);
		fPointer = NULL;
	}
	void	SetTo(const char* str)
	{
		SetTo(str, strlen(str));
	}
	char*	WinString()
	{
		if(fPointer)
			return fPointer;
		int32 bytes = CountBytes(0, CountChars());
		fPointer = new char[bytes];
		CopyInto(fPointer, 0, bytes);
		return fPointer;
	}
	char*	WinString() const
	{
		if(fPointer)
			return fPointer;
		int32 bytes = CountBytes(0, CountChars());
		char* c = new char[bytes];
		CopyInto(c, 0, bytes);
		return c;
	}
	void	WinRelease()
	{
		fPointer = NULL;
	}
	
	void	operator=(const char*a)
	{
		SetTo(a);
	}
	
	int32	AsInt32()
	{
		return atoi(String());
	}
	int32	AsInt32() const
	{
		return atoi(String());
	}
	
	void	operator=(const char*a) const
	{
		const_cast<BString16*>(this)->SetTo(a);
	}
private:
	char* fPointer;
};

#endif
