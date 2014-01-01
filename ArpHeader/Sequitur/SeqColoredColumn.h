/* SeqColoredColumn.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.03.10		hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef SEQUITUR_SEQCOLOREDCOLUMN_H
#define SEQUITUR_SEQCOLOREDCOLUMN_H

#include <be/experimental/ColumnListView.h>
#include <be/experimental/ColumnTypes.h>

/********************************************************
 * SEQ-COLORED-COLUMN
 * Exists purely so it can tell its field to set the
 * appropriate colour.  Obviously, all my fields need to
 * be SeqColoredField objects.
 ********************************************************/
class SeqColoredColumn : public BStringColumn
{
public:
	SeqColoredColumn(	const char *title, float width,
						float maxWidth, float minWidth,
						uint32 truncate);

	virtual void DrawField(BField* field, BRect rect, BView* parent);

private:
	typedef BStringColumn	inherited;
};

/********************************************************
 * SEQ-COLORED-FIELD
 * Exists purely to set what text colour to use.
 ********************************************************/
class SeqColoredField : public BStringField
{
public:
	SeqColoredField(const char *string, bool readOnly,
					bool isValid);

	void		SetReadOnly(bool readOnly);
	void		SetIsValid(bool isValid);
	void		PreDraw(BView* view);

private:
	typedef BStringField	inherited;
	bool		mReadOnly;
	bool		mIsValid;
};

#endif
