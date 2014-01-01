/* AmViewFactory.h
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
 * 10.28.98		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMVIEWFACTORY_H
#define AMPUBLIC_AMVIEWFACTORY_H

#include <be/interface/View.h>
#include <be/support/List.h>
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmViewPropertyI.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
#include "AmPublic/AmTimeConverter.h"
class AmPhraseRendererI;
class AmSelectionsI;
class AmTrackWinPropertiesI;

#define SZ_FACTORY_SIGNATURE	"factory signature"
#define SZ_FACTORY_VIEW_NAME	"factory view name"

extern const char*		AM_HEIGHT_PREF_STR;
#define					AM_MIN_FAC_VIEW_HEIGHT		(10)

enum {
	AM_STOP_MSG		= 'aStp'
};

/***************************************************************************
 * AM-VIEW-FACTORY
 * This object acts as an interface between the window classes and the
 * views for displaying and editing events.  Currently, it is only responsible
 * for generating new views of various types.  In the future, it will be
 * responsible for handling specific pieces of communication that occur
 * between the Sequitur window classes and the views it generates.
 ***************************************************************************/
#define STR_AMVIEWFACTORY		"AmViewFactory"

class AmViewFactory
{
public:
	AmViewFactory();
	virtual ~AmViewFactory()					{ }

	/* Subclasses can respond with a string that represents the type of data
	 * they can handle.  The currently defined types are:
	 *		"MIDI"
	 *		"Audio"
	 */
	virtual const char* EventMetaType() const = 0;
		
	BString&			Signature()							{ return mSignature; }

	virtual status_t	GetPhraseRendererInfo(	uint32 index,
												BString& outLabel,
												BString& outKey) const = 0;
	virtual status_t	DataNameAt(uint32 index, TrackViewType type, BString& outName) const = 0;
	/* Subclasses can override this to set the preferred data name to be
	 * any of the strings supplied in DataNameAt().  By default, the priamry
	 * data name at 0 is answered.
	 */
	virtual BString		PreferredDataName() const;

	virtual AmPhraseRendererI*	NewPhraseRenderer(	const AmTimeConverter& mtc,
													const AmViewPropertyI& property) const = 0;
	virtual BView*	NewDataView(	AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI* property,
									TrackViewType type) = 0;

	virtual BView*	NewInfoView(	AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI* property,
									TrackViewType type) = 0;

	virtual BView*	NewPrefView(	BRect f, BMessage* prefs,
									const BString& key) = 0;

protected:
	void SetSignature(const char *signature)		{ mSignature = signature; }

private:
	BString		mSignature;
};

/******************************************************************
 * AM-TRACK-WIN-PROPERTIES-I
 * Various properties that are supplied to views created by the
 * view factory.
 ******************************************************************/
class AmTrackWinPropertiesI
{
public:
	/* Answer the number of ordered tracks I have.  Since there
	 * must always be a primary track, this should always be 1 or
	 * greater.
	 */
	virtual uint32			CountOrderedTracks() const = 0;
	virtual AmTrackRef		OrderedTrackAt(uint32 index) const = 0;
	
	virtual const AmTimeConverter& TimeConverter() const = 0;
	/* Answer a selections object, or 0 if there isn't one.  Never
	 * delete this object -- if you want to clear the selections,
	 * send in 0 to SetSelections().
	 */
	virtual AmSelectionsI*	Selections() const = 0;
	/* Whatever list is sent into this method becomes the property
	 * of this class -- don't delete it, that will be handled
	 * automatically.
	 */
	virtual void			SetSelections(AmSelectionsI* selections) = 0;
	/* Answer the current grid value to quantize to.
	 */
	virtual AmTime			GridTime() const = 0;
	virtual void			GetSplitGridTime(int32* m, AmTime* v, int32* d) const = 0;
	virtual uint8			Velocity() const = 0;
	/* All tracks other than the primary can be displayed with
	 * some level of transparency -- this determines the level, from
	 * 0 (not displayed) to 1 (displayed completely opaque).  Shadow
	 * tracks are all tracks that are not currently ordered.
	 */
	virtual float			OrderedSaturation() const = 0;
	virtual float			ShadowSaturation() const = 0;

	/* Info and data views can communicate with each other through this
	 * mechanism.
	 */
	virtual void			PostMessageToDataView(BMessage& msg, view_id fromView) = 0;
	virtual void			PostMessageToInfoView(BMessage& msg, view_id fromView) = 0;
};

/******************************************************************
 * AM-FACTORY-MESSAGE-WRAPPER
 * Yuck!  This hack class is to make it easier to access the
 * factory prefs inside the prefs message.  Used to just be in
 * the interface but it's bubbled down here with the addition
 * of factories creating pref views.
 ******************************************************************/
class AmFactoryMessageWrapper
{
public:
	AmFactoryMessageWrapper(BMessage* msg);


	status_t	SetInt32Preference(	const char* fac, const char* view,
									const char* name, int32 i32, int32 n = 0);
	status_t	GetInt32Preference(	const char* fac, const char* view,
									const char* name, int32* outI32, int32 n = 0) const;

private:
	BMessage*		mMsg;
};

#endif 
