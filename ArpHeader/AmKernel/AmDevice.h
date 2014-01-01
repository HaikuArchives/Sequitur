/* AmDevice.h
 * Copyright (c)1996 - 2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This class stores preference and default information for one piece of
 * MIDI gear -- a synthesizer, a signal processor, etc. -- anything that
 * has either a MIDI in or MIDI out port.  Default information is such
 * items as how many banks of patches it contains, what the names are for
 * the various patches, what the names are for the control changes, and
 * hopefully one day items like how a bank change is accomplished.
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

#ifndef AMKERNEL_AMDEVICE_H
#define AMKERNEL_AMDEVICE_H

#include <be/app/Message.h>
#include <be/storage/File.h>
#include <be/support/SupportDefs.h>
#include "AmPublic/AmDeviceI.h"
#include "AmKernel/AmBank.h"
#include "AmKernel/AmFileRosterEntryI.h"
class AmSysExCommand;

/***************************************************************************
 * AM-DEVICE
 ***************************************************************************/
class AmDevice : public AmDeviceI,
				 public ArpRefable,
				 public AmFileRosterEntryI
{
public:
	AmDevice(	const char* name = NULL, const char* author = NULL,
				const char* email = NULL);
	AmDevice(const BMessage& config, bool readOnly, const char* filePath);
	AmDevice(const AmDevice& o);

	// --------------------------------------------------------
	// ARP-REFABLE INTERFACE
	// --------------------------------------------------------
	virtual void 			AddReference(ArpBaseRef* owner) const	{ ArpRefable::AddReference(owner); }
	virtual void 			RemReference(ArpBaseRef* owner) const	{ ArpRefable::RemReference(owner); }
	
	// --------------------------------------------------------
	// AM-DEVICE-I INTERFACE
	// --------------------------------------------------------
	virtual device_id			Id() const;
	virtual BString				Manufacturer() const;
	virtual BString				Product() const;
	virtual const BBitmap*		Icon(BPoint requestedSize) const;
		
	virtual uint32				CountBanks() const;
	virtual ArpCRef<AmBankI>	Bank(uint32 number) const;
	virtual ArpCRef<AmBankI>	Bank(const AmEvent* bankChange) const;
	virtual AmEvent*			NewBankSelection() const;

	virtual uint32				CountControls() const;
	virtual BString				ControlName(uint32 controlNumber, bool prependNumber = true) const;

	virtual AmCommandType		SysexCommandType() const;
	virtual status_t			GetSysexCommandKey(	uint32 index, BString& outKey) const;
	virtual status_t			GetSysexCommandInfo(	const AmEvent* event,
														uint32* outIndex, BString* outKey) const;
	virtual status_t			GetSysexCommandLabel(	const AmEvent* event, BString& outLabel,
														bool key = true, bool value = true) const;
	virtual AmEvent*			NewSysexCommand(const BString& key) const;
	virtual status_t			GetSysexCommandValue(	const AmEvent* event,
														int32* outValue) const;
	virtual status_t			SetSysexCommandValue(	AmEvent* event,
														int32 value) const;
	virtual status_t			TransformSysexCommand(AmEvent* event, uint32 index) const;
	virtual status_t			ConformSysexCommand(AmEvent* event) const;

	// --------------------------------------------------------
	// AM-FILE-ROSTER-ENTRY-I INTERFACE
	// --------------------------------------------------------
	virtual BString			Label() const;
	virtual BString			Key() const;
	virtual BString			ShortDescription() const;
	virtual BString			Author() const;
	virtual BString			Email() const;
	virtual BString			InputFilterKey() const;
	virtual bool			IsValid() const;
	virtual BString			LocalFileName() const;
	virtual BString			LocalFilePath() const;
	virtual status_t		WriteTo(BMessage& config) const;

	// --------------------------------------------------------
	// MODIFICATION
	// --------------------------------------------------------
	void					SetManufacturer(const char* mfg);
	void					SetName(const char* name);
	void					SetShortDescription(const char* s);
	void					SetAuthor(const char* author);
	void					SetEmail(const char* email);
	void					SetIsValid(bool isValid);
	void					SetControlCount(uint32 size);
	void					SetControlName(uint32 number, const char* name);
	void					SetBankCount(uint32 size);
	ArpRef<AmBank>			Bank(uint32 number);
	void					SetBank(uint32 number, AmBank* bank);
	status_t				AddBank(AmBank* bank);
	status_t				RemoveBank(ArpRef<AmBank> bank);
	void					SetBankSelection(const AmBankChange* bankEvent);
	AmBankChange*			BankSelection() const;
	status_t				SetIcon(const BBitmap* bitmap);		
	BRect					IconBounds() const;
	BBitmap*				Icon(BPoint requestedSize);
	void					SetInputFilterKey(const char* key);
	
	bool					IsReadOnly() const;

	AmDevice*				Copy() const;

	status_t				ReadFrom(const BMessage& config);

protected:
	virtual ~AmDevice();

	status_t				ReadSysExCommand(const BMessage& msg);
	void					InitControlNames(uint32 from, uint32 to);
	
private:
	BString						mManufacturer;
	BString						mName;
	BString						mShortDescription;
	BString						mAuthor;
	BString						mEmail;
	BString						mInputFilterKey;
	bool						mIsValid;
	vector<BString>				mControls;
	vector< ArpRef<AmBank> >	mBanks;
	AmBankChange*				mSelectionEvent;
	BBitmap*					mIcon;
	bool						mReadOnly;
	BString						mFilePath;
	vector<AmSysExCommand*>		mSysExCommands;
};


#endif
