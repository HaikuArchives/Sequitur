/* Copyright (c)1997 - 2001 by Angry Red Planet and Eric Hackborn.
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
 *	- None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * Jan 10, 1999		hackborn
 * Mutated this file from SeqTool, added the convenience functions for
 * the current tool (PrimaryTool(), SetPrimaryTool() etc).
 */
#ifndef AMKERNEL_AMTOOL_H
#define AMKERNEL_AMTOOL_H

#include <vector.h>
#include <be/interface/Menu.h>
#include <be/interface/Bitmap.h>
#include <be/storage/File.h>
#include "AmKernel/AmFileRosterEntryI.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPipelineSegment.h"
#include "AmKernel/MultiLocker.h"
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmTrackRef.h"
class AmGraphicEffect;
class AmPhraseEvent;
class AmSelectionsI;
class AmToolControl;
class AmToolControlList;
class AmToolKeyHandler;
class AmToolSeedI;
class AmToolTarget;

extern const char* SZ_TOOL_KEY;		// AmTool::Key()

/*************************************************************************
 * _AM-TOOL-VIEW-NODE
 *************************************************************************/
class _AmToolViewNode
{
public:
	_AmToolViewNode();
	_AmToolViewNode(const BString& name);
	_AmToolViewNode(const BString& name,
					const BString& key);
	_AmToolViewNode(const _AmToolViewNode& o);

	_AmToolViewNode&	operator=(const _AmToolViewNode &e);

	/* This method is assumed to be called on the root-level node.
	 */
	status_t			SetKey(	const BString& factoryKey,
								const BString& viewKey,
								const BString& key,
								const BMessage* config = NULL);
	status_t			GetKey(	vector<BString> path,
								BString& outKey,
								BMessage* config) const;
	/* This method is assumed to be called on the root-level node.
	 */
	status_t			Key(uint32 index, uint32* count,
							vector<BString>& outPath,
							BString& outKey) const;
	
	status_t			Flatten(BMessage* into) const;
	status_t			Unflatten(const BMessage* from);
	
	void				Print(int32 tabs = 0) const;
	
private:
	/* The name will be either the factory key or the view key,
	 * depending upon where this node is in the hierarchy (or
	 * empty, if this node is the root).
	 */
	BString			mName;
	BString			mKey;
	BMessage		mConfig;
	
	vector<_AmToolViewNode> mChildren;

	const _AmToolViewNode*	FindChild(const BString& name) const;
	_AmToolViewNode*		FindChild(const BString& name);
};

/*************************************************************************
 * _AM-INOUT-ENTRY
 *************************************************************************/
class _AmInOutEntry
{
public:
	_AmInOutEntry();
	_AmInOutEntry(const _AmInOutEntry& o);
	_AmInOutEntry(AmFilterHolderI* holder);

	_AmInOutEntry&		operator=(const _AmInOutEntry& o);
	
	AmFilterHolderI*	mHolder;
	AmTrack*			mTrack;
	AmPhraseEvent*		mTopPhrase;
};

/*************************************************************************
 * AM-TOOL
 * This is the generic interface for all tools.  There are five basic
 * responsibilities subclasses have: answer a Name(), install an image in
 * the constructor, then implement the three mouse messages to do something
 * interesting.
 *
 * The complete editing interface available to tools is the API's of the
 * AmToolTarget (used mainly to abstract any view-specific behaviour), the
 * AmTrack, and to some degree the AmSong.
 *************************************************************************/
