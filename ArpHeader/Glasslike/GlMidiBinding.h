/* GlMidiBinding.h
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
 * 2003.01.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLMIDIBINDING_H
#define GLASSLIKE_GLMIDIBINDING_H

#include <ArpCore/String16.h>
#include <ArpCore/StlVector.h>
#include <Glasslike/GlMidi.h>
class GlRootNode;

/***************************************************************************
 * GL-MIDI-BINDING
 * A binding between a midi event and a grid.
 ***************************************************************************/
class GlMidiBinding
{
public:
	GlMidiBinding();
	~GlMidiBinding();

	GlMidiEvent				event;
	int32					rt;			// A realtime controller, -1 (off) to 25 (Z)
	/* Bindings can either be on a realtime controller or on a
	 * node.  If node, then these fields are valid.
	 */
	BString16				filename;
	int32					rootIndex;
	int32					paramIndex;
	GlRootNode*				root;

	/* Only valid if this is a FAKE_F binding.
	 */
	BString16				portname;

	GlMidiBinding&			operator=(const GlMidiBinding& o);

	enum {
		FAKE_F				= 0x00000001	// The event is from a port that doesn't exist
	};
	uint32					Flags() const;

	status_t				SetValueRange(float min, float max);
	void					GetValueRange(float* min, float* max) const;

	const BString16*		PortName() const;

	/* Copy in the values and instantiate a new grid.
	 */
	status_t				Load(GlMidiBinding* binding);
	/* Instantiate a new grid based on my values.
	 */
	status_t				Load();

	status_t				WriteTo(BMessage& msg) const;
	status_t				ReadFrom(const BMessage& msg, uint32 flags = 0);

	void					Print() const;

private:
	float					mMin, mMax;
	uint32					mFlags;
};

/***************************************************************************
 * GL-MIDI-BINDING-LIST
 * The event is essentially the key for the binding list.
 * FIX:  THIS NEEDS TO BE A LOCKED OBJECT!
 ***************************************************************************/
class GlMidiBindingList
{
public:
	GlMidiBindingList();
	~GlMidiBindingList();

	bool					IsDirty() const;

	uint32					Size() const;
	const GlMidiBinding*	At(uint32 index) const;
	GlMidiBinding*			At(GlMidiEvent event);

	/* If no binding exists with the supplied ID then add a new one.
	 */
	status_t				Update(GlMidiBinding* binding);
	status_t				Delete(GlMidiEvent event);

	status_t				WriteTo(BMessage& msg) const;
	status_t				ReadFrom(const BMessage& msg);

private:
	std::vector<GlMidiBinding*>	mBindings;
	bool					mDirty;

	void					Free();
};

#endif
