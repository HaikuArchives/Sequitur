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
 * ArpSharedBody.h
 *
 * Abstract protocol for the body part of a handler/body pattern, which
 * is thread-safe.
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
 *	• 10/2/1999:
 *	  Created this file.
 *
 */

#ifndef ARPKERNEL_ARPSHAREDBODY_H
#define ARPKERNEL_ARPSHAREDBODY_H

#ifndef _SUPPORT_DEFS_H
#include <be/support/SupportDefs.h>
#endif

class ArpSharedBody
{
public:
	// Constructor.  Note that the destructor is private -- the object
	// can only be destroyed by its reference count going to zero when
	// DetachBody() is called.
	ArpSharedBody();
	
	// Copy constructor.  For use by Clone().
	ArpSharedBody(const ArpSharedBody& other);
	
	// Mark when your holder object starts and stops, respectively,
	// pointing to this body.
	void AttachBody() const;
	void DetachBody() const;
	
	// Start making changes to this body.  The returned body object is
	// the new official one you are using -- you should no longer reference
	// the old object after calling this function.
	ArpSharedBody* EditBody() const;

protected:
	// Create a duplicate of this body object.  Subclasses must override
	// this to return their subclass type, correctly copied.
	ArpSharedBody* CloneBody() const;
	
private:
	// only deleted by reference count going to zero.
	virtual ~ArpSharedBody();
	
	// no assignment.
	ArpSharedBody& operator=(const ArpSharedBody& other);
	
	// number of references on this object.
	mutable int32 mRefCount;
};

#endif
