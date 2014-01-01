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
 * ArpString.h
 *
 * Based on CString.h
 * Copyright 1995 -1996 Metrowerks Corporation, All Rights Reserved.
 * Modified by Dianne Hackborn (hackbod@angryredplanet.com), 1997
 *
 * General-purpose string class, modifed to support strings
 * with embedded nulls.
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
 * 11/21/998:
 *	• Cleaned up buffer management so that (I believe) the sharing of
 *	  string buffers is thread-safe.  This does -not- mean that the
 *	  ArpString class itself is thread-safe, but that any internal
 *	  buffers it shares across objects is, so that you don't have
 *	  to care at all that it is doing so.
 *	• Started deprecating the use of ichar.
 *
 * 8/26/1998:
 *	• Completely re-written.  It can now be much more efficient --
 *	  copying one ArpString to another copies pointers, intelligently
 *	  allocating new memory only when needed.  Can also be assigned
 *	  a static string, so that it does not allocate any storage.
 *
 * Aug. 6, 1998:
 *	• Removed 'off' parameter from constructor.  (This was old
 *	  legacy from porting ArpTelnet from Java; it is stupid in C++.)
 *
 * 0.1: Created this file.
 *
 */

#ifndef ARPKERNEL_ARPSTRING_H
#define ARPKERNEL_ARPSTRING_H

#ifndef ARPCOLLECTIONS_ARPVECTORI_H
#include <ArpCollections/ArpVectorI.h>
#endif

#ifndef __BSTRING__
#include <be/support/String.h>
#endif

#include <SupportDefs.h>
#include <string.h>

// This type is leftover dredge from the WebTerm days.
// (Or more accurately, the pre-DR9 days when I thought
//  Unicode was going to come as 16-bit characters, at
//  which point this would become such a character.)
// Use of this is deprecated, and will hopefully disappear
// at some time in the near future.
typedef uchar ichar;

/*****************************************************************************
 *
 *	ARP-STRING CLASS
 *
 *	This is Our Very Own string class.  [You can never have too many!]
 *	It was written before BString existed; if it had back then, I probably
 *	would never have written this.  And I still haven't decided whether to
 *	re-implement it as a thin subclass of BString.
 *
 *	Some features of ArpString that I don't think are in BString:
 *
 *	- You can store static literals in the string, without having to copy
 *	  them.  This is accomplished by setting the 'copy' flag in the
 *	  constructor to false (although normally you should use the ArpLIT()
 *	  and ArpLOC() macros instead, to better document what you are doing.)
 *	  Note that you should be careful when doing this: such a string
 *	  will -never- be copied, even when assigning one ArpString to another.
 *	  It is thus generally not a good idea to do this with any character
 *	  array you have dynamically allocated or have on the stack.
 *
 *	- The internal buffer mechanism tries to avoid copying strings.  When
 *	  assigning one string to another, only a reference to the buffer
 *	  is copied -- they will both share the same underlying data.  If
 *	  any ArpString later wants to modify its buffer, but this buffer
 *	  is being shared, it will then make its own private copy of the
 *	  buffer.  I believe this buffer sharing mechanism is thread-safe,
 *	  so you should never -have- to worry that it is doing this, even
 *	  when assigning ArpString objects across multiple threads.
 *
 *	  (But note that this is NOT saying the ArpString class itself is
 *	   thread-safe.  If multiple threads are accessing the same ArpString,
 *	   you will need to appropriately protect it.)
 *
 *	- ArpString fully supports strings with embedded \0 characters.
 *	  (I am not actually sure whether BString does or not -- it looks
 *	   like some of the interfaces do, but others don't seem to.)
 *	  In addition, ArpString always maintains an implicit \0 terminator
 *	  at the end of its string; this is not included in its Length(),
 *	  but attached so that its string is always a valid char*.  You can,
 *	  of course, create an ArpString with a terminating \0 that is
 *	  included in its length -- in that case, there will still be an
 *	  implicit \0 after it, though this one is not doing much good.
 *
 *	- A NULL string pointer is considered a valid value by ArpString,
 *	  and can be assigned to and retrieved from ArpString as such.  There
 *	  are two convenience functions for dealing with this -- IsNull()
 *	  returns true if its object contains a NULL pointer, while IsEmpty()
 *	  returns true if it contains a NULL pointer -or- the empty string ("").
 *
 *****************************************************************************/

