/* AmFilter.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * This class represents a class that events can be sprayed to (it will probably
 * end up being a subclass of some sort of AmSteam object or something).
 * Subclasses can modify the data they receive in any way they like.
  *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.07.98		hackborn
 * Created this file
 */

#ifndef AMKERNEL_AMSTDFILTERS_H
#define AMKERNEL_AMSTDFILTERS_H

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#ifndef AMKERNEL_AMDEVICE_H
#include <AmKernel/AmDevice.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

#define	PRODUCER_CLASS_NAME_STR		"arp:MidiProducer"
#define	CONSUMER_CLASS_NAME_STR		"arp:MidiConsumer"

/*****************************************************************************
 *
 *	AM-PRODUCER-FILTER and AM-PRODUCER-FILTER-ADDON CLASS
 *
 *	This filter generates events received from its installed BMidiProducer
 *	object.  You must supplied the name of the desired MidiProducer when
 *	instantiating the addon.
 *
 *****************************************************************************/

class BMidiLocalConsumer;
class BMidiProducer;
class AmProducerFilterAddOn;
class AmEventProducer;

class AmProducerFilter : public AmFilterI
{
public:
	AmProducerFilter(	AmProducerFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config );
	~AmProducerFilter();
	
	virtual BString		Label() const;

	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual status_t	GetNextConfiguration(BMessage* config) const;
	virtual status_t	GetToolTipText(BString* out) const;
	
	virtual ArpCRef<AmDeviceI> Device() const;
	virtual void		DeviceChanged();
	virtual int32		HintChannel() const;
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual void		MouseAction(BRect frame, BPoint where,
									uint32 buttons, bool released);
	/* This filter returns an image that reflects what its current
	 * channel is.
	 */
	virtual BBitmap*	Image(BPoint requestedSize) const;
		
	void				SetChannel(int32 channel);
	
	AmTime				MakeTime(bigtime_t sysTime) const;
	void				GenerateEvents(AmEvent* chain);

	void				HandleMMC(uint8 device, uint8 command);
	
private:
	mutable BLocker				mAccess;
	AmProducerFilterAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mChannel;
	int32						mTypeMask;
	// The consumer is taken from the roster based on the consumer name
	// supplied to this class.  The producer is given any MIDI events
	// that I receive, and it sprays them to the consumer.	
	AmEventProducer*			mConsumer;
	BMidiProducer*				mProducer;

	mutable BBitmap*			mImageBitmap;	// Cache bitmap for image.

	AmFilterHolderI*			mInputHolder;
	AmFilterI*					mInputFilter;
	void						CacheFilters();
	void						UncacheFilters();
};

class AmProducerFilterAddOn : public AmFilterAddOn
{
public:
	// A NULL consumer creates a midi synth consumer.
	AmProducerFilterAddOn(const void* cookie,
						  BMidiProducer* consumer=NULL,
						  int32 producerIndex = 0);		// Index is something of an added hack for
						  								// when multiple producers have the same name
						  								// -- it gives each a unique name
	virtual ~AmProducerFilterAddOn();
	
	virtual VersionType Version(void) const;
	virtual BString		Name() const;
	virtual status_t	GetLabel(	BString& outLabel, bool useName = true,
									BMessage* archivedFilter = NULL) const;
	virtual BString		Key() const;
	virtual BString		ShortDescription() const;
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const;
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const;
	virtual BString		KeyForType(type inType) const;
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual float		CheckInstantiation(const BMessage* config) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
	
	BMidiProducer* Producer() const;
	int32 ProducerID() const;
	
private:
	mutable BMidiProducer*	mProducer;
	int32					mProducerIndex;
	BBitmap*				mIcon;
};

/*****************************************************************************
 *
 *	AM-CONSUMER-FILTER and AM-CONSUMER-FILTER-ADDON CLASS
 *
 *	This filter sends its events to its installed BMidiConsumer object.  You
 *  must supply the name of the desired MidiConsumer when instantiating the
 *	addon.  By default, the addon is initialized with the BeOS MIDI synth
 *	as its consumer.
 *
 *	FIX:  Right now, the BMidiSynth filter is hacked into this one.  Pretty
 *	much all of this filter needs to be abstracted so I can weed out the
 *	minor differences.
 *
 *****************************************************************************/

class BMidiLocalProducer;
class BMidiConsumer;
class AmConsumerFilterAddOn;

#define		BE_MIDI_SYNTH_STR		"Be MIDI Synth"

class AmConsumerFilter : public AmFilterI
{
public:
	AmConsumerFilter(	AmConsumerFilterAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config );
	~AmConsumerFilter();

