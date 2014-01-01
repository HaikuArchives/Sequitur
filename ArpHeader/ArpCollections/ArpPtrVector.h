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
 * ArpPtrVector.h
 *
 * This is an ArpVectorI that is specifically for pointer types.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * Originally I was hoping to implement this with a BList, but that
 * hasn't worked so far.  Currently it is using an STL vector<void*>.
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

#ifndef ARPCOLLECTIONS_ARPPTRVECTOR_H
#define ARPCOLLECTIONS_ARPPTRVECTOR_H

#ifndef _ARP_BUILD_H
#include <ArpBuild.h>
#endif

#ifndef ARPCOLLECTIONS_ARPVECTORI_H
#include <ArpCollections/ArpVectorI.h>
#endif

#ifndef _LIST_H
#include <be/support/List.h>
#endif

#include <vector>

/*****************************************************************************
 *
 *	ARP-PTR-VECTOR CLASS
 *
 *	An ArpVectorI interface that is specifically for pointer-sized
 *	objects (e.g., void*, BView*, int32, etc).  It is a thin wrapper
 *	around BList, so instantiating one of these templates for a new
 *	type adds very little additional code to your application.
 *
 *	This class has two main advantages over using a raw BList:
 *
 *	- It can be used anywhere an ArpVectorI handle is required.
 *	- It is type-safe.  (And the BList implementation is hidden to
 *	  ensure type safety.)
 *
 *****************************************************************************/

#if 1

template<class T> class ArpPtrVector : public ArpVectorI<T> {
public:
	inline explicit ArpPtrVector()						{ }

	inline explicit
    ArpPtrVector(size_t n)								: mVector(n, 0) { }
	
	inline ArpPtrVector(const ArpPtrVector<T>& x)		: mVector(x.mVector) { }

	virtual ~ArpPtrVector()								{ }

	// element access
	virtual T&			operator[]( size_t n )			{ return *((T*)(&mVector[n])); }
	virtual const T&	operator[]( size_t n ) const	{ return *((const T*)(&mVector[n])); }
	virtual T&			at( size_t n )					{ return *((T*)(&mVector.operator[](n))); }
	virtual const T&	at( size_t n ) const			{ return *((const T*)(&mVector.operator[](n))); }
	
	virtual T&			front( )						{ return *((T*)(&mVector.front())); }
	virtual const T&	front( ) const					{ return *((const T*)(&mVector.front())); }
	virtual T&			back( )							{ return *((T*)(&mVector.back())); }
	virtual const T&	back( ) const					{ return *((const T*)(&mVector.back())); }
	
	// modification
	virtual ArpVectorI<T>& push_back( const T& e )		{ void* val = (void*)e; mVector.push_back(val); return *this; }

	// number of items in array
	virtual size_t		size() const					{ return mVector.size(); }
	virtual void		resize(size_t sz, const T& v=T()) { void* val = (void*)v; mVector.resize(sz, val); }

	// number of items space has been allocated for
	virtual size_t		capacity() const				{ return mVector.capacity(); }
	virtual void		reserve( size_t n )				{ mVector.reserve(n); }

private:
	vector<void*> mVector;
};

#endif

#if 0 // bah - this doesn't actually work

template<class T> class ArpPtrVector : public ArpVectorI<T> {
public:
	inline explicit ArpPtrVector(int32 itemsPerBlock = 20)
		: mList(itemsPerBlock)							{ }

	inline ArpPtrVector(const ArpPtrVector<T>& x)		: mList(x.mList) { }

	virtual ~ArpPtrVector()								{ }

	// note -- currently none of these do bounds checking or
	// throw errors. (ouch!)
	
	// element access
	virtual T&			operator[]( size_t n )			{ return (T&)(mList.Items()[n]); }
	virtual const T&	operator[]( size_t n ) const	{ return (T&)(mList.Items()[n]); }
	virtual T&			at( size_t n )					{ return (T&)(mList.Items()[n]); }
	virtual const T&	at( size_t n ) const			{ return (T&)(mList.Items()[n]); }
	
	virtual T&			front( )						{ return (T&)(mList.Items()[0]); }
	virtual const T&	front( ) const					{ return (T&)(mList.Items()[0]); }
	virtual T&			back( )							{ return (T&)(mList.Items()[mList.CountItems()-1]); }
	virtual const T&	back( ) const					{ return (T&)(mList.Items()[mList.CountItems()-1]); }
	
	// modification
	virtual ArpVectorI<T>&
						push_back( const T& e )			{ mList.AddItem((void*)e); return *this; }

	// number of items in array
	virtual size_t		size() const					{ return mList.CountItems(); }
	virtual void		resize(size_t sz, const T& v=T()) {
		int32 curSize = mList.CountItems();
		while( curSize < sz ) {
			mList.AddItem((void*)v);
			curSize++;
		}
		if( curSize > sz ) {
			mList.RemoveItems(sz, curSize-sz);
		}
	}

	// number of items space has been allocated for
	// currently, the capacity is always the same as the size
	virtual size_t		capacity() const {
		return size();
	}
	
	virtual void		reserve( size_t n ) {
		// can't do anything right now
	}

private:
	BList mList;
};

#endif

#endif
