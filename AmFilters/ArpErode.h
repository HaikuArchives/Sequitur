/* ErodeFilter.h
 * Copyright (c)2000 by Daniel 'The Place Where it All Falls Apart' Civello.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Daniel Civello,
 * at <civello@pacbell.net>.
 *
 * History
 * ~~~~~~~
 * 07.04.01		civello
 * Recompiled for Sequitur 2.0
 *
 * 10.04.00		civello 
 * PinkNoise implementation
 *
 * 06.16.00		hackborn
 * Created this file
 */
 
#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#include <Message.h>
#include <String.h>
#include <View.h>

class ErodeFilterAddOn;

class ErodeFilter : public AmFilterI
{
public:

	ErodeFilter(	ErodeFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* config);

	virtual ~ErodeFilter();

	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);	
	
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/)	{ return B_OK; }

private:
	ErodeFilterAddOn*	mAddOn;
	AmFilterHolderI*	mHolder;
};

class ErodeFilterAddOn : public AmFilterAddOn
{
public:
	ErodeFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Erode"; }
	virtual BString		Key() const						{ return "Erode"; }
	virtual BString		ShortDescription() const		{ return "Successive notes placed at half duration."; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Daniel Civello"; }
	virtual BString		Email() const					{ return "civello@pacbell.net"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = NULL)
			{ return new ErodeFilter(this, holder, config); }
};