class ArpString
{
public:
	ArpString(void);
	ArpString(const char * init, int32 cnt=-1, bool copy=true);
	ArpString(const char c);
	ArpString(int32 value, int32 radix=10);
	ArpString(float value);
	ArpString(const ArpString & clone);

	~ArpString();

	// Easy interchange with BString objects.  Note that
	// returning a BString is fairly inefficient, because of
	// all the copying that must be done.
	
	ArpString(const BString& string);
	operator BString () const;
	
	// Standard operators to look like a char array.
	
	operator const char * () const;
	const char& operator [] ( int32 idx ) const;

#define RELOP(OP) \
	friend bool operator OP(const ArpString& a, const ArpString& b) \
		{ return a.Compare(b) OP 0; } \
	friend bool operator OP(const ArpString& a, const ichar* b) \
		{ return a.Compare(b) OP 0; } \
	friend bool operator OP(const ichar* a, const ArpString& b) \
		{ return (-b.Compare(a)) OP 0; }

	RELOP(==) RELOP(!=) RELOP(<) RELOP(>) RELOP(<=) RELOP(>=)
#undef RELOP
		
	ArpString operator + (const ArpString & add) const;
	ArpString operator + (const ichar * add) const;
	ArpString operator + (ichar add) const;
	ArpString operator + (long add) const;
	
	ArpString& operator = (const ArpString & clone);
	ArpString& operator = (const ichar * clone);
	ArpString& operator = (int32 clone);

	friend ArpString& operator += (ArpString& o, const ArpString & add);
	friend ArpString& operator += (ArpString& o, const char * add);
	friend ArpString& operator += (ArpString& o, ichar add);
	friend ArpString& operator += (ArpString& o, int32 add);
	
	// And emulate the stream operators, just like BString
	
	friend ArpString& operator << (ArpString& o, const ArpString & add)	{ return o += add; }
	friend ArpString& operator << (ArpString& o, const char * add)		{ return o += add; }
	friend ArpString& operator << (ArpString& o, ichar add)				{ return o += add; }
	friend ArpString& operator << (ArpString& o, int32 add)				{ return o += add; }
	
	// Get and set the length of the string.
	
	int32 Length() const							{ return fSize&LENGTH_MASK; }
	void SetLength(int32 newlen);

	// Common states one wants to check a string for.
	
	bool IsNull() const;
	bool IsEmpty() const;
	
	// Simple conversions.
	
	int32 AsInt(int32 base=10, bool* valid=NULL) const;
	double AsDouble(bool* valid=NULL, int *precisionFound=NULL) const;
	static ArpString CharToString(char c);
	ArpString AsSequence() const;
	
	// Comparisons, like strcmp().
	
	int Compare(const ArpString& compare) const;
	int Compare(const char* compare) const;

	// Case-insensitive comparisons.
	
	int ICompare(const ArpString& compare) const;
	int ICompare(const char* compare) const;

	// Comparison that sort numeric sets correctly.
	
	int NumCompare(const ArpString& compare) const;
	int NumCompare(const char* compare) const;
	int INumCompare(const ArpString& compare) const;
	int INumCompare(const char* compare) const;
	
	// Append additional text to the string.  This does more than
	// the operator form, as you can specify the text length for
	// strings with embedded NULL characters.
	
	ArpString& Append(const ichar* str, int32 len=-1);
	
	// Similarily, assign new text to the string.
	
	void Set(const ichar * inText, long inLength=-1);
	
	// Search for a character in the string.
	
	int32 OffsetOf(ichar inChar) const;
	int32 ROffsetOf(ichar inChar) const;

	// Editing in-place
	
	char* LockBuffer(int32 maxLength);
	ArpString& UnlockBuffer(int32 length = -1);
	
	// These two are not yet implemented.
	
	ArpString& Insert(const ArpString& inArpString, long inOffset);
	ArpString& Replace(const ArpString& inArpString,
						long inOffset, long inLength);

	// Old ichar compatibility.
	
	ArpString(const ichar * init, int32 cnt=-1, bool copy=true);
	operator const ichar * () const;
	
protected:
	enum {
		STORAGE_MASK	= (int)0x80000000,
		LENGTH_MASK		= (int)0x7FFFFFFF
	};
	
	void ReadFrom(const ichar * clone, int32 size, bool copy);
	void ReadFrom(const char * clone, int32 size, bool copy);
	
	// Deallocate (or dereference) any string buffer currently
	// owned by the object.
	void FreeString();
	
