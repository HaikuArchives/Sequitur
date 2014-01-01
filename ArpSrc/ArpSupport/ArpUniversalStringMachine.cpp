#include <ArpCore/ArpChar.h>
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
	mString = arp_char_from_wchar(str->WinString());
	return mString;
}

const BString16* ArpUniversalStringMachine::String16(const char* str)
{
	mString16.WinRelease();
	if (str) mString16 = str;
	return &mString16;
}
