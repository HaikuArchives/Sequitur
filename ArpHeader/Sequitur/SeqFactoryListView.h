/* SeqFactoryListView.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.06.17				hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef SEQUITUR_SEQFACTORYLISTVIEW_H
#define SEQUITUR_SEQFACTORYLISTVIEW_H

#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>

/********************************************************
 * SEQ-FACTORY-LIST-VIEW
 ********************************************************/
class SeqFactoryRow;

class SeqFactoryListView : public BColumnListView
{
public:
	SeqFactoryListView(	BRect rect, const char* name,
						uint32 resizingMode = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	virtual void	SelectionChanged();
	status_t		GetCurrentKeys(BString& outFactoryKey, BString& outViewKey) const;

	void			BuildRows();

protected:
	virtual SeqFactoryRow*	NewRow(bool root = false) const;
	virtual SeqFactoryRow*	NewRow(const BString& factoryKey) const;
	virtual SeqFactoryRow*	NewRow(const BString& factoryKey, const BString& viewKey) const;

	void					ClearRowKeys(BRow* parent = NULL);
	SeqFactoryRow*			ViewRow(const BString& factoryKey,
									const BString& viewKey,
									BRow* parent = NULL);

private:
	typedef BColumnListView		inherited;

	bool					HasRow(	const BString& facKey,
									const BString& viewKey) const;
};

/********************************************************
 * SEQ-FACTORY-ROW
 ********************************************************/
class SeqFactoryRow : public BRow
{
public:
	SeqFactoryRow(bool root = false);
	SeqFactoryRow(const BString& factoryKey);
	SeqFactoryRow(const BString& factoryKey, const BString& viewKey);

	virtual bool		HasLatch() const;
	bool				Matches(const BString& factoryKey,
								const BString& viewKey) const;

	void				SetKey(const char* key);
	void				GetKeyInfo(BString& factoryKey, BString& viewKey, BString& key) const;

	void				Print() const;

protected:
	bool				mRoot;
	BString				mFactoryKey;
	BString				mViewKey;
	BString				mKey;
	BString				mLabel;

	virtual void		GetKeyLabel(const char* key, BString& outLabel);

private:
	typedef BRow	inherited;
};


#endif
