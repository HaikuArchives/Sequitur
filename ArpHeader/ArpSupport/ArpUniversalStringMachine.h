/* ArpUniversalStringMachine.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2004.06.16			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef ARPSUPPORT_ARPUNIVERSALSTRINGMACHINE_H
#define ARPSUPPORT_ARPUNIVERSALSTRINGMACHINE_H

#include <ArpCore/String8.h>
#include <ArpCore/String16.h>

/***************************************************************************
 * ARP-UNIVERSAL-STRING-MACHINE
 * A utility for massaging strings into the necessary format.
 ****************************************************************************/
class ArpUniversalStringMachine
{
public:
	ArpUniversalStringMachine();
	~ArpUniversalStringMachine();

	const char*			String(const BString16* str);

	const BString16*	String16(const char* str);

private:
	char*				mString;
	BString16			mString16;
};

#endif
