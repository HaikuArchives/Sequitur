/* AmFilter.h
 * Copyright (c)1998 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Dianne Hackborn
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Angry Red Planet,
 * at <hackborn@angryredplanet.com> or <hackbod@angryredplanet.com>.
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
 * 1998/09/25		hackbod
 * Created this file
 */

#ifndef AMKERNEL_AMFILTERHOLDER_H
#define AMKERNEL_AMFILTERHOLDER_H

#include <Messenger.h>
#include "AmPublic/AmFilterI.h"
#include "AmKernel/AmInputQueue.h"
class AmPipelineMatrixI;
class AmTrackLookahead;

/*****************************************************************************
 *
 *	AM-FILTER-HOLDER CLASS
 *
 *	AmFilterHolder is the actual implementation of the
 *	AmFilterHolderI interface.
 *
 *****************************************************************************/

class AmFilterHolder : public AmFilterHolderI
{
public:
	/* Things have become a bit cluttery over time:  There should always
	 * be a pipeline_id.  If there's also a track_id, it will be the same
	 * as the pipeline_id.
	 */
	AmFilterHolder(	pipeline_matrix_id matrix,
					track_id track,
					pipeline_id pipeline,
					AmInputTarget* inputTarget);
	
	/* Delete chain of -default- successors (only) as well.
	 */
	virtual void Delete();
	
	// -------------- ArpConfigurableI Interface --------------
	
	/* Pass ArpConfigurableI interface on to the filter's interface.
	 * These perform appropriate locking of the song when called, so
	 * you must not have it read locked yourself when calling.  This
	 * interface can also be watched by observers, so it should be
	 * used for applying any changes to a filter's state.
	 */
	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& views);
	
	// -------------- AmFilterHolderI Interface --------------
	
	virtual AmFilterI*			Filter() const;
	virtual AmFilterAddOn::type	Type() const;
	virtual const pipeline_matrix_id MatrixId() const;
	virtual const pipeline_id	PipelineId() const;	
	virtual const track_id		TrackId() const;
	virtual ArpCRef<AmDeviceI> 	TrackDevice() const;
	virtual bool				IsBypassed() const;
	virtual void				SetBypassed(bool bypass);
	/* This tells me not to report my next in line in the ConnectionAt()
	 * method.
	 */
	virtual bool				IsSuppressingNextInLine() const;
	void						SetSuppressNextInLine(bool suppress);	

	virtual AmTime RealtimeToPulse(bigtime_t time) const;
	virtual bigtime_t PulseToRealtime(AmTime pulse) const;
	virtual status_t GenerateEvents(AmEvent* events);

	virtual uint32				CountConnections() const;
	virtual AmFilterHolderI*	ConnectionAt(uint32 index = 0) const;
	virtual AmFilterHolderI*	FirstConnection() const;
	virtual AmFilterHolderI*	NextInLine() const;
	
	virtual void	FilterChangeNotice();

	// -------------- Internal Interface --------------
	
	void SetFilter(AmFilterI* filter);
	
	void SetRemoved(bool state);
	
	// Quick access to filter flags.
	uint32 FilterFlags() const;
	
	// When generating events, this is who receives them.  If
	// not set, the events will just be deleted.
	ArpRef<AmInputTarget> InputTarget() const;
	
	status_t GetConfigurationDirect(const AmPipelineMatrixI* song, BMessage* values) const;
	status_t PutConfigurationDirect(AmPipelineMatrixI* song, const BMessage* values);
	
	void CommitUndoState();
	
	// This is a messenger to the window currently showing a
	// configuration panel for this filter, to disallow the
	// creation of multiple configuration windows.
	void SetConfigWindow(BMessenger who);
	BMessenger ConfigWindow() const;
	
	// These methods provide BList-like manipulation of the
	// successor list.
	status_t		SetNextInLine(AmFilterHolder* item);
	status_t		AddConnection(AmFilterHolder* item);
	status_t		AddConnection(AmFilterHolder* item, int32 atIndex);
	AmFilterHolder* RemoveConnection(filter_id filterId);
	status_t		RemoveConnections(int32 index, int32 count);
	status_t		RemoveConnections();
	status_t		RemoveSelf();
	status_t		ReplaceConnection(int32 index, AmFilterHolder *newItem);
	void			EmptyConnections();
	/* UGH!  This is getting ugly.  This is for clients that don't want
	 * to be mislead by the suppressNextInLine mechanism, but just want
	 * raw access to the connections.
	 */
	int32			RawCountConnections() const;
	AmFilterHolder*	RawConnectionAt(uint32 index) const;

	AmFilterHolder*	PredecessorAt(int32 index) const;
	/* This is a convenience for any matrix that is removing this
	 * filter from itself.  Break any connections -- from or to --
	 * that this filter has to other pipelines.
	 */
	status_t		BreakConnections(AmPipelineMatrixI* matrix, AmPipelineType type);
	
