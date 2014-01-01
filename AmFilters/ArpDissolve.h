/* Dissolve.h
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

class DissolveFilterAddOn;

class DissolveFilter : public AmFilterI
{
public:

	DissolveFilter(	DissolveFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* config);

	virtual ~DissolveFilter();
	
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);

	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/)	{ return B_OK; }

private:
	DissolveFilterAddOn*	mAddOn;
	AmFilterHolderI*	mHolder;
};

class DissolveFilterAddOn : public AmFilterAddOn
{
public:
	DissolveFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	virtual BString		Name() const					{ return "Dissolve"; }
	virtual BString		Key() const						{ return "Dissolve"; }
	virtual BString		ShortDescription() const		{ return "1 a: to become dissipated or decomposed.  1 b: to fade away."; }
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Daniel Civello"; }
	virtual BString		Email() const					{ return "civello@pacbell.net"; }
	virtual void		GetVersion(int32* major, int32* minor) const;
	virtual type Type() const							{ return THROUGH_FILTER; }
	virtual BBitmap* Image(BPoint requestedSize) const;
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = NULL)
			{ return new DissolveFilter(this, holder, config); }
};
