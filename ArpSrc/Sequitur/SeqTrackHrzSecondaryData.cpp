/* SeqTrackHrzSecondaryData.cpp
 */
#include <stdio.h>
#include <be/interface/MenuItem.h>
#include <be/interface/StringView.h>
#include <be/interface/Window.h>
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "Sequitur/SeqNoView.h"
#include "Sequitur/SeqTrackHrzSecondaryData.h"

/*************************************************************************
 * SEQ-TRACK-HRZ-SECONDARY-DATA
 * Manages all the auxiliary (ie non-note) data editors that a track can
 * have - not just control change editors, but also program changes,
 * bank select, and whatever else we want to throw in there.
 *************************************************************************/
SeqTrackHrzSecondaryData::SeqTrackHrzSecondaryData(	AmSongRef songRef,
													AmTrackWinPropertiesI& trackWinProps,
													BRect rect,
													float separation)
		: ArpHrzViewManager(rect, "SeqTrackHrzSecondaryData",
							B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM),
		  AmSongObserver(songRef),
		  mTrackWinProps(trackWinProps)
{
	SetSeparation(separation);
	SetDefaultViewHeight(20);
	InitializeViews();
}

SeqTrackHrzSecondaryData::~SeqTrackHrzSecondaryData()
{
}

void SeqTrackHrzSecondaryData::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor( Prefs().Color( AM_DATA_BACKDROP_C ) );
}

void SeqTrackHrzSecondaryData::StoreViewProperties(AmTrack* track)
{
	assert( track );
	uint32		oldCount = track->CountProperties( SEC_VIEW );
	for( uint32 k = 0; k < oldCount; k++ ) track->SetProperty( 0, SEC_VIEW );

	AmTrackDataView*	view;
	AmViewProperty		prop;
	uint32				index = 0;
	for( int32 k = 0; (view = dynamic_cast<AmTrackDataView*>( ItemAt(k) )); k++ ) {
		BString			n = view->FactorySignature();
		prop.SetSignature(n);
		n = view->ViewName();
		prop.SetName(n);
		prop.SetConfiguration( view->ConfigurationData() );
		track->SetProperty(&prop, SEC_VIEW, index);
		index++;
	}
}

void SeqTrackHrzSecondaryData::InitializeViews()
{
	// READ SONG BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackWinProps.OrderedTrackAt(0) ) : NULL;
	if (track) {
		const AmViewPropertyI*	prop;	
		for (uint32 k=0; (prop = track->Property(SEC_VIEW, k)) != 0; k++) {
			BView*				view = NewSecondaryView(prop);
			if (view) AddMiniView(view);
		}
	}
	WriteUnlock(song);
	// END READ SONG BLOCK
}

BView* SeqTrackHrzSecondaryData::NewSecondaryView(const AmViewPropertyI* prop)
{
	assert( prop );
	BString		sigStr = prop->Signature();
	BString		nameStr = prop->Name();
	BString		problem;
	
	// If there was no string, just return a default overview.
	if ( (sigStr.String() == 0) || (sigStr.Length() == 0) )
		return NoSecondaryView("No View Factory was specified");

	// If the factory doesn't exist, return an overview describing the problem.
	AmViewFactory*	factory = AmGlobals().FactoryNamed( sigStr );
	if( !factory ) {
		problem << "No View Factory named " << sigStr << " is available";
		return NoSecondaryView( problem.String() );
	}

	// If the factory exists, but no overview was specified, return an overview
	// describing the problem.
	if ( (nameStr.String() == 0) || (nameStr.Length() == 0) ) {
		problem << "No Overview for View Factory " << sigStr << " was specified";
		return NoSecondaryView( problem.String() );
	}
	
	// Here's our success condition -- the factory exists, it supplied us with
	// the overview we asked for.
	BView	*v = factory->NewDataView(SongRef(), mTrackWinProps, prop, SEC_VIEW);
	if (v) return v;

	// If the factory does not have the overview specified, return an overview
	// describing the problem.
	problem << "View Factory named " << sigStr << " does not have secondary view " << nameStr;
	return NoSecondaryView( problem.String() );
}

BView* SeqTrackHrzSecondaryData::NoSecondaryView(const char *problem)
{
	SeqNoView	*v = new SeqNoView(BRect(0,0,0,0), "NoView",
								   B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
								   B_WILL_DRAW | B_FRAME_EVENTS);
	if (!v) return NULL;
	float	w = v->StringWidth(problem);
	BStringView		*sv = new BStringView(BRect(1, 1, w + 1, 18),
										  "ProblemText",
										  problem);
	if (!sv) {
		delete v;
		return NULL;
	}
	v->SetViewColor( ui_color(B_MENU_BACKGROUND_COLOR) );
	v->AddChild(sv);
	return v;
}
