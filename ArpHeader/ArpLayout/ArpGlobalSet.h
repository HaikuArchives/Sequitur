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
 * ArpGlobalSet.h
 *
 * A container for values that apply to an entire set of parameters.
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
 *	• 8/30/1998:
 *	  Created this file.
 *
 */

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#define ARPLAYOUT_ARPGLOBALSET_H

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

class ArpGlobalSetI
{
public:
	ArpGlobalSetI()											{ }
	virtual ~ArpGlobalSetI()								{ }
	
	virtual const BMessage* GlobalValues() const			= 0;
	virtual status_t AddGlobals(const BMessage* values)		= 0;
	virtual status_t UpdateGlobals(const BMessage* values)	= 0;
	virtual bool IsGlobalUpdate() const						= 0;

private:
	// no copy or assignment.
	ArpGlobalSetI(const ArpGlobalSetI& other);
	ArpGlobalSetI& operator=(const ArpGlobalSetI& other);
};

class ArpGlobalUpdate : public ArpGlobalSetI
{
public:
	ArpGlobalUpdate(const BMessage* newValues);
	virtual ~ArpGlobalUpdate();
	
	virtual const BMessage* GlobalValues() const;
	virtual status_t AddGlobals(const BMessage* values);
	virtual status_t UpdateGlobals(const BMessage* values);
	virtual bool IsGlobalUpdate() const;
	
private:
	// no copy or assignment.
	ArpGlobalUpdate(const ArpGlobalUpdate& other);
	ArpGlobalUpdate& operator=(const ArpGlobalUpdate& other);
	
	const BMessage* mValues;
};

class ArpStdGlobalSet : public ArpGlobalSetI
{
public:
	ArpStdGlobalSet()
	{
	}
	ArpStdGlobalSet(const BMessage& init)
		: mGlobals(init)
	{
	}
	ArpStdGlobalSet(const BMessage* init)
		: mGlobals(init ? (*init) : BMessage())
	{
	}
	virtual ~ArpStdGlobalSet()								{ }
	
	virtual const BMessage* GlobalValues() const
	{
		return &mGlobals;
	}
	
	virtual status_t AddGlobals(const BMessage* values)
	{
		return B_OK;
	}
	
	virtual status_t UpdateGlobals(const BMessage* values)
	{
		return B_OK;
	}
	
	virtual bool IsGlobalUpdate() const
	{
		return false;
	}
	
private:
	// no copy or assignment.
	ArpStdGlobalSet(const ArpStdGlobalSet& other);
	ArpStdGlobalSet& operator=(const ArpStdGlobalSet& other);
	
	BMessage mGlobals;
};

#endif
