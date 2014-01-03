/* MSpace.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 04.14.00		hackborn
 * Created this file.
 */

#ifndef MDUST_MSPACE_H
#define MDUST_MSPACE_H

#include <midi2/MidiConsumer.h>
#include <midi2/MidiProducer.h>
#include "MDustPublic/MSpaceI.h"

/***************************************************************************
 * M-SPACE
 * This defines one space, which is a collection of particles.
 ***************************************************************************/
class MSpace : public MSpaceI
{
public:
	MSpace(int32 xLimit, int32 yLimit, int32 zLimit);
	virtual ~MSpace();

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	virtual MParticleI* ParticleAt(uint32 index);
	void AddParticle(MParticleI* particle);
	void AddDust(MParticleI* particle);
	
	virtual void Increment(bool print);
	virtual void Perform(const ArpMidiEventI* event, uchar channel);
	virtual void Perform(MSequence* sequence, uchar channel);

	void Perform(ArpMidiT tick);

	virtual void Draw(BGLView* onView);
	
private:
	/* The bounding box of this space.
	 */
	int32				mXLimit, mYLimit, mZLimit;
	/* My collection of particles.  This will definitely change
	 * to something more 'sequenceable,' like a doubly-linked list.
	 * I suspect it will end up being fairly specialized.
	 */
	particle_vec		mParticles;
	/* These particles are applied after all the mParticles have
	 * modified each other, and generated and processed their events.
	 */
	particle_vec		mDust;
	/* The current tick I'm at in the playback, measured by the
	 * current PPQN.
	 */
	ArpMidiT			mTick;
	/* Right now I just have a couple hardcoded sequences.  Eventually
	 * I'll have one for every channel.
	 */
	MSequence			mChannel1, mChannel2;

	BMidiLocalProducer	*mProducer;
	BMidiConsumer		*mConsumer;

	void DrawBounds(BGLView* onView);
	void SetConsumer(const char* name);
	BMidiConsumer* ConsumerNamed(const char *name) const;
	void DeleteParticles();
	void DeleteDust();
};

#endif
