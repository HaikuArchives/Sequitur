#include <be/support/Autolock.h>
#include <ArpCore/ArpDebug.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlProcessStatus.h>

class _GlProcessPartition
{
public:
	int32		segments, cur;
	float		complete;
	float		left, right;
	
	_GlProcessPartition(int32 s = 1) : segments(s), cur(0), complete(0), left(0), right(1) { }
};

static void _get_range(	float parentL, float parentR,
						int32 segments, int32 cur,
						float* outL, float* outR);

static void _tab_over(uint32 size)	{ for (uint32 k = 0; k < size; k++) printf("\t"); }

/***************************************************************************
 * GL-PROCESS-STATUS
 ***************************************************************************/
status_t GlProcessStatus::Init()
{
	BAutolock l(&mLock);

	mLastPix = 0;	
	FreeParititonStack();
	_GlProcessPartition*	p = new _GlProcessPartition();
	if (!p) return B_NO_MEMORY;
	mPartitions.push_back(p);
	mCanceled = false;
	return B_OK;
}

status_t GlProcessStatus::PushPartition(int32 segments)
{
	ArpVALIDATE(segments > 0, return B_ERROR);
	BAutolock l(&mLock);
	if (mCanceled) return B_CANCELED;
//printf("Push %ld partitions\n", segments);
//_tab_over(mPartitions.size()); printf("PUSH PARTITION\n");
	_GlProcessPartition*	lastP = 0;
	if (mPartitions.size() > 0) lastP = mPartitions[mPartitions.size() - 1];
	
	_GlProcessPartition*	newP = new _GlProcessPartition(segments);
	if (!newP) return B_NO_MEMORY;
	mPartitions.push_back(newP);

	if (lastP) {
//_tab_over(mPartitions.size()); printf("\tparent (%f - %f)\n", lastP->left, lastP->right);
		_get_range( lastP->left, lastP->right, segments, 0,
					&(newP->left), &(newP->right) );
	}
//_tab_over(mPartitions.size()); printf("PushPartition segments %ld (%f - %f)\n", newP->segments, newP->left, newP->right);

	return B_OK;
}

status_t GlProcessStatus::IncSegment()
{
	BAutolock l(&mLock);
	if (mCanceled) return B_CANCELED;
	
	ArpVALIDATE(mPartitions.size() > 0, return B_ERROR);
	_GlProcessPartition*		p = mPartitions[mPartitions.size() - 1];
	p->cur++;
	if (mPartitions.size() > 1) {
		_GlProcessPartition*	lastP = mPartitions[mPartitions.size() - 2];
		_get_range( lastP->left, lastP->right, p->segments, p->cur,
					&(p->left), &(p->right) );
	}
//_tab_over(mPartitions.size()); printf("INC %ld / %ld (%f - %f)\n", p->cur, p->segments, p->left, p->right);
	SetPixel(p->left);

	ArpASSERT(p->cur <= p->segments);
	return B_OK;
}

status_t GlProcessStatus::SetComplete(float percent)
{
	BAutolock l(&mLock);
	if (mCanceled) return B_CANCELED;
	
	ArpVALIDATE(mPartitions.size() > 0, return B_ERROR);
	mPartitions[mPartitions.size() - 1]->complete = percent;
	return B_OK;
}

status_t GlProcessStatus::PopPartition()
{
	BAutolock l(&mLock);
	if (mCanceled) return B_CANCELED;
//printf("Pop partition\n");
	
	ArpVALIDATE(mPartitions.size() > 0, return B_ERROR);
	delete mPartitions[mPartitions.size() - 1];
	mPartitions.pop_back();
//_tab_over(mPartitions.size()); printf("POP PARTITION\n");
	return B_OK;
}

bool GlProcessStatus::Canceled()
{
	BAutolock l(&mLock);
	return mCanceled;
}

void GlProcessStatus::SetCanceled()
{
	BAutolock l(&mLock);

	mCanceled = true;
}

GlProcessStatus::GlProcessStatus()
		: mCode(0), mProgressWidth(0), mLastPix(0), mCanceled(false)
{
}

GlProcessStatus::~GlProcessStatus()
{
	FreeParititonStack();
}

void GlProcessStatus::SetTarget(BMessenger target, int32 code)
{
	mTarget = target;
	mCode = code;
}

void GlProcessStatus::SetProgressWidth(int32 pixels)
{
	mProgressWidth = pixels;
}

void GlProcessStatus::SetPixel(float percent)
{
	int32			pix = int32(percent * mProgressWidth);
//_tab_over(mPartitions.size()); printf("PIX %ld (percent %f)\n", pix, percent);
	if (pix <= mLastPix) return;
	mLastPix = pix;

	if (mTarget.IsValid()) {
		BMessage	msg(GL_STATUS_MSG);
		msg.AddFloat("f", percent);
		msg.AddInt32("c", mCode);
		mTarget.SendMessage(&msg);
	}
}

void GlProcessStatus::FreeParititonStack()
{
	for (uint32 k = 0; k < mPartitions.size(); k++) delete mPartitions[k];
	mPartitions.resize(0);
}

void GlProcessStatus::Print() const
{
	printf("GlProcessStatus %ld partitions\n", mPartitions.size());
	for (uint32 k = 0; k < mPartitions.size(); k++) {
		printf("%ld: %ld / %ld (%f - %f)\n", k, mPartitions[k]->cur,
				mPartitions[k]->segments, mPartitions[k]->left, mPartitions[k]->right);
	}
}

/***************************************************************************
 * Misc
 ***************************************************************************/
static void _get_range(	float parentL, float parentR,
						int32 segments, int32 cur,
						float* outL, float* outR)
{
	ArpASSERT(outL && outR);
	*outL = parentL + (((parentR - parentL) / segments) * cur);
	*outR = parentL + (((parentR - parentL) / segments) * (cur + 1));
}
