/* GlKeyTracker.h
 * Copyright (c)2004 by Magic Box.
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
 *	-¢ None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.04.22				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLKEYTRACKER_H
#define GLASSLIKE_GLKEYTRACKER_H

#include <support/Locker.h>
#include <ArpCore/StlVector.h>
#include <ArpCore/String16.h>

class _GlKeyTrackerEntry
{
public:
	BString16		creator;
	int32			key;

	_GlKeyTrackerEntry() : key(0)														{ }
	_GlKeyTrackerEntry(const BString16& c, int32 k) : creator(c), key(k)				{ }
	_GlKeyTrackerEntry(const _GlKeyTrackerEntry& o) : creator(o.creator), key(o.key)	{ }
	
	_GlKeyTrackerEntry&	operator=(const _GlKeyTrackerEntry& o);

	status_t					ReadFrom(const BMessage& config);
	status_t					WriteTo(BMessage& config) const;
};

/***************************************************************************
 * GL-KEY-TRACKER
 * I track all of the creators used by the app, along with their
 * current key.
 ***************************************************************************/
class GlKeyTracker
{
public:
	GlKeyTracker();

	bool						IsDirty() const;

	/* Never use the key from GetCurrent() -- that is just for display
	 * purposes.  Always use IncKey() when you're actually assigning the
	 * key to a node.
	 */
	status_t					GetCurrent(BString16& creator, int32* key) const;
	status_t					SetCurrent(const BString16& creator, int32 key);
	status_t					IncKey(int32* out);
	
	status_t					ReadFrom(const BMessage& config);
	status_t					WriteTo(BMessage& config) const;

private:
	mutable BLocker				mAccess;
	_GlKeyTrackerEntry			mCurrent;
	vector<_GlKeyTrackerEntry>	mEntries;
	mutable bool				mDirty;	

	int32						IndexFor(const BString16& creator) const;
};

#endif
