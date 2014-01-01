/* AmViewPropertyI.h
 * Copyright (c)2000 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.14.00		hackborn
 * Created this file
 */
#ifndef AMPUBLIC_AMVIEWPROPERTYI_H
#define AMPUBLIC_AMVIEWPROPERTYI_H

#include <vector.h>
#include <be/app/Message.h>
#include <be/support/String.h>
#include <ArpBuild.h>

/***************************************************************************
 * AM-VIEW-PROPERTY-I
 * This interface represents an identifier to a single type of view.  The
 * information supplied here is enough for the Sequitur engine to track
 * down and instatiate a new view.
 ***************************************************************************/
class AmViewPropertyI
{
public:
	virtual ~AmViewPropertyI()				{ }

	/* The signature is a controlled-name path to the view in question.
	 * The format used is "company_name/category", where company name is
	 * a controlled name supplied by Angry Red Planet, and category is
	 * internal to the company in question.  All default views are accessible
	 * with the pattern "Arp/Default".
	 */
	virtual const BString&	Signature() const = 0;
	virtual void			SetSignature(const BString& str) = 0;
	/* The name identifies which view in the signature to use.
	 */
	virtual const BString&	Name() const = 0;
	virtual void			SetName(const BString& str) = 0;
	/* The configuration data will vary based on each type of view available.
	 * This information is generated only by the view itself, and sent to
	 * any new instances of it.
	 */
	virtual const BMessage*	Configuration() const = 0;
	virtual void			SetConfiguration(const BMessage* msg) = 0;	
};

#endif
