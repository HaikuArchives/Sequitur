/* AmFileRosterEntryI.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * History
 * ~~~~~~~
 * 2001.03.28		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMKERNEL_AMFILEROSTERENTRYI_H
#define AMKERNEL_AMFILEROSTERENTRYI_H

#include <be/support/SupportDefs.h>
#include "AmPublic/AmDefs.h"
#include "AmKernel/AmKernelDefs.h"

/***************************************************************************
 * AM-FILE-ROSTER-ENTRY-I
 * This is the abstract superclass for any object that can appear in a
 * file roster.
 ****************************************************************************/
class AmFileRosterEntryI
{
public:
	virtual ~AmFileRosterEntryI()		{ }

	virtual file_entry_id	Id() const = 0;

	virtual BString			Label() const = 0;
	/* The key must uniquely identify this file entry in the system.
	 * It is of the form '[implementor]:[name]', for example
	 * 'arp:Pencil'.
	 */
	virtual BString			Key() const = 0;
	/* A brief explanation of this object.
	 */
	virtual BString			ShortDescription() const = 0;
	/* Contact info for the author of this entry.
	 */
	virtual BString			Author() const = 0;
	virtual BString			Email() const = 0;
	/* Answer true if this entry is valid and useable.  Entries
	 * will generally not be valid because there is already an
	 * entry with the same key.
	 */
	virtual bool			IsValid() const = 0;
	/* Answer true if NOTHING can be done with this entry --
	 * no editing, no deleting, no copying, nothing.
	 */
	virtual bool			IsImmutable() const { return false; }
	/* Implementors must answer a valid file name.
	 */
	virtual BString			LocalFileName() const = 0;
	/* If this entry was created based on a file, answer the
	 * file whence it came.
	 */
	virtual BString			LocalFilePath() const = 0;
	/* Implementors must read and write themselves to the
	 * supplied configuration message.
	 */
	virtual status_t		WriteTo(BMessage& config) const = 0;
};

/* A simple utility that tries to make sure the str is friendly
 * to all known file systems through some simple replacement
 * rules:
 *	_ = _0
 *	/ = _1
 *	\ = _2
 *	: = _3
 * and whatever else I can think of that would be a problem.
 */
BString convert_to_filename(const BString& str);

#endif
