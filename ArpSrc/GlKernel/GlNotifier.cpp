#include <be/support/Autolock.h>
#include <ArpCore/ArpDebug.h>
#include <ArpCore/StlVector.h>
#include <ArpSupport/ArpSafeDelivery.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlNotifier.h>

// _GL-OBSERVER
class _GlObserver
{
public:
	BMessenger			target;
	uint32				code;

	_GlObserver(const BMessenger& m, uint32 code);

	void				ReportChange(BMessage* msg, BMessenger sender);
};

// _GL-NOTIFIER-DATA
class _GlNotifierData
{
public:
	_GlNotifierData();
	~_GlNotifierData();
	
	_GlObserver*			ObserverFor(const BMessenger& m);
	status_t				AddObserver(_GlObserver* obs);
	status_t				RemoveCode(const BMessenger& m, uint32 code);

	status_t				ReportChange(uint32 code, BMessage* msg, BMessenger sender);

private:
	vector<_GlObserver*>	mObservers;
};

/***************************************************************************
 * GL-NOTIFIER
 ****************************************************************************/
GlNotifier::GlNotifier()
		: mData(new _GlNotifierData()), mSender(0)
{
}

GlNotifier::~GlNotifier()
{
	delete mData;
}

status_t GlNotifier::AddObserver(uint32 code, const BMessenger& m)
{
	ArpVALIDATE(mData, return B_ERROR);
	BAutolock		l(&mNotifierLock);
	_GlObserver*	obs = mData->ObserverFor(m);
	if (!obs) {
		obs = new _GlObserver(m, code);
		if (!obs) return B_NO_MEMORY;
		return mData->AddObserver(obs);
	}
	ArpASSERT(obs);
	obs->code |= code;
	return B_OK;
}

status_t GlNotifier::RemoveObserver(uint32 code, const BMessenger& m)
{
	ArpVALIDATE(mData, return B_ERROR);
	BAutolock		l(&mNotifierLock);
	return mData->RemoveCode(m, code);
}

void GlNotifier::RemoveAll()
{
	delete mData;
	mData = new _GlNotifierData();
}

void GlNotifier::SetSender(void* sender)
{
	mSender = sender;
}

void GlNotifier::ReportMsgChange(uint32 code, BMessage* msg, BMessenger sender)
{
	ArpVALIDATE(mData && msg, return);
	ArpASSERT(msg->what == CHANGE_MSG);
	if (mSender) msg->AddPointer(GL_SENDER_STR, mSender);
	
	BAutolock		l(&mNotifierLock);
	mData->ReportChange(code, msg, sender);
}

// #pragma mark -

/***************************************************************************
 * GL-OBSERVER
 ****************************************************************************/
_GlObserver::_GlObserver(const BMessenger& m, uint32 c)
		: target(m), code(c)
{
}

void _GlObserver::ReportChange(BMessage* msg, BMessenger sender)
{
	ArpASSERT(msg);
	if (target.IsValid() && (target != sender))
		SafeSendMessage(target, msg);
}

// #pragma mark -

/***************************************************************************
 * _GL-NOTIFIER-DATA
 ****************************************************************************/
_GlNotifierData::_GlNotifierData()
{
}

_GlNotifierData::~_GlNotifierData()
{
	for (uint32 k = 0; k < mObservers.size(); k++) delete mObservers[k];
	mObservers.resize(0);
}
	
_GlObserver* _GlNotifierData::ObserverFor(const BMessenger& m)
{
	for (uint32 k = 0; k < mObservers.size(); k++) {
		if (mObservers[k] && mObservers[k]->target == m) return mObservers[k];
	}
	return 0;
}

status_t _GlNotifierData::AddObserver(_GlObserver* obs)
{
	ArpVALIDATE(obs, return B_ERROR);
	ArpASSERT(ObserverFor(obs->target) == 0);
	mObservers.push_back(obs);
	return B_OK;
}

status_t _GlNotifierData::RemoveCode(const BMessenger& m, uint32 code)
{
	_GlObserver*	obs = ObserverFor(m);
	if (!obs) return B_BAD_INDEX;
	obs->code &= ~code;
	if (obs->code != 0) return B_OK;
	for (uint32 k = 0; k < mObservers.size(); k++) {
		if (mObservers[k]->target == m) {
			delete mObservers[k];
			mObservers.erase(mObservers.begin() + k);
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t _GlNotifierData::ReportChange(uint32 code, BMessage* msg, BMessenger sender)
{
	for (uint32 k = 0; k < mObservers.size(); k++) {
		if (mObservers[k] && (mObservers[k]->code&code)) {
			mObservers[k]->ReportChange(msg, sender);
		}
	}
	return B_OK;
}

