/* SeqPipelineMatrixView.h
 * Copyright (c)2001 by Angry Red Planet.
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
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.04.11		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef SEQUITUR_SEQPIPELINEMATRIXVIEW_H
#define SEQUITUR_SEQPIPELINEMATRIXVIEW_H

#include <vector>
#include <app/Messenger.h>
#include <interface/View.h>
#include "ArpKernel/ArpLineArrayCache.h"
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmPipelineMatrixRef.h"
#include "AmPublic/AmSongRef.h"
#include "BeExp/ToolTip.h"
class AmFilterHolder;
class _SeqFilterCell;
class _SeqFilterGrid;
class _SeqFilterLoc;

enum {
	SEQ_SUPPRESS_BACKGROUND		= 0x00000001,
	SEQ_FORCE_VIEW_COLOR		= 0x00000002	// Set automatically by ForceViewColor()
};

/*************************************************************************
 * SEQ-PIPELINE-MATRIX-VIEW
 * This class displays all pipelines from a pipeline matrix in a single
 * view.
 *************************************************************************/
class SeqPipelineMatrixView : public BView,
							  public BToolTipable
{
public:
	SeqPipelineMatrixView(	BRect frame,
							const char* name,
							AmPipelineMatrixRef matrixRef,
							AmPipelineType pipelineType,
							uint32 flags = 0);
	virtual ~SeqPipelineMatrixView();

	virtual void	SetMatrixRef(AmPipelineMatrixRef matrixRef);
	/* If true, then when a filter property menu is invoked, it
	 * will let the user set the Hide Properties property.
	 */
	void			SetShowProperties(bool state);
	
	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void	Draw(BRect clip);
	virtual void	FrameResized(float new_width, float new_height);
	virtual	void	GetPreferredSize(float *width, float *height);
	virtual	void	KeyDown(const char *bytes, int32 numBytes);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage* dragMessage);
	virtual	void	MouseUp(BPoint where);

	void			SetHorizontalScrollBar(BScrollBar* sb);
	void			InitializeScrollBars();
	void			ClearMouseOver();
	/* Normally I grab my parent's view colour in AttachedToWindow(),
	 * but this will force me to use the supplied colour.
	 */
	void			ForceViewColor(rgb_color c);
	
	virtual status_t GetToolTipInfo(BPoint where, BRect* out_region,
									BToolTipInfo* out_info = 0);

	/* Intelligent clients -- well, sneaky ones -- might circumvent
	 * my update mechanism for rebuilding the metric info.  If someone
	 * does that, they should call this method before I redraw.
	 */
	void					FillMetrics();
	void					FillMetrics(const AmPipelineMatrixI* matrix);

protected:
	AmPipelineType			mPipelineType;

	virtual void			ApplyToTrack(pipeline_id pid, filter_id fid)	{ }
	void					RemoveFilter(pipeline_id pid, filter_id fid);
	
private:
	typedef	BView			inherited;
	AmPipelineMatrixRef		mMatrixRef;
	_SeqFilterGrid*			mGrid;

	/* A convenience object for drawing all my lines.
	 */
	ArpLineArrayCache		mLines;
	BScrollBar*				mHsb;
	int32					mDownButtons;
	/* I cache the filter slot that the mouse is over.
	 */
	_SeqFilterLoc*			mOverLoc;
	/* I cache a different filter slot when the mouse
	 * is over a potential connection target.
	 */
	_SeqFilterLoc*			mConnLoc;
	BPoint					mDownPoint;
	rgb_color				mViewColor;
	BPoint					mFilterSize;
	_SeqFilterLoc*			mDragLoc;
	enum {
		DRAG_LEFT			= 1,
		DRAG_CENTER			= 2,
		DRAG_RIGHT			= 3,
		NO_DRAG				= 4
	};
	uint32					mDragCode;
	uint32					mFlags;
	BPoint					mConnectionPt;
		
	bool					mShowProperties;
	
	void				SetBypassed(pipeline_id pid, filter_id fid);
	void				SetHideProperties(pipeline_id pid, filter_id fid);

	/* Answer true if this view can accept the addon, false otherwise.
	 */
	bool				Accept(const BMessage* addon) const;
	bool				Accept(AmFilterAddOn::type type) const;
	/* Answer true if I can drag filters (for purposes of moving and
	 * removing them), false otherwise.
	 */
	bool				CanDrag() const;

	void				DrawOn(BView* view, BRect clip);
	void				TrackChangeReceived(BMessage* msg);

	void	InitializeHsb();
	void	SetHsbRange();
	void	SetHsbSteps();
	float	HorizontalWidth() const;

	void	SetVsbRange();
	void	SetVsbSteps();

	void				HandlePropertyMenu(	_SeqFilterCell* cell,
											BPoint where,
											BMessage* archivedFilter = NULL);

	void				StartAddOnDrag(		const AmPipelineMatrixI* matrix,
											_SeqFilterCell* cell,
											int32 buttons);
	void				HandleAddOnDrag(BPoint where);

	void				HandleAddOnDrop(const BMessage* msg, const BMessage* addon);
	bool				LockedHandleAddOnDrop(	AmPipelineMatrixI* matrix,
												const BMessage* msg,
												const BMessage* archive);
	uint32				DroppedMenu(BPoint where) const;

	void				HandleConnectionDrop(pipeline_id pid, filter_id fid, BPoint where);
	
	void				HandleChangeToMsg(const BMessage* msg);
	void				HandleFilterPropertiesMsg(const BMessage* msg);

	BRect				BoundsForPipeline(int32 pipelineIndex) const;
	float				LeftBracket() const;
	float				RightBracket() const;
	float				Border() const;
};

/*************************************************************************
 * SEQ-SONG-PIPELINE-MATRIX-VIEW
 * This class is for when the pipeline matrix is a song.
 *************************************************************************/
class SeqSongPipelineMatrixView : public SeqPipelineMatrixView
{
public:
	SeqSongPipelineMatrixView(	BRect frame,
								const char* name,
								AmPipelineMatrixRef matrixRef,
								AmPipelineType pipelineType,
								AmSongRef songRef);

	virtual	void		AttachedToWindow();
	virtual	void		DetachedFromWindow();

protected:
	virtual void		ApplyToTrack(pipeline_id pid, filter_id fid);

private:
	typedef SeqPipelineMatrixView		inherited;
	AmSongRef		mSongRef;

	void			HandleApplyToTrackMsg(AmTrack* track, filter_id id);
	void			HandleApplyToTrackMsg(	AmTrack* track, AmFilterI* filter,
											AmPhraseEvent* topPhrase, AmEvent* event,
											AmPhrase* parent);
};

#endif
