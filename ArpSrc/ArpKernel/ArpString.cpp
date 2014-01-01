/*
	
	ArpString.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

//========================================================================
//	ArpString.cpp
//  Based on CString
//	Copyright 1995 -1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	General-purpose string class.  Modifed to allow strings to
//  contain embedded nulls.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef ARPKERNEL_ARPSTRING_H
#include "ArpKernel/ArpString.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

// ArpMOD();	// not its own module -- part of the core module.

char ArpString::temp = 0;


ichar *
p2istr(
	unsigned char * str)
{
	int len = *str;
	memmove(str, str+1, len);
	str[len] = 0;
	return (ichar *) str;
}


unsigned char *
i2pstr(
	ichar * str)
{
	int len = strlen((const char*)str);
	if (len > 255)
	{
		len = 255;
	}
	memmove(str+1, str, len);
	str[0] = len;
	return (ichar *)str;
}


void
p2istrncpy(
	ichar * cstr,
	const unsigned char * pstr,
	int32 size)
{
	int len = *pstr;
	if (len >= size)
	{
		len = size-1;
	}
	memmove(cstr, pstr+1, len);
	cstr[len] = 0;
}


void
i2pstrncpy(
	unsigned char * pstr,
	const ichar * cstr,
	int32 size)
{
	int len = strlen((const char*)cstr);
	if (len >= size)
	{
		len = size-1;
	}
	memmove(pstr+1, cstr, len);
	pstr[0] = len;
}

void
ArpString::buffer::addRef()
{
	if( this ) atomic_add(&refs,1);
}

void
ArpString::buffer::remRef()
{
	if( this ) {
		// I believe this is thread-safe: if the current reference
		// count is one (so we are bringing it to zero and hence
		// deleting it), we are the only one holding a reference on
		// it...  so no other thread can get in the way.
		if( atomic_add(&refs, -1) == 1 ) free(this);
	}
}

void
ArpString::FreeString()
{
	if( (fSize&STORAGE_MASK) && fData ) ((buffer*)fData)->remRef();
	fData = NULL;
	fSize = 0;
}

ichar*
ArpString::ResizeString(int newSize)
{
	const int oldLen = Length();
	const bool oldStore = (fSize&STORAGE_MASK) ? true : false;
	int pad = newSize; //( newSize + Length() ) * 2;
	if( pad < 4 ) pad = 4;
	
	if( oldStore && fData ) {
		buffer* oldBuf = (buffer*)fData;
		// I believe this is thread-safe.  There are two situations:
		// 	* If the current reference count is one, we are the only
		//	  one holding this object, so no other thread can get in
		//	  our way.
		//	* If the current reference count is greater than one,
		//	  nobody else is allowed to change the buffer -- we thus
		//	  are free to make a copy of it to copy into our own
		//	  private buffer.  The worst that can happen is two threads
		//	  wanting to resize the buffer at the same time, in which
		//	  case they both will make their own local copies, and one
		//	  will (definitely) end up deleting the old buffer.  Maybe
		//	  a little extra work, but no serious problem.
		if( oldBuf->refs == 1 && oldBuf->size >= newSize ) {
			fSize = newSize|STORAGE_MASK;
			return &oldBuf->data[0];
		}
		buffer* newBuf = (buffer*)malloc(sizeof(buffer) + newSize + pad);
		newBuf->size = newSize + pad;
		newBuf->refs = 1;
		memcpy( &newBuf->data[0], &oldBuf->data[0],
				oldLen < newSize ? oldLen : newSize );
		newBuf->data[newSize] = 0;
		fData = newBuf;
		fSize = newSize|STORAGE_MASK;
		oldBuf->remRef();
		return &newBuf->data[0];
	} else {
		buffer* newBuf = (buffer*)malloc(sizeof(buffer) + newSize);
		newBuf->size = newSize;
		newBuf->refs = 1;
		if( fData ) {
			memcpy( &newBuf->data[0], (char*)fData,
					oldLen < newSize ? oldLen : newSize );
		}
		newBuf->data[newSize] = 0;
		fData = newBuf;
		fSize = newSize|STORAGE_MASK;
		return &newBuf->data[0];
	}
}

ichar*
ArpString::NewString(int minSize)
{
	return ResizeString(minSize);
}

const ichar*
ArpString::GetString() const
{
	if( !fData ) return NULL;
	if( fSize&STORAGE_MASK ) return &( ((buffer*)fData)->data[0] );
	else return (ichar*)fData;
}

void
ArpString::ReadFrom(const ichar * clone, int32 size, bool copy)
{
	if( clone && copy ) {
		ichar* str = NewString(size);
		memcpy(str, clone, size);
		str[size] = 0;
	} else {
		FreeString();
		fData = (void*)clone;
		fSize = clone ? (size&LENGTH_MASK) : 0;
	}
}


void
ArpString::ReadFrom(const char * clone, int32 size, bool copy)
{
	if( clone && copy ) {
		ichar* str = NewString(size);
		memcpy(str, clone, size);
		str[size] = 0;
	} else {
		FreeString();
		fData = (void*)clone;
		fSize = clone ? (size&LENGTH_MASK) : 0;
	}
}


ArpString::ArpString(void)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making empty ArpString" << endl);
}


ArpString::ArpString(const ichar * init, int32 cnt, bool copy)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making ArpString for istring: "
				<< ((const char *)init ? (const char *)init : "<null>") << endl);
	if( init && cnt < 0 ) cnt = strlen((const char*)init);
	ReadFrom(init, cnt, copy);
	//cdb << "Created string '" << (const char*)GetString()
	//	<< "' from '" << (init ? init:(const ichar*)"") << endl;
}


ArpString::ArpString(const char * init, int32 cnt, bool copy)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making ArpString for string: "
					<< (init ? init : "<null>") << endl);
	if( init && cnt < 0 ) cnt = strlen(init);
	ReadFrom(init, cnt, copy);
	//cdb << "Created string '" << (const char*)GetString()
	//	<< "' from '" << (init ? init:"") << endl;
}


ArpString::ArpString(char init)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making ArpString for char: " << init << endl);
	ichar* str = NewString(1);
	str[0] = init;
	str[1] = 0;
}


ArpString::ArpString(const ArpString & clone)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Copying ArpString from: " << clone << endl);
				
	if( clone.fData ) {
		if( clone.fSize&STORAGE_MASK ) {
			buffer* buf = (buffer*)clone.fData;
			fData = buf;
			fSize = clone.fSize;
			buf->addRef();
		} else {
			ReadFrom(clone.GetString(), clone.Length(), false);
		}
	}
}


ArpString::ArpString(int32 value, int32 radix)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making ArpString for long: " << value << endl);

	ichar		tempArpString[16];
	sprintf((char*)tempArpString, "%ld", value);
	ReadFrom(tempArpString, strlen((const char*)tempArpString), true);
}

ArpString::ArpString(float 	value)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making ArpString for float: " << value << endl);

	ichar		tempArpString[32];
	sprintf((char*)tempArpString, "%f", value);
	ReadFrom(tempArpString, strlen((const char*)tempArpString), true);
}

ArpString::ArpString(const BString& string)
{
	fData = NULL;
	fSize = 0;
	
	ArpDL("core", 3, cdb << ADH << "Making ArpString for BString: "
					<< string.String() << endl);
	ReadFrom((const ichar*)string.String(), string.Length(), true);
}


ArpString::~ArpString()
{
	ArpDL("core", 3, cdb << ADH << "Deleting ArpString: " << (*this) << endl);
	FreeString();
}


void ArpString::SetLength(int32 newlen)
{
	ichar* str = ResizeString(newlen);
	str[newlen] = 0;
}


ArpString::operator const ichar * () const
{
	return GetString();
}


ArpString::operator const char * () const
{
	return (const char*)GetString();
}

ArpString::operator BString () const
{
	BString ret;
	ret.SetTo((const char*)GetString(), Length());
	return ret;
}

const char& ArpString::operator [] ( int32 idx ) const
{
	const ichar* str = GetString();
	if( !str || idx < 0 || idx >= Length() ) {
		temp = 0;
		return temp;
	}
	return (const char&)(str[idx]);
}

bool
ArpString::IsNull() const
{
	return (fData == NULL) ? true : false;
}

bool
ArpString::IsEmpty() const
{
	const ichar* str = GetString();
	return (str == NULL || (*str) == 0) ? true : false;
}

int32 ArpString::AsInt(int32 base, bool* valid) const
{
	int32	value = 0;
	char*	end = (char*)GetString();
	
	if( end ) {
		while( *end && isspace(*end) ) end++;
		value = strtol(end,&end,base);
	}
	if( valid ) {
		if( end && (!*end || isspace(*end)) ) *valid = true;
		else *valid = false;
	}
	return value;
}

double ArpString::AsDouble(bool* valid, int *precisionFound) const
{
	double	value = 0;
	char*	end = (char*)GetString();
	bool	converted = false;

	if( end ) {
		while( *end && isspace(*end) ) end++;
		value = strtod(end,&end);
	}

	// Determine if the conversion succeeded.
	if(( end ) &&
	  ((!*end ) ||
	   ( isspace( *end )))) {
	   	 converted = true;
	}

	// Return validity if requested.	
	if( valid ) {
		*valid = converted;
	}

	// Return the digits of precision if requested.
	if(( precisionFound ) &&
	   ( converted )) {
		char	*start = strchr( (char*)GetString(), '.' );

		if( start ) {
			*precisionFound = end - ( start + 1 );
		}
		else {
			*precisionFound = 0;
		}
	}

	return value;
}

static const char* codes_00_20[] = {
	"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
	"BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
	"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
	"CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
	"[]"
};

static const char* codes_7f_9f[] = {
	"DEL",
	"$80", "$81", "$82", "$83", "IND", "NEL", "SSA", "ESA",
	"HTS", "HTJ", "VTS", "PLD", "PLU", "RI",  "SS2", "SS3",
	"DCS", "PU1", "PU2", "STS", "CCH", "MW",  "SPA", "EPA",
	"$98", "$99", "$9a", "CSI", "ST",  "OSC", "PM",  "APC"
};

ArpString
ArpString::CharToString(char c)
{
	const int32 val = (int32)(uint8)c;
	
	if( val >= 0x00 && val <= 0x20 ) {
		return ArpString(ArpLIT(codes_00_20[val]));
	} else if( val >= 0x7f && val <= 0x9f ) {
		return ArpString(ArpLIT(codes_7f_9f[val-0x7f]));
	} else if( val == 0xff ) {
		return ArpString(ArpLIT("$ff"));
	} else if( sizeof(char) > 1 && (val < 0x00 || val >= 0x100) ) {
		return ArpString("$") + ArpString(val,16);
	}
	return ArpString(c);
}

ArpString
ArpString::AsSequence() const
{
	ArpString res("");
	
	const char* str = (const char*)(*this);
	for( int i=0; i<Length(); i++ ) {
		if( i > 0 ) res += " ";
		res += CharToString(str[i]);
	}
	return res;
}

int
ArpString::Compare(const ArpString & compare) const
{
	if( !fData || !compare.fData ) {
		return fData ? 1 : (compare.fData ? -1 : 0);
	}
	
	const int32 tSize = Length();
	const int32 cSize = compare.Length();
	return memcmp((const char*)GetString(),
				  (const char*)compare.GetString(),
				  ((tSize < cSize) ? tSize : cSize) + 1);
}

int
ArpString::ICompare(const ArpString & compare) const
{
	const int32 tSize = Length();
	const int32 cSize = compare.Length();
	return memcasecmp((const char*)GetString(),
					  (const char*)compare.GetString(),
					  ((tSize < cSize) ? tSize : cSize) + 1);
}

int
ArpString::NumCompare(const ArpString & compare) const
{
	const int32 tSize = Length();
	const int32 cSize = compare.Length();
	return memnumcmp((const char*)GetString(),
					 (const char*)compare.GetString(),
					 ((tSize < cSize) ? tSize : cSize) + 1);
}

int
ArpString::INumCompare(const ArpString & compare) const
{
	const int32 tSize = Length();
	const int32 cSize = compare.Length();
	return memcasenumcmp((const char*)GetString(),
						 (const char*)compare.GetString(),
						 ((tSize < cSize) ? tSize : cSize) + 1);
}

int
ArpString::Compare(const char * compare) const
{
	if( !fData || !compare ) {
		return fData ? 1 : (compare ? -1 : 0);
	}
	
	const int32 tSize = Length();
	int32 len = strlen(compare);
	return memcmp((const char*)GetString(), compare,
				  ((tSize < len) ? tSize : len) + 1);
}

int
ArpString::ICompare(const char * compare) const
{
	const int32 tSize = Length();
	int32 len = strlen(compare);
	return memcasecmp((const char*)GetString(), compare,
					  ((tSize < len) ? tSize : len) + 1);
}

int
ArpString::NumCompare(const char * compare) const
{
	const int32 tSize = Length();
	int32 len = strlen(compare);
	return memnumcmp((const char*)GetString(), compare,
					 ((tSize < len) ? tSize : len) + 1);
}

int
ArpString::INumCompare(const char * compare) const
{
	const int32 tSize = Length();
	int32 len = strlen(compare);
	return memcasenumcmp((const char*)GetString(), compare,
						 ((tSize < len) ? tSize : len) + 1);
}

ArpString
ArpString::operator + (const ArpString & add) const
{
	ArpString ret(*this);
	ret += add;
	return ret;
}


ArpString
ArpString::operator + (const ichar * add) const
{
	ArpString ret(*this);
	ret += add;
	return ret;
}


ArpString
ArpString::operator + (ichar add) const
{
	ArpString ret(*this);
	ret += add;
	return ret;
}


ArpString &
operator += (ArpString& o, const ArpString & add)
{
	ArpDL("core", 3, cdb << ADH << "Add ArpString to ArpString: " << add);
	if( add.fSize > 0 ) {
		const int32 origLen = o.Length();
		const int32 appLen = add.Length();
		ichar* str = o.ResizeString(origLen + appLen);
		memcpy(str+origLen, add.GetString(), appLen);
		str[origLen+appLen] = 0;
	}
	ArpDL("core", 3, cdb << ADH << ", result=" << o << endl);

	return o;
}


ArpString &
operator += (ArpString& o, const char * add)
{
	return o.Append((const ichar*)add);
}


ArpString &
operator += (ArpString& o, ichar add)
{
	ArpDL("core", 3, cdb << ADH << "Add ichar to ArpString: " << add);
	const int32 origLen = o.Length();
	ichar* str = o.ResizeString(origLen + 1);
	str[origLen] = add;
	str[origLen+1] = 0;
	ArpDL("core", 3, cdb << ADH << ", result=" << o << endl);

	return o;
}


// BDS
ArpString &
operator += (ArpString& o, int32 add)
{
	ArpDL("core", 3, cdb << ADH << "Add long to ArpString: " << add << endl);

	ichar		tempArpString[16];
	sprintf((char *)tempArpString, "%ld", add);

	return o.Append((const ichar*)tempArpString);
}


// BDS
ArpString
ArpString::operator + (int32 add) const
{
	ArpString ret(*this);
	ret += add;
	return ret;
}


//BDS
ArpString &
ArpString::operator = (int32 clone)
{
	ichar		tempArpString[16];
	sprintf((char *)tempArpString, "%ld", clone);
	ReadFrom(tempArpString, strlen((const char*)tempArpString), true);
	return * this;
}


ArpString &
ArpString::operator = (const ArpString & clone)
{
	if( clone.fData ) {
		if( clone.fSize&STORAGE_MASK ) {
			FreeString();
			buffer* buf = (buffer*)clone.fData;
			fData = buf;
			fSize = clone.fSize;
			buf->addRef();
		} else {
			ReadFrom(clone.GetString(), clone.Length(), false);
		}
	} else {
		FreeString();
	}
	return * this;
}


ArpString &
ArpString::operator = (const ichar * clone)
{
	Set(clone);
	return *this;
}


ArpString &
ArpString::Append(const ichar * add, int32 len)
{
	ArpDL("core", 3, cdb << ADH << "Add istring to ArpString: "
			<< ((const char*)add ? (const char*)add : "<null>") << endl);
	if( add && *add ) {
		const int32 origLen = Length();
		if( len < 0 ) len = strlen((const char*)add);
		ichar* str = ResizeString(origLen + len);
		memcpy(str+origLen, add, len);
		str[origLen+len] = 0;
	}
	ArpDL("core", 3, cdb << ADH << ", result=" << (*this) << endl);
	
	return *this;
}


char*
ArpString::LockBuffer(int32 maxLength)
{
	if( maxLength <= 0 ) maxLength = Length();
	else if( maxLength > 0 ) {
		// the length argument passed to this function includes
		// the terminating \0 of the string, but internally we
		// think of lengths without the terminating \0.
		maxLength--;
	}
	ichar* str = ResizeString(maxLength);
	//cdb << "Locking buffer '" << (char*)str << "' with length "
	//	<< maxLength << endl;
	return (char*)str;
}

ArpString&
ArpString::UnlockBuffer(int32 length)
{
	if( length < 0 ) {
		const ichar* str = GetString();
		const int LEN = Length();
		length = 0;
		while( str && *str && length < LEN ) {
			length++;
			str++;
		}
		//cdb << "Found length to unlock buffer: length="
		//	<< length << ", max=" << LEN << ", str='"
		//	<< (const char*)str << "'" << endl;
	}
	
	if( length < 0 ) length = 0;
	
	//cdb << "Unlocking buffer '" << (const char*)GetString()
	//	<< "' with length " << length << endl;
	ichar* str = ResizeString(length);
	str[length] = 0;
	
	//cdb << "Final string is '" << (char*)str << "'" << endl;
	
	return *this;
}

//BDS
//	Insert a string at the specified offset
ArpString &
ArpString::Insert(const ArpString& inStr, long inOffset)
{
	ASSERT(0);
	return *this;
#if 0
	if (inOffset > fSize)
		inOffset = fSize;

	if (inOffset < 0)
		inOffset = 0;

	int32 		newSize = strlen((const char*)inArpString) + fSize;
	ichar * 		newData = new ichar[newSize+1];

	memcpy(newData, fData, inOffset);
	memcpy(newData+inOffset, inArpString, newSize-fSize);
	memcpy(newData+newSize-inOffset, fData+inOffset, fSize-inOffset);
	newData[newSize] = 0;
	
	FreeStorage();
	fData = newData;
	fSize = newSize;

	return *this;
#endif
}


//BDS
//	Replace the ichars defined by inOffset and inLength by inArpString
ArpString &
ArpString::Replace(const ArpString& inStr,long inOffset,
					long inLength)
{
	ASSERT(0);
	return *this;
#if 0
	if (inOffset > fSize)
		inOffset = fSize;
	if (inOffset + inLength > fSize)
		inLength = (inOffset + inLength) - fSize;

	int32 		newSize = strlen((const char*)inArpString)
						+ fSize - inLength;
	ichar * 		newData = new ichar[newSize+1];

	memcpy(newData, fData, inOffset);
	memcpy(newData+inOffset, inArpString, newSize-fSize+inLength);
	memcpy(newData+newSize-inOffset+inLength,
		   fData+inOffset+inLength, fSize-inOffset-inLength);
	newData[newSize] = 0;
	
	FreeStorage();
	fData = newData;
	fSize = newSize;

	return *this;
#endif
}

int32
ArpString::OffsetOf(ichar inChar) const
{
	int result = -1;
	
	const ichar* str = GetString();
	if( str ) {
		void* ptr = memchr((const void *)str, (int)inChar, Length());
		if (ptr) result = ( ((ichar*)ptr) - GetString() ) / sizeof(ichar);
	}
	
	return result;
}

static void* memrchr(const void* mem, int c, int length)
{
	const char* pos = ((const char*)mem) + length;
	while( (--pos) >= (const char*)mem ) {
		if( (*pos) == (char)c ) return (void*)pos;
	}
	
	return 0;
}

int32
ArpString::ROffsetOf(ichar inChar) const
{
	int result = -1;
	
	const ichar* str = GetString();
	if( str ) {
		void* ptr = memrchr((const void *)str, (int)inChar, Length());
		if (ptr) result = ( ((ichar*)ptr) - str ) / sizeof(ichar);
	}
	
	return result;
}

// Set the contents to the text defined by inArpString and inLength
void
ArpString::Set(const ichar * inText, long inLength)
{
	if( inLength < 0 && inText )
		inLength = strlen( (const char*)inText );
	ReadFrom(inText, inLength, true);
}

// -------------------------- ArpStrTok --------------------------

ArpStrTok::ArpStrTok(const ArpString& s, const ArpString& delimiter)
	: mString(s), mCurPos((const char*)mString),
	  mSeparators(delimiter.IsNull() ? ArpLIT("") : delimiter),
	  mLastDelim("")
{
}

ArpStrTok::ArpStrTok(const ArpString& s)
	: mString(s), mCurPos((const char*)mString),
	  mSeparators((const char*)0), mLastDelim("")
{
}

const ArpString& ArpStrTok::Next(const ArpString& delim)
{
	char* r;
	char* q = const_cast<char*>(mCurPos);
	if(q == 0 || *q == '\0') { // return if no tokens remaining
		mCurPos = 0;
		mLastTok = ArpString();
		mLastDelim = mLastTok;
	} else if((r = strpbrk(q, (const char*) delim)) == NULL) { // move past token
		mCurPos = 0;	/* indicate this is last token */
		mLastTok.Set((ichar*)q);
		mLastDelim = "";
	} else {
		if( *r ) mLastDelim.Set((ichar*)r, 1);
		else mLastDelim = "";
		if( r >= q ) mLastTok.Set((ichar*)q, (int)(r-q));
		else mLastTok.Set(NULL);
		mCurPos = ++r;
	}
	return mLastTok;
}

