/* AmViewProperty.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include "AmKernel/AmViewProperty.h"

static const char*	V_SIG_STR		= "signature";
static const char*	V_NAME_STR		= "name";
static const char*	V_CONFIG_STR	= "config";

/***************************************************************************
 * AM-VIEW-PROPERTY
 ***************************************************************************/
AmViewProperty::AmViewProperty(const char* signature, const char* name)
		: mConfiguration(0)
{
	if ( signature != 0 ) mSignature = signature;
	if ( name != 0 ) mName = name;
}

AmViewProperty::AmViewProperty( const AmViewPropertyI& property )
		: mConfiguration(0)
{
	*this = property;
}

AmViewProperty::AmViewProperty( const BMessage* config )
		: mConfiguration(0)
{
	if( config ) ReadFrom(config);
}

AmViewProperty::~AmViewProperty()
{
	delete mConfiguration;
}

AmViewProperty& AmViewProperty::operator=(const AmViewPropertyI &from)
{
	SetSignature( from.Signature() );
	SetName( from.Name() );
	SetConfiguration( from.Configuration() );
	return *this;
}

const BString& AmViewProperty::Signature() const
{
	return mSignature;
}

void AmViewProperty::SetSignature(const BString& str)
{
	mSignature = str;
}

const BString& AmViewProperty::Name() const
{
	return mName;
}

void AmViewProperty::SetName(const BString& str)
{
	mName = str;
}

const BMessage* AmViewProperty::Configuration() const
{
	return mConfiguration;
}

void AmViewProperty::SetConfiguration(const BMessage* msg)
{
	delete mConfiguration;
	mConfiguration = 0;
	if ( msg != 0 ) mConfiguration = new BMessage(*msg);
}

status_t AmViewProperty::WriteTo(BMessage* config) const
{
	if( !config ) return B_ERROR;
	status_t	err;
	if( (err=config->AddString( V_SIG_STR, mSignature.String() )) != B_OK ) return err;
	if( (err=config->AddString( V_NAME_STR, mName.String() )) != B_OK ) return err;
	if( mConfiguration ) config->AddMessage(V_CONFIG_STR, mConfiguration);
	return B_OK;
}

status_t AmViewProperty::ReadFrom(const BMessage* config)
{
	if( !config ) return B_ERROR;
	SetConfiguration(0);
	const char*		str;
	BMessage		msg;
	if( config->FindString(V_SIG_STR, &str) == B_OK ) {
		/* FIX:  Dianne is using a different key style than
		 * I was, so I'm standardizing on hers.  Fortunately this isn't
		 * public, so I can cleanup behind the scenes.  This goes out
		 * with the 1.1 release, so it can be taken out after that.
		 */
		if ( str && (strcmp(str, "Arp/Default") == 0) ) str = "arp:Default";
		mSignature = str;
	}
	if( config->FindString(V_NAME_STR, &str) == B_OK ) mName = str;
	if( config->FindMessage(V_CONFIG_STR, &msg) == B_OK ) SetConfiguration(&msg);
	return B_OK;
}
