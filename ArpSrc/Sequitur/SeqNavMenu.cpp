/* SeqNavMenu.cpp
 */
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <interface/Bitmap.h>
#include <interface/Window.h>
#include <storage/Node.h>
#include <storage/NodeInfo.h>
#include <storage/Volume.h>
#include <storage/VolumeRoster.h>
#include <storage/Entry.h>
#include <storage/Path.h>
#include "Sequitur/SeqNavMenu.h"
#include "Sequitur/SequiturDefs.h"

static bool sort_items(BMenuItem* item1, BMenuItem* item2)
{
	if( item1->Label() == 0 ) return false;
	if( item2->Label() == 0 ) return true;
	BString		str( item1->Label() );
	if( str.ICompare( item2->Label() ) <= 0 ) return true;
	return false;
}

/***************************************************************************
 * _AM-ICON_MENU_ITEM
 ***************************************************************************/
class _AmIconMenuItem : public BMenuItem
{
public:
	_AmIconMenuItem(const char* label, BMessage* message);
	_AmIconMenuItem(BMenu *menu);
	virtual ~_AmIconMenuItem();
	
	void			SetIcon(BBitmap* icon);

protected:
	virtual	void	GetContentSize(float* width, float* height);
	virtual	void	DrawContent();

private:
	typedef	BMenuItem		inherited;
	BBitmap*		mIcon;
};

/***************************************************************************
 * SEQ-NAV-MENU
 ***************************************************************************/
SeqNavMenu::SeqNavMenu(	const char* title,
						const BHandler* target,
						menu_layout layout)
	:	inherited(title, layout),
		mMenuBuilt(false), mTarget( target, target->Looper() ),
		mSkipLoneDirectory(false)
{
}

SeqNavMenu::SeqNavMenu(	const char* title,
						const BMessenger& target,
						menu_layout layout)
	:	inherited(title, layout),
		mMenuBuilt(false), mTarget(target),
		mSkipLoneDirectory(false)
{
}

void SeqNavMenu::SetSkipLoneDirectory(bool skipLoneDirectory)
{
	mSkipLoneDirectory = skipLoneDirectory;
}

void SeqNavMenu::SetPath(const char* path)
{
	mPath = path;
	mPredicate = BString();
}

void SeqNavMenu::SetPredicate(const char* predicate)
{
	mPredicate = predicate;
	mPath = BString();
}

bool SeqNavMenu::IsPathMenu() const
{
	return mPath.String() != 0 && mPath.Length() > 0;
}

bool SeqNavMenu::IsPredicateMenu() const
{
	return mPredicate.String() != 0 && mPredicate.Length() > 0;
}

void SeqNavMenu::Rewind()
{
	RemoveItems( 0, CountItems(), true );
	mMenuBuilt = false;
	ClearMenuBuildingState();
}

static const int32		kItemsToAddChunk = 20;
static const bigtime_t	kMaxTimeBuildingMenu = 200000;

bool SeqNavMenu::AddDynamicItem(add_state state)
{
	if( mMenuBuilt )
		return false;
	
	if( state == B_ABORT ) {
		ClearMenuBuildingState();
		return false;
	}

	if( state == B_INITIAL_ADD && !StartBuildingItemList() ) {
		ClearMenuBuildingState();
		return false;
	}

	bigtime_t timeToBail = system_time() + kMaxTimeBuildingMenu;
	for( int32 count = 0; count < kItemsToAddChunk; count++ ) {
		if( !AddNextItem() ) {
			mMenuBuilt = true;
			DoneBuildingItemList();
			ClearMenuBuildingState();
			return false;
				// done with menu, don't call again
		}
		if( system_time() > timeToBail )
			// we have been in here long enough, come back later
			break;
	}

	return true;	// call me again, got more to show
}

bool SeqNavMenu::StartBuildingItemList()
{
	mFirstEntry = NO_ENTRY;
	if( IsPredicateMenu() ) return StartBuildingForPredicate();
	if( IsPathMenu() ) return StartBuildingForPath();
	return false;
}

bool SeqNavMenu::StartBuildingForPredicate()
{
	BVolumeRoster 	roster;
	BVolume			volume;
	roster.Rewind();
	if( roster.GetBootVolume( &volume ) != B_OK ) {
		printf("couldn't get a volume\n");
		return false;
	}
	mQuery.Clear();
	mQuery.SetVolume( &volume );
	mQuery.SetPredicate( mPredicate.String() );
	if( mQuery.Fetch() != B_OK ) {
		printf("query can't fetch\n");
		return false;
	}
	return true;
}

bool SeqNavMenu::StartBuildingForPath()
{
	mDirectory.Rewind();
	mDirectory.SetTo( mPath.String() );
	if( mDirectory.InitCheck() != B_OK ) return false;
	return true;
}