const ArpString& ArpStrTok::NextChar()
{
	if( mCurPos == 0 || *mCurPos == '\0' ) {
		mLastTok = ArpString();
		mLastDelim = mLastTok;
	} else {
		mLastTok.Set((ichar*)mCurPos, 1);
		mCurPos++;
	}
	return mLastTok;
}

// -------------------------- ArpParseURL --------------------------

status_t ArpParseURL(const ArpString& URL,
					 ArpString* Protocol, ArpString* Address, ArpString* Path,
					 ArpVectorI<ArpString>* Parameters,
					 ArpString* Location)
{
	status_t retval = B_NO_ERROR;
	
	static const ArpString EmptyStr(ArpLIT(""));
	static const ArpString PathSep(ArpLIT("/"));
	static const ArpString ParamSep(ArpLIT("?"));
	static const ArpString LocSep(ArpLIT("#"));
	static const ArpString Separators(ArpLIT("/?#"));
	
	if( Protocol ) Protocol->Set(NULL);
	if( Address ) Address->Set(NULL);
	if( Path ) Path->Set(NULL);
	if( Location ) Location->Set(NULL);
	
	ArpStrTok tok(URL);
	ArpString strret = tok.Next(ArpLIT(":/?"));
	if( tok.AtDelim() == ArpLIT(":") ) {
		if( Protocol ) {
			status_t err = ArpUnescapeURL(strret, Protocol);
			if( err ) retval = err;
		}
		// This stops referencing the string's memory, so that the
		// next tokenization can re-use it.
		strret.Set(0);
		
		strret = tok.Next(Separators);
	}
	
	ArpString pathret;
	if( !strret.IsEmpty() ) {
		pathret = strret;
		pathret += PathSep;
	}
	
	if( tok.AtDelim() == PathSep && strret == EmptyStr ) {
		strret.Set(0);
		pathret = PathSep;
		strret = tok.Next(Separators);
		if( !strret.IsEmpty() ) {
			pathret += strret;
			pathret += PathSep;
		}
		if( tok.AtDelim() == PathSep && strret == EmptyStr ) {
			strret.Set(0);
			strret = tok.Next(Separators);
			if( Address ) (*Address) = strret;
			strret.Set(0);
		}
	}
	
	strret.Set(0);
		
	if( tok.AtDelim() == PathSep ) {
		if( pathret.IsNull() ) pathret = "";
		pathret += tok.Next(ParamSep);
	}
	
	if( Path ) {
		status_t err = ArpUnescapeURL(pathret, Path);
		if( err ) retval = err;
	}
	
	ArpString paramStr;
	while( tok.AtDelim() == ParamSep ) {
		strret = tok.Next(Separators);
		ArpDL("core", 3, cdb << ADH << "Found param=" << strret
					<< ", delim=" << tok.AtDelim() << endl);
		if( Parameters && !strret.IsNull() ) Parameters->push_back(strret);
	}
	
	if( tok.AtDelim() == LocSep ) {
		strret = tok.Next(EmptyStr);
		if( Location ) {
			status_t err = ArpUnescapeURL(strret, Location);
			if( err ) retval = err;
		}
	}
	
	return retval;
}

