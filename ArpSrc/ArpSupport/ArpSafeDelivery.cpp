#include <ArpKernel/ArpDebug.h>
#include <ArpSupport/ArpSafeDelivery.h>

#include <kernel/OS.h>
#include <support/Autolock.h>
#include <app/Looper.h>
#include <app/MessageRunner.h>

//#include <list.h>

#include <list>
//using namespace std;		// The right way
using std::list;			// The Microsoft way

//ArpMOD();

// This is derived form BWindow so that it will get quit when
// the application it shutting down.  Bleah.
class ArpSafeDeliveryLooper : public BLooper
{
private:
	typedef BLooper inherited;
	
public:
	ArpSafeDeliveryLooper(ArpSafeDelivery* owner)
		: BLooper("Safe Message Delivery"),
		  mOwner(owner), mPulse(NULL)
	{
	}
	
	virtual ~ArpSafeDeliveryLooper()
	{
		mOwner->LooperShuttingDown();
		delete mPulse;
		mPulse = NULL;
	}
	
	void AddMessage(const BMessenger& target, const BMessage* msg)
	{
		BAutolock l(this);
		if (mWaiting.empty()) StartPulse();
		std::list<target_entry>::iterator i = mWaiting.begin();
		while (i != mWaiting.end()) {
			if (i->target == target) break;
			i++;
		}
		if (i == mWaiting.end()) {
// FIX: Compiler doesn't like this form, which I assume is equivalent
// to the following
//			mWaiting.push_back();
			mWaiting.push_back(target_entry());
			i = mWaiting.end();
			i--;
			i->target = target;
		}
// FIX: Compiler doesn't like this form, which I assume is equivalent
// to the following
//		i->messages.push_back();
		i->messages.push_back(message_entry());
		message_entry& me = i->messages.back();
		me.message = *msg;
	}
	
	virtual void MessageReceived(BMessage *msg)
	{
		if (msg->what == 'puls') {
			std::list<target_entry>::iterator i = mWaiting.begin();
			while (i != mWaiting.end()) {
				std::list<message_entry>::iterator j = i->messages.begin();
				while (j != i->messages.end()) {
					status_t err = i->target.SendMessage(&(j->message), (BHandler*)NULL, 1);
					if (err != B_WOULD_BLOCK && err != B_TIMED_OUT && err != B_INTERRUPTED) {
						if (err != B_OK) printf("*** Error sending message: %s\n", strerror(err));
						j = i->messages.erase(j);
					} else {
						break;
					}
				}
				if (i->messages.empty()) {
					i = mWaiting.erase(i);
				} else {
					i++;
				}
			}
			if (!mWaiting.empty()) StartPulse();
			else {
				delete mPulse;
				mPulse = NULL;
			}
		} else {
			inherited::MessageReceived(msg);
		}
	}
	
private:
	struct message_entry {
		BMessage				message;
	};
	struct target_entry {
		BMessenger				target;
		std::list<message_entry>		messages;
	};
	
	void StartPulse()
	{
		if (mPulse) {
			delete mPulse;
			mPulse = NULL;
		}
		mPulse = new BMessageRunner(BMessenger(this), new BMessage('puls'),
									50000, 1);
	}
	
	ArpSafeDelivery* mOwner;
	BMessageRunner* mPulse;
	std::list<target_entry> mWaiting;
};

ArpSafeDelivery::ArpSafeDelivery()
	: mAllocated(0), mLooper(NULL)
{
}

ArpSafeDelivery::~ArpSafeDelivery()
{
	if (mLooper && mLooper->Lock()) mLooper->Quit();
	
}

static ArpSafeDelivery gSafeDelivery;

ArpSafeDelivery* ArpSafeDelivery::Default()
{
	return &gSafeDelivery;
}
	
status_t ArpSafeDelivery::SendMessage(const BMessenger& target, BMessage* msg)
{
	status_t err = target.SendMessage(msg, (BHandler*)NULL, 1);
	if (err != B_WOULD_BLOCK && err != B_TIMED_OUT && err != B_INTERRUPTED) {
		if (err != B_OK) fprintf(stderr, "*** Delivery error: %s\n", strerror(err));
		return err;
	}
	
	uint32 alloc = atomic_or(&mAllocated, 1);
	if (!alloc) {
		ArpSafeDeliveryLooper* loop = new ArpSafeDeliveryLooper(this);
		loop->Run();
		mLooper = loop;
	} else {
		while (!mLooper) snooze(10000);
	}
	
	mLooper->AddMessage(target, msg);
	return B_OK;
}

void ArpSafeDelivery::LooperShuttingDown()
{
	mLooper = NULL;
}

status_t SafeSendMessage(const BMessenger& target, BMessage* msg)
{
	return ArpSafeDelivery::Default()->SendMessage(target, msg);
}

