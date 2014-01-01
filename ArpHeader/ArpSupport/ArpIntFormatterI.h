/* ArpIntFormatterI.h
 * Copyright (c)2000 by Eric Hackborn.
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * To do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2000.09.04		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPSUPPORT_ARPINTFORMATTERI_H
#define ARPSUPPORT_ARPINTFORMATTERI_H

#include <ArpCore/String16.h>

/***************************************************************************
 * ARP-INT-FORMATTER-I
 * This interface is used to take ints and convert them into strings for
 * display to the user.  The intention is to provide clients with pluggable
 * behaviour for completely customizing the display of numbers.
 *
 * This class is quite similar to the ArpIntToStringMapI.  It would be nice
 * if there was only one class to handle this general int-to-string-to-int
 * conversion, but right now there are a few differences that make me want
 * to have this separate class:  The map is intended to translate both
 * from ints to strings and back again.  Also, the map is intended to work
 * on a known set of ints and strings -- i.e., it's not creating strings.
 * This formatter creates a string with each use.
 ***************************************************************************/
class ArpIntFormatterI
{
public:
	virtual void FormatInt(int32 number, BString16& out) const = 0;
};

#endif

