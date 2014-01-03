#include <support/Autolock.h>
#include <ArpCore/ArpDebug.h>
#include <GlPublic/GlRecorder.h>

//static uint32	gCount = 0;

/***************************************************************************
 * GL-RECORDER
 ****************************************************************************/
GlRecorder::GlRecorder()
		: mStep(0.0), mRefs(0)
{
//	gCount++;
//	printf("Recorder() %ld\n", gCount);
}

GlRecorder::~GlRecorder()
{
//	gCount--;
//	printf("~Recorder() %ld\n", gCount);
}

void GlRecorder::IncRefs()
{
	atomic_add(&mRefs, 1);
}

void GlRecorder::DecRefs()
{
	int32		last = atomic_add(&mRefs, -1);
	if (last <= 1) delete this;
}

void GlRecorder::Init()
{
	mStep = 0.0;
}

void GlRecorder::SetState(GlControlState& s) const
{
}

int32 GlRecorder::ViewIndex() const
{
	return 0;
}

void GlRecorder::LockedSetStep(float step)
{
	mStep = step;
}
