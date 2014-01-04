/* AmBankI.h
 * Copyright (c)1996 - 2001 by Eric Hackborn.
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
 * 08.10.98		hackborn
 * Mutated this file from its original incarnation.
 */


#ifndef AMPUBLIC_AMBANKI_H
#define AMPUBLIC_AMBANKI_H

#include <support/SupportDefs.h>
#include <support/String.h>
#include "ArpKernel/ArpRef.h"
#include "AmPublic/AmEvents.h"

/***************************************************************************
 * AM-BANK-I
 * This is the abstract interface for a single bank in a device.  Mostly
 * clients will be interested in patch names, but other behaviour is
 * available.
 ***************************************************************************/
class AmBankI : public ArpRefableI
{
public:
	virtual ~AmBankI() { }

	/* Answer the number I am indexed at in my device.
	 */
	virtual uint32		BankNumber() const = 0;
	virtual BString		Name() const = 0;
	virtual uint32		CountPatches() const = 0;
	virtual BString		PatchName(uint32 patchNumber) const = 0;
	/* Answer an offset for patch numbering -- 0 is standard, if the
	 * answer is 1 then patches are numbered starting with 1, etc.
	 */
	virtual uint32		FirstPatchNumber() const = 0;
	/* Answer a new AmEvent or NULL.  The AmEvent should be an
	 * AmBankChange, although that will change.  When performed, this
	 * event will set the actual physical device to this bank.
	 */
	virtual AmEvent*	NewBankSelection() const = 0;
};

#endif