status_t ArpUnescapeURL(const ArpString& URL, ArpString* Output)
{
	if( !Output ) return B_BAD_VALUE;
	if( URL.IsNull() ) {
		(*Output) = URL;
		return B_NO_ERROR;
	}
	
	status_t retval = B_NO_ERROR;
	
	static const ArpString EscapeStr(ArpLIT("%"));
	
	ArpStrTok tok(URL, EscapeStr);
	(*Output) = "";
	(*Output) += tok.Next();
	while( !tok.AtEnd() ) {
		ichar c = 0;
		ichar val = tok.NextChar()[0];
		if( val >= '0' && val <= '9' ) c += (val - '0') << 4;
		else if( val >= 'a' && val <= 'f' ) c += (val - 'a' + 10) << 4;
		else if( val >= 'A' && val <= 'F' ) c += (val - 'A' + 10) << 4;
		else {
			retval = B_BAD_VALUE;
			tok.NextChar();
			continue;
		}
		val = tok.NextChar()[0];
		if( val >= '0' && val <= '9' ) c += val - '0';
		else if( val >= 'a' && val <= 'f' ) c += val - 'a' + 10;
		else if( val >= 'A' && val <= 'F' ) c += val - 'A' + 10;
		else {
			retval = B_BAD_VALUE;
			continue;
		}
		(*Output) += c;
		(*Output) += tok.Next();
	}
	
	return retval;
}

