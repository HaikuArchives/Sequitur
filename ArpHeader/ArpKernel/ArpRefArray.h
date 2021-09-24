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
 * ArpRefArray.h
 *
 * A dynamically resizable array that keeps track of reference
 * counts on the objects it contains.
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
 * 0.2: July 7, 1997
 *      Fixed a bounds problem: according to the C++ spec,
 *      the modulus operator is allowed to have an either
 *      positive or negative result when one of its operands
 *      is negative...  and PowerPC goes the negative route,
 *      causing us to allow negative array indices.  Ouch.
 *
 * 0.1: Created this file.
 *
 */

#pragma once

#ifndef ARPKERNEL_ARPREFARRAY_H
#define ARPKERNEL_ARPREFARRAY_H

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef DBREFARR
#define DBREFARR 1
#endif

ArpMODL(__FILE__, 0, arp_decl_ArpRefArray_mod);

// Thoroughly and completely undocumented...  ah well.

#if 1

template<class T> class ArpRefArray {
public:
	ArpRefArray(int init_size=0) : size(0), top(0),
									  array(NULL) {
		SetSize(init_size);
	}
	~ArpRefArray() {
		SetSize(0);
	}
	
	void SetSize(long new_size) {
		T** new_array = NULL;
		ArpD(cdb << ADH << "ArpRefArray: SetSize(" << new_size
					<< "), oldsize=" << size << std::endl);
		if( new_size > 0 ) new_array = new T*[new_size];
		long i = 0;
		if( array && size ) {
			for( ; i<size && i<new_size; i++ ) {
				new_array[i] = array[(top+i)%size];
			}
		}
		for( ; i<new_size; i++ ) new_array[i] = NULL;
		if( array && size ) {
			for( ; i<size; i++ ) {
				if( array[(top+i)%size] != NULL ) {
					array[(top+i)%size]->Deref();
				}
			}
		}
		if( array ) delete array;
		array = new_array;
		size = new_size;
		top = 0;
		ArpD(cdb << ADH << "Resulting array:" << std::endl;
				for( int j=0; j<size; j++ ) {
					cdb << "Element " << j << " = 0x"
						<< (void*)array[j] << std::endl;
				}
			);
	}

	inline long Size(void) const { return size; }
	
	// positive scrolls new elements in to bottom
	void ScrollView(int offset) {
		if( size <= 0 ) return;
		top = (top+offset)%size;
		// make sure index is positive...  the % operator
		// on a negative may result in either negative or
		// positive, depending on the implementation...
		// PowerPC apparently goes the negative route.
		while( top < 0 ) top += size;
		if( offset > 0 ) {
			for( long i=0; i<offset; i++ ) {
				SetElem(size-i-1,NULL);
			}
		} else {
			offset = -offset;
			for( long i=0; i<offset; i++ ) {
				SetElem(i,NULL);
			}
		}
	}
	
	T* GetElem(long index) {
		if( size <= 0 ) return NULL;
		index = (index+top)%size;
		while( index < 0 ) index += size;
		return array[index];
	}
	const T* GetElem(long index) const {
		if( size <= 0 ) return NULL;
		index = (index+top)%size;
		while( index < 0 ) index += size;
		return array[index];
	}
	
	void SetElem(long index, T* val) {
		if( size <= 0 ) return;
		index = (index+top)%size;
		while( index < 0 ) index += size;
		ArpD(cdb << ADH << "Replace element " << index
					<< ", was 0x" << ((void*)array[index])
					<< ", now 0x" << ((void*)val) << std::endl);
		if( val ) val->Ref();
		if( array[index] != NULL ) array[index]->Deref();
		array[index] = val;
	}

	inline T* operator [] (long index) {
		return GetElem(index);
	}
	inline const T* operator [] (long index) const {
		return GetElem(index);
	}
	
private:
	long size;			// Total number of entries in array
	long top;			// Current top position of array
	T** array;			// The actual array
};

#else

// If this class is going to be used a lot, it would probably be
// much better designed as a base class that is not a template,
// and just a tiny template subclass that converts the base
// class's void* into its proper type.
//
// But this isn't actually implemented, yet...
//
// (Or make an ArpReferable base class that any objects in it
//  need to inherit from...  might be a better solution.)

class ArpRefArrayBase {
public:
	ArpRefArray(int init_size=0);
	~ArpRefArray();
	
	void SetSize(long new_size);

	long Size(void) { return size; }
	
	// positive scrolls new elements in to bottom
	void ScrollView(int offset);
	void* GetVoidElem(long index);
	void SetVoidElem(long index, void* val);
	
	virtual void DoDeref(void* elem) { };
	virtual void DoRef(void* elem) { };
	
private:
	long size;			// Total number of entries in array
	long top;			// Current top position of array
	void** array;		// The actual array
};

#endif

#endif

