/* SeqTrackHrzSecondaryInfo.cpp
 */
#include <stdio.h>
#include <be/interface/MenuItem.h>
#include <be/interface/StringView.h>
#include <be/interface/Window.h>
#include "ArpViews/ArpMultiScrollBar.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmSong.h"
#include "Sequitur/SeqNoView.h"
#include "Sequitur/SeqTrackHrzSecondaryInfo.h"

/*************************************************************************
 * _EMPTY-INFO-VIEW
 * This class is a simple info view that is invisible except for a property
 * field.  This hrz view manager makes sure it always has a property field
 * as its last view, so that even if there are no secondary views, the user
 * can add one.
 *************************************************************************/
class _EmptyInfoView : public AmTrackInfoView
{
public:
	_EmptyInfoView(	AmSongRef songRef,
					AmTrackWinPropertiesI& trackWinProps,
					const BString& factorySignature);

	virtual	void	AttachedToWindow();
	virtual	void	GetPreferredSize(float *width, float *height);

protected:
	/* Overwrite this to turn off the left line delineator.
	 */
	virtual void		PreDrawSliceOn(BView* view, BRect clip)		{ }
	virtual void		PostDrawSliceOn(BView* view, BRect clip)	{ }
	/* I overwrite this to disable the duplicate and remove and menu items.
	 */
	virtual BPopUpMenu*	NewPropertiesMenu() const;

private:
	typedef AmTrackInfoView		inherited;
};

/*************************************************************************
* SeqTrackHrzSecondaryInfo
* Manages all the auxiliary (ie non-note) data editors that a track can
* have - not just control change editors, but also program changes,
* bank select, and whatever else we want to throw in there.
*************************************************************************/

SeqTrackHrzSecondaryInfo::SeqTrackHrzSecondaryInfo(	AmSongRef songRef,
													AmTrackWinPropertiesI& trackWinProps,
													BRect rect,
													BString factorySignature,
													float separation)
		: inherited(rect,
					"SeqTrackHrzSecondaryInfo",
					B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM),
		  AmSongObserver(songRef),
		  mTrackWinProps(trackWinProps), mFactorySignature(factorySignature),
		  mVsb(0), mInitializing(true)
{
	SetSeparation(separation);
	SetDefaultViewHeight(20);
	InitializeViews();
}

SeqTrackHrzSecondaryInfo::~SeqTrackHrzSecondaryInfo()
{
}

void SeqTrackHrzSecondaryInfo::SetVerticalScrollBar(ArpMultiScrollBar* sb)
{
	mVsb = sb;
	if (mVsb) SetVsb();
}

void SeqTrackHrzSecondaryInfo::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor( Prefs().Color( AM_DATA_BACKDROP_C ) );
}

void SeqTrackHrzSecondaryInfo::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	if( mVsb ) SetVsb();

	BMessage	updateScrollbar('xups');
	if( Window() ) Window()->PostMessage( &updateScrollbar );
}

void SeqTrackHrzSecondaryInfo::InitializeViews()
{
	BView*		view;
	if (mTrackWinProps.CountOrderedTracks() > 0) {
		mInitializing = true;
		// READ SONG BLOCK
		AmSong*		song = WriteLock();
		AmTrack*	track = song ? song->Track(mTrackWinProps.OrderedTrackAt(0) ) : NULL;
		if (track) {
			const AmViewPropertyI*	prop;	
			for (uint32 k=0; (prop = track->Property(SEC_VIEW, k)) != 0; k++) {
				view = NewSecondaryView (prop);
				if (view) AddMiniView (view);
			}
		}
		WriteUnlock( song );
		// END READ SONG BLOCK
		mInitializing = false;
	}
	view = new _EmptyInfoView(SongRef(), mTrackWinProps, mFactorySignature);
	if (view) AddMiniView(view);
}

BView* SeqTrackHrzSecondaryInfo::NewSecondaryView(const AmViewPropertyI* prop)
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
	BView*	v = factory->NewInfoView(SongRef(), mTrackWinProps, prop, SEC_VIEW);
	if (v) return v;

	// If the factory does not have the overview specified, return an overview
	// describing the problem.
	problem << "View Factory named " << sigStr << " does not have secondary view " << nameStr;
	return NoSecondaryView( problem.String() );
}