// -------------------------------------------------------------

int memcasecmp(const char* str1, const char* str2, size_t len)
{
	if( str1 == str2 ) return 0;
	if( !str1 ) return -1;
	if( !str2 ) return 1;
	
	while( len > 0 ) {
		// Compare current characters.
		if( tolower(*str1) < tolower(*str2) ) return -1;
		if( tolower(*str1) > tolower(*str2) ) return 1;
		
		// Move to next character in strings.
		len--;
		str1++;
		str2++;
	}
	
	return 0;
}

static int do_numcmp(const char** str1, const char** str2, size_t* len)
{
	// Initially assume numbers are the same.
	int res = 0;
	bool num1 = true, num2 = true;
	
	do {
		// If these two characters are different, store which
		// is larger.
		if( !res ) {
			if( **str1 < **str2 ) res = -1;
			if( **str1 > **str2 ) res = 1;
		}
		
		// If at end of string, return stored result.
		if( len ) {
			(*len)--;
			if( *len <= 0 ) return res;
		} else {
			if( **str1 == 0 && **str2  == 0 ) return res;
		}
		
		// Move to next character in the string.
		(*str1)++;
		(*str2)++;
		
		// Are the strings still numbers?
		num1 = !(**str1 < '0' || **str1 > '9');
		num2 = !(**str2 < '0' || **str2 > '9');
		
		// If only one of the strings is no longer a
		// number, the string with the shorter number is
		// less than the other...  irregardless of any other
		// difference found.
		if( num1 != num2 ) {
			if( num1 ) return 1;
			if( num2 ) return -1;
		}
		
		// Continue as long as both characters are numeric.
	} while( num1 && num2 );
	
	// Return result of any difference found.
	return res;
}

