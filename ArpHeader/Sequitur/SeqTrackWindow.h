/* TrackWindow.h
 */

#ifndef SEQUITUR_SEQTRACKWINDOW_H
#define SEQUITUR_SEQTRACKWINDOW_H

#include <experimental/BitmapButton.h>
#include <interface/ScrollBar.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmEventInspectors.h"
#include "AmKernel/AmPropertiesTool.h"
#include "AmPublic/AmTrackRef.h"
#include "BeExp/ToolTip.h"
#include "Sequitur/SeqWindowStateI.h"
class BMenu;
class BMenuField;
class BMenuItem;
class AmDurationControl;
class AmVelocityControl;
class ArpMultiScrollBar;
class SeqMeasureControl;
class SeqTrackHrzSecondaryData;
class SeqTrackHrzSecondaryInfo;

/*************************************************************************
 * SEQ-TRACK-WINDOW
 *************************************************************************/
class SeqTrackWindow : public BWindow,
					   public AmSongObserver,
					   public AmTrackWinPropertiesI,
					   public ArpBitmapCache,
					   public BToolTipFilter,
					   public SeqWindowStateI
{
public:
	SeqTrackWindow(	AmSongRef songRef,
					AmTrackRef trackRef,
					AmTime time, 
					BString factorySignature);
	SeqTrackWindow(	AmSongRef songRef,
					const BMessage* config);
	virtual ~SeqTrackWindow();

	static SeqTrackWindow* ClassOpen(	AmSongRef songRef,
										AmTrackRef trackRef,
										AmTime time,
										BString factorySignature);
	static SeqTrackWindow* ClassOpen(	AmSongRef songRef,
										const BMessage* config);
	
	virtual	void	DispatchMessage(BMessage *message, BHandler *handler);
	virtual	void	FrameResized(float new_width, float new_height);
	virtual	void	MenusBeginning();
	virtual void	MessageReceived(BMessage*);
	virtual	void	WindowActivated(bool state);
	virtual	void	Quit();
	virtual	bool	QuitRequested();

	void			CutSelectedEvents();
	void			CopySelectedEvents();
	void			PasteSelectedEvents();

	/* Scroll so that the supplied time is centered.
	 */
	void			ScrollToTime(AmTime time);
	/* The ID of my current primary track.
	 */
	track_id		TrackId() const;
	/* A convenience for the song window, this forces the window
	 * to store its current state to its track.
	 */
	void			SaveState();
	
	/*---------------------------------------------------------
	 * AM-TRACK-WIN-PROPERTIES-I INTERFACE
	 *---------------------------------------------------------*/
	virtual uint32			CountOrderedTracks() const;
	virtual AmTrackRef		OrderedTrackAt(uint32 index) const;
	virtual const AmTimeConverter& TimeConverter() const;
	virtual AmSelectionsI*	Selections() const;
	virtual void			SetSelections(AmSelectionsI* selections);
	virtual AmTime			GridTime() const;
	virtual void			GetSplitGridTime(int32* m, AmTime* v, int32* d) const;
	virtual uint8			Velocity() const;
	virtual float			OrderedSaturation() const;
	virtual float			ShadowSaturation() const;
	virtual void			PostMessageToDataView(BMessage& msg, view_id fromView);
	virtual void			PostMessageToInfoView(BMessage& msg, view_id fromView);

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool			IsSignificant() const;
	virtual status_t		GetConfiguration(BMessage* config);

protected:
	status_t				SetConfiguration(const BMessage* config);
	SeqMeasureControl*		MeasureControl() const;

private:
	typedef BWindow				inherited;
	std::vector<AmTrackRef>			mTrackRefs;
	AmSelectionsI*				mSelections;
	AmTimeConverter				mMtc;
	AmTime						mCachedEndTime;
	BString						mFactorySignature;
	AmPropertiesTool			mPropertiesTool;
	AmDurationControl*			mDurationCtrl;
	AmVelocityControl*			mVelocityCtrl;
	float						mOrderedSaturation;
	float						mShadowSaturation;
	BMessenger					mSatWin;
	BMessenger					mMotionWin;
	bool						mLoop;
	BPoint						mToolBarLeftTop;	// This just stores the top left
													// corner of the first tool bar,
													// to make it easier on me whenever
													// I need to rebuild them.
	
	BView*						mPriDataView;
	BView*						mPriInfoView;
	/* This string caches the factory's name for the primary
	 * view (the name that gets displayed in the Change To menu).
	 * It's here merely to place a check next to the name of
	 * the current view.  I did this to appease Jace.
	 */
	BString						mPriName;
	BMenu*						mChangeToMenu;
	BMenu*						mShowToolBarMenu;
	BMenu*						mDeleteToolBarMenu;
	BMenuItem*					mFilterItem;
	BMenuField*					mTracksField;
	
	SeqTrackHrzSecondaryInfo*	mSecondaryInfo;
	SeqTrackHrzSecondaryData*	mSecondaryData;
		
	BView*						mBgView;
		
	ArpMultiScrollBar*			mHsb;	// The horizontal scroll
										// bar for the window.
	AmInspectorFactory*			mInspectorFactory;
	BMenuItem* 					mUndoItem;
	BMenuItem* 					mRedoItem;

	void		HandlePlay(const AmTrackRef& solo);
	void		TransportChangeReceived(BMessage* msg);
	void		TrackSongPosition(AmTime time);

	BMessenger	mSignatureWin;
	void		ShowSignatureWindow(int32 measure, uint32 beats, uint32 beatValue);
	status_t	GetLeftSignature(AmSignature& sig);

	void		SetupHsb();
		
	/* Answer the primary vertical scroll bar.
	 */
	ArpMultiScrollBar*			PrimaryVertical() const;
	
	void	ObserverMessageReceived(const BMessage* msg);
	void	TrackChangeReceived(const BMessage* msg);
	void	DuplicateInfoReceived(const BMessage* msg);
	void	ChangeInfoReceived(const BMessage* msg);
	void	AmScrollBy(float x, float pri_y, float sec_y);
	void	SetPrimaryTrack(track_id tid, const char* trackName);
	void	SetOrderedTrack(track_id tid, uint32 order);
	void	ClearOrderedTrack(track_id tid);
	void	StoreViewConfig();
	
	enum {
		POST_TO_INFO_VIEWS		= 0x00000001,
		POST_TO_DATA_VIEWS		= 0x00000002
	};
	void PostToViews(BMessage* msg, uint32 flags = POST_TO_INFO_VIEWS | POST_TO_DATA_VIEWS);
	
	status_t	AddChangeToMenuItem(BMenu* toMenu);
	void		HandleChangeToMsg(BMessage* msg);
	bool		HasStripViews() const;

	/* All this just to build the views when the window is constructed.
	 */
	void						Init();
	void						AddMainMenu();
	bool						AddBackgroundView();
	BPoint						AddButtons(BPoint);
	BPoint						AddActiveTools(BPoint start);
	BPoint						AddToolBars();
	void						AddDataViews(float top);
	SeqMeasureControl*			AddMeasureView(BRect r, float left, float right);
	BView*						NewPrimaryPanel(BRect frame, float divider);
	BView*						NewSecondaryPanel(BRect frame, float divider, BScrollBar** outSb, BRect* outSbF);
	BView* 						NewSplitter(BRect leftTop, BRect rightBottom,
											const char* name,
											uint32 resizeMask, orientation direction) const;
	BView*						NewPrimaryDataView(BRect frame);
	BView*						NewPrimaryInfoView(BRect frame);
	SeqTrackHrzSecondaryInfo*	NewSecondaryInfo(BRect frame);
	SeqTrackHrzSecondaryData*	NewSecondaryData(BRect frame);

	/* The thread that watches the modifier keys and displays the
	 * appropriate active tools.
	 */
	thread_id					mActiveToolSetThread;
	static int32				ActiveToolSetThreadEntry(void* castToTrackWin);
	void						ActiveToolSetChanged();
};

#endif