BView* SeqTrackHrzSecondaryInfo::NoSecondaryView(const char* problem)
{ 
	SeqNoView	*v = new SeqNoView(BRect(0,0,0,0), "NoView",
								   B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
								   B_WILL_DRAW | B_FRAME_EVENTS);
	if (v == 0) return 0;
	float	w = v->StringWidth(problem);
	BStringView		*sv = new BStringView(BRect(1, 1, w + 1, 18),
										  "ProblemText",
										  problem);
	if (sv == 0) {
		delete v;
		return 0;
	}
	v->SetViewColor( ui_color(B_MENU_BACKGROUND_COLOR) );
	v->AddChild(sv);
	return v;
}

void SeqTrackHrzSecondaryInfo::ManagerOperationFinished(uint32 op)
{
	if( !mInitializing ) AddEmptyView();
	if( mVsb ) SetVsb();
}

void SeqTrackHrzSecondaryInfo::AddEmptyView()
{
	BView*	v = (BView*)mViewList.LastItem();
	_EmptyInfoView*	view = dynamic_cast<_EmptyInfoView*>(v);
	if (view) return;	

	view = new _EmptyInfoView(SongRef(), mTrackWinProps, mFactorySignature);
	if (view) AddMiniView(view);
}

void SeqTrackHrzSecondaryInfo::SetVsb()
{
	/* Set the range
	 */
	float 	height = VerticalHeight();
	float	min = 0, max = height;
	if( max <= Frame().Height() ) max = 0;
	else max = height - Frame().Height();
	mVsb->SetRange( min, max );
	/* Set the steps
	 */
	if( min == 0 && max == 0 ) return;
	BRect	bounds = Bounds();
	float	bigStep = Frame().Height() - mDefaultViewHeight;
	float	smallStep = mDefaultViewHeight;
	if( bigStep < mDefaultViewHeight ) bigStep = mDefaultViewHeight;
	mVsb->SetSteps( smallStep, bigStep );
	/* Set the proportion
	 */	
	float	prop = bounds.Height() / height;
	if( prop > 1 ) prop = 1;
	if( prop < 0 ) prop = 0;
	mVsb->SetProportion( prop );
}

float SeqTrackHrzSecondaryInfo::VerticalHeight() const
{
	BView*		v;
	float		bottom = 0;
	for( int32 k = 0; (v = ItemAt(k)); k++ ) {
		if( v->Frame().bottom > bottom ) bottom = v->Frame().bottom;
	}
	return bottom + Bounds().top;
}

/*************************************************************************
 * _EMPTY-INFO-VIEW
 * This class is a simple info view that is invisible except for a property
 * field.  This hrz view manager makes sure it always has a property field
 * as its last view, so that even if there are no secondary views, the user
 * can add one.
 *************************************************************************/
_EmptyInfoView::_EmptyInfoView(	AmSongRef songRef,
								AmTrackWinPropertiesI& trackWinProps,
								const BString& factorySignature)
		: inherited(BRect(0, 0, 0, 0), "empty info view", songRef, trackWinProps, SEC_VIEW)
{
	mFactorySignature = factorySignature;
}

void _EmptyInfoView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if ( Parent() ) mViewColor = Parent()->ViewColor();
//	if( Parent() ) SetViewColor( Prefs().Color( AM_DATA_BACKDROP_C ) );
}

void _EmptyInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = 15;
}

BPopUpMenu*	_EmptyInfoView::NewPropertiesMenu() const
{
	BPopUpMenu*		menu = inherited::NewPropertiesMenu();
	if (!menu) return menu;

	BMenuItem*	item;
	for (int32 index = 0; (item = menu->ItemAt(index)); index++) {
		if( item->Message() ) {
			if( item->Message()->what == DUPLICATE_INFO_MSG
					|| item->Message()->what == REMOVE_INFO_MSG )
				item->SetEnabled(false);
		}
		if (BString(AM_INFO_CHANGE_VIEW_STR).Compare(item->Label()) == 0) {
			item->SetLabel("Add View");
		}
	}
	return menu;
}
