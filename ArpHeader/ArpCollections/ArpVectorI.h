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
 * ArpVectorI.h
 *
 * An abstract interface to a "vector-like" object.  The intention
 * here is that methods in libraries (particularily shared libraries)
 * can get and put vectors using this interface, rather than exposing
 * the implementation details of their vector.
 *
 * In other words, it should provide a way to use STL vector objects
 * in shared interfaces.
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
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef ARPKERNEL_ARPVECTORI_H
#define ARPKERNEL_ARPVECTORI_H

#ifndef _ARP_BUILD_H
#include <ArpBuild.h>
#endif

#ifndef _SUPPORT_DEFS_H
#include <be/support/SupportDefs.h>
#endif

/*****************************************************************************
 *
 *	ARP-VECTOR-I CLASS
 *
 *	The ArpVectorI is a template class that defines an abstract interface
 *	to an array-like object.  This is modelled directly after the STL
 *	vector class.  [And in fact, its common implementation is just
 *	exactly that.]
 *
 *****************************************************************************/

template<class T> class ArpVectorI {
public:
	virtual ~ArpVectorI() { }

	// element access
	virtual T&			operator[]( size_t n ) = 0;
	virtual const T&	operator[]( size_t n ) const = 0;
	virtual T&			at( size_t n ) = 0;
	virtual const T&	at( size_t n ) const = 0;
	
	virtual T&			front( ) = 0;
	virtual const T&	front( ) const = 0;
	virtual T&			back( ) = 0;
	virtual const T&	back( ) const = 0;
	
	// modification
	virtual ArpVectorI<T>&
						push_back( const T& e ) = 0;

	// number of items in array
	virtual size_t		size() const = 0;
	virtual void		resize( size_t sz, const T& v = T () ) = 0;

	// number of items space has been allocated for
	virtual size_t		capacity() const = 0;
	virtual void		reserve( size_t n ) = 0;

protected:
	inline ArpVectorI() { }
	inline ArpVectorI(const ArpVectorI& o)				{ };
	inline ArpVectorI& operator=(const ArpVectorI& o)	{ return *this; };
};

#endif