	/* --------------- STRING ACCESS ---------------
	 *
	 * These methods are used to retrieve the string that
	 * this object contains.  The first two are used to
	 * retrieve a string that can be modified -- you MUST use
	 * these to get the string, so that we can properly
	 * assure you have exclusive access to the buffer.
	 */
	 
	// Make sure the current buffer can hold at least 'newSize'
	// characters.  If the current buffer is being used by any
	// other ArpString objects, make a copy of it so that we
	// have our own private buffer.
	ichar* ResizeString(int newSize);
	
	// This is currently really just the same as ResizeString().
	ichar* NewString(int minSize);
	
	// Retrieve the current string for read-only access.
	const ichar* GetString() const;
	
	static char temp;
	
	// =========  THE VERY VERY VERY PRIVATE PART  =========
	// You should never have to write code that touches these.
	// Use Length(), ResizeString(), NewString(), and GetString()
	// instead.
	struct buffer {
		void addRef();
		void remRef();
		int32 size;
		int32 refs;
		ichar data[1];
	};
	void*	fData;
	uint32	fSize;
};

#define ArpLIT(x) ArpString(x, -1, false)
#define ArpLOC(x) ArpString(x, -1, false)

ichar *	p2istr(unsigned char * str);
unsigned char * i2pstr(ichar * str);
void p2istrncpy(ichar * cstr, const unsigned char * pstr, int32 maxLen);
void i2pstrncpy(unsigned char * cstr, const ichar * pstr, int32 maxLen);

/*****************************************************************************
 *
 *	ARP-STR-TOK CLASS
 *
 *	Like standard C strtok(), except it doesn't eat multiple occurances of
 *	the delimiter, and it maintains state in the object rather than buried in
 *	a function in a static.  This means you can actually use it in a
 *	multi-threading environment.  (And allows you to have multiple ArpStrTok
 *	objects iterating concurrently over separate strings without losing track
 *	of them.)
 *
 *****************************************************************************/

class ArpStrTok {
public:
	// create the object; pass the delimiter(s) to use
	ArpStrTok(const ArpString& s, const ArpString& delimiter);
	ArpStrTok(const ArpString& s);

	// return the next substring from the delimiters passed into
	// the constructor.
	const ArpString& Next()			{ return Next(mSeparators); }
	
	// return the next substring from a custom set of delimiters.
	const ArpString& Next(const ArpString& delim);
	
	// return the next single character in the string.
	const ArpString& NextChar();
	
	// return the delimiter that ended the last Next() operation.
	const ArpString& AtDelim()		{ return mLastDelim; }

	// return true if at end of string, without advancing
	bool AtEnd()					{ return mCurPos == NULL ? true : false; }

private:
	const ArpString&	mString;
	const char*			mCurPos;
	ArpString			mSeparators;
	ArpString			mLastDelim;
	ArpString			mLastTok;
};

/*****************************************************************************
 *
 *	ARP-PARSE-URL and ARP-UNESCAPE-URL FUNCTIONS
 *
 *	ArpParseURL extracts the protocol, address, path, and any parameters
 *	from a URL, i.e.:
 *		Protocol://Address/Path?Parameters?Parameters?...#Location
 *	All of the function's parameters are optional; if you don't care about
 *	getting one, pass in a NULL for that parameter.
 *
 *	On return, any fields that were not specified are filled with a NULL
 *	string.  Otherwise, they will contain the actual value of that field
 *	in the URL -- which may be an empty string.
 *
 *	Most fields are returned with their escapes expanded (so that you do not
 *	need to call ArpUnescapeURL() on the result).  The exceptions to this are
 *	Address and Parameters, since you will likely need to do further parsing on
 *	on them -- for example, to extract out the Host and Port fields of an
 *	Address or the variable and value parts of a Parameter.
 *
 *****************************************************************************/

status_t ArpParseURL(const ArpString& URL,
					 ArpString* Protocol, ArpString* Address, ArpString* Path,
					 ArpVectorI<ArpString>* Parameters,
					 ArpString* Location);

status_t ArpUnescapeURL(const ArpString& URL, ArpString* Output);

int memcasecmp(const char* str1, const char* str2, size_t len);

int strnumcmp(const char* str1, const char* str2);
int strcasenumcmp(const char* str1, const char* str2);
int memnumcmp(const char* str1, const char* str2, size_t len);
int memcasenumcmp(const char* str1, const char* str2, size_t len);

#endif
