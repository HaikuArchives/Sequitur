/* MSpace.cpp
 */
#include <cstdio>
#include <cassert>
#include <cstring>
#include <midi2/MidiRoster.h>
#include <ArpKernel/ArpDebug.h>
#include "ArpMidi2/ArpMidiEvents.h"
#include "MDust/MSpace.h"
#include "MDust/MParticle.h"

//static const char* PORT_NAME = "/dev/midi/sonic_vibes/1";
static const char* PORT_NAME = "/dev/midi/mo/BETA-9911170003/00";

/***************************************************************************
 * M-SPACE
 ***************************************************************************/
MSpace::MSpace(int32 xLimit, int32 yLimit, int32 zLimit)
		: mXLimit(xLimit), mYLimit(yLimit), mZLimit(zLimit),
		mTick(0), mProducer(0), mConsumer(0)
{
	SetConsumer(PORT_NAME);
}

MSpace::~MSpace()
{
	DeleteParticles();
	DeleteDust();
	if ( (mProducer != 0) && (mConsumer != 0) ) {
		for (uint32 k=0; k<=127; k++) mProducer->SprayNoteOff( 0, k, 0 );
		for (uint32 k=0; k<=127; k++) mProducer->SprayNoteOff( 1, k, 0 );
		mProducer->Disconnect(mConsumer);
		mProducer->Release();
		mConsumer->Release();
	}
}

MParticleI* MSpace::ParticleAt(uint32 index)
{
	if (index >= mParticles.size()) return 0;
	return mParticles[index];
}

void MSpace::AddParticle(MParticleI* particle)
{
	assert(particle != 0);
	mParticles.push_back(particle);
}

void MSpace::AddDust(MParticleI* particle)
{
	assert(particle != 0);
	mDust.push_back(particle);
}

void MSpace::Increment(bool print)
{
if (print) {
	printf("**** Calculate velocity and speed\n");
}


	// Calculate the new position of all the particles
	for (uint32 k=0; k<mParticles.size(); k++)
		mParticles[k]->Cycle(*this);
	// Generate the MIDI data.
	for (uint32 k=0; k<mParticles.size(); k++)
		mParticles[k]->Generate(mTick);
	for (uint32 k=0; k<mParticles.size(); k++)
		mParticles[k]->Process(mTick, *this);
	for (uint32 k=0; k<mDust.size(); k++)
		mDust[k]->Process(mTick, *this);
	// Install the MIDI data into this space.
	for (uint32 k=0; k<mParticles.size(); k++)
		mParticles[k]->Perform(*this);
	// Perform the MIDI data.
	Perform(mTick);

	mTick++;	
}

void MSpace::Perform(const ArpMidiEventI* event, uchar channel)
{
	if (event == 0) return;
	if (mProducer == 0) return;

	if (event->Type() == event->NOTEON_EVENT) {
		ArpMidiNoteOn*	no = (ArpMidiNoteOn*)event;
		mProducer->SprayNoteOn( channel, no->Note(), no->Velocity() );
	} else if (event->Type() == event->NOTEOFF_EVENT) {
		ArpMidiNoteOff*	no = (ArpMidiNoteOff*)event;
		mProducer->SprayNoteOff( channel, no->Note(), no->Velocity() );
	} else if (event->Type() == event->CONTROLCHANGE_EVENT) {
		ArpMidiControlChange*	cc = (ArpMidiControlChange*)event;
		mProducer->SprayControlChange( channel, cc->ControlNumber(), cc->ControlValue() );
	}
}

void MSpace::Perform(MSequence* sequence, uchar channel)
{
	assert(sequence != 0);
	if (channel == 0) mChannel1.Merge(sequence);
	else if (channel == 1) mChannel2.Merge(sequence);
}

void MSpace::Perform(ArpMidiT tick)
{
	ArpMidiEvent*	event;
	while (mChannel1.RemoveAt(tick, &event) == B_OK) {
		Perform(event, 0);
		event->Delete();
	}
	while (mChannel2.RemoveAt(tick, &event) == B_OK) {
		Perform(event, 1);
		event->Delete();
	}
}

void MSpace::Draw(BGLView* onView)
{
	DrawBounds(onView);
	for (uint32 k=0; k<mParticles.size(); k++) {
		mParticles[k]->Draw(onView, mXLimit, mYLimit, mZLimit);
	}
}

void MSpace::DrawBounds(BGLView* onView)
{
    glBegin(GL_LINE_LOOP);
	glColor4f(1, 0, 0, 1);
	glVertex3f(-1, 1, 1);
	glVertex3f(-1, -1, 1);
	glVertex3f(1, -1, 1);
	glVertex3f(1, 1, 1);
	glEnd();
	
    glBegin(GL_LINE_LOOP);
	glVertex3f(-1, 1, -1);
	glVertex3f(-1, -1, -1);
	glVertex3f(1, -1, -1);
	glVertex3f(1, 1, -1);
	glEnd();

    glBegin(GL_LINES);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, -1, 1);
	glVertex3f(-1, 1, -1);
	glVertex3f(-1, 1, 1);
	glVertex3f(1, -1, -1);
	glVertex3f(1, -1, 1);
	glVertex3f(1, 1, -1);
	glVertex3f(1, 1, 1);
	glEnd();
}

void MSpace::SetConsumer(const char* name)
{
	mConsumer = ConsumerNamed(name);
	if (mConsumer == 0) return;
	if (mConsumer->Acquire() != B_OK) {
		DB(DBALL, std::cerr << "MSpace::MSpace() couldn't acquire the consumer." << std::endl);
		mConsumer->Release();
		mConsumer = 0;
		return;
	}

	mProducer = new BMidiLocalProducer();
	if (mProducer == 0) {
		mConsumer->Release();
		mConsumer = 0;
		return;
	}
	mProducer->Connect(mConsumer);
}

BMidiConsumer* MSpace::ConsumerNamed(const char *name) const
{
	BMidiConsumer*	cons;
	for (int32 k = 0; (cons = BMidiRoster::NextConsumer(&k)) != 0; k++) {
		if (strcmp(cons->Name(), name) == 0) return cons;
	}
	return 0;
}

void MSpace::DeleteParticles()
{
	while (mParticles.size() > 0) {
		particle_vec::iterator		i = mParticles.begin();
		delete (*i);
		mParticles.erase(i);
	}
}

void MSpace::DeleteDust()
{
	while (mDust.size() > 0) {
		particle_vec::iterator		i = mDust.begin();
		delete (*i);
		mDust.erase(i);
	}
}
