/* GlControlTarget.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2004.03.22			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLCONTROLTARGET_H
#define GLKERNEL_GLCONTROLTARGET_H

#include <GlPublic/GlControlState.h>
#include <GlPublic/GlMidiEvent.h>
#include <GlPublic/GlParamWrap.h>
class GlRecorderBitmapView;

/* The ParamEvent() codes
 */
enum {
	GL_NO_PARAM_CHANGE		= 0,
	GL_PARAM_CHANGING		= 'GLpc',	// The param is changing
	GL_PARAM_CHANGED		= 'GLpf'	// The param is finished
};

/***************************************************************************
 * GL-CONTROL-TARGET-POPULATOR
 * A support class necessary for certain targets to make a connection
 * to view components.
 ***************************************************************************/
class GlControlTargetPopulator
{
public:
	/* A name of 0 answers the first view found.
	 */
	virtual GlRecorderBitmapView*		RecorderView(const char* name = 0) = 0;	
};

/***************************************************************************
 * GL-CONTROL-TARGET
 * An interface class for anyone receiving control input -- either from MIDI
 * events or onscreen controls.
 ***************************************************************************/
class GlControlTarget
{
public:
	virtual ~GlControlTarget();

	enum {
		EXCLUSIVE_F			= 0x00000001
	};
	uint32					Flags() const;

	virtual status_t		ParamEvent(	gl_param_key key, int32 code,
										const GlParamWrap& wrap) = 0;
	virtual status_t		MidiEvent(	GlMidiEvent event, int32 letter,
										bigtime_t time) = 0;

	virtual status_t		StartRecording();
	virtual void			StopRecording();

	/* Subclasses should cache any view-based info they need.
	 */
	virtual void			Populate(GlControlTargetPopulator& p);
	/* Subclasses should set the state object according to their
	 * internal state.
	 */	
	virtual void			SetState(GlControlState& s) const;
	
protected:
	uint32					mFlags;

	GlControlTarget(uint32 flags = 0);
};

/***************************************************************************
 * GL-CONTROL-CHANNEL
 * A control target manager.
 ***************************************************************************/
class GlControlChannel
{
public:
	virtual status_t		ParamEvent(	gl_param_key key, int32 code,
										const GlParamWrap& wrap) = 0;
	virtual status_t		MidiEvent(	GlMidiEvent event, bigtime_t time) = 0;

	virtual status_t		Add(GlControlTarget* target) = 0;

	virtual void			Populate(GlControlTargetPopulator& p) = 0;
	virtual void			SetState(GlControlState& s) const = 0;
};

#endif
