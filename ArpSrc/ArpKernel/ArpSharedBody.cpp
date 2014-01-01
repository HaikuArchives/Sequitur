#include <ArpKernel/ArpSharedBody.h>
#include <ArpKernel/ArpDebug.h>

ArpSharedBody::ArpSharedBody()
	: mRefCount(0)
{
}

ArpSharedBody::ArpSharedBody(const ArpSharedBody& other)
	: mRefCount(0)
{
	// Note that we don't copy the reference count -- this is a
	// new object, so it has its own reference count.
}
	
ArpSharedBody::~ArpSharedBody()
{
	ArpASSERT(mRefCount == 0);
}
	
void ArpSharedBody::AttachBody() const
{
	if( this ) atomic_add(&mRefCount, 1);
}
	
void ArpSharedBody::DetachBody() const
{
	if( this ) {
		// I believe this is thread-safe: if the current reference
		// count is one (so we are bringing it to zero and hence
		// deleting it), we are the only one holding a reference on
		// it...  so no other thread can get in the way.
		if( atomic_add(&mRefCount, -1) == 1 ) {
			delete const_cast<ArpSharedBody*>(this);
		}
	}
}

ArpSharedBody* ArpSharedBody::EditBody() const
{
	if( !this ) return 0;
	
	// I believe this is thread-safe.  There are two situations:
	// 	* If the current reference count is one, we are the only
	//	  one holding this object, so no other thread can get in
	//	  our way.
	//	* If the current reference count is greater than one,
	//	  nobody else is allowed to change the object -- we thus
	//	  are free to make a copy of it into a new object.
	//	  The worst that can happen is two threads wanting to
	//	  edit the object at the same time, in which case they
	//	  both will make their own local clones, and one will
	//	  (definitely) end up deleting the old object.  Maybe
	//	  a little extra work, but no serious problem.
	if( mRefCount == 1 ) {
		return const_cast<ArpSharedBody*>(this);
	}
	
	// Create a copy of this object.
	ArpSharedBody* clone = CloneBody();
	
	// The calling holder is new referencing the new body...
	if( clone ) clone->AttachBody();
	// ...and no longer referencing the old body.
	DetachBody();
	
	// This is the new body the holder should use.
	return clone;
}

ArpSharedBody* ArpSharedBody::CloneBody() const
{
	return new ArpSharedBody;
}
