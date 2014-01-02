/* SeqColoredColumn.cpp
 */
#include <stdio.h>
#include <experimental/ColorTools.h>
#include "ArpKernel/ArpDebug.h"
#include "Sequitur/SeqColoredColumn.h"

/********************************************************
 * SEQ-COLORED-COLUMN
 ********************************************************/
SeqColoredColumn::SeqColoredColumn(	const char *title, float width,
									float maxWidth, float minWidth,
									uint32 truncate)
		: inherited(title, width, maxWidth, minWidth,
					truncate)
{
}

void SeqColoredColumn::DrawField(	BField *field, BRect rect,
									BView *parent)
{
	SeqColoredField*	f = dynamic_cast<SeqColoredField*>(field);
	if (f) f->PreDraw(parent);
	inherited::DrawField(field, rect, parent);
}

/********************************************************
 * SEQ-COLORED-FIELD
 ********************************************************/
SeqColoredField::SeqColoredField(	const char *string,
									bool readOnly, bool isValid)
		: inherited(string),
		  mReadOnly(readOnly), mIsValid(isValid)
{
}

void SeqColoredField::SetReadOnly(bool readOnly)
{
	mReadOnly = readOnly;
}

void SeqColoredField::SetIsValid(bool isValid)
{
	mIsValid = isValid;
}

void SeqColoredField::PreDraw(BView* view)
{
	if (!mIsValid)
		view->SetHighColor(255, 0, 0);
	else if (mReadOnly)
		view->SetHighColor( mix_color(view->HighColor(), view->LowColor(), uint8(140) ) );
}