static int do_strcmp(const char** str1, const char** str2, size_t* len)
{
	do {
		// Compare current characters.
		if( **str1 < **str2 ) return -1;
		if( **str1 > **str2 ) return 1;
		
		// If at end of string, return stored result.
		if( len ) {
			(*len)--;
			if( *len <= 0 ) return 0;
		} else {
			if( **str1 == 0 && **str2  == 0 ) return 0;
		}
		
		// Move to next character in strings.
		(*str1)++;
		(*str2)++;
		
		// Continue as long as either string is non-numeric.
	} while( **str1 < '0' || **str1 > '9'
			 || **str2 < '0' || **str2 > '9' );
	
	return 0;
}

static int do_strcasecmp(const char** str1, const char** str2, size_t* len)
{
	do {
		// Compare current characters.
		if( tolower(**str1) < tolower(**str2) ) return -1;
		if( tolower(**str1) > tolower(**str2) ) return 1;
		
		// If at end of string, return stored result.
		if( len ) {
			(*len)--;
			if( *len <= 0 ) return 0;
		} else {
			if( **str1 == 0 && **str2  == 0 ) return 0;
		}
		
		// Move to next character in strings.
		(*str1)++;
		(*str2)++;
		
		// Continue as long as either string is non-numeric.
	} while( **str1 < '0' || **str1 > '9'
			 || **str2 < '0' || **str2 > '9' );
	
	return 0;
}

