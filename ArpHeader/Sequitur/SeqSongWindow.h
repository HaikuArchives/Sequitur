/* SeqSongWindow.h
 * Copyright (c)1996 - 2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This window holds all the views to display one AmSong.
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
 *	• HandleContextMenuFor() is barely started, needs to be able to handle
 * all of the messages returned by the BuildContextMenuFor() function.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.16.98		hackborn
 * Mutated this file from SsVersionWindow.
 */

#ifndef SEQUITUR_SEQSONGWINDOW_H
#define SEQUITUR_SEQSONGWINDOW_H

#ifndef _MESSENGER_H
#include <Messenger.h>
#endif
// HACK cause we can't get the file panel to run properly
#ifndef _FILE_PANEL_H
#include <FilePanel.h>
#endif

#include <Window.h>

class BMenuItem;

#include "ArpKernel/ArpBitmapCache.h"
#include "AmPublic/AmViewFactory.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmTimeConverter.h"
#include "AmKernel/AmTrack.h"
#include "BeExp/ToolTip.h"
#include "Sequitur/SeqSongSelections.h"
#include "Sequitur/SeqWindowStateI.h"
class SeqMeasureControl;
class SeqSongIndexMatrixView;
class SeqSongSelections;
class SeqPhraseMatrixView;
class SeqPipelineMatrixView;
class SeqSongTitleMatrixView;
class SeqSongHrzManager;
class SeqTempoControl;
class SeqTimeHmsmView;
class SeqTimeMbtView;

/*************************************************************************
 * SEQ-SONG-WINDOW
 *************************************************************************/
