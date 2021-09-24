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
 * ArpSTLVector.h
 *
 * An implementation of ArpVectorI using the STL vector class.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * operator[]() and at() should do bounds checking.
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

#ifndef ARPCOLLECTIONS_ARPSTLVECTOR_H
#define ARPCOLLECTIONS_ARPSTLVECTOR_H

#ifndef _ARP_BUILD_H
#include <ArpBuild.h>
#endif

#ifndef ARPCOLLECTIONS_ARPVECTORI_H
#include <ArpCollections/ArpVectorI.h>
#endif

#include <vector>

/*****************************************************************************
 *
 *	ARP-STL-VECTOR CLASS
 *
 *	An ArpVectorI interface that is implemented using the STL vector
 *	type.
 *
 *****************************************************************************/

template<class T> class ArpSTLVector : public std::vector<T>, public ArpVectorI<T> {
public:
	inline explicit ArpSTLVector()						{ }

	inline explicit
    ArpSTLVector(size_t n, const T& value = T ())		: std::vector<T>(n, value) { }
	
	inline ArpSTLVector(const ArpSTLVector<T>& x)		: std::vector<T>(x) { }

	virtual ~ArpSTLVector()								{ }

	// element access
	virtual T&			operator[]( size_t n )			{ return std::vector<T>::operator[](n); }
	virtual const T&	operator[]( size_t n ) const	{ return std::vector<T>::operator[](n); }
	virtual T&			at( size_t n )					{ return std::vector<T>::operator[](n); }
	virtual const T&	at( size_t n ) const			{ return std::vector<T>::operator[](n); }
	
	virtual T&			front( )						{ return std::vector<T>::front(); }
	virtual const T&	front( ) const					{ return std::vector<T>::front(); }
	virtual T&			back( )							{ return std::vector<T>::back(); }
	virtual const T&	back( ) const					{ return std::vector<T>::back(); }
	
	// modification
	virtual ArpVectorI<T>& push_back( const T& e )		{ std::vector<T>::push_back(e); return *this; }

	// number of items in array
	virtual size_t		size() const					{ return std::vector<T>::size(); }
	virtual void		resize(size_t sz, const T& v=T()) { std::vector<T>::resize(sz, v); }

	// number of items space has been allocated for
	virtual size_t		capacity() const				{ return std::vector<T>::capacity(); }
	virtual void		reserve( size_t n )				{ std::vector<T>::reserve(n); }
};

#endif
