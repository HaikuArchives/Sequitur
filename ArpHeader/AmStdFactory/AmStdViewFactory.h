/* AmStdViewFactory.h
 * Copyright (c)1998-2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This class defines a group of views that can extend the Sequitur system.
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
 * 11.04.98		hackborn
 * Created this file
 */

#ifndef AMSTDFACTORY_AMSTDVIEWFACTORY_H
#define AMSTDFACTORY_AMSTDVIEWFACTORY_H

#include "AmPublic/AmViewFactory.h"

/***************************************************************************
 * AM-STD-VIEW-FACTORY
 ***************************************************************************/
class AmStdViewFactory : public AmViewFactory
{
public:
	AmStdViewFactory();

	virtual const char* EventMetaType() const		{ return "MIDI"; }

	virtual status_t	GetPhraseRendererInfo(	uint32 index,
												BString& outLabel,
												BString& outKey) const;
	virtual status_t	DataNameAt(uint32 index, TrackViewType type, BString& outName) const;

	virtual AmPhraseRendererI*	NewPhraseRenderer(	const AmTimeConverter& mtc,
													const AmViewPropertyI& property) const;
	virtual BView*				NewDataView(	AmSongRef songRef,
												AmTrackWinPropertiesI& trackWinProps,
												const AmViewPropertyI* property,
												TrackViewType type);

	virtual BView*				NewInfoView(	AmSongRef songRef,
												AmTrackWinPropertiesI& trackWinProps,
												const AmViewPropertyI* property,
												TrackViewType type);

	virtual BView*				NewPrefView(	BRect f, BMessage* prefs,
												const BString& key);
};

#endif 
