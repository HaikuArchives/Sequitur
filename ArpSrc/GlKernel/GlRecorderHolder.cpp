#include <be/support/Autolock.h>
#include <ArpCore/ArpDebug.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlRecorder.h>
#include <GlKernel/GlRecorderHolder.h>

static const uint32	_DRAW_RECORDER_MSG		= 'iDr_';

/***************************************************************************
 * GL-RECORDER-HOLDER
 ****************************************************************************/
GlRecorderHolder::GlRecorderHolder(GlRecorder* r)
		: mRecorder(r), mDrawThread(0), mStepThread(0),
		  mRunning(false)
{
	ArpASSERT(mRecorder);
	if (mRecorder) mRecorder->IncRefs();
}

GlRecorderHolder::~GlRecorderHolder()
{
	StopRecording();
	/* FIX:  Probably need a lock to keep myself
	 * from accessing the recorder?  Or a wait?
	 */
	if (mRecorder) mRecorder->DecRefs();
}

const GlRecorder* GlRecorderHolder::Recorder() const
{
	return mRecorder;
}

status_t GlRecorderHolder::ParamEvent(	gl_param_key key, int32 code,
										const GlParamWrap& wrap)
{
//	printf("GlRecorderHolder::ParamEvent()\n");
	if (!mRecorder) return B_ERROR;
	mRecorder->ParamEvent(key, wrap);
	return B_OK;
}

status_t GlRecorderHolder::MidiEvent(	GlMidiEvent event, int32 letter,
										bigtime_t time)
{
	if (!mRecorder) return B_ERROR;
	mRecorder->MidiEvent(event, letter);
	return B_OK;
}

status_t GlRecorderHolder::StartRecording()
{
	ArpASSERT(!mRunning);
	StopRecording();
	printf("Start recording\n");

	if (mRecorder) mRecorder->Init();
	mRunning = true;
	mFlags |= EXCLUSIVE_F;
	
	mDrawThread = spawn_thread(	DrawThreadEntry,
								"GL Recorder Draw",
								B_NORMAL_PRIORITY,
								this);
	if (mDrawThread <= 0) return B_ERROR;
	resume_thread(mDrawThread);

	mStepThread = spawn_thread(	StepThreadEntry,
								"GL Recorder Step",
								B_NORMAL_PRIORITY,
								this);
	if (mStepThread <= 0) return B_ERROR;
	resume_thread(mStepThread);

	return B_OK;
}

void GlRecorderHolder::StopRecording()
{
	if (!mRunning) return;

	printf("Stop recording\n");
	mRunning = false;
	mFlags &= ~EXCLUSIVE_F;
	status_t		err;
	if (mDrawThread > 0) {
		wait_for_thread(mDrawThread, &err);
		mDrawThread = 0;
	}
	if (mStepThread > 0) {
		wait_for_thread(mStepThread, &err);
		mStepThread = 0;
	}

	if (!mRecorder) return;
	mRecorder->UpdateSource();
}

void GlRecorderHolder::Populate(GlControlTargetPopulator& p)
{
	if (!mRecorder) return;

	GlRecorderBitmapView* view = p.RecorderView();
	if (!view) mViewTarget = BMessenger();
	else {
		mViewTarget = BMessenger(view);
		view->SetRecorder(mRecorder);
	}
}

void GlRecorderHolder::SetState(GlControlState& s) const
{
	if (!mRecorder) return;
	s.SetRecordable();
	mRecorder->SetState(s);
}

int32 GlRecorderHolder::DrawThreadEntry(void *arg)
{
	GlRecorderHolder*		rh = (GlRecorderHolder*)arg;
	if (!rh) return B_ERROR;
	/* Refresh 30 frames a second.
	 */
	bigtime_t				snoozeTime = bigtime_t(float(1000000) / 30);

	while (rh->mRunning) {
		snooze(snoozeTime);
		rh->mViewTarget.SendMessage(_DRAW_RECORDER_MSG);
	}

	return B_OK;
}

int32 GlRecorderHolder::StepThreadEntry(void *arg)
{
	GlRecorderHolder*		rh = (GlRecorderHolder*)arg;
	if (!rh) return B_ERROR;
	GlRecorder*				r = rh->mRecorder;
	if (!r) return B_ERROR;
	/* FIX: Figure the snooze time based on the tempo.
	 */
//	float					f = 
//	bigtime_t				snoozeTime = 20000;
	bigtime_t				snoozeTime = bigtime_t(float(1000000) / 60);
	float					delta = 0.01f, step = 0.0f;
	
	while (rh->mRunning) {
		snooze(snoozeTime);
		step += delta;
		if (step >= 1.0) step = 0.0;
		{
			BAutolock		l(r->mAccess);
			r->LockedSetStep(step);
		}
	}

	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-RECORDER-BITMAP-VIEW
 ***************************************************************************/
GlRecorderBitmapView::GlRecorderBitmapView(	BRect frame, const char* name,
											uint32 resizeMask)
		: inherited(frame, name, resizeMask), mRecorder(0)
{
}

GlRecorderBitmapView::~GlRecorderBitmapView()
{
	SetRecorder(0);
}

void GlRecorderBitmapView::SetRecorder(GlRecorder* r)
{
	if (mRecorder) mRecorder->DecRefs();
	mRecorder = r;
	if (mRecorder) mRecorder->IncRefs();
}

void GlRecorderBitmapView::MessageReceived(BMessage* msg)
{
	if (msg->what == _DRAW_RECORDER_MSG) {
		if (!mRecorder) return;
		DrawRecorder();
		return;
	}
	inherited::MessageReceived(msg);
}

void GlRecorderBitmapView::DrawRecorder()
{
	ArpASSERT(mRecorder);
	ArpVALIDATE(mBitmap, return);

	ArpBitmap*		newBm = mBitmap;
	status_t		err = mRecorder->Draw(&newBm);
	if (newBm != mBitmap) TakeBitmap(newBm);
	if (err == B_OK) this->Invalidate();
}
