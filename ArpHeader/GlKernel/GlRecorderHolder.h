/* GlRecorder.h
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
 * 2004.03.18			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLRECORDERHOLDER_H
#define GLKERNEL_GLRECORDERHOLDER_H

#include <be/app/Messenger.h>
//#include <be/support/Locker.h>
#include <ArpInterface/ArpBitmapView.h>
#include <GlPublic/GlControlTarget.h>
class GlRecorder;
class GlRecorderBitmapView;

/***************************************************************************
 * GL-RECORDER-HOLDER
 * I am an object that records data and updates a display.
 * The owner will receive information about when I start and stop.
 ***************************************************************************/
class GlRecorderHolder : public GlControlTarget
{
public:
	GlRecorderHolder(GlRecorder* r);
	virtual ~GlRecorderHolder();

	const GlRecorder*		Recorder() const;
	
	virtual status_t		ParamEvent(	gl_param_key key, int32 code,
										const GlParamWrap& wrap);
	virtual status_t		MidiEvent(	GlMidiEvent event, int32 letter,
										bigtime_t time);

	virtual status_t		StartRecording();
	virtual void			StopRecording();
	virtual void			Populate(GlControlTargetPopulator& p);
	virtual void			SetState(GlControlState& s) const;

private:
	GlRecorder*				mRecorder;
	BMessenger				mViewTarget;
//	BLocker					mThreadAccess;
	thread_id				mDrawThread, mStepThread;
	bool					mRunning;
	
	static int32			DrawThreadEntry(void *arg);
	static int32			StepThreadEntry(void *arg);
};

/***************************************************************************
 * GL-RECORDER-BITMAP-VIEW
 * This is a support object that holds onto a recorder, gets pulsed to
 * do an auto update, and uses the recorder to generate a new bitmap to
 * display.
 ***************************************************************************/
class GlRecorderBitmapView : public ArpBitmapView
{
public:
	GlRecorderBitmapView(	BRect frame, const char* name,
							uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~GlRecorderBitmapView();

	void					SetRecorder(GlRecorder* r);

	virtual void			MessageReceived(BMessage* msg);

private:
	typedef ArpBitmapView	inherited;
	GlRecorder*				mRecorder;

	void					DrawRecorder();
};

#endif
