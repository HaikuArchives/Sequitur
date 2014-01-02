/* AmMultiFilter.h
 * Copyright (c)2001 by Eric Hackborn
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
 * 2001.05.11			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef AMKERNEL_AMMULTIFILTER_H
#define AMKERNEL_AMMULTIFILTER_H

#include <vector.h>
#include <app/Message.h>
#include <interface/View.h>
#include <support/String.h>
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmKernel/AmFileRosterEntryI.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPipelineSegment.h"

/*****************************************************************************
 * AM-MULTI-FILTER
 * A filter made up of other filters.
 *****************************************************************************/
class AmMultiFilterAddOn;

class AmMultiFilter : public AmFilterI,
					  public AmPipelineMatrixI
{
public:
	AmMultiFilter(	AmMultiFilterAddOn* addon,
					AmFilterHolderI* holder,
					const BMessage& definition,
					const BMessage* settings);
	virtual ~AmMultiFilter();

	/* Deliver these messages to all the filters I contain.
	 */	
	virtual AmEvent*	StartSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params = NULL);
	virtual AmEvent*	FinishSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params = NULL);
	
	virtual void			Start(uint32 context);
	virtual void			Stop(uint32 context);

	virtual AmEvent*		HandleEvent(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*		HandleToolEvent(AmEvent* event,
											const am_filter_params* params = NULL,
											const am_tool_filter_params* toolParams = NULL);
	virtual AmEvent*		HandleRealtimeEvent(AmEvent* event,
												const am_filter_params* params = NULL,
												const am_realtime_filter_params* realtimeParams = NULL);
	virtual AmEvent*		HandleBatchEvents(AmEvent* event, const am_filter_params* params = NULL);
	virtual AmEvent*		HandleBatchToolEvents(	AmEvent* event,
													const am_filter_params* params = NULL,
													const am_tool_filter_params* toolParams = NULL);
	virtual AmEvent*		HandleBatchRealtimeEvents(	AmEvent* event,
														const am_filter_params* params = NULL,
														const am_realtime_filter_params* realtimeParams = NULL);

	virtual status_t 		GetConfiguration(BMessage* values) const;
	virtual status_t		PutConfiguration(const BMessage* values);
	virtual status_t		Configure(ArpVectorI<BView*>& panels);
	virtual void			ConfigWindowOpened();
	virtual void			ConfigWindowClosed();

	// --------------------------------------------------------
	// AM-PIPELINE-MATRIX-I INTERFACE
	// --------------------------------------------------------
	virtual void				AddRef() const;
	virtual void				RemoveRef() const;
	virtual bool				ReadLock() const;
	virtual bool				WriteLock(const char* name);
	virtual bool				ReadUnlock() const;
	virtual bool				WriteUnlock();

	virtual void				SetDirty(bool dirty);

	virtual pipeline_matrix_id	Id() const;
	virtual uint32				CountPipelines() const;
	virtual pipeline_id			PipelineId(uint32 pipelineIndex) const;
	virtual status_t			PipelineHeight(uint32 pipelineIndex, float* outHeight) const;

	virtual const BUndoContext*	UndoContext() const;
	virtual BUndoContext*		UndoContext();

	virtual AmFilterHolderI*	Filter(	pipeline_id id,
										AmPipelineType type,
										filter_id filterId = 0) const;
	virtual status_t			InsertFilter(	AmFilterAddOn* addon,
												pipeline_id id,
												AmPipelineType type,
												int32 beforeIndex = -1,
												const BMessage* config = 0);
	virtual status_t			ReplaceFilter(	AmFilterAddOn* addon,
												pipeline_id id,
												AmPipelineType type,
												int32 atIndex = 0,
												const BMessage* config = 0);
	virtual status_t			RemoveFilter(	pipeline_id id,
												AmPipelineType type,
												filter_id filterId);
	virtual status_t			MakeConnection(	pipeline_id fromPid,
												AmPipelineType fromType,
												filter_id fromFid,
												pipeline_id toPid,
												AmPipelineType toType,
												filter_id toFid);
	virtual status_t			BreakConnection(pipeline_id fromPid,
												AmPipelineType fromType,
												filter_id fromFid,
												pipeline_id toPid,
												AmPipelineType toType,
												filter_id toFid);
	virtual void				PipelineChanged(pipeline_id id, AmPipelineType type);
	virtual void				FilterChanged(pipeline_id id, AmPipelineType type);

	virtual status_t			AddMatrixPipelineObserver(pipeline_id id, BHandler* handler);
	virtual status_t			AddMatrixFilterObserver(pipeline_id id, BHandler* handler);
	virtual status_t			RemoveMatrixObserver(pipeline_id id, BHandler* handler);

	virtual status_t			WriteTo(BMessage& config) const;

	status_t					PushPipeline();
	status_t					PopPipeline();

	uint32						CountOutputs() const;

	virtual void				TurnOffWtfHack();

private:
	AmMultiFilterAddOn*		mAddOn;
	AmFilterHolderI*		mHolder;

	bool					mWtfHack;

	vector<AmPipelineSegment>	mPipelines;
	AmNotifier				mNotifier;

	AmEvent*				UnifiedHandleEvent(	AmEvent* event, filter_exec_type type,
												am_filter_params* params = NULL,
												am_tool_filter_params* toolParams = NULL,
												am_realtime_filter_params* realtimeParams = NULL);

	int32					PipelineIndex(pipeline_id id) const;

	status_t				ReadFrom(const BMessage& msg);
	status_t				FlattenConnections(BMessage* into) const;
	status_t				AddConnectionInfo(AmFilterHolderI* connection, BMessage& msg) const;
	status_t				UnflattenConnections(const BMessage* into);
	status_t				UnflattenConnection(int32 source_pi, int32 source_fi,
												int32 dest_pi, int32 dest_fi);
};

