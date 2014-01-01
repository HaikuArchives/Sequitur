/* ArpVaccine.h
 * Copyright (c)2002 by Eric Hackborn
 * All rights reserved.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.07.30			hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>
class AmMotionI;
class ArpVaccineAddOn;

/*****************************************************************************
 * ARP-VACCINE-FILTER
 * Inject a motion into the received events.
 *****************************************************************************/
class ArpVaccineFilter : public AmFilterI
{
public:
	ArpVaccineFilter(	ArpVaccineAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	virtual ~ArpVaccineFilter();
	
	virtual AmEvent*	HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* 	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);
	virtual AmEvent*	HandleBatchToolEvents(	AmEvent* event,
												const am_filter_params* params = NULL,
												const am_tool_filter_params* toolParams = NULL,
												const AmEvent* lookahead = NULL);

	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	virtual void		Start(uint32 context);

	enum {
		FOLLOW_Y_FLAG				= 0x00000010,
		FOLLOW_X_FLAG				= 0x00000020,
		INVERT_X_FLAG				= 0x00000040,

		MOTION_FROM_TRACK_FLAG		= 0x00001000
	};
	
private:
	ArpVaccineAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;
	int32					mSeed;
	AmMotionI*				mMotion;
	uint32					mChangeFlags;
	int32					mNoteFlags;
	int32					mFrequency;
	float					mAmount;
	
	void					VaccinateBatchEvents(	AmEvent* event,
													const am_filter_params* params,
													AmMotionChange* curMotion,
													const am_tool_filter_params* toolParams);
	void					VaccinateControlChange(AmEvent* event, float amount, float yAmount);
	void					VaccinatePitchBend(AmEvent* event, float amount, float yAmount);
	void					VaccinateNoteOn(AmEvent* event, float amount, float yAmount);
	void					VaccinateNoteOff(AmEvent* event, float amount, float yAmount);
	void					VaccinateTempoChange(AmEvent* event, float amount, float yAmount);
	void					VaccinateChannelPressure(AmEvent* event, float amount, float yAmount);

	bool					CanVaccinate(AmEvent::EventType et) const;
	bool					ShouldVaccinate() const;
	void					InitMotion();
};

/*****************************************************************************
 * ARP-VACCINE-ADD-ON
 *****************************************************************************/
class ArpVaccineAddOn : public AmFilterAddOn
{
public:
	ArpVaccineAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Vaccine"; }
	virtual BString		Key() const						{ return "arp:Vaccine"; }
	virtual BString		ShortDescription() const		{ return "Inject a motion into all valid events I receive"; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpVaccineFilter(this, holder, config); }
};
