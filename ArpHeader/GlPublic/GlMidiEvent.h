/* GlMidiEvent.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2004.02.19			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLMIDIEVENT_H
#define GLPUBLIC_GLMIDIEVENT_H

#include <ArpCore/String16.h>
#include <app/Message.h>

/***************************************************************************
 * GL-MIDI-EVENT
 * REALLY simple implementation of all the events I handle.
 ***************************************************************************/
class GlMidiEvent
{
public:
	GlMidiEvent();
	GlMidiEvent(int32 inPortId, int32 inType, int32 inChannel,
				int32 inV1, int32 inV2);
	GlMidiEvent(const GlMidiEvent& o);

	int32					portId;
	enum {
		UNKNOWN				= 0,
		CONTROL_CHANGE		= 1,	// value1 is control number, value2 is
									// control value
		MMC					= 2,	// value1 is device ID, value2 is command
		
		_TYPE_SIZE
	};
	int32					type;
	int32					channel;
	int32					value1, value2;

	bool					operator==(const GlMidiEvent& o) const;
	bool					operator!=(const GlMidiEvent& o) const;
	GlMidiEvent&			operator=(const GlMidiEvent& o);

	const BString16*		PortName() const;

	/* Copy value1, value2, both or neither, depending on my type.
	 */
	void					GetValue(const GlMidiEvent& o);
	/* Answer a floating value (0 - 1) based on my type.
	 */
	float					ScaledValue() const;

	/* If portName is true, then the portId is translated to the
	 * string name for the port.  This is only needed when reading/
	 * writing to a file.
	 */
	status_t				ReadFrom(const BMessage& msg, bool portName = false);
	status_t				WriteTo(BMessage& msg, bool portName = false) const;

	status_t				ReadFakeFrom(const BMessage& msg, BString16* portName);
	status_t				WriteFakeTo(BMessage& msg, const BString16* portName) const;

	void					Print() const;
};

#endif
