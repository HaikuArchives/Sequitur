#ifndef ARPSUPPORT_ARPSAFEDELETE_H
#define ARPSUPPORT_ARPSAFEDELETE_H

#include <be/support/SupportDefs.h>

/***************************************************************************
 * ARP-SAFE-DELETE
 * A simple reference counted class.
 ****************************************************************************/
class ArpSafeDelete
{
public:
	ArpSafeDelete();

	virtual void Delete();
	bool IsDeleted() const;
	
	void IncRefs() const;
	void DecRefs() const;

protected:
	virtual ~ArpSafeDelete();

	virtual void	RealDelete();
	
private:
	mutable int32	fRefCount;
	int				fDeleted;
};


#endif
