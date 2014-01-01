//Temporary fix
#include <String.h>

class BString16 : public BString {
public:
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
private:
	char* fPointer;
};
