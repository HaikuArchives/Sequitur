/* ArpTilTheEnd.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.07.03			hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>
class ArpTilTheEndAddOn;

/*****************************************************************************
 * ARP-TIL-THE-END-FILTER
 * Fill events until the end of the measure.
 *****************************************************************************/
class ArpTilTheEndFilter : public AmFilterI
{
public:
	ArpTilTheEndFilter(	ArpTilTheEndAddOn* addon,
						AmFilterHolderI* holder,
						const BMessage* settings);
	~ArpTilTheEndFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent* HandleToolEvent(	AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/);

	enum {
		FILL_MEASURES_MODE		= 1,
		COPY_MEASURES_MODE		= 2
	};

	enum {
		MY_GRID					= 1,
		TOOL_GRID				= 2
	};

private:
	ArpTilTheEndAddOn*			mAddOn;
	AmFilterHolderI*			mHolder;
	int32						mMode;
	int32						mDepth;
	int32						mGridChoice;
	/* For MY_GRID mode.
	 */
	int32						mMultiplier;
	AmTime						mQuantize;
	int32						mEighths;

	void						PerformFillMeasuresMode(AmEvent* event,
														const am_filter_params* params,
														AmTime grid);
	void						PerformCopyMeasuresMode(AmEvent* event,
														const am_filter_params* params);
};

/*****************************************************************************
 * ARP-TIL-THE-END-ADD-OM
 *****************************************************************************/
class ArpTilTheEndAddOn : public AmFilterAddOn
{
public:
	ArpTilTheEndAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Til The End"; }
	virtual BString		Key() const						{ return "arp:TilTheEnd"; }
	virtual int32		MaxConnections() const			{ return 2; }
	virtual BString		ShortDescription() const		{ return 0; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Eric Hackborn"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual BString		Email() const					{ return "hackborn@angryredplanet.com"; }
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap*	Image(BPoint requestedSize) const;
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = 0)
		{ return new ArpTilTheEndFilter(this, holder, config); }
};
