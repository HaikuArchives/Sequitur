/*
	
	ArpGlobalSet.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#include <ArpLayout/ArpGlobalSet.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

// --------------------------------------------------------------------------
//                                ArpGlobalUpdate
// --------------------------------------------------------------------------
	
ArpGlobalUpdate::ArpGlobalUpdate(const BMessage* newValues)
	: mValues(newValues)
{
}
ArpGlobalUpdate::~ArpGlobalUpdate()
{
}

const BMessage* ArpGlobalUpdate::GlobalValues() const
{
	return mValues;
}

status_t ArpGlobalUpdate::AddGlobals(const BMessage* values)
{
	return B_ERROR;
}

status_t ArpGlobalUpdate::UpdateGlobals(const BMessage* values)
{
	return B_ERROR;
}

bool ArpGlobalUpdate::IsGlobalUpdate() const
{
	return true;
}
