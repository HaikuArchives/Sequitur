/* GlRootInfoInspector.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2003.01.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLROOTINFOINSPECTOR_H
#define GLASSLIKE_GLROOTINFOINSPECTOR_H

#include <GlPublic/GlControlView.h>
#include <GlPublic/GlRootRef.h>

/***************************************************************************
 * GL-ROOT-INSPECTOR
 * A view that displays all a node's parameters.
 ***************************************************************************/
class GlRootInfoInspector : public GlControlView
{
public:
	GlRootInfoInspector(BRect frame, const GlRootNode* root);

protected:
	virtual	status_t		ControlMessage(uint32 what);

private:
	typedef GlControlView	inherited;
	GlRootRef				mRef;
	BTextControl*			mCreator;
	BTextControl*			mKey;
	BTextControl*			mCategory;
	BTextControl*			mLabel;
	BTextControl*			mPath;

	void					SetString(BTextControl* ctrl);
	void					SetKey();
};


#endif
