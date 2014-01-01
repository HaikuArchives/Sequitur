/* GlConfigView.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2004.02.11			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLCONFIGVIEW_H
#define GLKERNEL_GLCONFIGVIEW_H

#include <ArpCore/StlVector.h>
#include <GlPublic/GlControlView.h>
#include <GlPublic/GlRootRef.h>
class _ConfigListView;
class _ParamRow;

/***************************************************************************
 * GL-CONFIG-VIEW
 * A view that displays all a node's parameters.
 ***************************************************************************/
class GlConfigView : public GlControlView
{
public:
	GlConfigView(	const BRect& frame, const GlRootRef& ref,
					const GlNode& node, const GlStrainedParamList& params);
	virtual ~GlConfigView();

	virtual void			AttachedToWindow();
	virtual	void			GetPreferredSize(float* width, float* height);
	virtual void			MessageReceived(BMessage* msg);

protected:
	virtual	status_t		ControlMessage(uint32 what);

private:
	typedef GlControlView	inherited;
	GlRootRef				mRef;
	gl_node_id				mNid;
	_ConfigListView*		mListView;
	BCheckBox*				mOn;
	BTextControl*			mLabel;
	ArpMenuControl*			mMidi;
	
	status_t				SetStrainedParam(const _ParamRow* row);
};


#endif
