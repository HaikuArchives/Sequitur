/* AmSysExCommand.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.03.28			hackborn@angryredplanet.com
 * Pulled this class out of AmDevice.h.
 */

#ifndef AMKERNEL_AMSYSEXCOMMAND_H
#define AMKERNEL_AMSYSEXCOMMAND_H

#include <vector.h>
#include <be/support/SupportDefs.h>
#include <be/support/String.h>
#include "AmPublic/AmDefs.h"
class AmEvent;
class AmSystemExclusive;

#define ARP_TWOS_COMPLEMENT			(0x00000001)

/*************************************************************************
 * _AM-COMMAND-VALUE
 * Support class for determining the mutable bytes of sysex data.  This
 * object determines which piece of sysex I apply to (index), what bytes
 * in that sysex I apply to (start through end), and what the valid values
 * are for those bytes (min through max).
 *************************************************************************/
class _AmCommandValue
{
public:
	_AmCommandValue();
	_AmCommandValue(int32 inIndex, uint32 inStart, uint32 inEnd,
					int32 inMin, int32 inMax);
	_AmCommandValue(const _AmCommandValue& o);

	_AmCommandValue&			operator=(const _AmCommandValue& o);
	
	int32			index;		// The command can store multiple pieces
								// of sysex data.  This specifies which
								// piece I apply to.  -1 for none.
	uint32			start, end;	// Specify the first and last byte for the value.
	int32			min, max;	// The minimum and maximum values.

	bool			MutableByte(int32 inIndex, uint32 b) const;
	void			Print() const;
};

/***************************************************************************
 * AM-SYS-EX-COMMAND
 * This class represents a unique sysex command for a device.  It provides
 * some user conveniences, like giving the command a name, specifying
 * bytes that the user can change, and giving names to those values.
 *
 * This is just an abstract class -- the subclasses define whether or not
 * the command is composed of a single sysex event or a phrase containing
 * multiple sysex events.  THESE TWO STYLES ARE INCOMPATIBLE IN A SINGLE
 * DEVICE.  Basically, some devices need the multi style, but not all of
 * them, and there's quite a bit more overhead for it so I'm letting a
 * device use the single style if it likes.
 ***************************************************************************/
class AmSysExCommand
{
public:
	AmSysExCommand(	const BString& key, const _AmCommandValue& deviceId,
					const _AmCommandValue& value, vector<BString>* valueLabels,
					uint32 bitNum = 14, uint32 flags = ARP_TWOS_COMPLEMENT);
	AmSysExCommand(const AmSysExCommand& o);
	virtual ~AmSysExCommand();
	
	virtual AmSysExCommand*	Copy() const = 0;
	virtual AmCommandType	Type() const = 0;
	
	bool					Matches(const BString& key) const;

	status_t				GetKey(BString& outKey) const;
	status_t				GetLabel(	const AmEvent* event,
										BString& outLabel,
										bool showKey, bool showValue) const;
	status_t				GetValue(const AmEvent* event, int32* outValue) const;
	status_t				SetValue(AmEvent* event, int32 value) const;

	virtual bool			Matches(const AmEvent* event) const = 0;
	virtual AmEvent*		NewEvent() const = 0;
	virtual status_t		ImposeSysEx(AmEvent* event) const = 0;
	/* Set any propteries of event to conform to mine.
	 */
	virtual status_t		Conform(AmEvent* event) const = 0;

protected:
	BString				mKey;			// Unique name for the command.
	_AmCommandValue		mDeviceId;
	_AmCommandValue		mValue;

	virtual const AmSystemExclusive*	SysExAt(int32 index, const AmEvent* event) const = 0;
	virtual AmSystemExclusive*			SysExAt(int32 index, AmEvent* event) const = 0;

	status_t					GetSxValue(const AmSystemExclusive* sx, int32* outValue) const;
	status_t					SetSxValue(AmSystemExclusive* sx, int32 value) const;
	bool						PrimMatches(int32 index, const AmSystemExclusive* sx1,
											const AmSystemExclusive* sx2) const;
	status_t					CopyInto(	const AmSystemExclusive* source,
											AmSystemExclusive* dest) const;
	
private:	
	vector<BString>		mValueLabels;
	uint32				mBitNum;
	uint32				mFlags;

	bool				MutableByte(int32 index, uint32 b) const;

public:
	virtual void		Print() const;
};

/***************************************************************************
 * AM-SYS-EX-SINGLE-COMMAND
 * This type of sysex command is composed of but a single system
 * exclusive event.
 ***************************************************************************/
class AmSysExSingleCommand : public AmSysExCommand
{
public:
	AmSysExSingleCommand(	AmSystemExclusive* sysex, int32 initValue,
							const BString& key, const _AmCommandValue& deviceId,
							const _AmCommandValue& value, vector<BString>* valueLabels,
							uint32 bitNum = 14, uint32 flags = ARP_TWOS_COMPLEMENT);
	AmSysExSingleCommand(const AmSysExSingleCommand& o);
	virtual ~AmSysExSingleCommand();
	
	virtual AmSysExCommand*		Copy() const;
	virtual AmCommandType		Type() const;

	virtual bool				Matches(const AmEvent* event) const;
	virtual AmEvent*			NewEvent() const;
	virtual status_t			ImposeSysEx(AmEvent* event) const;
	virtual status_t			Conform(AmEvent* event) const;

protected:
	virtual const AmSystemExclusive*	SysExAt(int32 index, const AmEvent* event) const;
	virtual AmSystemExclusive*			SysExAt(int32 index, AmEvent* event) const ;

private:
	typedef AmSysExCommand		inherited;
	AmSystemExclusive*	mSysEx;

public:
	virtual void		Print() const;
};

/***************************************************************************
 * AM-SYS-EX-MULTI-COMMAND
 * This type of sysex command is composed of phrase events, which are
 * composed of any number of system exclusive events.
 ***************************************************************************/
class AmSysExMultiCommand : public AmSysExCommand
{
public:
	AmSysExMultiCommand(vector<AmSystemExclusive*>& sysex, int32 initValue, const BString& key,
						const _AmCommandValue& deviceId, const _AmCommandValue& value,
						const _AmCommandValue* channel,
						vector<BString>* valueLabels, uint32 bitNum = 14,
						uint32 flags = ARP_TWOS_COMPLEMENT);
	AmSysExMultiCommand(const AmSysExMultiCommand& o);
	virtual ~AmSysExMultiCommand();
	
	virtual AmSysExCommand*		Copy() const;
	virtual AmCommandType		Type() const;

	virtual bool				Matches(const AmEvent* event) const;
	virtual AmEvent*			NewEvent() const;
	virtual status_t			ImposeSysEx(AmEvent* event) const;
	virtual status_t			Conform(AmEvent* event) const;

protected:
	virtual const AmSystemExclusive*	SysExAt(int32 index, const AmEvent* event) const;
	virtual AmSystemExclusive*			SysExAt(int32 index, AmEvent* event) const;

private:
	typedef AmSysExCommand		inherited;
	vector<AmSystemExclusive*> mSysEx;	// Commands can contain multiple sysex data

public:
	virtual void		Print() const;
};


#endif
