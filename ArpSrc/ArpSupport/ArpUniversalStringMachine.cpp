#include <ArpSupport/ArpUniversalStringMachine.h>

/*******************************************************
 * ARP-UNIVERSAL-STRING-MACHINE
 *******************************************************/
ArpUniversalStringMachine::ArpUniversalStringMachine()
		: mString(NULL)
{
}

ArpUniversalStringMachine::~ArpUniversalStringMachine()
{
	delete[] mString;
}

const char* ArpUniversalStringMachine::String(const BString16* str)
{
	if (!str || !(str->WinString())) return NULL;
	delete[] mString;
	mString = str->WinString();
	return mString;
}

const BString16* ArpUniversalStringMachine::String16(const char* str)
{
	mString16.WinRelease();
	if (str) mString16.SetTo(str);
	return &mString16;
}
