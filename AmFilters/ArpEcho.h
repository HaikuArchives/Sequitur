/* ArpEcho.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * An echo filter.
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

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

/*****************************************************************************
 *
 *	ARP-ECHO-FILTER and ARP-ECHO-FILTER-ADDON CLASS
 *
 *	These classes are a simple filter to produce an echo for note events.
 *	The only event it processes is NOTEON; events of all
 *	other types are passed through untouched.  When it gets a NOTEON
 *	event, it echoes it based on the depth setting.  This was written so that
 *	there are actually TWO different non-output filters to work with.
 *
 *****************************************************************************/

class ArpEchoFilterAddOn;

class ArpEchoFilter : public AmFilterI {
public:
	ArpEchoFilter(ArpEchoFilterAddOn* addon,
					 AmFilterHolderI* holder,
					 const BMessage* settings);
	~ArpEchoFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* HandleToolEvent(	AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	
	// The ArpConfigurableI implementation.
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

	enum {
		DURATION_MODE			= 1,
		MOUSE_DISTANCE_MODE		= 2,
		MOUSE_LOCATION_MODE		= 3
	};
	enum {
		DESCENDING_VELOCITY				= 1,
		ASCENDING_VELOCITY				= 2,
		EVEN_VELOCITY					= 3
	};
	enum {
		EXPONENT_CURVE					= 1,
		GEOMETRIC_CURVE					= 2,
		SINUSOIDAL_CURVE				= 3,
		SINUSOIDAL_DECAY_CURVE			= 4,
		RANDOM_CURVE					= 5,
		RANDOM_DECAY_CURVE				= 6
	};
	
	enum {
		MY_GRID					= 1,
		TOOL_GRID				= 2
	};

private:
	ArpEchoFilterAddOn*			mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mMode;
	int32						mDepth;
	int32						mVelocityChange;
	int32						mCurve;
	float						mStart;
	float						mStop;
	float						mExponent;
	float						mPeriod;
	int32						mGridChoice;
	/* For MY_GRID mode.
	 */
	int32						mMultiplier;
	AmTime						mQuantize;
	int32						mEighths;

	uint8						VelocityCurveAt(uint8 base, int32 depth, int32 which);
	
	void						PerformDurationMode(AmEvent* event, AmTime grid);
	AmEvent*					PerformMouseDistanceMode(	AmNoteOn* orig,
															const am_tool_filter_params* toolParams,
															AmTime grid);
	AmEvent*					PerformMouseLocationMode(	AmNoteOn* orig,
															const am_tool_filter_params* toolParams,
															AmTime grid);
};

class ArpEchoFilterAddOn : public AmFilterAddOn {
public:
	ArpEchoFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Echo"; }
	virtual BString		Key() const						{ return "arp:Echo"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dianne and Eric Hackborn"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(AmFilterHolderI* holder,
										const BMessage* config = 0)
		{ return new ArpEchoFilter(this, holder, config); }
};
