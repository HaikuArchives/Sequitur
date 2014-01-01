/* AmGlobalsImpl.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 05.15.00		hackborn
 * Created this file.
 */

#ifndef AMKERNEL_AMGLOBALSIMPL_H
#define AMKERNEL_AMGLOBALSIMPL_H

#include <be/app/Messenger.h>
#include <be/support/Locker.h>
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmStudio.h"
class BClipboard;
class AmFilterAddOn;

typedef vector<AmViewFactory*>			fac_vec;
typedef vector<BMessenger>				observer_vec;

/***************************************************************************
 * AM-GLOBALS-IMPL
 ***************************************************************************/
class AmGlobalsImpl : public AmGlobalsI, public AmClockTarget
{
public:
	AmGlobalsImpl();
	virtual ~AmGlobalsImpl();

	virtual void				Initialize();
	void						Shutdown();
	
	virtual void				AddObserver(uint32 code, BHandler* handler);
	virtual void				RemoveObserver(uint32 code, BHandler* handler);

	virtual status_t			NewSong(AmSongRef& songRef, int32 undoHistory);
	virtual status_t			RemoveSong(song_id id);
	virtual AmSongRef			SongRef(song_id id) const;
	virtual	void				SelectSong(const AmSongRef& song);
	virtual AmPipelineMatrixRef	PipelineMatrixRef(pipeline_matrix_id id) const;

	virtual AmToolRef			ToolAt(uint32 index) const;
	virtual AmToolRef			Tool(uint32 button, uint32 modifierKeys) const;
	virtual AmToolRef			FindTool(const BString& uniqueName) const;
	virtual AmToolRef			FindTool(int32 key) const;
	virtual void				SetTool(const BString& toolName, uint32 button, uint32 modifierKeys);

	virtual AmToolBarRef		ToolBarAt(uint32 index, bool init = true) const;
	virtual AmToolBarRef		FindToolBar(const BString& toolBarName, bool init = true) const;
	status_t					AddToolBar(AmToolBar* bar);
	/* This is pretty annoying, but this class needs to send out a tool bar
	 * changed message, which is why this method has to be here.
	 */
	status_t					ToggleShowing(const BString& toolBarName);
	status_t					DeleteToolBar(const BString& toolBarName);
	
	virtual AmViewFactory*		FactoryAt(uint32 index) const;
	virtual AmViewFactory*		FactoryNamed(const BString& signature) const;

	AmStudio&					Studio();
	const AmStudio&				Studio() const;

	virtual status_t			GetDeviceInfo(	const am_studio_endpoint& endpoint,
												BString& deviceMfg,
												BString& deviceProduct,
												BString& deviceLabel) const;
	virtual ArpCRef<AmDeviceI>	DeviceAt(uint32 index) const;
	virtual ArpCRef<AmDeviceI>	DeviceAt(am_studio_endpoint& endpoint) const;
	virtual ArpCRef<AmDeviceI>	DeviceNamed(const char* mfg, const char* name) const;
	virtual status_t			GetStudioInfoAt(uint32 index, am_studio_endpoint& outEndpoint,
												BString* outLabel = NULL, BString* outDevMfg = NULL,
												BString* outDevName = NULL) const;

	virtual status_t			GetMotionInfo(	uint32 index,
												BString& outLabel,
												BString& outKey) const;
	virtual AmMotionI*			NewMotion(const BString& key) const;

	virtual AmTime				EndTimeBuffer() const;
	virtual AmPhraseEvent*		PhraseEventNear(const AmTrack* track, AmTime time) const;
	virtual AmPhraseEvent*		PhraseEventNear(const AmTrack* track,
												AmTime time,
												AmTime tolerance) const;
	virtual status_t			GetMeasure(	const AmPhrase& signatures,
											AmTime time, AmSignature* signature) const;
	virtual void				SetUndoHistory(int32 size);
	virtual BClipboard*			Clipboard() const;

	virtual status_t			WriteTo(BMessage* state);
	virtual status_t			ReadFrom(const BMessage* state);
	
	virtual	void				AddClockTarget(AmFilterAddOn* target);
	virtual	status_t			RemoveClockTarget(AmFilterAddOn* target);
	
	/* Implement notification from AmClockTarget.
	 */
	virtual	void				Clock(AmTime time);
	
	/* This is a hack because, due to the order that the app links in,
	 * this globals object can't create a new instance of the AmStdViewFactory,
	 * so instead it's supplied an instance by the app.  Hopefully this can
	 * be worked out.
	 */
	void AddFactory(AmViewFactory* factory);
	/* This is a little bit of a hack -- in certain situations, pipeline matrix
	 * objects exist that haven't and may not be registered with the globals, but
	 * external objects want to gain access to them, so the globals need some way
	 * of being aware of them.  (The specific case is when the user is editing the
	 * pipeline of a tool -- the pipeline should participate in the global system,
	 * namely PipelineMatrixRef(), but it is not yet assigned as an actual tool.
	 */
	void RegisterTemporaryMatrix(AmPipelineMatrixI* matrix);
	void UnregisterTemporaryMatrix(pipeline_matrix_id id);
	
private:
	mutable BLocker	mSongLock;
	songref_vec		mSongRefs;
	AmStudio		mStudio;
	AmSongRef		mSelectedSong;
	vector<AmFilterAddOn*> mClockTargets;
	
	/* Protection for tool and tool observer data.
	 */
	mutable BLocker mObserverLock;

	/* The tool bar mechanism.
	 */
	toolbarref_vec	mToolBars;
	mutable BLocker mToolBarLock;
	observer_vec	mToolBarObservers;
	/* If there are no toolbars, initialize to the defaults.  Not sure if this
	 * is the right place for this, but I'm not sure how else to handle it since
	 * the tools are created one at a time from the search path.  Clients should
	 * lock the observers before calling this.
	 */
	void			InitializeToolBars();
	void			InitializeCurvesToolBar();
	
	/* Right now the observer mechanism is extremely simple -- since currently you
	 * can only observe tool changes, I just keep a list of the handlers observing.
	 */
	observer_vec	mDeviceObservers;
	/* This stores my list of installed view factories.
	 */
	fac_vec			mFactories;

	pipelinematrixref_vec mTemporaryMatrices;
		
	bool			RemoveObserver(observer_vec& observers, BMessenger messenger);
	
	/* This is used to lock accessing the state properties -- currently
	 * nothing, but something will likely be added.
	 */
	mutable BLocker mStateLock;
	/* TEMPORARY!  Until the events can read and write to a file format, sequitur
	 * creates its own private clipboard to hold local data.
	 */
	mutable BClipboard*	mClipboard;
	void			FreeClipboard();
};

#endif