bool SeqNavMenu::AddNextItem()
{
	// limit nav menus to 500 items only
	if( mItems.size() >= 500 ) return false;

	BEntry			entry;
	bool			useLeafForLabel = true;
	if( IsPathMenu() ) {
		if( mDirectory.GetNextEntry( &entry ) != B_OK ) return false;
	} else if( IsPredicateMenu() ) {
		if( mQuery.GetNextEntry( &entry ) != B_OK ) return false;
		useLeafForLabel = false;
	}
	if( entry.InitCheck() != B_OK ) return false;
	AddEntry( entry, useLeafForLabel );
	return true;
}

bool SeqNavMenu::AddEntry(BEntry& entry, bool useLeafForLabel)
{
	BPath			path;
	if( entry.GetPath( &path ) != B_OK ) {
		printf("\tquery returned an entry but couldn't get the path\n");
		return false;
	}
	const char*		label = (useLeafForLabel) ? path.Leaf(): path.Path();
	if( !label ) return false;
	_AmIconMenuItem* item = 0;
	uint32			tmpEntry;
	if( entry.IsDirectory() ) {
		tmpEntry = DIR_ENTRY;
		SeqNavMenu*	nm = new SeqNavMenu( label, mTarget );
		if( nm && (item = new _AmIconMenuItem( nm )) ) {
			nm->SetPath( path.Path() );
		}
	} else {
		tmpEntry = OTHER_ENTRY;
		BMessage*		msg = new BMessage( B_REFS_RECEIVED );
		entry_ref		ref;
		if( msg && (entry.GetRef( &ref ) == B_OK) ) {
			msg->AddRef( "refs", &ref );
			item = new _AmIconMenuItem( label, msg );
		}
	}

	if( item ) {
		mItems.push_back( item );
		item->SetIcon( GetIcon( entry ) );
		if( mFirstEntry == NO_ENTRY ) mFirstEntry = tmpEntry;
	}
	return true;
}

BBitmap* SeqNavMenu::GetIcon(BEntry& entry)
{
	BNode		node( &entry );
	BNodeInfo	info( &node );
	BBitmap*	bm = 0;
	if( info.InitCheck() != B_OK ) return 0;
	bm = new BBitmap( BRect(0, 0, 15, 15), B_COLOR_8_BIT );
	if( !bm ) return 0;
	if( info.GetTrackerIcon( bm, B_MINI_ICON ) != B_OK ) {
		delete bm;
		return 0;
	}
	return bm;
}

void SeqNavMenu::DoneBuildingItemList()
{
	if( mSkipLoneDirectory && mFirstEntry == DIR_ENTRY && mItems.size() == 1 )
		SkipLoneDirectory();
	
	sort( mItems.begin(), mItems.end(), sort_items );
	for( uint32 k = 0; k < mItems.size(); k++ )
		AddItem( mItems[k] );
	mItems.resize(0);

	SetTargetForItems( mTarget );
}

void SeqNavMenu::ClearMenuBuildingState()
{
	mQuery.Clear();
	mDirectory.SetTo( "<invalid>" );
	/* For whatever reason, the operation has been aborted.  Need
	 * to delete any items in the list.
	 */
	for( uint32 k = 0; k < mItems.size(); k++ )
		delete mItems[k];
	mItems.resize(0);
}

void SeqNavMenu::SkipLoneDirectory()
{
	BMenuItem*	item = mItems[0];
	SeqNavMenu*	nm = dynamic_cast<SeqNavMenu*>( item->Submenu() );
	if( !nm ) return;
	BDirectory	dir( nm->mPath.String() );
	if( dir.InitCheck() != B_OK ) return;

	mItems.resize(0);
	delete item;
	dir.Rewind();
	BEntry		entry;
	for( uint32 k = 0; k < 500; k++ ) {
		if( dir.GetNextEntry( &entry ) != B_OK ) return;
		if( entry.InitCheck() == B_OK )
			AddEntry( entry, true );
	}
}

/***************************************************************************
 * _AM-ICON_MENU_ITEM
 ***************************************************************************/
_AmIconMenuItem::_AmIconMenuItem(const char* label, BMessage* message)
		: inherited( label, message ),
		mIcon(0)
{
}

_AmIconMenuItem::_AmIconMenuItem(BMenu* menu)
		: inherited( menu ),
		mIcon(0)
{
}

_AmIconMenuItem::~_AmIconMenuItem()
{
	delete mIcon;
}

void _AmIconMenuItem::SetIcon(BBitmap* icon)
{
	delete mIcon;
	mIcon = icon;
}

void _AmIconMenuItem::GetContentSize(float* width, float* height)
{
	inherited::GetContentSize(width, height);
	*width += 20;
	*height += 3;
}

void _AmIconMenuItem::DrawContent()
{
	BPoint	drawPoint( ContentLocation() );
	drawPoint.x += 20;
	Menu()->MovePenTo( drawPoint );
	inherited::DrawContent();
	
	BPoint	where( ContentLocation() );
	where.y = Frame().top;
	
	if( mIcon ) {
		if( IsEnabled() ) Menu()->SetDrawingMode( B_OP_OVER );
		else Menu()->SetDrawingMode( B_OP_BLEND );
		
		Menu()->DrawBitmapAsync( mIcon, where );
	}
}
