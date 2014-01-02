/* SeqNoView.h
 * Copyright (c)1998-2001 by Eric Hackborn.
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
 * 2001.05.06		hackborn@angryredplanet.com
 * Extracted the no view from AmPublic/AmViewFactory.  That REALLY
 * was not the correct place for it.
 */

#ifndef SEQUITUR_SEQNOVIEW_H
#define SEQUITUR_SEQNOVIEW_H

#include <interface/View.h>

/*************************************************************************
 * SEQ-NO-VIEW 
 * This is a small class used to generate new 'error' views --
 * views that indicate to the user something was wrong trying to generate
 * the view that has been requested.  For example, if a track uses an
 * AmViewFactory that is not loaded, this class can be instantiated
 * to inform the user.  Typically, a BStringView with the problem is added
 * as a child of this view.
 *************************************************************************/
class SeqNoView : public BView
{
public:
	SeqNoView(	BRect frame,
				const char *name,
				uint32 resizeMask,
				uint32 flags);

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);

private:
	typedef BView	inherited;
};


#endif 
