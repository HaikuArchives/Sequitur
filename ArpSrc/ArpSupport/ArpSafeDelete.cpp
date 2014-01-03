#include <support/SupportDefs.h>
#include <ArpCore/ArpDebug.h>
#include <ArpSupport/ArpSafeDelete.h>

ArpSafeDelete::ArpSafeDelete()
	: fRefCount(0), fDeleted(0)
{
}

void ArpSafeDelete::Delete()
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

bool ArpSafeDelete::IsDeleted() const
{
	return fDeleted != 0;
}

void ArpSafeDelete::IncRefs() const
{
	atomic_add(&fRefCount, 1);
}

void ArpSafeDelete::DecRefs() const
{
	int	 last = atomic_add(&fRefCount, -1);
//	if (last == 1 && fDeleted != 0) const_cast<ArpSafeDelete*>(this)->RealDelete();
	if (last == 1)
		const_cast<ArpSafeDelete*>(this)->RealDelete();
	else if (last < 0)
		ArpASSERT(false);
}

//#include <KernelKit.h>

ArpSafeDelete::~ArpSafeDelete()
{
	if (fRefCount != 0) {
//debugger("Sdsd");
		ArpASSERT(false);
	}
}

void ArpSafeDelete::RealDelete()
{
	delete this;
}
