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
#ifndef GLASSLIKE_GLMAINWINAUX_H
#define GLASSLIKE_GLMAINWINAUX_H

#include <be/app/Messenger.h>
#include <be/InterfaceKit.h>
#include <be/support/Locker.h>
#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpControlLayer.h>
#include <GlPublic/GlControlTarget.h>
#include <GlPublic/GlProcessStatus.h>
#include <GlKernel/GlMidiTargetI.h>
#include <Glasslike/GlMidi.h>
#include <Glasslike/GlProjectList.h>
class GlImage;
class GlChain;
class GlMainLabels;
class GlMainImages;
class GlMainPreview;
class GlResultCache;
class _GlPerformerData;

// GL-PATH-CONTROL-LAYER
class GlPathControlLayer : public ArpControlLayer
{
public:
	int32				index;		// The node I'm arranged for
		
	GlPathControlLayer();
	virtual ~GlPathControlLayer();
	
	void				Arrange(BRect frame, int32 index);
	
private:
	ArpBitmap			*mRootPropsExp, *mRootPropsOvr,
						*mManageExp, *mManageOvr;
};

// GL-MIDI-STATE
class GlMidiState : public GlControlState
{
public:
	bool					mRecordable;
	bool					mMidi[GL_MIDI_SIZE];
	
	GlMidiState();
	
	void					Clear();
	
	virtual void			SetRecordable();
	virtual void			SetMidi(int32 letter);
};

/********************************************************
 * GL-MAIN-PATH-VIEW
 * Display the current path (as text).
 ********************************************************/
class GlMainPathView : public BView
{
public:
	GlMainPathView(BRect frame, GlProjectList& path);
	virtual ~GlMainPathView();
	
	virtual void			Draw(BRect clip);
	virtual void			MessageReceived(BMessage* msg);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(	BPoint where, uint32 code,
										const BMessage* dragMessage);
	virtual	void			MouseUp(BPoint where);

private:
	typedef BView			inherited;
	GlProjectList&			mPath;
	GlMainLabels*			mNodes;
	int32					mMouseDownIndex;
	GlPathControlLayer		mControls;

	status_t				PathPressed(int32 index);
	status_t				RootPressed(int32 index);
	status_t				ChangeRoot(int32 index);

	status_t				RootChanged();
		
	void					DrawOn(BRect clip, BView* v);
	status_t				Recache();
};

/********************************************************
 * GL-MAIN-MIDI-VIEW
 * Display the active midi controllers
 ********************************************************/
class GlMainMidiView : public BView
{
public:
	GlMainMidiView(BRect frame);
	virtual ~GlMainMidiView();
	
	virtual void			Draw(BRect clip);
	virtual void			MessageReceived(BMessage* msg);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(	BPoint where, uint32 code,
										const BMessage* dragMessage);
	virtual	void			MouseUp(BPoint where);

	void					SetState(	GlControlChannel* channel,
										int32 viewType);
	
private:
	typedef BView			inherited;

	GlMidiState				mState;

	ArpBitmap*				mImages[GL_MIDI_SIZE];
	ArpBitmap*				mQImage;
	ArpBitmap*				mRecordBm;
	
	int32					mMouseDown;

	void					DrawOn(BRect clip, BView* v);

	bool					OnRecord(BPoint where);
};

/********************************************************
 * GL-MAIN-CONTROL-CHANNEL
 * Hold a list of targets and forward events on to them.
 ********************************************************/
class GlMainControlChannel : public GlControlChannel
{
public:
	GlMainControlChannel(BMessenger window);
	virtual ~GlMainControlChannel();

	void						SetPath(GlProjectList* path);

	virtual status_t			ParamEvent(	gl_param_key key, int32 code,
											const GlParamWrap& wrap);
	virtual status_t			MidiEvent(	GlMidiEvent event, bigtime_t time);

	virtual status_t			Add(GlControlTarget* target);

	status_t					StartRecording();
	void						StopRecording();

	virtual void				Populate(GlControlTargetPopulator& p);
	virtual void				SetState(GlControlState& s) const;

	void						DeleteTargets();

private:
	BMessenger					mWindow;
	GlProjectList*				mPath;
	vector<GlControlTarget*>	mTargets;

	status_t					CreateRoot(GlMidiBinding* b, float v);
	status_t					SetParamValue(float v, int32 paramIndex, GlNode* n) const;
};

/*************************************************************************
 * GL-PERFORMER
 *************************************************************************/
class GlPerformer
{
public:
	GlPerformer(GlResultCache& result);
	virtual ~GlPerformer();

	status_t			InitStatus(	const BMessenger& target, int32 code,
									float width);

	status_t			StartRoot(	const GlRootRef* ref, bool debug = false);
	void				Stop();

protected:
	GlResultCache&		mResult;
	uint32				mArgsFlags;
	
	virtual GlImage*	NewInputImage() const;
	virtual status_t	SetResult(GlImage* img);
	/* Something has happened to cause me to play, but there's
	 * no algorithm to run.
	 */
	virtual void		PlayNothing()		{ }

private:
	BMessenger			mTarget;
	int32				mCode;
	
	/* Performance thread
	 */
	enum {
		ROOT_WAITING	= 1,
		ROOT_RUNNING	= 2
	};
	int32				mState;
	thread_id			mThread;
	BLocker				mAccess;
	GlProcessStatus		mStatus;
	/* This is only held by me temporarily -- as soon as the thread
	 * starts up, it takes ownership.
	 */
	_GlPerformerData*	mData;

	status_t			SetupFromProject(const GlRootRef& ref);
	
	_GlPerformerData*	GiveData();
	
	void				PerformFinished(_GlPerformerData* data, status_t err);
	void				FreePerformData();
	static int32		PerformThreadEntry(void *arg);
};

/*************************************************************************
 * GL-PREVIEW-PERFORMER
 *************************************************************************/
class GlPreviewPerformer : public GlPerformer
{
public:
	GlPreviewPerformer(GlResultCache& result);

	void				SetPreview(GlMainPreview* preview);

protected:
	virtual GlImage*	NewInputImage() const;
	virtual status_t	SetResult(GlImage* img);
	virtual void		PlayNothing();

private:
	typedef GlPerformer	inherited;

	GlMainPreview*		mPreview;
	int32				mW, mH;
};

/********************************************************
 * GL-MAIN-PREVIEW
 * Display the preview image.
 ********************************************************/
class GlMainPreview : public BView
{
public:
	GlMainPreview(	BRect frame, GlProjectList& path,
					GlResultCache& result);
	virtual ~GlMainPreview();
	
	virtual	void			AttachedToWindow();
	virtual void			Draw(BRect clip);
	virtual void			MessageReceived(BMessage* msg);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(	BPoint where, uint32 code,
										const BMessage* dragMessage);
	virtual	void			MouseUp(BPoint where);

	void					TakeBitmap(ArpBitmap* bm);
	status_t				Process();
		
private:
	typedef BView			inherited;
	BLocker					mAccess; // For the bitmap
	ArpBitmap*				mBitmap;
	GlProjectList&			mPath;
	GlPreviewPerformer		mPerformer;
	rgb_color				mBgC;
	
	status_t				RootChanged();

	void					DrawOn(BRect clip, BView* v);
};

/********************************************************
 * Misc
 ********************************************************/
BMenuItem* add_menu_item(	BMenu* toMenu, const BString16* label,
							BMessage* msg, char shortcut,
							uint32 modifiers = B_COMMAND_KEY);
BMenuItem* add_menu_item(	BMenu* toMenu, const BString16* label,
							uint32 what, char shortcut,
							uint32 modifiers = B_COMMAND_KEY);

#endif
