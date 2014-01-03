/* GlMainNavView.h
 * Copyright (c)2004 by Eric Hackborn.
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
#ifndef GLASSLIKE_GLMAINNAVVIEW_H
#define GLASSLIKE_GLMAINNAVVIEW_H

#include <app/Messenger.h>
#include <InterfaceKit.h>
#include <support/Locker.h>
#include <ArpInterface/ArpControlLayer.h>
#include <Glasslike/GlProjectList.h>
class GlImage;
class GlChain;
class GlMainLabels;
class GlMainImageEntry;
class GlMainImages;

// GL-NAV-CONTROL-LAYER
class GlNavControlLayer : public ArpControlLayer
{
public:
	gl_id				id;		// The node I'm arranged for
		
	GlNavControlLayer();
	virtual ~GlNavControlLayer();
	
	void				Arrange(GlMainImageEntry* node);
	
private:
	ArpBitmap			*mDownExp, *mDownOvr,
						*mConfigExp, *mConfigOvr,
						*mPixExp, *mPixOvr;
};

/* A little class to cache whatever control the mouse is over.
 */
class GlNavControl
{
public:
	enum {
		GL_ON_NOTHING		= 0,
		GL_ON_CONTROL		= 1,
		GL_ON_CHAIN			= 2,
		GL_ON_NODE			= 3
	};
	int32					code;

	int32					index;		// _CONTROL: control id, _CHAIN
										// or _NODE: entry id
	gl_id					id;			// _CONTROL or _NODE: gl_node_id,
										// _CHAIN: gl_chain_id

	GlNavControl();
	GlNavControl(const GlNavControl& o);

	bool			operator==(const GlNavControl& o) const;
	bool			operator!=(const GlNavControl& o) const;
	GlNavControl&	operator=(const GlNavControl& o);
};

/********************************************************
 * GL-MAIN-NAV-VIEW
 * Display the current chain.
 ********************************************************/
class GlMainNavView : public BView
{
public:
	GlMainNavView(BRect frame, GlProjectList& path);
	virtual ~GlMainNavView();
	
	virtual	void			AttachedToWindow();
	virtual void			Draw(BRect clip);
	virtual	void			KeyDown(const char *bytes, int32 numBytes);
	virtual void			MessageReceived(BMessage* msg);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(	BPoint where, uint32 code,
										const BMessage* dragMessage);
	virtual	void			MouseUp(BPoint where);

	void					SetContext(const GlChain* chain);

private:
	typedef BView			inherited;
	GlProjectList&			mPath;
	gl_node_id				mParentId;
	gl_chain_id				mContextId;
	GlMainImages*			mNodes;
	GlMainLabels*			mLabels;
	GlNavControlLayer		mControls;

	int32					mMouseDownButtons;
	GlNavControl			mDownCache;
	GlNavControl			mOverCache;
			
	status_t				NodePressed(int32 index, int32 viewType);
	status_t				NodeDeleted(gl_node_id nid);
	status_t				ChainPressed(int32 index);
	status_t				AddOnDropped(gl_node_add_on_id id, BPoint pt);
	status_t				PathDown(gl_chain_id cid, gl_node_id nid);
		
	void					DrawOn(BRect clip, BView* v);
	status_t				Recache();
	status_t				RecacheLabels(const GlNode* node, BPoint origin);

	status_t				CacheControl(BPoint& pt, GlNavControl& cache);

	enum {
		BEFORE_ACTION		= 1,
		AFTER_ACTION		= 2,
		REPLACE_ACTION		= 3
	};
	status_t				GetDropAction(const BPoint& pt, int32* index, int32* action) const;
};

#endif
