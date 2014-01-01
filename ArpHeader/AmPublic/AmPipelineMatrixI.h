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
 * 2001.02.16			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMPUBLIC_AMPIPELINEMATRIXI_H
#define AMPUBLIC_AMPIPELINEMATRIXI_H

#include <be/app/Message.h>
#include <UndoContext.h>
#include "AmPublic/AmDefs.h"
class AmFilterAddOn;
class AmFilterHolderI;

/* Matrices can be identified through a unique ID.  This is used in
 * various parts of the system, for example:  Positively identifying
 * a matrix, passing references to the matrix via BMessages, etc.
 */
#define MATRIX_ID_STR		"matrix_id"

/*****************************************************************************
 * AM-PIPELINE-MATRIX-I
 * This class defines the interface for accessing and editing anything that
 * contains a series of pipelines.
 *****************************************************************************/
class AmPipelineMatrixI
{
public:
	virtual ~AmPipelineMatrixI()		{ }

	virtual void				AddRef() const = 0;
	virtual void				RemoveRef() const = 0;
	virtual bool				ReadLock() const = 0;
	virtual bool				WriteLock(const char* name = NULL) = 0;
	virtual bool				ReadUnlock() const = 0;
	virtual bool				WriteUnlock() = 0;

	virtual void				SetDirty(bool dirty = true) = 0;

	virtual pipeline_matrix_id	Id() const = 0;
	virtual uint32				CountPipelines() const = 0;
	/* Answer the ID for the pipeline at the supplied index.
	 */
	virtual pipeline_id			PipelineId(uint32 pipelineIndex) const = 0;
	/* Answer the pixel height of the pipeline at the given index.
	 */
	virtual status_t			PipelineHeight(uint32 pipelineIndex, float* outHeight) const = 0;
	
	virtual const BUndoContext*	UndoContext() const = 0;
	virtual BUndoContext*		UndoContext() = 0;

	/* Track down the requested filter in the supplied pipeline.  Index
	 * is the pipeline's index in the matrix.  Type is the type of pipeline.
	 * An id of 0 answers the head filter.
	 */
	virtual AmFilterHolderI*	Filter(	pipeline_id id,
										AmPipelineType type,
										filter_id filterId = 0) const = 0;
	/* Insert the filter into the filter list before the given index (i.e., a
	 * beforeIndex value of 0 will insert at the head, 1 will insert between
	 * the head and the second filter, etc).  If the index is invalid (-1, for
	 * example), add the filter at the end of the list.  If the filter is an output
	 * filter, ignore the position and replace the current output filter.  The
	 * filter is generated from the addon.  If the filter is a copy of an existing
	 * filter, the client should request the config from the existing filter and
	 * supply that here.
	 */
	virtual status_t			InsertFilter(	AmFilterAddOn* addon,
												pipeline_id id,
												AmPipelineType type,
												int32 beforeIndex = -1,
												const BMessage* config = 0) = 0;
	/* Replace the filter at the given index with a new instance of addon.  If
	 * the pipeline type is a single filter endpoint -- either a SOURCE or
	 * DESTINATION, and the addon matches, then always succeed.  Otherwise, this
	 * will only succeed if the specified pipeline actually has a filter at the
	 * index supplied, otherwise B_ERROR will be returned.
	 */
	virtual status_t			ReplaceFilter(	AmFilterAddOn* addon,
												pipeline_id id,
												AmPipelineType type,
												int32 atIndex = 0,
												const BMessage* config = 0) = 0;
	/* Track down the filter at filter ID and remove it.  Send out a notification
	 * that I've changed.  If I don't actually have the filter, send out no notification
	 * and respond with B_ERROR.
	 */
	virtual status_t			RemoveFilter(	pipeline_id id,
												AmPipelineType type,
												filter_id filterId) = 0;

	/* Add and remove a connection from a filter to a filter.
	 */
	virtual status_t			MakeConnection(	pipeline_id fromPid,
												AmPipelineType fromType,
												filter_id fromFid,
												pipeline_id toPid,
												AmPipelineType toType,
												filter_id toFid) = 0;
	virtual status_t			BreakConnection(pipeline_id fromPid,
												AmPipelineType fromType,
												filter_id fromFid,
												pipeline_id toPid,
												AmPipelineType toType,
												filter_id toFid) = 0;

	/* If a client makes a change that does not automatically generate
	 * notification, they can use this method during a Lock() / Unlock()
	 * session to generate the notification.  Right now, pretty much
	 * everything generates a notification.  Setting a filter to bypassed
	 * is the only thing that doesn't.  PipelineChanged() is used for
	 * changes in a pipeline structure -- filters added, removed, etc.
	 * FilterChanged() is used for changes to an actual filter's values.
	 * Note that currently, setting a filter to bypassed is considered
	 * a structure change.  The pipeline type in either case can be
	 * _NUM_PIPELINE to indicate that notice should be sent to all pipeline
	 * segments.
	 */
	virtual void				PipelineChanged(pipeline_id id, AmPipelineType type) = 0;
	virtual void				FilterChanged(pipeline_id id, AmPipelineType type) = 0;
	
	virtual status_t			AddMatrixPipelineObserver(pipeline_id id, BHandler* handler) = 0;
	virtual status_t			AddMatrixFilterObserver(pipeline_id id, BHandler* handler) = 0;
	virtual status_t			RemoveMatrixObserver(pipeline_id id, BHandler* handler) = 0;
};

#endif
