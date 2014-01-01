/* AmViewProperty.h
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
#ifndef AMKERNEL_AMVIEWPROPERTY_H
#define AMKERNEL_AMVIEWPROPERTY_H

#include "AmPublic/AmViewPropertyI.h"

/***************************************************************************
 * AM-VIEW-PROPERTY
 * An implementation of the AmViewPropertyI interface.
 ***************************************************************************/
class AmViewProperty : public AmViewPropertyI
{
public:
	AmViewProperty( const char* signature = 0, const char* name = 0 );
	AmViewProperty( const AmViewPropertyI& property );
	/* The BMessage is intended to be something that resulted from
	 * my WriteTo() call, below.
	 */
	AmViewProperty( const BMessage* config );	 
	virtual ~AmViewProperty();

	AmViewProperty&			operator=(const AmViewPropertyI& from);

	virtual const BString&	Signature() const;
	virtual void			SetSignature(const BString& str);
	virtual const BString&	Name() const;
	virtual void			SetName(const BString& str);
	virtual const BMessage*	Configuration() const;
	virtual void			SetConfiguration(const BMessage* msg);
	/* Persistence information:  These methods write and read my entire
	 * state (which means my configuration message along with my
	 * signature and name).
	 */
	status_t				WriteTo(BMessage* config) const;
	status_t				ReadFrom(const BMessage* config);
	
private:
	BString		mSignature;
	BString		mName;
	BMessage*	mConfiguration;
};

#endif
