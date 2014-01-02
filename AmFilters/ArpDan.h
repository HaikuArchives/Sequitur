/* AmDanFilter.h
 * Copyright (c)2000 by Daniel 'The Place Where it All Falls Apart' Civello.
 * All rights reserved.
 *
 * This is a demonstration filter, liberally commented so even the most
 * dimwitted individual who holds an advanced degree in hypergraphic computer
 * morphology can understand it.
 *
 * To write a succesful filter, you need to implement two classes:
 * 		1.  The main filter class is a subclass of AmFilterI.  This is the class
 * responsible for receiving MIDI events and responding with new ones.
 *		2.  The filter addon class, a subclass of AmFilterAddOn.  This is mostly
 * a housekeeping class, responsible for generating new instances of the filter
 * and storing meta information, like the filter's name.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Daniel Civello,
 * at <civello@angryredplanet.com>.
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
 * 06.16.00		hackborn
 * Created this file
 */

#include <app/Message.h>
#include <interface/View.h>
#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 * ARP-DAN-FILTER
 * This is a filter to demonstrate writing a filter.  Don't let the large
 * number of methods scare you -- almost all of them are empty in the actual
 * implementation.  The important method is HandleEvent(), which does the
 * filtering.
 *****************************************************************************/
class ArpDanFilterAddOn;

class ArpDanFilter : public AmFilterI
{
public:
	/* The constructor.  Most filters will follow this basic pattern -- they
	 * are supplied a pointer to the addon that created them to get access to
	 * some of the meta information.  They are supplied a pointer to the holder
	 * that contains them to get access to the next filter in the pipeline.
	 * The BMessage provides optional configuration data.  If the filter doesn't
	 * have any configuration, it can ignore this parameter.
	 */
	ArpDanFilter(	ArpDanFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage* config);
	/* The destructor.
	 */
	virtual ~ArpDanFilter();
	
	/* This is the main method of this class.  Given the supplied event, answer
	 * the result of filtering it.
	 */
	virtual AmEvent* HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	/* This is optional.  Ignore it for now.
	 */
	virtual BView* NewEditView(BPoint requestedSize) const;
	
	/* The following methods are used when a filter has properties that
	 * can be set.  Ignore them for now.
	 */
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& /*panels*/)	{ return B_OK; }

private:
	/* A pointer to the addon on that created me, supplied in the constructor.
	 */
	ArpDanFilterAddOn*	mAddOn;
	/* A pointer to the holder that contains me, supplied in the constructor.
	 */
	AmFilterHolderI*	mHolder;
};

/*****************************************************************************
 * ARP-DAN-FILTER-ADD-ON
 * This is the addon, which is primarily responsible for answering new
 * instances of the filter.
 *****************************************************************************/
class ArpDanFilterAddOn : public AmFilterAddOn
{
public:
	/* Your constructor must pass this system context to the base class.
	 */
	ArpDanFilterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
	{
	}
	
	virtual VersionType Version(void) const				{ return VERSION_CURRENT; }
	/* Answer a user-friendly name for the filter.  This is the name that the
	 * user sees in the filter list window.
	 */
	virtual BString		Name() const					{ return "Dan"; }
	/* Answer a unique name for the filter.  This name is used by the file IO
	 * to uniquely identify the filter.  The key should be of the format
	 *		[company or individual ID]:[filter name]
	 * where [company or individual ID] is assigned from a controlled list maintained
	 * by Angry Red Planet.  The [filter name] is assigned by the company
	 * or individual, who is responsible for making sure they have no other filters
	 * with the same name.
	 */
	virtual BString		Key() const						{ return "arp:Dan"; }
	/* Answer a user-friendly short description of the filter.  This is the
	 * description the user sees in the filter list.
	 */
	virtual BString		ShortDescription() const		{ return "Code example"; }
	/* Call my parent's method, then append to the string an HTML-formatted
	 * description of the filter.  This description is uses in the filter
	 * documentation that gets generated whenever Sequitur is launched.
	 */
	virtual void		LongDescription(BString& name, BString& str) const;
	virtual BString		Author() const					{ return "Dan Civello"; }
	virtual BString		Email() const					{ return "civello@pacbell.net"; }
	/* Supply a version number to be displayed to the user.  If the major
	 * is supplies as 1 and the minor as 0, the user sees 'v1.0'.
	 */
	virtual void		GetVersion(int32* major, int32* minor) const;
	/* Answer with the type of filter this is.  Most are THROUGH_FILTER.
	 * If you produce events (i.e., sit at the front of the input pipeline)
	 * then answer SOURCE_FILTER.  If you consume events (u.e., sit at the
	 * end of the output pipeline) then answer DESTINATION_FILTER.
	 */
	virtual type Type() const							{ return THROUGH_FILTER; }
	/* Answer an image for the filter.  The image is stored in the accompanying
	 * ArpDan.rsrc file.  Use QuickRes, available from Be, to edit it.
	 */
	virtual BBitmap* Image(BPoint requestedSize) const;
	/* This is the important method.  Answer a new instance of the filter.
	 */
	virtual AmFilterI* NewInstance(	AmFilterHolderI* holder,
									const BMessage* config = 0);
};
