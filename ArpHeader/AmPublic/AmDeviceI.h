/* AmDeviceI.h
 * Copyright (c)1998-2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This class is the public means of accessing device definitions.
 * A device is one concrete piece of MIDI gear - a Korg Wavestation,
 * for example.
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
 * 09.30.98		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMDEVICEI_H
#define AMPUBLIC_AMDEVICEI_H

#include <support/SupportDefs.h>
#include <AmPublic/AmBankI.h>
class BBitmap;
class AmEvent;
class AmSystemExclusive;

/***************************************************************************
 * AM-DEVICE-I
 * This is the interface for all devices.  A device is typically a MIDI
 * instrument, but it can be anything that understands MIDI.
 ***************************************************************************/
class AmDeviceI : public ArpRefableI
{
public:
	virtual ~AmDeviceI() { }

	/* This message is used to construct a standard label out of my
	 * manufacturer and product names.
	 */
	static BString				MakeLabel(const BString& mfg, const BString& product);

	virtual device_id			Id() const = 0;
	virtual BString				Label() const = 0;
	virtual BString				Manufacturer() const = 0;
	virtual BString				Product() const = 0;
	virtual BString				InputFilterKey() const = 0;
	virtual const BBitmap*		Icon(BPoint requestedSize) const = 0;
	
	virtual uint32				CountBanks() const = 0;
	virtual ArpCRef<AmBankI>	Bank(uint32 number) const = 0;
	/* Answer the bank at the given bank change event, or NULL.
	 */
	virtual ArpCRef<AmBankI>	Bank(const AmEvent* bankChange) const = 0;
	/* Answer a new AmEvent or NULL.  The AmEvent should be an
	 * AmBankChange.  When performed, this event will set the
	 * actual physical device to the specified bank.
	 */
	virtual AmEvent*			NewBankSelection() const = 0;

	virtual uint32				CountControls() const = 0;
	/* If prependNumber is true, then the answered string will include
	 * the controller's number.
	 */
	virtual BString				ControlName(uint32 controlNumber, bool prependNumber = true) const = 0;

	/* About a year after I finished working on Sequitur, I'm hacking
	 * on some sysex stuff.
	 */
	/* Answer what type of commands this device is currently using,
	 * multi or single.  This can also be used as a test to see
	 * if the device even has any commands.
	 */
	virtual AmCommandType		SysexCommandType() const = 0;
	virtual status_t			GetSysexCommandKey(	uint32 index, BString& outKey) const = 0;
	virtual status_t			GetSysexCommandInfo(	const AmEvent* event,
														uint32* outIndex, BString* outKey) const = 0;
	virtual status_t			GetSysexCommandLabel(	const AmEvent* event, BString& outLabel,
														bool key = true, bool value = true) const = 0;
	virtual AmEvent*			NewSysexCommand(const BString& key) const = 0;
	virtual status_t			GetSysexCommandValue(	const AmEvent* event,
														int32* outValue) const = 0;
	virtual status_t			SetSysexCommandValue(	AmEvent* event,
														int32 value) const = 0;
	virtual status_t			TransformSysexCommand(AmEvent* event, uint32 index) const = 0;
	/* this is solely for the file IO.  It finds which sysex command
	 * matches even (returning an error if none do), than sets any
	 * required info in event to match the command.
	 */
	virtual status_t			ConformSysexCommand(AmEvent* event) const = 0;
};


#endif
