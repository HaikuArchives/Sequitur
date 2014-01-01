/* GlPixelTargetView.h
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
 * 2004.02.23			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLPIXELTARGETVIEW_H
#define GLKERNEL_GLPIXELTARGETVIEW_H

#include <GlPublic/GlControlView.h>
#include <GlPublic/GlPlaneNode.h>
#include <GlPublic/GlRootRef.h>

/***************************************************************************
 * GL-PIXEL-TARGET-VIEW
 * A view that displays all the components of a pixel, and lets users
 * turn them on or off for a node.
 ***************************************************************************/
class GlPixelTargetView : public GlControlView
{
public:
	GlPixelTargetView(	const BRect& frame, const GlRootRef& ref,
						const GlPlaneNode& node);

protected:
	virtual	status_t		ControlMessage(uint32 what);

private:
	typedef GlControlView	inherited;
	GlRootRef				mRef;
	gl_node_id				mNid;

	BCheckBox*				mR;
	BCheckBox*				mG;
	BCheckBox*				mB;
	BCheckBox*				mA;
	BCheckBox*				mZ;
	BCheckBox*				mDiff;
	BCheckBox*				mSpec;
	BCheckBox*				mD;
	BCheckBox*				mC;
	BCheckBox*				mF;

	uint32					MakeTargets() const;
};

#endif
