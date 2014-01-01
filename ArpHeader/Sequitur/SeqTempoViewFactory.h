/* SeqTempoViewFactory.h
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 10.09.00		hackborn
 * Created this file
 */

#ifndef SEQUITUR_SEQTEMPOVIEWFACTORY_H
#define SEQUITUR_SEQTEMPOVIEWFACTORY_H

#include "AmPublic/AmViewFactory.h"

/*************************************************************************
 * SEQ-TEMPO-VIEW-FACTORY
 *************************************************************************/
class SeqTempoViewFactory : public AmViewFactory
{
public:
	SeqTempoViewFactory();

	virtual const char* EventMetaType() const		{ return "MIDI Tempo"; }

	virtual status_t	GetPhraseRendererInfo(	uint32 index,
												BString& outLabel,
												BString& outKey) const;
	virtual status_t	DataNameAt(uint32 index, TrackViewType type, BString& outName) const;

	virtual AmPhraseRendererI*	NewPhraseRenderer(	const AmTimeConverter& mtc,
													const AmViewPropertyI& property) const;
	virtual BView*		NewDataView(AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI* property,
									TrackViewType type);

	virtual BView*		NewInfoView(AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI* property,
									TrackViewType type);

	virtual BView*		NewPrefView(BRect f, BMessage* prefs,
									const BString& key);
};

#endif 
