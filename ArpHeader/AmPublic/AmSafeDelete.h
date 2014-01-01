#ifndef AMPUBLIC_AMSAFEDELETE_H
#define AMPUBLIC_AMSAFEDELETE_H

#include <SupportDefs.h>
#include <ArpBuild.h>

class AmSafeDelete
{
public:
	AmSafeDelete();

	virtual void Delete();
	bool IsDeleted() const;
	
	void IncRefs() const;
	void DecRefs() const;

protected:
	virtual ~AmSafeDelete();

	virtual void RealDelete();

	/* Right now this is an ugly hack to deal with the
	 * special ability of toolseeds and tracks -- the
	 * ability to restore events that someone else deleted.
	 */
	friend class AmTrackEventUndo;
	friend class _AmTrackMoveEntry;
	friend class _AmTrackRefilterEntry;
	virtual void Undelete();
	
private:
	mutable int32 fRefCount;
	int32 fDeleted;
};

#endif
