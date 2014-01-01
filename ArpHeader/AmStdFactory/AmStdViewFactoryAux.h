/* AmStdViewFactoryAux.h
 * Copyright (c)1998-2000 by Eric Hackborn.
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
 * 2001.05.26		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMSTDFACTORY_AMSTDVIEWFACTORYAUX_H
#define AMSTDFACTORY_AMSTDVIEWFACTORYAUX_H

#include <interface/View.h>
#include "AmPublic/AmViewFactory.h"
#include "AmPublic/AmBackgrounds.h"
//class AmTrackWinPropertiesI;
class ArpIntControl;

/*************************************************************************
 * AM-GRID-BACKGROUND
 *************************************************************************/
class AmPropGridBackground : public AmGridBackground
{
public:
	AmPropGridBackground(AmTrackWinPropertiesI& trackWinProps);
	
protected:
	virtual AmTime					GridTime() const;
	virtual const AmTimeConverter&	TimeConverter() const;

private:
	AmTrackWinPropertiesI&	mProps;
};

/*************************************************************************
 * AM-STD-FACTORY-PREF-VIEW
 *************************************************************************/
class AmStdFactoryPrefView : public BView
{
public:
	AmStdFactoryPrefView(	BRect f, BMessage* prefs,
							const BString& facSig,
							const BString& facKey);

	virtual	void			AttachedToWindow();
	virtual void			MessageReceived(BMessage* msg);

	virtual void			AddViews();

protected:
	BString					mFacSig, mFacKey;
	AmFactoryMessageWrapper	mPrefs;

	float					AddY() const;
	
	void					AddHeightView(BPoint where);
	
private:
	typedef BView			inherited;
	ArpIntControl*			mHeightCtrl;
};

#endif 