class SeqSongWindow : public BWindow,
					  public AmSongObserver,
					  public SeqSongWinPropertiesI,
					  public ArpBitmapCache,
					  public BToolTipFilter,
					  public SeqWindowStateI
{
private:
	typedef BWindow	inherited;
	
public:
	SeqSongWindow(entry_ref *ref, const char *title,
		window_look look, window_feel feel, uint32 flags, uint32 workspace);
	virtual ~SeqSongWindow();

	virtual bool		IsDirty();
	enum {
		NO_CONFIGURATION	= (1<<0)
	};
	virtual status_t			Load(BEntry *e, bool merge, uint32 flags = 0);
	virtual status_t			Load(BEntry *e)					{ return Load(e, false); }
	virtual status_t			Save(BEntry *e, const BMessage* args = 0);
	virtual void				WindowFrame(BRect *proposed);
	
	virtual	void				MenusBeginning();
	virtual	void				MenusEnded();
	virtual	void				DispatchMessage(BMessage *message, BHandler *handler);
	virtual void				MessageReceived(BMessage*);
	virtual void				Quit();
	virtual	void				WindowActivated(bool state);
	/* Clients can use this command to scroll the arrange window
	 * by the supplied amount.
	 */
	void						ScrollArrangeBy(float delta);
	void						ScrollArrangeTo(float value);

	/*---------------------------------------------------------
	 * SEQ-SONG-WIN-PROPERTIES-I INTERFACE
	 *---------------------------------------------------------*/
	virtual SeqSongSelections*	Selections() const;
	virtual void				SetSelections(SeqSongSelections* selections);
	virtual bool				IsRecording() const;

	void						SelectAll();

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool				IsSignificant() const;
	virtual status_t			GetConfiguration(BMessage* config);

protected:
	status_t					SetConfiguration(const BMessage* config);
	/* Answer B_OK if I actually turned recording on and it
	 * wasn't already, false otherwise.
	 */
	status_t					SetIsRecording(bool isRecording);
	
	void TransportChangeReceived(BMessage* msg);
	void TrackSongPosition(AmTime time);
	void MergePhrases();
	void MergeTrackPhrases(AmTrack* track, AmRange range);
	void SeparatePhrases();
	void SeparateTrackPhrases(AmTrack* track, AmRange range);
	void DeleteSelection();
	void Delete(AmTrack* track, AmRange range);
	void ExpandMarkedRange();
	void ContractMarkedRange();
	void Expand(AmTrack* track, AmTime start, AmTime expand,
				BUndoContext* undoContext);
	void EditTrackName();
	void PerformSongFunction(const BMessage* msg);
	/* Add a new track to the song.
	 */
	void NewTrack(const BMessage* device = NULL);
	void DeleteSelectedTracks();
	void MoveTracksBy(int32 delta);
	void GroupTracks();
	void UngroupTracks();
	
	SeqTempoControl* TempoControl() const;
	SeqMeasureControl* MeasureControl() const;


private:
	/* The background view for the top control panel.
	 */
	BView*					mControlBg;
	bool					mIsRecording;
	
	/* The current time-to-pixel converter is shared by all the views
	 * in this window, to allow them easy access to any changes without
	 * sending out a notification.
	 */
	AmTimeConverter			mMtc;
	SeqSongSelections*		mSelections;
	// Stores the height, in pixels, of the overviews
	float					mOverViewHeight;
	/* Set to true if I should be scrolling my arrange view with
	 * the song position.
	 */
	bool					mTrackSongPosition;

	void		InitData(AmSongRef songRef);
	void		AddMainMenu();

	void		AddViews();
	void		InitializeControlLayer(float top, float height);
	long		InitializeMeasureLayer(float, float);
	long		InitializeTrackLayer(float);
	BView*		NewHeaderPanel(BRect frame) const;
	BView*		NewInputPanel(BRect frame) const;
	/* ARGH!  This can't be const or else the command to
	 * start watching the zoom control fails.  Stupid, stupid.
	 */
	BView*		NewArrangePanel(BRect frame, BScrollBar** outSb, BRect* outSbF);
	BView*		NewOutputPanel(BRect frame) const;
	BView*		NewSplitter(BRect leftTop, BRect rightBottom,
							const char* name,
							uint32 resizeMask, orientation direction) const;
	/* Construct the scrollbar and connect it up with all the manager
	 * views.  This step should be done after all the panesl are added.
	 */
	BView*		NewVerticalScrollBar(BRect frame);
	
	void		OpenTempoWindow();
	void		HandlePlaySong();
	void		HandleSetSongPosition(AmTime time);
	void		HandleMoveSongPosition(AmTime offset);
	void		HandleStopSong(bool panic = false);
	void		HandleDoubleClickTrack(const BMessage* msg);
	void		ObserverMessageReceived(const BMessage* msg);
//	void HandleContextMenuFor(AmTrack *track, AmTime time);

	// Answer a message that contains our basic operations on
	// a track
//	BPopUpMenu* BuildContextMenuFor(AmTrack *track);
	// Add the portions dependent upon the presence of AmViewFactories
	// to the menu answered above.
//	bool AddViewFactoryMenuItems(BMenu *toMenu, AmTrack *track);
	// A convenience for adding menu items to the 

	BMessenger	mSignatureWin;
	void		ShowSignatureWindow(int32 measure, uint32 beats, uint32 beatValue);
	status_t	GetLeftSignature(AmSignature& sig);
	/* A quick hack to load in system exclusive files.
	 */
	status_t	LoadSyx(entry_ref& ref, bool merge, uint32 flags);
	
	void RequestTrack(BMessage *msg);
	
	BMenuItem*	mOpenMenu;
	BMenuItem*	mUndoItem;
	BMenuItem*	mRedoItem;
	BMenuItem*	mMergeItem;
	BMenuItem*	mSeparateItem;
	BMenuItem*	mDeleteItem;
	BMenuItem*	mExpandItem;
	BMenuItem*	mContractItem;
	BMenuItem*	mDeviceItem;
	BMenuItem*	mFilterItem;
	BMenuItem*	mMotionItem;
	BMenuItem*	mToolItem;
	BFilePanel*	mImportPanel;
	BFilePanel*	mSavePanel;
	/* Sequitur doesn't have a global midi receiver -- every input
	 * filter is receiving every event.  This is used to weed out
	 * multiple MMC events.
	 */
	bigtime_t				mLastMmcTime;

	SeqSongIndexMatrixView*	IndexMatrix() const;
//	SeqSongHrzTitles*		HeaderManager() const;
	SeqSongTitleMatrixView*	HeaderManager() const;
	SeqPipelineMatrixView*	InputManager() const;
	SeqPhraseMatrixView*	ArrangeManager() const;
	SeqPipelineMatrixView*	OutputManager() const;
	SeqPipelineMatrixView*	DestinationManager() const;
	BScrollBar*				ArrangeScrollBar() const;
	SeqTimeMbtView*			TimeMbtView() const;
	SeqTimeHmsmView*		TimeHmsmView() const;

	AmTime				LeftTick() const;
	/* Major hack time.  With the change to using Quit() instead of
	 * QuitRequested() to shutdown the app, this caused problems with
	 * getting the ref added to the settings file.  This is a little hack
	 * to help with that.
	 */
	bool				mAddedRefToSettings;
	void				AddRefToSettings();

	/* Keep track of every track window I open so I can close them
	 * all down when I'm closed.
	 */
	vector<BMessenger>	mTrackWins;
	void				CloseTrackWindows();
};

#endif
