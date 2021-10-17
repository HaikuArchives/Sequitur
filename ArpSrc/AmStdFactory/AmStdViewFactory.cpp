#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "AmStdFactory/AmBankChangeView.h"
#include "AmStdFactory/AmChannelPressureView.h"
#include "AmStdFactory/AmCommandView.h"
#include "AmStdFactory/AmControlChangeView.h"
#include "AmStdFactory/AmHybridView.h"
#include "AmStdFactory/AmNoteRenderer.h"
#include "AmStdFactory/AmPianoRollView.h"
#include "AmStdFactory/AmPitchBendView.h"
#include "AmStdFactory/AmProgramChangeView.h"
#include "AmStdFactory/AmStdViewFactory.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"
#include "AmStdFactory/AmQuantizeView.h"
#include "AmStdFactory/AmVelocityView.h"

static const char*	DEFAULT_NOTE		= "Default Note";

static const char*	PIANO_ROLL			= "Piano Roll";
static const char*	HYBRID_NOTE			= "Hybrid Note";
static const char*	BANK_CHANGE			= "Bank / Program Change";
static const char*	COMMAND				= "Command";
static const char*	CONTROL_CHANGE		= "Control Change";
static const char*	PITCH_BEND			= "Pitch Bend";
static const char*	PROGRAM_CHANGE		= "Program Change";
static const char*	CHANNEL_AFTERTOUCH	= "Channel Aftertouch";
static const char*	VELOCITY			= "Velocity";
//static const char*	QUANTIZE			= "Quantize";

/*************************************************************************
 * AM-STD-VIEW-FACTORY
 *************************************************************************/
AmStdViewFactory::AmStdViewFactory()
{
	SetSignature(DEFAULT_FACTORY_SIGNATURE);
}

status_t AmStdViewFactory::GetPhraseRendererInfo(	uint32 index,
													BString& outLabel,
													BString& outKey) const
{
	if (index == 0) {
		outLabel = DEFAULT_NOTE;
		outKey = DEFAULT_NOTE;
		return B_OK;
	}
	return B_ERROR;
}

status_t AmStdViewFactory::DataNameAt(uint32 index, TrackViewType type, BString& outName) const
{
	if (type == PRI_VIEW) {
		if (index == 0) outName << PIANO_ROLL;
		else if (index == 1) outName << HYBRID_NOTE;
		else return B_ERROR;
		return B_OK;
	} else if (type == SEC_VIEW) {
		if (index == 0) outName << BANK_CHANGE;
		else if (index == 1) outName << CHANNEL_AFTERTOUCH;
		else if (index == 2) outName << COMMAND;
		else if (index == 3) outName << CONTROL_CHANGE;
		else if (index == 4) outName << PITCH_BEND;
		else if (index == 5) outName << PROGRAM_CHANGE;
		else if (index == 6) outName << VELOCITY;
//		else if (index == 7) outName << QUANTIZE;
		else return B_ERROR;
		return B_OK;
	} else {
		return B_ERROR;
	}
}

AmPhraseRendererI* AmStdViewFactory::NewPhraseRenderer(	const AmTimeConverter& mtc,
														const AmViewPropertyI& property) const
{
	const BString&		nameStr = property.Name();
	if (nameStr == DEFAULT_NOTE)
		return new AmNoteRenderer(mtc);
	return NULL;
}

BView* AmStdViewFactory::NewDataView(	AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI* property,
										TrackViewType type)
{
	if (!property) return NULL;
	BString			nameStr = property->Name();
	if (nameStr.Length() < 1) return NULL;

	if (nameStr == PIANO_ROLL)
		return new AmPianoRollDataView(		BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == HYBRID_NOTE)
		return new AmHybridDataView(		BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == BANK_CHANGE)
		return new AmBankChangeDataView(	BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == CHANNEL_AFTERTOUCH)
		return new AmChannelPressureDataView(BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == COMMAND)
		return new AmCommandDataView(		BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == CONTROL_CHANGE)
		return new AmControlChangeDataView(	BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == PITCH_BEND)
		return new AmPitchBendDataView(		BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	if (nameStr == PROGRAM_CHANGE)
		return new AmProgramChangeDataView(	BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
#if 0
	if (nameStr == QUANTIZE)
		return new AmQuantizeDataView(		BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
#endif
	if (nameStr == VELOCITY)
		return new AmVelocityDataView(		BRect(0,0,0,0), songRef,
											trackWinProps,
											*property, type);
	return NULL;
}

BView* AmStdViewFactory::NewInfoView(	AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI* property,
										TrackViewType type)
{
	if (!property) return 0;
	const BString&		nameStr = property->Name();
	if ( (nameStr.String() == 0) || (nameStr.Length() == 0) )
		return 0;

	if (nameStr == PIANO_ROLL)
		return new AmPianoRollInfoView(		BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == HYBRID_NOTE)
		return new AmHybridInfoView(		BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == BANK_CHANGE)
		return new AmBankChangeInfoView(	BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == CHANNEL_AFTERTOUCH)
		return new AmChannelPressureInfoView(BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == COMMAND)
		return new AmCommandInfoView(		BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == CONTROL_CHANGE)
		return new AmControlChangeInfoView(	BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == PITCH_BEND)
		return new AmPitchBendInfoView(		BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	if (nameStr == PROGRAM_CHANGE)
		return new AmProgramChangeInfoView(	BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
#if 0
	if (nameStr == QUANTIZE)
		return new AmQuantizeInfoView(		BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
#endif
	if (nameStr == VELOCITY)
		return new AmVelocityInfoView(		BRect(0, 0, 0, 0),
											songRef,
											trackWinProps,
											property,
											type );
	return 0;
}

BView* AmStdViewFactory::NewPrefView(	BRect f, BMessage* prefs,
										const BString& key)
{
	if (key == CONTROL_CHANGE)
		return new AmControlChangePrefView(f, prefs, Signature(), key);

	AmStdFactoryPrefView*	v = new AmStdFactoryPrefView(f, prefs, Signature(), key);
	if (v) v->AddViews();
	return v;
}
