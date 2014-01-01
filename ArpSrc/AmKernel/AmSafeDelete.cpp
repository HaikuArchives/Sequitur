#include <AmPublic/AmSafeDelete.h>
#include <Debug.h>

AmSafeDelete::AmSafeDelete()
	: fRefCount(0), fDeleted(0)
{
}

void AmSafeDelete::Delete()
{
	if (fRefCount <= 0) {
		// If nobody else has a reference on this event, it is
		// safe to just delete it.
		RealDelete();
	} else {
		// Otherwise, set the mDeleted flag, avoiding any race
		// conditions with other threads that might have references.
		IncRefs();
		fDeleted = 1;
		DecRefs();
	}
}

bool AmSafeDelete::IsDeleted() const
{
	return fDeleted;
}

void AmSafeDelete::IncRefs() const
{
	atomic_add(&fRefCount, 1);
}

void AmSafeDelete::DecRefs() const
{
	int32 last = atomic_add(&fRefCount, -1);
	if (last == 1 && fDeleted) const_cast<AmSafeDelete*>(this)->RealDelete();
	else if (last < 1) {
		debugger("AmSafeDelete reference count went below zero");
	}
}

AmSafeDelete::~AmSafeDelete()
{
	if (fRefCount != 0) debugger("AmSafeDelete destructor called while object is still referenced");
}

void AmSafeDelete::RealDelete()
{
	delete this;
}

void AmSafeDelete::Undelete()
{
	IncRefs();
	fDeleted = 0;
	DecRefs();
}