class AmTool : public AmPipelineMatrixI,
			   public AmFileRosterEntryI
{
public:
	AmTool(	const char* label, const char* key, const char* toolTip,
			const char* author, const char* email, bool readOnly = false); 
	AmTool(const BMessage& config, bool readOnly, const char* filePath);
	virtual ~AmTool();

	// --------------------------------------------------------
	// AM-PIPELINE-MATRIX-I INTERFACE
	// --------------------------------------------------------
	virtual void				AddRef() const;
	virtual void				RemoveRef() const;
	virtual bool				ReadLock() const;
	virtual bool				WriteLock(const char* name = NULL);
	virtual bool				ReadUnlock() const;
	virtual bool				WriteUnlock();

	virtual void				SetDirty(bool dirty = true);

	virtual tool_id				Id() const;
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

	// --------------------------------------------------------
	// AM-FILE-ROSTER-ENTRY-I INTERFACE
	// --------------------------------------------------------
	virtual BString			Label() const;
	virtual BString			Key() const;
	virtual BString			ShortDescription() const;
	virtual BString			Author() const;
	virtual BString			Email() const;
	virtual bool			IsValid() const;
	virtual BString			LocalFileName() const;
	virtual BString			LocalFilePath() const;
	virtual status_t		WriteTo(BMessage& config) const;

	// --------------------------------------------------------
	// AM-TOOL INTERFACE
	// --------------------------------------------------------
	const char*			ToolTip() const;
	/* Answer descriptions for this tool.  The short description is
	 * a brief explanation to be displayed in the manage tools window.
	 * The long description is a full, HTML-formatted explanation.
	 */
	void				LongDescription(BString& str) const;

	const BBitmap*		Icon() const;
	BRect				IconBounds() const;
	
	/* Subclasses override these messages to deal with mouse notifications
	 * coming from a view.
	 */
	virtual void		MouseDown(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where);

	virtual void		MouseUp(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where);

	virtual void		MouseMoved(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where,
									uint32 code);
	/* This is the view that the tool is currently acting on.  Defer
	 * this to the seed, if any.
	 */
	virtual void		DrawOn(BView* view, BRect clip);

	/* The AmToolSeedI is an object for turning mouse gestures into
	 * MIDI events to send to the pipeline.  A tool can have a seed
	 * for each installed view.
	 */
	status_t			GetSeed(	uint32 index,
									BString& outFactoryKey,
									BString& outViewKey,
									BString& outSeedKey) const;
	status_t			GetSeed(	const BString& factoryKey,
									const BString& viewKey,
									BString& outSeedKey,
									BMessage* outConfig = NULL) const;
	status_t			SetSeed(	const BString& factoryKey,
									const BString& viewKey,
									const BString& seedKey,
									const BMessage* config = NULL);
	const AmToolSeedI*	CurrentSeed() const;

	AmToolControlList*	NewControlList() const;
	/* The AmGraphicEffect is just a bit of polish that tools can
	 * have.  A tool can have a graphic effect for each installed view.
	 */
	status_t			Graphic(	uint32 index,
									BString& outFactoryKey,
									BString& outViewKey,
									BString& outGraphicKey) const;
	AmGraphicEffect*	NewGraphic(	const BString& factoryKey,
									const BString& viewKey) const;
	status_t			SetGraphic(	const BString& factoryKey,
									const BString& viewKey,
									const BString& graphicKey,
									const BMessage* config = NULL);
	/* Tools can optionally return a handler for key events.
	 */
	AmToolKeyHandler*	NewToolKeyHandler() const;
	void				KeyDown(AmSongRef songRef,
								AmToolTarget* target, char byte);

	// --------------------------------------------------------
	// MODIFICATION
	// --------------------------------------------------------
	status_t			Prepare(const BString& factoryKey,
								const BString& viewKey);
	void				SetLabel(const char* label);
	void				SetToolTip(const char* toolTip);
	void				SetKey(const char* key);
	void				SetAuthor(const char* author);
	void				SetEmail(const char* email);
	void				SetIsValid(bool isValid);
	void				SetShortDescription(const char* s);
	void				SetLongDescription(const char* s);
	BBitmap*			Icon();
	status_t			PushPipeline();
	status_t			PopPipeline();
	
	bool				IsReadOnly() const;
	status_t			ReadFrom(const BMessage& config);
	AmTool*				Copy() const;
	void				Print() const;

	/* This is priviledged accessing to my internal tool control list.
	 * Used by clients that make modifications to my controls.
	 */
	AmToolControlList*	ControlList() const;

protected:
	/* This will move to private when the tools are finished.
	 * Currently the AmPropertiesTool is holding it here.
	 */
	BBitmap*			mIcon;
	
private:
	int32					mRefCount;
	mutable MultiLocker		mLock;
	BString					mLabel;
	BString					mKey;
	BString					mAuthor;
	BString					mEmail;
	bool					mIsValid;
	mutable BString			mToolTip;
	AmToolSeedI*			mCurrSeed;
	vector<AmPipelineSegment> mPipelines;
	BString					mShortDescription;
	BString					mLongDescription;
	uint32					mActions;
	AmNotifier				mNotifier;
	_AmToolViewNode			mSeedRoot;
	_AmToolViewNode			mGraphicRoot;
	mutable AmToolControlList*	mControlList;
	bool					mReadOnly;
	BString					mFilePath;

	am_filter_params		mParams;
	bool					mBuiltDynamicParams;
	BPoint					mOriginPt;
	AmTime					mOriginTime;
	int32					mOriginYValue;
	AmRange					mSelectionRange;
	AmSelectionsI*			ProcessSelections(	AmSongRef songRef, AmToolTarget* target,
												AmSelectionsI* selections, BPoint where);
	AmSelectionsI*			ProcessSelections(	AmTrack* track, uint32 trackIndex,
												AmToolTarget* target,
												AmSelectionsI* oldSelections, BPoint where,
												AmSelectionsI* newSelections);
	/* All my input and output filters are cached, in correct
	 * order, in the MouseDown(), then released in the MouseUp();
	 * This is support for ProcessSelections().
	 */
	vector<_AmInOutEntry>	mInEntries;
	vector<_AmInOutEntry>	mOutEntries;
	void					CacheInOutEntries();
	_AmInOutEntry*			InEntry(track_id tid);
	_AmInOutEntry*			OutEntry(filter_id fid);	
	void					FillToolParams(	am_tool_filter_params& params,
											track_id tid, AmToolTarget* target,
											BPoint where);
	void					CleanToolParams();
	
	int32					PipelineIndex(pipeline_id id) const;

	status_t				FlattenConnections(BMessage* into) const;
	status_t				AddConnectionInfo(AmFilterHolderI* connection, BMessage& msg) const;
	status_t				UnflattenConnections(const BMessage* into);
	status_t				UnflattenConnection(int32 source_pi, int32 source_fi,
												int32 dest_pi, int32 dest_fi);
};

#endif 

