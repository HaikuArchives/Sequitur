/* AmPipelineSegment.h
 * Copyright (c)1998-2001 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.02.05		hackborn
 * Extracted the pipeline from the AmTrack file.
 */
#ifndef AMKERNEL_AMPIPELINESEGMENT_H
#define AMKERNEL_AMPIPELINESEGMENT_H

#include <BeExp/UndoContext.h>
#include "AmPublic/AmDefs.h"
class AmFilterHolderI;
class AmFilterHolder;
class AmFilterAddOn;
class AmPipelineMatrixI;
class AmTrack;

/***************************************************************************
 * AM-PIPELINE-SEGMENT
 * This class bundles up a series of filters.  The name might be a little
 * confusing:  Why is it a pipeline segment and not a pipeline?  Actually,
 * it's both.  Currently, a 'pipeline' is a bit of an abstract concept.
 * Technically a pipeline is composed of 1 or more pipeline segments --
 * meaning that there is some unspecified container class for the segments.
 * Right now, a track is a conceptual pipeline, since it contains pipeline
 * segments.  A pipeline segment is also a conceptual pipeline, since it
 * contains one pipeline segment.  Did that actually answer the question?
 * Bottom line is I don't think it's necessary to have the notion of pipeline
 * defined separately, so right now there's just the segment.
 ****************************************************************************/
/* The flags for various pipeline operations.
 */
enum {
	AM_PLAIN		= 0x00000001,	// Determine which part of pipeline to use
	AM_SOURCE		= 0x00000002,
	AM_DESTINATION	= 0x00000004,
	
	AM_APPEND		= 0x00000010	// Append to pipeline (instead of replace)
};

class AmPipelineSegment
{
public:
	AmPipelineSegment();
	AmPipelineSegment(const AmPipelineSegment& o);
	/* A constructor that combines the Setup() and UnflattenFilters() calls.
	 */
	AmPipelineSegment(	AmPipelineType type, AmPipelineMatrixI* matrix, AmTrack* track,
						const BMessage* filters, uint32 flags);
	virtual ~AmPipelineSegment();

	/* Either value can be null, but a NULL matrix means user's won't
	 * be able to set the properties of any of the filters in the pipeline.
	 * A NULL track means undo is not available.
	 */
	void				Setup(AmPipelineMatrixI* matrix, AmTrack* track, AmPipelineType type);
	
	AmFilterHolderI*	Filter(filter_id id) const;
	AmFilterHolderI*	Head() const;
	AmFilterHolderI*	Tail() const;
	AmFilterHolderI*	Source() const;
	AmFilterHolderI*	Destination() const;

	/* Insert a filter before the given index.  If the index is less than 0, insert
	 * at the end of the pipeline.  Fail if the addon is either a source or destination --
	 * those must be added with SetSource() and SetDestination(), respectively.
	 */
	status_t			InsertFilter(AmFilterAddOn* addon, int32 beforeIndex, const BMessage* config);
	status_t			ReplaceFilter(AmFilterAddOn* addon, int32 atIndex, const BMessage* config);
	status_t			RemoveFilter(filter_id id, const char* undoName = "Remove Filter", bool merge = false);
	status_t			SetSource(AmFilterAddOn* addon, const BMessage* config);
	status_t			SetDestination(AmFilterAddOn* addon, const BMessage* config);
	status_t			FlattenFilters(BMessage* into, uint32 flags) const;
	status_t			UnflattenFilters(const BMessage* from, uint32 flags);
	status_t			Clear(uint32 flags);

	status_t			MakeConnection(	AmPipelineMatrixI* matrix, pipeline_id fromPid,
										AmPipelineType fromType, filter_id fromFid,
										AmPipelineSegment* toPipeline, filter_id toFid,
										const void* undoOwner = NULL, BUndoContext* undo = NULL);
	status_t			BreakConnection(AmPipelineMatrixI* matrix, pipeline_id fromPid,
										AmPipelineType fromType, filter_id fromFid,
										AmPipelineSegment* toPipeline, filter_id toFid,
										const void* undoOwner = NULL, BUndoContext* undo = NULL);

	/* Low-level pipeline manipulation. */
	void				InsertFilter(AmFilterHolder* where, AmFilterHolder* which,
									 BUndoContext* undo, const char* undoName = "Add ");

	/* This is a special method that bypasses undo.  It's
	 * a convenience for very knowledgeable clients.
	 */
	status_t			RemoveConnections();
	
	AmPipelineSegment&	operator=(const AmPipelineSegment& o);

	void				Print(uint32 tabs = 0) const;
	
private:
	AmPipelineType		mType;
	AmPipelineMatrixI*	mMatrix;	// raw pointers so there are no
	AmTrack*			mTrack;		// circular references.
	AmFilterHolder*		mHead;
	friend class		AmFilterConnectionUndo;
	friend class		AmTrackFilterUndo;
	
	void PrevAndNextFor(int32 beforeIndex, AmFilterHolder** prev, AmFilterHolder** next) const;

	/* Low-level pipeline manipulation. */
	void				RemoveFilter(	AmFilterHolder* pred, AmFilterHolder* which,
										BUndoContext* undo, const char* undoName = "Remove ");
	bool				break_pred_connection(	AmFilterHolder* toFilter,
												BUndoContext* undo);
	bool				break_next_connection(	AmFilterHolder* fromFilter,
												BUndoContext* undo);

	status_t			MakeConnection(	AmPipelineMatrixI* matrix, pipeline_id fromPid,
										AmPipelineType fromType,
										AmFilterHolder* fromFilter, AmFilterHolder* toFilter,
										const void* undoOwner, BUndoContext* undo);
	status_t			BreakConnection(AmPipelineMatrixI* matrix, pipeline_id fromPid,
										AmPipelineType fromType,
										AmFilterHolder* fromFilter, AmFilterHolder* toFilter,
										const void* undoOwner, BUndoContext* undo);
};

#endif 