protected:
	virtual ~AmFilterHolder();

private:
	// copying constructor and assignment are not allowed.
	AmFilterHolder(const AmFilterHolder&);
	AmFilterHolder& operator=(const AmFilterHolder&);
	
	const pipeline_matrix_id	mMatrix;
	const track_id				mTrack;
	const pipeline_id			mPipeline;
	
	// Target for generating events.
	const ArpRef<AmInputTarget> mInputTarget;
	
	// Window showing this filter's configuration panel.
	BMessenger			mConfigWindow;
	
	AmFilterI*			mFilter;

	AmFilterHolder*		mNextInLine;
	BList				mConnections;
#if 0
	AmFilterHolder* mDefSuccessor;
	BList	mSuccessors;
#endif
	BList	mPredecessors;
	AmFilterAddOn::type	mType;			// copied from mFilter->Type()
	bool	mRemoved;
	/* If set to true, skip this filter when processing.
	 */
	bool				mBypassed;
	/* If this is true, I don't report my mNextInLine in the
	 * ConnectionAt() method.
	 */
	bool				mSuppressNextInLine;
};

/*****************************************************************************
 *
 *	ARP-MAKE-FILTER-BITMAP FUNCTION
 *
 *	Returns a displayable bitmap for the selected filter add-on.
 *	This will do any compositing or replacement needed to turn the
 *	raw bitmap generated by the add-on into the final image.
 *
 *****************************************************************************/

BBitmap* ArpMakeFilterBitmap(BBitmap* original, BPoint requestedSize, int32 bgIndex = 0);
BBitmap* ArpMakeFilterBitmap(const AmFilterAddOn* addOn, BPoint requestedSize, int32 bgIndex = 0);
BBitmap* ArpMakeFilterBitmap(const AmFilterI* addOn, BPoint requestedSize, int32 bgIndex = 0);

/*****************************************************************************
 *
 *	ARP-EXEC-FILTERS FUNCTION
 *
 *	This function applies a connected set of filters to an entire
 *	event list.  The input 'list' is the events to be filtered; before
 *	calling this function, they must all have their NextFilter() attribute
 *	set to the first filter that the event will be run through.  (All of
 *	the events do not necessarily need to start off with the same filter.)
 *	The function will repeatedly call HandleEvent() on all the events,
 *	following the filters' successor linking.  This continues until there
 *	are no more events with a useable NextFilter().
 *
 *	The input list must be in time sorted order.  The list returned by the
 *	function is guaranteed to be in time sorted order, and to have every
 *	event with a NextFilter() of either NULL or a filter whose IsDestination()
 *	flag is true.  If 'deleteNULL' is true, events that end up with a
 *	NULL filter are removed from the list and deleted.
 *
 *****************************************************************************/

enum filter_exec_type {
	NORMAL_EXEC_TYPE,
	TOOL_EXEC_TYPE,
	REALTIME_EXEC_TYPE
};

AmEvent* ArpExecFilters(AmEvent* list, filter_exec_type type, bool deleteNULL=false,
						am_filter_params* params = NULL,
						am_tool_filter_params* toolParams = NULL,
						am_realtime_filter_params* realtimeParams = NULL,
						AmEvent* tempoChain = NULL, AmEvent* signatureChain = NULL,
						const AmTrackLookahead* lookahead = NULL);

#endif
