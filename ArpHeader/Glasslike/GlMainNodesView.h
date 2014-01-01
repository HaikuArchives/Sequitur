/* GlMainWinAux.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.04.22		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLMAINNODESVIEW_H
#define GLASSLIKE_GLMAINNODESVIEW_H

#include <be/app/Messenger.h>
#include <be/InterfaceKit.h>
#include <be/support/Locker.h>
#include <ArpInterface/ArpControlLayer.h>
#include <Glasslike/GlMidi.h>
#include <Glasslike/GlProjectList.h>
class GlImage;
class GlChain;
class GlMainImageEntry;
class _GlMainNodesCacheManager;

/********************************************************
 * GL-MAIN-NODES-VIEW
 * Display the available nodes.
 ********************************************************/
class GlMainNodesView : public BView
{
public:
	GlMainNodesView(BRect frame, GlProjectList& path);
	virtual ~GlMainNodesView();
	
	virtual void				Draw(BRect clip);
	virtual void				MessageReceived(BMessage* msg);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(	BPoint where, uint32 code,
											const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);

	void						SetContext(const GlChain* chain);

private:
	typedef BView				inherited;
	GlProjectList&				mPath;
	_GlMainNodesCacheManager*	mManager;

	int32						mDownBarIndex;
	int32						mDownNodeIndex;
	BPoint						mDownPt;
	int32						mDownButtons;
	
	int32						mMouseOverBarIndex;
	int32						mMouseOverNodeIndex;

	status_t					NodePressed(int32 barIndex, int32 nodeIndex);
	status_t					StartDrag(int32 barIndex, int32 nodeIndex);
	void						DrawOn(BRect clip, BView* v);

	GlMainImageEntry*			EntryAt(int32 barIndex, int32 nodeIndex);
};

#endif
