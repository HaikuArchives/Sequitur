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
 * ArpIteratorI.h
 *
 * This is is going to be an abstract interface to an iterator
 * over some collection.  It is still under development.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * Except that it doesn't work...?
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
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef ARPKERNEL_ARPITERATORI_H
#define ARPKERNEL_ARPITERATORI_H

#ifndef _ARP_BUILD_H
#include <ArpBuild.h>
#endif

#ifndef _SUPPORT_DEFS_H
#include <be/support/SupportDefs.h>
#endif

/*****************************************************************************
 *
 *	ARP-ITERATOR-I CLASS
 *
 *	The ArpIteratorI is a template class that defines an iterator
 *	over an abstract collections.  This is still being developed.
 *
 *****************************************************************************/

template<class T> class ArpIteratorI {
public:
	virtual ~ArpIteratorI() { }
	
	virtual T&			operator*() = 0;
	virtual const T&	operator*() const = 0;
	
	ArpIteratorI<T>&	operator++() = 0;
	ArpIteratorI<T>&	operator++(int) = 0;
	ArpIteratorI<T>&	operator--() = 0;
	ArpIteratorI<T>&	operator--(int) = 0;

	ArpIteratorI<T>&	operator+=( size_type n ) = 0;
	ArpIteratorI<T>&	operator-=( size_type n ) = 0;
};

#endif
