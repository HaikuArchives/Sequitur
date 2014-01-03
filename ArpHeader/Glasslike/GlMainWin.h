/* GlMainWin.h
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
 *	-¢ None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.04.27				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef _GL_MAIN_WIN_H
#define _GL_MAIN_WIN_H

#include <InterfaceKit.h>
#include <SupportKit.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <GlPublic/GlNodeData.h>
#include <Glasslike/GlMainWinAux.h>
#include <Glasslike/GlMidi.h>
#include <Glasslike/GlProjectList.h>
#include <Glasslike/GlResultCache.h>
class BFilePanel;
class GlMainNavView;
class GlMainNodesView;
class GlRecorderHolder;
class GlResultView;
class _StatusView;

/***************************************************************************
 * GL-MIXING-WIN
 * The sole Glasslike window -- top pane is the mixing grid, bottom
 * pane is the current image.
 ***************************************************************************/
class GlMainWin : public BWindow,
 				  public ArpBitmapCache
{
public:
	GlMainWin(BRect frame, const BMessage& config);
	virtual ~GlMainWin();

	virtual void			MessageReceived(BMessage *msg);
	virtual	void			Quit();
//	virtual	bool			QuitRequested();
	virtual	void			WindowActivated(bool state);

private:
	typedef BWindow 		inherited;

	GlProjectList			mPath;
	GlMainControlChannel	mChannel;
	
	GlMainPreview*			mPreview;
	GlMainPathView*			mPathView;
	GlMainNavView*			mChain;
	GlMainNodesView*		mNodes;
	GlMainMidiView*			mMidiView;
	BView*					mInspector;
	BView*					mInspectTarget;
	BButton					*mPlay, *mStop, *mRewind;
	BStatusBar*				mStatusBar;
	GlResultView*			mResultView;
	_StatusView*			mStatusView;
	
	GlResultCache			mResult;
	GlPerformer				mPerformer;
	bool					mRecording;
	
	/* Last known inspector state, so I don't inspect
	 * the same thing twice.
	 */
	gl_chain_id				mInspectorCid;
	gl_node_id				mInspectorNid;
	int32					mInspectorVt;
	int32					mInspectorViewMsg;
		
#if 0
	BScrollBar*				m_MatrixVsb;
	BScrollBar*				m_MatrixHsb;
	BScrollBar*				m_PreviewVsb;
	BScrollBar*				m_PreviewHsb;
	
	BString					mProjectDir, mProjectName;
#endif
	BFilePanel*				mOpenPanel;
	BFilePanel*				mSavePanel;

	status_t				SaveFile(const BMessage* msg, int32 format);

	status_t				NewImageInput(ArpBitmap* bm);

	status_t				AddRoot();
	status_t				RootChanged();
	status_t				PathChanged();
	status_t				InspectNode(gl_chain_id cid, gl_node_id nid,
										int32 vt, int32 viewMsg = 0);
	status_t				Record();
	
	status_t				Open(BMessage* msg);
	status_t				Save(const BEntry& entry, const char* name);
	status_t				ReadSettings(const BMessage& config);
	status_t				WriteSettings();

	void					AddMainMenu(BRect frame);

	void					EnablePlayControls(bool enable);
};

#endif