int strnumcmp(const char* str1, const char* str2)
{
	if( str1 == str2 ) return 0;
	if( !str1 ) return -1;
	if( !str2 ) return 1;
	
	// Initially looking at alphabetic portion?
	bool alpha = *str1 < '0' || *str1 > '9'
			   || *str2 < '0' || *str2 > '9';
	
	int res = 0;
	do {
		// Perform appropriate type of string comparison.
		if( alpha ) res = do_strcmp(&str1, &str2, 0);
		else res = do_numcmp(&str1, &str2, 0);
		
		// A return meant either the end of the strings,
		// 'res' is non-zero, or we need to switch the type
		// of comparison.
		alpha = !alpha;
	} while( res == 0 && *str1 && *str2);
	
	return res;
}

int strcasenumcmp(const char* str1, const char* str2)
{
	if( str1 == str2 ) return 0;
	if( !str1 ) return -1;
	if( !str2 ) return 1;
	
	// Initially looking at alphabetic portion?
	bool alpha = *str1 < '0' || *str1 > '9'
			   || *str2 < '0' || *str2 > '9';
	
	int res = 0;
	do {
		// Perform appropriate type of string comparison.
		if( alpha ) res = do_strcasecmp(&str1, &str2, 0);
		else res = do_numcmp(&str1, &str2, 0);
		
		// A return meant either the end of the strings,
		// 'res' is non-zero, or we need to switch the type
		// of comparison.
		alpha = !alpha;
	} while( res == 0 && *str1 && *str2);
	
	return res;
}