/*****************************************************************************
 * AM-MULTI-FILTER-ADD-ON
 *****************************************************************************/
class AmMultiFilterAddOn : public AmFilterAddOn,
						   public AmFileRosterEntryI
{
public:
	AmMultiFilterAddOn();
	AmMultiFilterAddOn(const char* author, const char* email);
	AmMultiFilterAddOn(const AmMultiFilterAddOn& o);
	AmMultiFilterAddOn(const BMessage& config, bool readOnly, const char* filePath);
	virtual ~AmMultiFilterAddOn();

	AmMultiFilterAddOn&		operator=(const AmMultiFilterAddOn& o);

	BString					UniqueName() const;
	bool					IsReadOnly() const;
	AmMultiFilterAddOn*		Copy() const;

	// --------------------------------------------------------
	// AM-FILTER-ADD-ON INTERFACE
	// --------------------------------------------------------
	virtual VersionType		Version(void) const;
	virtual BString			Name() const;
	virtual BString			Key() const;
	virtual int32			MaxConnections() const;
	virtual BString			ShortDescription() const;
	virtual void			LongDescription(BString& name, BString& str) const;
	virtual void			GetVersion(int32* major, int32* minor) const;
	virtual type			Type() const;
	virtual subtype			Subtype() const;
	virtual BBitmap*		Image(BPoint requestedSize) const;
	virtual AmFilterI*		NewInstance(AmFilterHolderI* holder,
										const BMessage* config = NULL);

	// --------------------------------------------------------
	// AM-FILE-ROSTER-ENTRY-I INTERFACE
	// --------------------------------------------------------
	virtual file_entry_id	Id() const;
	virtual BString			Label() const;
	virtual BString			Author() const;
	virtual BString			Email() const;
	virtual bool			IsValid() const;
	virtual BString			LocalFileName() const;
	virtual BString			LocalFilePath() const;
	virtual status_t		WriteTo(BMessage& config) const;

	// --------------------------------------------------------
	// MODIFICATION
	// --------------------------------------------------------
	void					SetDefinition(const BMessage& definition);
	void					SetLabel(const char* s);
	void					SetKey(const char* s);
	void					SetAuthor(const char* author);
	void					SetEmail(const char* email);
	void					SetIsValid(bool isValid);
	void					SetShortDescription(const char* s);
	void					SetLongDescription(const char* s);
	void					SetVersion(int32 major, int32 minor);
	const BBitmap*			Icon(BPoint size) const;
	void					SetIcon(const BBitmap* icon);
	/* This is for the editor -- the normal long description method
	 * annotates the description with info from my superclass.  This
	 * gives direct access to my exact long description.
	 */
	const char*				LongDescriptionContents() const;

private:
	typedef AmFilterAddOn	inherited;

	BMessage				mDefinition;
	BString					mUniqueName;
	BString					mLabel;
	BString					mAuthor;
	BString			 		mEmail;
	bool					mIsValid;
	int32					mMaxConnections;
	BString					mShortDescription;
	BString					mLongDescription;
	int32					mMajorVersion, mMinorVersion;
	BBitmap*				mIcon;
	bool					mReadOnly;
	BString					mFilePath;

	status_t				ReadFrom(const BMessage& config);
};

#endif
