/* ArpIntToStringMapI.h
 * Copyright (c)1999 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * To do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 07.02.99		hackborn
 * Created this file.
 */

#ifndef ARPVIEWSPUBLIC_ARPINTTOSTRINGMAPI_H
#define ARPVIEWSPUBLIC_ARPINTTOSTRINGMAPI_H

/***************************************************************************
 * ARP-INT-TO-STRING-MAP-I
 * This interface class defines a simple protocol for three-way conversions
 * between IDs, indexes, and names.
 *
 * An ID is an int32 that defines some unique identifier for each entry.
 * The ID value is assigned by the synthesizer using the map, so they are
 * completely arbitrary:  They could be in any order (or none), have missing
 * gaps, etc.
 *
 * An index is an int32 between 0 and the size of the map  In this case,
 * you are thinking of the map as an array.
 * 
 * A name is a user-friendly string for each entry.
 *
 * In "good" implementors of this protocol, indexes and IDs will be the
 * same.  That is, the entries will be ordered and numbered from 0 to
 * the total number of entries.  "Bad" implementors break this, and just
 * assign arbitrary IDs to each entry (good and bad are in quotes because
 * how the mapping occurs is based on the synth's params, not any
 * intrinsic problem with the subclass implementation).
 *
 * This class is set up to assume that the subclass is good, meaning that
 * indexes and IDs are mapped to the same values.  If the subclass is bad,
 * it will need to override the methods provided here.
 ***************************************************************************/
class ArpIntToStringMapI
{
public:
	/* Clients should treat this as the destructor for the class, calling it
	 * whenever they'd call delete.  A typical subclass will implement this
	 * method by simple deleting themselves.
	 *
	 * Special subclasses might be singletons or other odd instances, and not
	 * implement this.  These subclasses of course will require that the
	 * instance have an alternative means of destruction.
	 */
	virtual void Release() { delete this; }

	virtual status_t IdForIndex(int32 index,
								int32 *answer) const
						{ *answer = index; return B_OK; }
	virtual status_t IdForName(const char *name, int32 *answer) const = 0;

	virtual status_t IndexForId(int32 id,
								int32 *answer) const
						{ *answer = id; return B_OK; }
	virtual status_t IndexForName(	const char *name,
									int32 *answer) const
						{ return IdForName(name, answer); }

	virtual status_t NameForId(	int32 id,
								char **answer) const = 0;
	virtual status_t NameForIndex(	int32 index,
									char **answer) const
						{ return NameForId(index, answer); }

protected:
	virtual ~ArpIntToStringMapI() { ; }
};

#endif

