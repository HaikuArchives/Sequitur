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
 * ArpSFFStructs.h
 *
 * Definition of the structures used in ARP's "structured file format".
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * Still in development.
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
 * Feb. 6, 1999:
 *	Created this file.
 *
 */

#ifndef SBF_SBFCRC_H
#define SBF_SBFCRC_H

#ifndef _SUPPORT_DEFS_H
#include <support/SupportDefs.h>
#endif

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

uint32 SBFUpdateCRC(uint32 crc, uint8 *buf, size_t len);

/* Return the CRC of the bytes buf[0..len-1]. */
uint32 SBFCRC(uint8 *buf, size_t len);

#endif
