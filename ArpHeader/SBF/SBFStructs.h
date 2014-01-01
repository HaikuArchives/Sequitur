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

#ifndef SBF_SBFSTRUCTS_H
#define SBF_SBFSTRUCTS_H

#ifndef _SUPPORT_DEFS_H
#include <be/support/SupportDefs.h>
#endif

// Signature:
// \211 (0x89) S B F \r \n B I N A R Y \032 \n \f \000

extern const char SBFSignature[16];

struct SBFChunkHead2 {
	int32 mLength;
	uint16 mType;
	uint8 mHeadLength;
	uint8 mReserved;
	uint32 mName;
	uint32 mObject;
	int64 mContinuation;
};

// This is the definition of a full-fledged data chunk in the file.
// The overhead for a chunk is currently 52 bytes -- 48 for the header
// and 4 for the tail.

struct SBFChunkHead {
	// This is the basic type of the chunk.  Chunk types are defined by
	// the structured file format, and are what actually define the
	// "structure" of a particular file.  You may only choose from one
	// of the types defined below.
	uint32 mType;									// base + 0 bytes
	/*
		Case definitions of mType field:
		
		1:	Must be upper case (byte order detection.)
		2:	Uppercase: Start tag; Lowercase: End tag.
		3:	Unnused (always uppercase)
		4:	Must be lower case (byte order detection.)
	*/
	enum {
		BASIC_TYPE = 'CHNk',		// A basic piece of data.
		DEPENDENCY_TYPE = 'DPDn',	// A group of dependent chunks.
		FORM_TYPE = 'FORm',			// A set of chunks that work together
									// (e.g., a bitmap and its palette.)
		CATALOG_TYPE = 'CATa',		// A set of independent, related chunks
									// (e.g., two different bitmap formats
									//  of the same picture.)
		CONTINUATION_TYPE = 'CONt',	// A continuation chunk.
		
		// End chunks corresponding to the above.
		GROUP_END = 'DpDn',
		FORM_END = 'FoRm',
		CATALOG_END = 'CaTa'
	};
	
	// This is the name of the chunk, which defines the type of data it
	// contains.  It must be in the form "vendor:type", where "vendor"
	// is a unique identifier for a particular software house, and
	// "type" is a unique name within that vendor space.  For example,
	// Angry Red Planet creates chunks of type "arp:blah".  Note that
	// the name must always be \0-terminated, so the maximum number of
	// characters is 11.
	int8 mName[12];									// base + 4 bytes
	
	uint16 mFlags;									// base + 16 bytes
	enum {
		// If set, you must be able to understand this chunk to correctly
		// parse the data in its group.
		REQUIRED_FLAG = 0x0001,
		// If set, you can safely copy this chunk when changing other
		// chunks in its group, even if you don't understand what it
		// is.  (For example: a chunk containing a text annotation.
		SAFETOCOPY_FLAG = 0x0002,
	};
	
	// This is the length of the header.  Currently it is always
	// sizeof(SBFChunkHead).  You should read this many bytes to get to
	// the start of the chunk data.
	uint8 mHeadLength;								// base + 18 bytes
	
	// This is the length of the tail.  Currently it is always
	// sizeof(SBFChunkTail).  You should read this many bytes to get from
	// the end of the chunk data to the next chunk.
	uint8 mTailLength;								// base + 19 bytes
	
	// Unique object identifier for this chunk.  Zero means there is no
	// object associated with this chunk.
	uint32 mObject;									// base + 20 bytes
	
	// This is the length of data in this chunk, not including the header
	// and trailing CRC error detection integer.
	// Note that chunks must always be aligned on 4-byte boundaries,
	// so the actual amount of bytes to the next chunk is
	// (mLength+3)&0xFFFFFFFC.
	// If this has the value -1, the length of this chunk is not known.
	// This is only valid for one of the grouping chunks, in which case
	// that group will be terminated by an end tag that contains no data.
	int64 mLength;									// base + 24 bytes
	
	// This field is used for growing or shrinking the data in a chunk.
	// If 0, this is a normal chunk.  If less than zero, this is the
	// number of bytes in the chunk's mLength that aren't actually used.
	// If greater than zero, this is the location of a continuation chunk.
	// The location is relative to the start of this chunk header.
	int64 mContinuation;							// base + 32 bytes
};

// CHUNK DATA										// base + mHeadLength bytes
// N bytes of chunk data, where N is padded to the
// next 4-byte boundary.
// (That is, N = ((mLength+3)&0xFFFFFFFC) - 40.)

struct SBFChunkTail {
	// This is the error check code that was calculated for the chunk.  It
	// is computed from all of the data within a chunk and its header,
	// except for the mLength and mContinuation fields.  In addition, if
	// this is a grouping chunk (DEPENDENCY_TYPE, FORM_TYPE, CATALOG_TYPE),
	// its CRC does not include the data of any of the chunks it contains.
	// (Since these child chunks have their own CRC.)
	uint32 mCRC;									// base + mHeadLength
													//      + mLength bytes
};
	
// Start of next chunk								// base + mHeadLength
													//      + mLength
													//      + mTailLength bytes


struct SBFChunkShort {
	uint8 mFlags;
	enum {
		SHORTCHUNK_FLAG = 0x80,		// Must always be set.
		DIVIDER_FLAG = 0x40,
		USERTYPE_FLAG = 0x20,
	};
	
	uint8 mType;
	enum {
		BOOLFALSE_TYPE = 0x00,		// 0 bytes
		BOOLTRUE_TYPE = 0x01,		// 0 bytes
		STRING_TYPE = 0x02,			// n bytes
		INT32_TYPE = 0x03,			// 4 bytes
		FLOAT_TYPE = 0x04,			// 4 bytes
		ARGB8_TYPE = 0x06,			// 4 bytes
		DOUBLE_TYPE = 0x05,			// 8 bytes
	};
	
	uint16 mLength;
};

#endif
