/*
	
	ArpLayout.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	This is the base from which non-view layout managers
	should derive.
*/

#ifndef ARPLAYOUT_ARPLAYOUT_H
#include "ArpLayout/ArpLayout.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

ArpMOD();

ArpLayout::ArpLayout(const char* name)
	: BHandler(name)
{
}

ArpLayout* ArpLayout::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpLayout") ) 
		return new ArpLayout(archive); 
	return 0;
}
