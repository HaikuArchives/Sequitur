#include <ArpKernel/ArpDebug.h>
#include <ArpSupport/ArpIntToStringMapI.h>

/***************************************************************************
 * ARP-INT-TO-STRING-MAP-I
 ****************************************************************************/
void ArpIntToStringMapI::Release()
{
	delete this;
}

status_t ArpIntToStringMapI::IdForIndex(int32 index,
										int32 *answer) const
{
	*answer = index;
	return B_OK;
}

status_t ArpIntToStringMapI::IndexForId(int32 id,
										int32 *answer) const
{
	*answer = id;
	return B_OK;
}

status_t ArpIntToStringMapI::IndexForName(	const BString16 *name,
											int32 *answer) const
{
	return IdForName(name, answer);
}

status_t ArpIntToStringMapI::NameForIndex(	int32 index,
											BString16 *answer) const
{
	return NameForId(index, answer);
}

ArpIntToStringMapI::~ArpIntToStringMapI()
{
}
