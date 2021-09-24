#include <cstdio>
#include <typeinfo>
#include <vector>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmSweep.h"

static std::vector<int16>		gKeys;

/***************************************************************************
 * AM-SWEEP
 ****************************************************************************/
AmSweep::~AmSweep()
{
	delete mNext;
}

int16 AmSweep::Key() const
{
	return mKey;
}

float AmSweep::At(float v) const
{
	float			answer = Process(v);
	if (!mNext) return answer;
	return answer * mNext->At(v);
}

status_t AmSweep::SetNext(AmSweep* next)
{
	ArpASSERT(!mNext);
	if (mNext) delete mNext;
	mNext = next;
	return B_OK;
}

bool AmSweep::IsRandom() const
{
	const AmSweep*	c = this;
	while (c) {
		if (c->is_random()) return true;
		c = c->mNext;
	}
	return false;
}

void AmSweep::SetTargetSize(uint32 size)
{
	AmSweep*	c = this;
	while (c) {
		c->set_target_size(size);
		c = c->mNext;
	}
}

void AmSweep::HandleEvent(AmEvent* event)
{
}

AmSweep::AmSweep(int16 key)
		: mKey(key), mNext(0)
{
}

AmSweep::AmSweep(const AmSweep& o)
		: mKey(o.mKey), mNext(0)
{
	if (o.mNext) mNext = o.mNext->Clone();
}

bool AmSweep::is_random() const
{
	return false;
}

void AmSweep::set_target_size(uint32 size)
{
}

bool AmSweep::RegisterKey(int16 key)
{
	for (uint32 k = 0; k < gKeys.size(); k++) {
		if (key == gKeys[k]) return false;
	}
	gKeys.push_back(key);
	return true;
}

void AmSweep::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("AmSweep type: %s key: %d\n", typeid(*this).name(), mKey);
}