	virtual BString		Label() const;

	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual status_t	GetNextConfiguration(BMessage* config) const;
	virtual status_t	GetToolTipText(BString* out) const;

	virtual ArpCRef<AmDeviceI> Device() const;
	virtual int32		HintChannel() const;
	virtual	void		Stop(uint32 context);
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual void		MouseAction(BRect frame, BPoint where,
									uint32 buttons, bool released);
	/* This filter returns an image that reflects what its current
	 * channel is.
	 */
	virtual BBitmap*	Image(BPoint requestedSize) const;
		
	void SetChannel(int32 channel);
	void SetDevice(ArpCRef<AmDeviceI> device);
	
private:
	mutable BLocker				mAccess;
	AmConsumerFilterAddOn*		mAddOn;
	AmFilterHolderI*			mHolder;
	ArpCRef<AmDeviceI>			mDevice;	// Current instr descr
	int32						mChannel;
	int32						mTypeMask;
	/* While playing, capture all notes that turn on so I can be sure they're off.
	 */
	uint8						mNoteOns[128];
	int32						mNotesOffCc;
	// The consumer is taken from the roster based on the consumer name
	// supplied to this class.  The producer is given any MIDI events
	// that I receive, and it sprays them to the consumer.	
	BMidiLocalProducer*			mProducer;
	BMidiConsumer*				mConsumer;

	mutable BBitmap*			mImageBitmap;	// Cache bitmap for image.
};

class AmConsumerFilterAddOn : public AmFilterAddOn
{
public:
	// A NULL consumer creates a midi synth consumer.
	AmConsumerFilterAddOn(const void* cookie,
						  BMidiConsumer* consumer = NULL,
						  int32 consumerIndex = 0);		// Index is something of an added hack for
						  								// when multiple consumers have the same name
						  								// -- it gives each a unique name
	virtual ~AmConsumerFilterAddOn();
	
	virtual VersionType Version(void) const;
	virtual BString		Name() const;
	virtual status_t	GetLabel(	BString& outLabel, bool useName = true,
									BMessage* archivedFilter = NULL) const;
	virtual BString		Key() const;
	virtual BString		ShortDescription() const;
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const;
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const;
	virtual BString		KeyForType(type inType) const;
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual float		CheckInstantiation(const BMessage* config) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
	virtual void		Clock(AmTime time);
	
	BMidiConsumer* Consumer() const;
	int32 ConsumerID() const;
	
	void InstanceGone();
	void SetClockEnabled(bool state);
	bool ClockEnabled() const;
	
private:
	mutable BLocker			mAccess;
	BMidiLocalProducer*		mProducer;
	mutable BMidiConsumer*	mConsumer;
	int32					mConsumerIndex;
	BBitmap*				mIcon;
	int32					mInstanceCount;
	bool					mClockEnabled;
};

/*****************************************************************************
 * AM-NULL-INPUT-FILTER
 *****************************************************************************/
class AmNullInputAddOn;

class AmNullInputFilter : public AmFilterI
{
public:
	AmNullInputFilter(	AmNullInputAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config );
	~AmNullInputFilter();

	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
private:
	AmNullInputAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;
};

/*****************************************************************************
 * AM-NULL-INPUT-ADD-ON
 *****************************************************************************/
class AmNullInputAddOn : public AmFilterAddOn
{
public:
	AmNullInputAddOn(const void* cookie);
	virtual ~AmNullInputAddOn();
	
	virtual VersionType Version(void) const;
	virtual BString		Name() const;
	virtual BString		Key() const;
	virtual BString		ShortDescription() const;
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const;
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const;
	virtual subtype		Subtype() const;
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
};

/*****************************************************************************
 * AM-NULL-OUTPUT-FILTER
 *****************************************************************************/
class AmNullOutputAddOn;

class AmNullOutputFilter : public AmFilterI
{
public:
	AmNullOutputFilter(	AmNullOutputAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* config );
	~AmNullOutputFilter();

	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	
private:
	AmNullOutputAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;
};

/*****************************************************************************
 * AM-NULL-OUTPUT-ADD-ON
 *****************************************************************************/
class AmNullOutputAddOn : public AmFilterAddOn
{
public:
	AmNullOutputAddOn(const void* cookie);
	virtual ~AmNullOutputAddOn();
	
	virtual VersionType Version(void) const;
	virtual BString		Name() const;
	virtual BString		Key() const;
	virtual BString		ShortDescription() const;
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const;
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type		Type() const;
	virtual subtype		Subtype() const;
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0);
};

#endif