int memnumcmp(const char* str1, const char* str2, size_t len)
{
	if( str1 == str2 ) return 0;
	if( !str1 ) return -1;
	if( !str2 ) return 1;
	if( len <= 0 ) return 0;
	
	// Initially looking at alphabetic portion?
	bool alpha = *str1 < '0' || *str1 > '9'
			   || *str2 < '0' || *str2 > '9';
	
	int res = 0;
	do {
		// Perform appropriate type of string comparison.
		if( alpha ) res = do_strcmp(&str1, &str2, &len);
		else res = do_numcmp(&str1, &str2, &len);
		
		// A return meant either the end of the strings,
		// 'res' is non-zero, or we need to switch the type
		// of comparison.
		alpha = !alpha;
	} while( res == 0 && len > 0 );
	
	return res;
}

int memcasenumcmp(const char* str1, const char* str2, size_t len)
{
	if( str1 == str2 ) return 0;
	if( !str1 ) return -1;
	if( !str2 ) return 1;
	if( len <= 0 ) return 0;
	
	// Initially looking at alphabetic portion?
	bool alpha = *str1 < '0' || *str1 > '9'
			   || *str2 < '0' || *str2 > '9';
	
	int res = 0;
	do {
		// Perform appropriate type of string comparison.
		if( alpha ) res = do_strcasecmp(&str1, &str2, &len);
		else res = do_numcmp(&str1, &str2, &len);
		
		// A return meant either the end of the strings,
		// 'res' is non-zero, or we need to switch the type
		// of comparison.
		alpha = !alpha;
	} while( res == 0 && len > 0 );
	
	return res;
}
