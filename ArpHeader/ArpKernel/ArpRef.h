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
 * ArpRef.h
 *
 * A template class for holding a pointer to a reference counted
 * object.  The referenced object must have the methods AddReference()
 * and RemReference(), which do the expected thing.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * None.
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
 * Aug. 6, 1998:
 * 	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPREF_H
#define ARPKERNEL_ARPREF_H

#include <ArpKernel/ArpString.h>
#include <be/support/SupportDefs.h>

/*****************************************************************************
 *
 *	The ArpBaseRef class is the base upon which the reference counting pointers
 *  are built.  It can contain an optional name for the pointer.
 *  This class is mostly useful for debugging, so that the referenced
 *  object can keep track of who is referencing it.
 *
 *****************************************************************************/

class ArpBaseRef {
public:
	ArpBaseRef(const ArpString& s = ArpString()) : name(s) { }
	
	const ArpString& Name() const		{ return name; }
	void Name(const ArpString& n)		{ name = n; }
private:
	ArpString name;
};

/*****************************************************************************
 *
 *	The ArpRef class is a simple template that looks like a pointer to
 *  its template type.  In addition, it calls the functions Ref() and
 *  Deref() on objects as they are assigned and removed from it,
 *  so that reference counts are automatically tracked.
 *  The ArpCRef class is the same thing, but is the equivalent of a
 *  const pointer.
 *
 *  Any object that is being reference counted must define these two
 *  functions:
 *
 *    AddReference(VzBaseRef* ref = NULL) const;
 *    RemReference(VzBaseRef* ref = NULL) const;
 *
 *  which do what you would expect.
 *
 *****************************************************************************/

template<class T> class ArpRef : public ArpBaseRef
{
public:
	ArpRef()					{ object = NULL; }
	ArpRef(T* obj)				{ object = obj; if(object) object->AddReference(this); }
	ArpRef(const ArpRef<T>& o)	{ object = o.object; if(object) object->AddReference(this); }
	ArpRef(const ArpString& s) : ArpBaseRef(s)
								{ object = NULL; }
	ArpRef(const ArpString& s, T* obj) : ArpBaseRef(s)
								{ object = obj; if(object) object->AddReference(this); }
	ArpRef(const ArpString& s, const ArpRef<T>& o) : ArpBaseRef(s)
								{ object = o.object; if(object) object->AddReference(this); }
	~ArpRef()					{ if(object) object->RemReference(this); object = NULL; }

	ArpRef<T>& operator = (const ArpRef<T>& o)
							{ if(o.object) o.object->AddReference(this);
							  if(object) object->RemReference(this);
							  object = o.object;
							  return *this; }
	ArpRef<T>& operator = (T* obj)
		{ if(obj) obj->AddReference(this); if(object) object->RemReference(this);
		  object = obj; return *this; }

	operator T* () const	{ return object; }
	
	T* operator -> () const	{ return object; }
	T&	operator * () const	{ return *object; }

	// Deal with relational comparisons of two references
#define	RELOP(OP)													\
	friend bool operator OP(const ArpRef<T>& a, const ArpRef<T>& b)	\
		{ return a.object OP b.object; }

	RELOP(<) RELOP(>) RELOP(<=) RELOP(>=)
#undef	RELOP
	
private:
	T* object;
};

// For const pointers

template<class T> class ArpCRef : public ArpBaseRef
{
public:
	ArpCRef()					{ object = NULL; }
	ArpCRef(const T* obj)		{ object = obj; if(object) object->AddReference(this); }
	ArpCRef(const ArpCRef<T>& o)	{ object =o.object; if(object) object->AddReference(this); }
	ArpCRef(const ArpString& s) : ArpBaseRef(s)
								{ object = NULL; }
	ArpCRef(const ArpString& s, const T* obj) : ArpBaseRef(s)
								{ object = obj; if(object) object->AddReference(this); }
	ArpCRef(const ArpString& s, const ArpCRef<T>& o) : ArpBaseRef(s)
								{ object = o.object; if(object) object->AddReference(this); }
	~ArpCRef()					{ if(object) object->RemReference(this); object = NULL;}

	ArpCRef<T>& operator = (const ArpCRef<T>& o)
							{ if(o.object) o.object->AddReference(this);
							  if(object) object->RemReference(this);
							  object = o.object;
							  return *this; }
	ArpCRef<T>& operator = (const T* obj)
		{ if(obj) obj->AddReference(this); if(object) object->RemReference(this);
		  object = obj; return *this; }

	operator const T* () const		{ return object; }
	
	const T* operator -> () const	{ return object; }
	const T&	operator * () const	{ return *object; }

	// Deal with relational comparisons of two references
#define	RELOP(OP)													\
	friend bool operator OP(const ArpCRef<T>& a, const ArpCRef<T>& b)	\
		{ return a.object OP b.object; }

	RELOP(<) RELOP(>) RELOP(<=) RELOP(>=)
#undef	RELOP
	
private:
	const T* object;
};

/*****************************************************************************
 *
 *	ARP-REFABLE-I INTERFACE
 *
 *	This is the basic interface to an object that is reference counted.  It
 *	works with the ArpRef and ArpCRef template classes above.
 *
 *****************************************************************************/

class ArpRefableI {
public:
	virtual void AddReference(ArpBaseRef* owner=NULL) const = 0;
	virtual void RemReference(ArpBaseRef* owner=NULL) const = 0;

protected:
	virtual ~ArpRefableI() 			{ }		// You may not directly delete these.
};

/*****************************************************************************
 *
 *	ARP-REFABLE CLASS
 *
 *	The most common implementation of the ArpRefableI interface.
 *
 *****************************************************************************/

class ArpRefable : public ArpRefableI {
protected:
	ArpRefable()					: mRefCount(0) { }
	virtual ~ArpRefable() 			{ }
	
	virtual void Delete()			{ delete this; }

public:
	virtual void AddReference(ArpBaseRef* owner=NULL) const {
		ArpRefable* me = const_cast<ArpRefable*>(this);
		atomic_add(&me->mRefCount, 1);
	}
	virtual void RemReference(ArpBaseRef* owner=NULL) const {
		ArpRefable* me = const_cast<ArpRefable*>(this);
		if( atomic_add(&me->mRefCount, -1) == 1 ) me->Delete();
	}

	int32 RefCount() const { return mRefCount; }
	
private:
	int32 mRefCount;
};

#endif
