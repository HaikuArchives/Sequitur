#include <cstdio>
#include <cstring>
#include <interface/Window.h>
#include <midi2/MidiProducer.h>
#include <midi2/MidiRoster.h>
#include <StorageKit.h>
#include <support/Autolock.h>
#include <ArpCore/StlVector.h>
#include "GlPublic/GlImage.h"
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlParamWrap.h"
#include "GlPublic/GlStrainedParamList.h"
#include "Glasslike/GlMidi.h"
#include "Glasslike/GlDefs.h"

/*************************************************************************
 * GL-EVENT-CONSUMER
 * A local consumer for producer I receive events from.
 *************************************************************************/
class GlEventConsumer : public BMidiLocalConsumer
{
public:
	GlEventConsumer(GlMidi& target, int32 portId)
			: mTarget(target), mPortId(portId)			{ }
	virtual ~GlEventConsumer()							{ }

	virtual	void		ControlChange(	uchar channel, uchar number, 
										uchar value, 
										bigtime_t time)
	{
		GlMidiEvent		event(	mPortId, GlMidiEvent::CONTROL_CHANGE,
								channel, number, value);
		mTarget.ReceiveMidi(event, time);
	}

	virtual	void		SystemExclusive(void* data, size_t dataLength, 
										bigtime_t time)
	{
		if (dataLength == 4 && ((uint8*)data)[0] == 0x7f && ((uint8*)data)[2] == 6) {
			GlMidiEvent		event(	mPortId, GlMidiEvent::MMC,
									-1, ((uint8*)data)[1], ((uint8*)data)[3]);
			mTarget.ReceiveMidi(event, time);
			return;
		}
		/* Ya know, I have NO CLUE who is responsible for deallocating
		 * that data passed in.  I assume the caller owns it, but then
		 * why isn't it const?
		 */
		BMidiLocalConsumer::SystemExclusive(data, dataLength, time);
	}

private:
	GlMidi&				mTarget;
	int32				mPortId;
};

/*************************************************************************
 * GL-MIDI-PRODUCER
 * A local consumer for each producer whose events I receive.
 *************************************************************************/
class GlMidiProducer
{
public:
	GlMidiProducer(BMidiProducer* p, GlMidi& target)
		: mProducer(p), mConsumer(0)
	{
		ArpVALIDATE(mProducer, return);
		if (mProducer->Acquire() != B_OK) return;
		mConsumer = new GlEventConsumer(target, mProducer->ID());
		if (mConsumer) {
			mConsumer->Acquire();
			mProducer->Connect(mConsumer);
		}
	}

	~GlMidiProducer()
	{
		ArpVALIDATE(mProducer, return);
		if (mConsumer) {
			mProducer->Disconnect(mConsumer);
			mConsumer->Release();
		}
		mProducer->Release();
	}

private:
	BMidiProducer*		mProducer;
	GlEventConsumer*	mConsumer;
};

/*************************************************************************
 * GL-MIDI-PRODUCER-LIST
 * A list of all MIDI producers in the system
 *************************************************************************/
class GlMidiProducerList
{
public:
	GlMidiProducerList(GlMidi& target);
	~GlMidiProducerList();

private:
	std::vector<GlMidiProducer*>		mProducers;
};

/***************************************************************************
 * GL-MIDI
 ***************************************************************************/
GlMidi::GlMidi()
		: mList(0)
{
	mList = new GlMidiProducerList(*this);
}

GlMidi::~GlMidi()
{
	{
		BAutolock		l(mAccess);
		delete mList;
	}
}

status_t GlMidi::ReceiveMidi(GlMidiEvent event, bigtime_t time)
{
	BMessage		eventMsg(GL_MIDI_EVENT_MSG);
	if (event.WriteTo(eventMsg) != B_OK) return B_ERROR;

	BAutolock		l(mAccess);
	if (!mTarget.IsValid()) return B_OK;
	return mTarget.SendMessage(&eventMsg);
}

void GlMidi::SetTarget(const BMessenger& target)
{
	BAutolock		l(mAccess);
	mTarget = target;
//	if (mTarget.IsValid()) mTarget->Activate();
}

void GlMidi::UnsetTarget(const BMessenger& target)
{
	BAutolock		l(mAccess);
	if (mTarget == target) {
//		mTarget->Unset();
		mTarget = BMessenger();
	}
}

// #pragma mark -

/***************************************************************************
 * GL-MIDI-PRODUCER-LIST
 ***************************************************************************/
GlMidiProducerList::GlMidiProducerList(GlMidi& target)
{
	int32					id = 0;
	BMidiProducer*			prod;
	while ((prod = BMidiRoster::NextProducer(&id)) != NULL) {
		if (prod->IsValid() && prod->Name()) {
			printf("Midi producer %s\n", prod->Name());
			GlMidiProducer*	ep = new GlMidiProducer(prod, target);
			if (ep) mProducers.push_back(ep);
		}
	}
}

GlMidiProducerList::~GlMidiProducerList()
{
	for (uint32 k = 0; k < mProducers.size(); k++) {
		delete mProducers[k];
	}
	mProducers.resize(0);
}

