/* SeqSongWindowMenu.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This file handles the dynamic aspects of the song window's
 * main menu bar -- primarily, building menus based on queries.
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
 *	-None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 07.22.00		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQNAVMENU_H
#define SEQUITUR_SEQNAVMENU_H

#include <vector.h>
#include <app/Messenger.h>
#include <interface/Menu.h>
#include <interface/MenuItem.h>
#include <storage/Directory.h>
#include <storage/Query.h>
#include <support/String.h>

typedef vector<BMenuItem*>		item_vec;

/*************************************************************************
 * SEQ-NAV-MENU
 * This is taken from Tracker's SlowMenu and NavMenu.  It's a menu that
 * is dynamically built based on either a query (i.e. predicate) or
 * directory.
 *************************************************************************/
class SeqNavMenu : public BMenu
{
public:
	SeqNavMenu(	const char* title,
				const BHandler* target,
				menu_layout layout = B_ITEMS_IN_COLUMN);
	SeqNavMenu(	const char* title,
				const BMessenger& target,
				menu_layout layout = B_ITEMS_IN_COLUMN);

	/* This odd option is false by default.  If you set it to true, it
	 * does this:  If the result of building this menu is a single item
	 * whose entry is a directory, then instead of displaying that single
	 * directory, this menu will display the directory's contents.
	 *
	 * This is here because the whole reason I did these nav menus is to
	 * have shortcuts into the file system:  Typically, I have queries that
	 * result in a single top-level directory that contains all the music
	 * files I want.  But what I'm not interested in is that top level
	 * directory, but the contents it contains, so turning this on lets
	 * me skip one extranneous menu level.
	 */
	void		SetSkipLoneDirectory(bool skipLoneDirectory);
	/* Nav menus can be based on either paths or predicates (i.e. queries)
	 * depending upon which mode is set.  They are mutually exclusive,
	 * so setting a menu to one mode invalidates the other.  Since a path
	 * can be set to either a folder or a query and it will behave
	 * identically, directly setting a predicate is a little unusal --
	 * typically, clients would set a path to a query.
	 */
	void		SetPath(const char* path);
	void		SetPredicate(const char* predicate);
	bool		IsPathMenu() const;
	bool		IsPredicateMenu() const;
	/* Once the dynamic menu has been built, it stays built until something
	 * explicity releases it with the Rewind() command.  A common place to
	 * do this would be in the MenusEnded() hook of the controlling window.
	 */
	void		Rewind();

protected:
	virtual bool AddDynamicItem(add_state state);

	virtual bool StartBuildingItemList();
	virtual bool AddNextItem();
	virtual void DoneBuildingItemList();
	virtual void ClearMenuBuildingState();

private:
	typedef BMenu	inherited;

	bool			mMenuBuilt;
	BMessenger		mTarget;
	bool			mSkipLoneDirectory;
	/* This is an odd little guy that caches the type of object the
	 * first entry is.  It starts out as NO_ENTRY.  When the dynamic
	 * menu is built, with the first entry it comes across, if that
	 * entry is a directory, it's set to DIR_ENTRY, otherwise it's
	 * set to OTHER_ENTRY.  This is used in conjunction with
	 * mSkipLoneDirectory, so, if that flag is on and there's only a
	 * single menu item when I get to DoneBuildingItemList(), that
	 * method doesn't have to create an entry out of the item just to
	 * see if it's a directory.
	 */
	enum {
		NO_ENTRY,
		DIR_ENTRY,
		OTHER_ENTRY
	};
	uint32			mFirstEntry;
	/* Nav menus can be based on either a path or a predicate.  If it's
	 * a predicate, a new query is constructed for the boot volume and
	 * run whenever this menu is accessed.  If it's a path, then the
	 * entry_ref for that path is tracked down, and the file or folder
	 * is added to the menu.
	 */
	BString			mPath;
	BString			mPredicate;
	item_vec		mItems;
	bool			AddEntry(BEntry& entry, bool useLeafForLabel = true);
	BBitmap*		GetIcon(BEntry& entry);
	
	bool			StartBuildingForPath();
	BDirectory		mDirectory;
	
	bool			StartBuildingForPredicate();
	BQuery			mQuery;

	/* If, when I hit DoneBuildingItemList(), mSkipLoneDirectory is true,
	 * and mFirstEntry is DIR_ENTRY, and there is a single item in the item
	 * list, then this method will be called.  This method will remove the
	 * single item in the item list, which is now known to be a directory,
	 * and replace it with that directories contents.
	 */
	void			SkipLoneDirectory();
};

#endif
