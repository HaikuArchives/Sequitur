/* AmBank.h
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


#ifndef AMKERNEL_AMBANK_H
#define AMKERNEL_AMBANK_H

#include <vector.h>
#include "AmPublic/AmBankI.h"
#include "AmKernel/AmPhraseEvent.h"

/***************************************************************************
 * AM-BANK
 ***************************************************************************/
class AmBank : public AmBankI,
			   public ArpRefable
{
public:
	AmBank(const char* name = NULL, uint32 patchSize = 128);
	AmBank(const BMessage& msg);
	AmBank(const AmBank& o);

	// Attach ArpRefableI interface
	virtual void		AddReference(ArpBaseRef* owner) const	{ ArpRefable::AddReference(owner); }
	virtual void		RemReference(ArpBaseRef* owner) const	{ ArpRefable::RemReference(owner); }
		
	// AmBankI interface
	virtual uint32		BankNumber() const;
	virtual BString		Name() const;
	virtual uint32		CountPatches() const;
	virtual BString		PatchName(uint32 number) const;
	virtual uint32		FirstPatchNumber() const;
	virtual AmEvent*	NewBankSelection() const;
		
	// Modification
	void				SetBankNumber(uint32 number);
	void				SetName(const char* name);
	void				SetPatchCount(uint32 size);
	void				SetPatchName(uint32 number, const char* name);
	void				SetFirstPatchNumber(uint32 number);
	void				SetBankSelection(const AmBankChange* bankEvent);
	AmBankChange*		BankSelection() const;
	
	AmBank*				Copy() const;
	status_t			WriteTo(BMessage& msg) const;
	status_t			ReadFrom(const BMessage& msg);

protected:
	virtual ~AmBank();
		
	virtual void		InitPatchNames(uint32 from, uint32 to);
	
private:
	uint32				mBank;
	BString				mName;
	vector<BString>		mPatches;
	uint32				mFirstPatchNumber;
	AmBankChange*		mSelectionEvent;
};

#endif /* ARP_MIDI_BANK_H */
