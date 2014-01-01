
#ifndef AMPUBLIC_AMFILTERI_H
#define AMPUBLIC_AMFILTERI_H

#include <be/experimental/ResourceSet.h>
#include <be/interface/Bitmap.h>
#include <be/support/List.h>
#include <be/support/String.h>

#include <ArpBuild.h>

#include <AmPublic/AmEvents.h>
#include <AmPublic/AmDeviceI.h>
#include <ArpKernel/ArpConfigurableI.h>
#include <ArpKernel/ArpConfigureWatch.h>
#include <ArpKernel/ArpRef.h>
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
class BMessage;
class BView;
class AmFilterHolderI;
class AmFilterI;
class AmFilterAddOnHandle;
class AmMotionI;

/* Names for various string constants used in filter
 * messages.  These are the message values filled in
 * by AmFilterAddOn::GetArchiveTemplate() and
 * AmFilterI::Archive().
 */
extern const char* SZ_FILTER_NAME;					// AmFilterAddOn::Name()
extern const char* SZ_FILTER_KEY;					// AmFilterAddOn::ClassName()
extern const char* SZ_FILTER_MAX_CONNECTIONS;		// AmFilterAddOn::MaxConnections()
extern const char* SZ_FILTER_SHORT_DESCRIPTION;		// AmFilterAddOn::ShortDescription()
extern const char* SZ_FILTER_LONG_DESCRIPTION;		// AmFilterAddOn::LongDescription()
extern const char* SZ_FILTER_LONG_DESCRIPTION_NAME;	// AmFilterAddOn::LongDescription()
extern const char* SZ_FILTER_AUTHOR;				// AmFilterAddOn::Author()
extern const char* SZ_FILTER_EMAIL;					// AmFilterAddOn::Email()
extern const char* SZ_FILTER_MAJOR_VERSION;			// AmFilterAddOn::GetVersion()
extern const char* SZ_FILTER_MINOR_VERSION;			// AmFilterAddOn::GetVersion()
extern const char* SZ_FILTER_TYPE;					// AmFilterAddOn::Type()
extern const char* SZ_FILTER_SUBTYPE;				// AmFilterAddOn::SubType()
extern const char* SZ_FILTER_ASSOC_TYPE;			// AmFilterAddOn::ClassNameForType()
extern const char* SZ_FILTER_IMAGE;					// AmFilterAddOn::Image()
extern const char* SZ_FILTER_LABEL;					// AmFilterI::Label()
extern const char* SZ_FILTER_ARCHIVE;				// All of the above

enum {
	AM_ARCHIVE_IMAGE			= 1<<0,
	AM_ARCHIVE_DESCRIPTION		= 1<<1,
	
	AM_ARCHIVE_ALL				= 0xFFFF
};
	
/*****************************************************************************
 *
 *	AM-FILTER-ADD-ON CLASS
 *
 *	The AmFilterAddOn class is the public definition of a type of
 *	filter add-on.
 *
 *****************************************************************************/

class AmFilterAddOn : public ArpRefable
{
public:
	AmFilterAddOn(const void* cookie);
	
	virtual void AddReference(ArpBaseRef* owner=NULL) const;
	virtual void RemReference(ArpBaseRef* owner=NULL) const;
	
	// --------------------------------------------------------
	// INTERFACE VERSIONING.
	// --------------------------------------------------------
	
	/* Return the version of the addon interface that this
	 * object is implementing.  Add ons should define this
	 * to always return VERSION_CURRENT; the MIDI system will
	 * use this to make sure it doesn't try to make your addon
	 * do anything it can't.
	 */

	enum {
		VERSION_1 = 19980926,	// First version
		
		VERSION_CURRENT = VERSION_1
	};
	typedef uint32 VersionType;
	
	virtual VersionType Version(void) const = 0;

	// --------------------------------------------------------
	// BASIC INTERFACE.
	// --------------------------------------------------------
	
	/* Return a name for this filter.  This is shown to the user.
	 */
	virtual BString		Name() const = 0;
	/* Write my label to the string.  If I have no label and
	 * useName is true, write my name to the string.  A good
	 * client will provide an archive of the filter I want a
	 * label for, although there's no guarantee.
	 */
	virtual status_t	GetLabel(	BString& outLabel, bool useName = true,
									BMessage* archivedFilter = NULL) const;
	/* Return a uniquely identifying name for this filter.
	 * It should be formed as an implementor prefix followed
	 * by the name, i.e. "arp:Echo" or "be:Quantize".
	 */
	virtual BString		Key() const = 0;
	/* Answer the maximum number of connections I allow.  By
	 * default I answer 1.  If the answer is less tham 0, that
	 * means there is no limit.  An answer of 0 is currently undefined.
	 */
	virtual int32		MaxConnections() const;
	/* Return a short description for the filter.
	 */
	virtual BString		ShortDescription() const = 0;
	/* The long description is used for the generated HTML documenation
	 * for this filter -- i.e. the string is actually viewed
	 * by an HTML viewer, so you can use HTML tags.  Please first
	 * call the inherited version of this method, so your filter
	 * docs fit the basic pattern.  The first argument, name will
	 * be the name given to the filter in the docs.  The str
	 * argument is the actual documentation.
	 */
	virtual void		LongDescription(BString& name, BString& str) const;
	/* Return the author's name, typically first and last, i.e.
	 * "Jane Smith", and an email for contact, if you like.
	 */
	virtual BString		Author() const = 0;
	virtual BString		Email() const;
	/* Fill the major and minor version numbers in the supplied args.
	 * An addon which supplies a major number of 1 and a minor of 0
	 * would appear to the user as 'v1.0'.
	 */
	virtual void		GetVersion(int32* major, int32* minor) const = 0;
	
	/* Return the type of this filter, one of the values
	 * in the "type" enum.
	 */
	enum type {
		// A through filter is one that transforms an input event
		// into an output event stream.  A filter of this type
		// will usually run in a batch mode.
		THROUGH_FILTER		= 0,
		// A source filter is one that interfaces with an input
		// device.  It can only be the first filter in a track's
		// input pipeline.
		SOURCE_FILTER		= 1,
		// A destination filter is on that interfaces with an
		// output device.  It can only be the last filter in a
		// track's output pipeline, and its HandleEvent() method
		// executes in the playback thread context.  This means
		// that it can (and actually must) block until it is time
		// for the event given to it to be performed.
		DESTINATION_FILTER	= 2
	};
	virtual type		Type() const = 0;

	enum subtype {
		NO_SUBTYPE					= 0x00000001,
		TOOL_SUBTYPE				= 0x00000002,
		MULTI_SUBTYPE				= 0x00000003,
		_NUM_SUBTYPE
	};
	virtual subtype		Subtype() const;

	/* Return the name for a corresponding filter class name of
	 * the given type.  The default implementation returns Key()
	 * for your Type(), and NULL for all others.  You can implement
	 * this to associate SOURCE and DESTINATION filters.
	 */
	virtual BString		KeyForType(type inType) const;
	
	/* Answer with a bitmap that is equal to or larger than the
	 * requested dimensions.  Currently, all subclasses should
	 * respond to this with a bitmap that is 20 pixels by 20 pixels;
	 * this might change in the future.
	 * NOTE: BE PREPARED TO RECEIVE A NULL BITMAP.
	 */
	virtual BBitmap*	Image(BPoint requestedSize) const;
	
	/* Fill in the BMessage with an archive for the prototypical
	 * instance of this filter add-on.  You only need to implement
	 * this if you need to store non-standard information in the
	 * archive.
	 */
	virtual status_t	GetArchiveTemplate(BMessage* into, uint32 flags) const;
	
	/* Return how appropriate you are at instantiating the filter
	 * in 'config'.  The default implementation returns a match
	 * quality of .5 for add-ons of the same class, and -1 ("no
	 * match") for non-matching classes.  You only need to implement
	 * this if you need to modify that behaviour.
	 */
	virtual float		CheckInstantiation(const BMessage* archive) const;
	
	/* Create a new instance of this filter.  The instance, an
	 * AmFilterI, is what does the actual work.  During creation,
	 * you are given an AmFilterHolderI object, which is this
	 * filter instance's context inside of the MIDI system,  You will
	 * almost definately want to keep ahold of this object inside of
	 * your filter instance [it is guaranteed to remain valid for the
	 * lifetime of your AmFilterI], as it provides your main
	 * interface back into the MIDI system.
	 *
	 * The optional 'config' parameter provides an initial settings
	 * configuration for this filter instance; see
	 * AmFilterI::GetConfiguration() for how to get a BMessage
	 * to use here.
	 */
	virtual AmFilterI*	NewInstance(AmFilterHolderI* holder,
									const BMessage* config = NULL) = 0;

	static float		MatchClassArchive(	const char* key,
											const BMessage* archive);
	
	// --------------------------------------------------------
	// SUPPORT FUNCTIONS.
	// --------------------------------------------------------
	
	/* Start/stop sending song clock reports to this filter class.
	 * Currently, only an interval of PPQN/24 is supported.
	 */
	void				StartClock(AmTime interval = PPQN/24);
	void				StopClock();
	
	/* This function is called at every clock tick in the song.
	 * Note that it is called as part of the real-time performer
	 * thread, so you should be careful about what you do in it.
	 */
	virtual void		Clock(AmTime time);
	
	/* Return the image for this add-on that should be displayed
	 * to the user.  This is Image() with any requested tinting
	 * transformation added.
	 */
	BBitmap*			FinalImage(BPoint requestedSize) const;
	
	/* Change the tinting for this add-on.  The default tint is
	 * B_TRANSPARENT_COLOR, i.e. nothing.
	 */
	void				SetTint(rgb_color tint);
	rgb_color			Tint() const;
	
	/* Helper function for others who want to perform tinting.
	 */
	static BBitmap*		TintImage(BBitmap* image, rgb_color tint);
	
protected:
	virtual ~AmFilterAddOn();

private:
	virtual	void		_ReservedAmFilterAddOn1();
	virtual	void		_ReservedAmFilterAddOn2();
	virtual	void		_ReservedAmFilterAddOn3();
	virtual	void		_ReservedAmFilterAddOn4();
	virtual	void		_ReservedAmFilterAddOn5();
	virtual	void		_ReservedAmFilterAddOn6();
	virtual	void		_ReservedAmFilterAddOn7();
	virtual	void		_ReservedAmFilterAddOn8();
	virtual	void		_ReservedAmFilterAddOn9();
	virtual	void		_ReservedAmFilterAddOn10();
	virtual	void		_ReservedAmFilterAddOn11();
	virtual	void		_ReservedAmFilterAddOn12();
	virtual	void		_ReservedAmFilterAddOn13();
	virtual	void		_ReservedAmFilterAddOn14();
	virtual	void		_ReservedAmFilterAddOn15();
	virtual	void		_ReservedAmFilterAddOn16();
	
	friend class AmFilterAddOnHandle;
	
	const void* mCookie;
	rgb_color mTint;
	size_t _reserved_data[12];
};

/*****************************************************************************
 *
 *	AM-FILTER-HOLDER-I CLASS
 *
 *	The AmFilterHolderI class is the AmKernel system's context for
 *	a filter in a pipeline.  It "holds" the actual instance of a particular
 *	filter, an AmFilterI object.  The AmFilterI defines the
 *	interface from the MIDI system into a particular filter implementation;
 *	this AmFilterHolderI class defines the interface from the filter
 *	addon back into its surrounding context in the MIDI system, most
 *	importantly the filters connected to it in the pipeline.
 *
 *	NOTE: The ArpConfigurableI interface implemented by this class calls
 *	through to its AmFilterI's configurable interface.  However, this
 *	class adds observer notification and thread safety, so you -must-
 *	use this one when implementing configuration views.
 *
 *****************************************************************************/

class AmFilterHolderI : public AmSafeDelete, public ArpConfigureWatch
{
public:
	/* This is the actual filter instance that the object holds.
	 */
	virtual AmFilterI*			Filter() const = 0;
	/* Shortcut for Filter()->AddOn()->Type().
	 */
	virtual AmFilterAddOn::type	Type() const = 0;
	/* Answer the matrix that contains me.
	 */
	virtual const pipeline_matrix_id MatrixId() const = 0;	
	/* Answer the pipeline that contains me.
	 */
	virtual const pipeline_id	PipelineId() const = 0;	
	/* Answer the track that contains me, if any.
	 */
	virtual const track_id		TrackId() const = 0;	
	/* Answer the device for the track I reside in, or NULL if none.
	 */
	virtual ArpCRef<AmDeviceI>	TrackDevice() const = 0;
	/* Answer true if this filter's processing should be bypassed.
	 */
	virtual bool				IsBypassed() const = 0;
	virtual void				SetBypassed(bool bypass) = 0;
	virtual bool				IsSuppressingNextInLine() const = 0;
	
	/* Transform between real-time and pulses.  Only valid while
	 * a song is playing; when not playing, these always return
	 * -1.
	 */
	virtual AmTime				RealtimeToPulse(bigtime_t time) const = 0;
	virtual bigtime_t			PulseToRealtime(AmTime pulse) const = 0;
	
	/* Record the given even chain into the currently recording
	 * song.  Ownership of the given events is transfered to the
	 * callee.
	 */
	virtual status_t			GenerateEvents(AmEvent* events) = 0;
	
	/* This provides access to my list of connections.  If I am a
	 * filter that allows more than 1 connection, then the connection
	 * at 0 will always be the next holder in my pipeline, and all
	 * others will be connections to filters in different pipelines.
	 * If I only allow 1 connection, then the connection at 0 is always
	 * just that connection.  Note that it's possible to have no next holder
	 * in my pipeline, but still have connections to other pipelines.
	 * In this case, the connection at 0 will be NULL, but the
	 * connection at 1 will be valid.  For this reason, you should
	 * always iterate over the list using the count, and still check
	 * that the items returned are not NULL.
	 */
	virtual uint32				CountConnections() const = 0;
	virtual AmFilterHolderI*	ConnectionAt(uint32 index = 0) const = 0;
	/* This is a convenience, it will answer the connection at 0
	 * or, if that's NULL, the connection at 1.
	 */
	virtual AmFilterHolderI*	FirstConnection() const = 0;
	/* This is useful primary for clients that need to draw a pipeline --
	 * it gets the next filter in this filter's pipeline, regardless
	 * of any connections to other pipelines.
	 */
	virtual AmFilterHolderI*	NextInLine() const = 0;
	
	/* This is a convenience method that will refresh the display of
	 * all views on my filter.
	 */
	virtual void				FilterChangeNotice() = 0;

protected:
	/* You are not allowed to delete these yourself. */
	virtual ~AmFilterHolderI() { }
};

/*****************************************************************************
 * FILTER PARAMETER STRUCTS
 * These structs provide the parameters to the HandleEvent(), HandleToolEvent(),
 * and HandleRealtimeEvent() methods, respectively.
 *****************************************************************************/

// -------- HandleEvent() params --------
enum {
	AMFF_RECORDING		= (1<<0),
	AMFF_STARTING		= (1<<1),
};

struct am_filter_params {
	size_t					size;				// amount of data in structure
	uint32					flags;				// state flags
	bigtime_t				performance_time;	// for destination filter, exact
												// time to perform for all other
												// filters, zero.
	const AmTempoChange*	cur_tempo;			// last tempo event in song.
	const AmSignature*		cur_signature;		// last signature event in song.

	status_t				AddMotionChanges(const AmSong* song);
	status_t				DeleteMotionChanges();
	AmMotionChange*			MotionChange(track_id trackId) const;

	am_filter_params();
	am_filter_params(const am_filter_params& o);
	~am_filter_params();
	am_filter_params& operator=(const am_filter_params& o);

private:
	bool operator==(const am_filter_params& o) const;
	bool operator!=(const am_filter_params& o) const;

	AmMotionChange**		cur_motion_list;
	int32					motion_size;
};

// -------- HandleToolEvent() params --------
struct am_tool_filter_params {
	size_t					size;			// amount of data in structure
	uint32					flags;			// state flags

	AmTime					orig_time;		// The time at which the mouse was clicked
	AmTime					cur_time;		// The current time of the mouse
	AmTime					start_time;		// The start time for the currently-processed selections 
	AmTime					end_time;		// The end time for the currently-processed selections 
	float					orig_y_pixel;	// The y pixel at which the mouse was clicked
	float					cur_y_pixel;	// The current y pixel of the mouse
 	AmEvent::EventType		view_type;		// the types of events that the
											// following Y values are valid for
	int32					orig_y_value;	// The y value at which the mouse was clicked,
											// as appropriate for the current view_type
	int32					cur_y_value;	// The current y value of the mouse
	track_id				track_context;	// The track upon which the tool is currently operating
	/* The current grid time of the track.  Anyone doing their own
	 * quantizing will be interested in the multiplier, value, and
	 * divider.  Most clients will probably just want to know the
	 * actual current GridTime().
	 */
	int32					grid_multiplier;
	AmTime					grid_value;
	int32					grid_divider;
	AmTime					GridTime() const { return (((grid_value*2)/grid_divider)*grid_multiplier); }

	am_tool_filter_params();
	am_tool_filter_params(const am_tool_filter_params& o);
	~am_tool_filter_params();
	am_tool_filter_params& operator=(const am_tool_filter_params& o);

private:
	bool operator==(const am_tool_filter_params& o) const;
	bool operator!=(const am_tool_filter_params& o) const;
};

// -------- HandleRealtimeEvent() params --------
struct am_realtime_filter_params {
	size_t					size;				// amount of data in structure

	am_realtime_filter_params();
	am_realtime_filter_params(const am_realtime_filter_params& o);
	~am_realtime_filter_params();
	am_realtime_filter_params& operator=(const am_realtime_filter_params& o);

private:
	bool operator==(const am_realtime_filter_params& o) const;
	bool operator!=(const am_realtime_filter_params& o) const;
};


#define AM_SIZE_TO(STRUCT, FIELD)		\
		( ((size_t)&((STRUCT*)0)->FIELD) + sizeof(STRUCT::FIELD) )

/*****************************************************************************
 * AM-FILTER-I
 * The AmFilterI class is the public interface to an instance of a filter.
 * Filters are generic MIDI-processing objects.  The main filter method is
 * HandleEvent(), which performs all processing (the methods HandleToolEvent()
 * and HandleRealtimeEvent() are available for special situations).
 *****************************************************************************/

class AmFilterI : public ArpConfigurableI
{
public:
	AmFilterI(AmFilterAddOn* addon);
	
	/* The MIDI system will delete this filter instance when it is
	 * no longer needed.
	 */
	virtual ~AmFilterI() { }

	/* Answer a unique ID for this filter.
	 */
	filter_id			Id() const;
	/* Answer a name for this filter.  By default, answer the filter
	 * addon's name.  However, users can override this to supply their
	 * own name for the filter.
	 */
	BString				Name() const;
	/* Every filter instance can optionally be labelled.  If no label
	 * is set, Label() returns the Name(); otherwise, the selected
	 * label is returned.  The filter's label is handled in the
	 * default implemention of GetConfiguration() and PutConfiguration().
	 */
	bool				HasLabel() const;
	virtual BString		Label() const;
	/* Return the addon class definition for the filter -- i.e., the
	 * object whose NewInstance() method created this object.
	 */
	AmFilterAddOn*		AddOn() const;
	/* Flags for this filter.
	 * HIDE_PROPERTIES_FLAG: Set so that this filter does not report
	 * its configure panels in certain situations.
	 * BATCH_FLAG: Set to have your filter run in batch mode.
	 * FLUSH_FLAG: Set to have your Ready() and Flush() methods called.
	 * WATCH_DEVICE_FLAG: Set to receive the DeviceChanged() notice.
	 * OSCILLATOR_FLAG: Set if this filter is one that is continuously
	 * generating events (not just in response to other events) -- these
	 * are oscillator-style filters.
	 */
	enum {
		HIDE_PROPERTIES_FLAG		= 0x00000001,
		BATCH_FLAG					= 0x00000002,
		FLUSH_FLAG					= 0x00000004,
		WATCH_DEVICE_FLAG			= 0x00000008,
		OSCILLATOR_FLAG				= 0x00000010
	};
	uint32				Flags() const;
	void				SetFlag(uint32 flag, bool state);
	/* The lookahead time can be used to look at filters after the
	 * current one being processed.  By default, the time is 0. and
	 * no lookahead is generated.  Currently, only the batch mode
	 * methods will receive lookhead events.
	 */
	virtual AmTime		LookaheadTime() const;
	/* Default ArpConfigurableI implementation.  Be sure to call the
	 * inherited form when overriding.
	 * NOTE: You should never call PutConfiguration() here.  Instead,
	 * call the filter's AmFilerHolderI::PutConfiguration(), which
	 * will send out a change notification to all objects watching
	 * the filter.
	 */
	virtual status_t	GetConfiguration(BMessage* values) const;
	virtual status_t	PutConfiguration(const BMessage* values);
	virtual status_t	Configure(ArpVectorI<BView*>& panels);
	/* Supply a list of all the properties this filter responds to.
	 * You can use the following methods to construct a property message:
	 *		AddBool().  The name of the bool becomes the name of the
	 * property.  Whether the value is true or false doesn't matter.
	 *		AddInt32().  Typically this method will be used to add
	 * a range.  To specify a range, you need two int32's with the
	 * same name.  Single values of an int32 can be targeted as well.
	 */
	virtual status_t	GetProperties(BMessage* properties) const;
	
	/* Fill in BMessage with data describing this filter instance,
	 * which can be passed into NewInstance() to make a duplicate.
	 */
	status_t			Archive(BMessage* into, uint32 flags) const;
	
	/* Fill in a configuration message that will set up one of
	 * your instances for the "next" in order from yourself.  Return
	 * an error if you are the last.
	 */
	virtual status_t	GetNextConfiguration(BMessage* config) const;
	
	/* Fill in the string with text you would like to display for
	 * your tool tip.  The default implementation simply fills in
	 * the filter add-on's Name().
	 */
	virtual status_t	GetToolTipText(BString* out) const;
	
	/* By default, answer the addon's image.
	 */
	virtual BBitmap*	Image(BPoint requestedSize) const;
	
	/* This is called when the user invokes an action on the
	 * filter.  Normally, this is B_SECONDARY_BUTTON to show the
	 * popup menu; 'released' is true if being called as a result
	 * of the button being released.
	 */
	virtual void		MouseAction(BRect frame, BPoint where,
									uint32 buttons, bool released);
	
	/* Return a device.
	 */
	virtual ArpCRef<AmDeviceI> Device() const;
	/* Notice that a device changed -- note, right now this could
	 * be ANY device, not just my device.  Default does nothing.
	 */
	virtual void		DeviceChanged();
	
	/* Return a hint about what midi channel this filter
	 * corresponds to.  Only really useful for output filters,
	 * and then currently only used for writing standard MIDI
	 * files.  The default implementation returns -1, indicating
	 * it has nothing to hint.
	 */
	virtual int32		HintChannel() const;
	/* Report the delay you will experience between receiving an
	 * event and processing it.  Only ouptut filters need to worry
	 * about latency.  (NOT YET IMPLEMENTED)
	 */
	virtual bigtime_t	Latency() const;
	/* These are called as the performance moves through the song,
	 * playing it in batches.  When the transport begins processing
	 * a range of time, StartSection() is called; when over,
	 * FinishSection() is called.  For both of these, you can return
	 * any events you want to generate during that range.
	 *
	 * Note that you should -not- expect to handle events whose time
	 * is only in the given range.  For many reasons, such as filters
	 * before you modifying event times, no such assumption can be
	 * made.
	 *
	 * The default implementation of these simply returns NULL.
	 */
	virtual AmEvent*	StartSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params = NULL);
	virtual AmEvent*	FinishSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params = NULL);
	/* This is called when the user causes processing to begin or end.
	 */
	enum {
		TRANSPORT_CONTEXT	= 0x00000000,		// Song start and stop buttons
		TOOL_CONTEXT		= 0x00000001,		// Mouse down and up
		PANIC_CONTEXT		= 0x00000002,		// In response to user panic
		MASK_CONTEXT		= 0x0000000F		// Which bits define the context
	};
	virtual void		Start(uint32 context);
	virtual void		Stop(uint32 context);
	
	/* The main reason to exist: this function implements your
	 * actual filter algorithm.  The MIDI system will call it with
	 * an AmEvent object that is to be filtered; you may do
	 * whatever you want with it -- change it, delete it, copy it,
	 * add events before or after it -- and then return the resulting
	 * changed and/or new events.
	 * Note that if you return more than one event, they must be linked
	 * in time sorted order, with the earliest event positioned first
	 * and returned as the result.  These events will then be merged
	 * back into the sequence that is being filtered.
	 * The 'params' argument suppies additional performance information.
	 * It may be NULL.
	 */
	virtual AmEvent*	HandleEvent(AmEvent* event,
									const am_filter_params* params = NULL) = 0;
	/* This function is called when an event is being filtered by a tool.
	 * These will occur outside of corresponding StartSection() and
	 * FinishSection() calls.
	 *
	 * The default implementation is to call HandleEvent().  You can
	 * override this version if you need to do something special with
	 * this situation.
	 */
	virtual AmEvent*	HandleToolEvent(AmEvent* event,
										const am_filter_params* params = NULL,
										const am_tool_filter_params* toolParams = NULL);
	/* This function is called when a real-time event is moving through
	 * your filter.  (That is, one that came directly from an input
	 * device rather than an existing sequence.)  The semantics are
	 * slightly different in this case -- such events will appear out of
	 * sequence with other regular events, and you will see individual
	 * AmNoteOn (with duration of 0) and AmNoteOff events, rather than
	 * one AmNoteOn with a duration.
	 *
	 * The default implementation is to call HandleEvent().  You can
	 * override this version if you need to do something special with
	 * this situation.
	 */
	virtual AmEvent*	HandleRealtimeEvent(AmEvent* event,
											const am_filter_params* params = NULL,
											const am_realtime_filter_params* realtimeParams = NULL);
	
	/* If BATCH_FLAG is set, these functions will be called instead of
	 * HandleEvent(), HandleToolEvent(), and HandleRealtimeEvent().  The only
	 * difference between these functions and the above are their input
	 * semantics -- here you are given an entire chain of events, and should
	 * return the result of your filter manipulating every event in that chain.
	 */
	virtual AmEvent*	HandleBatchEvents(	AmEvent* event,
											const am_filter_params* params = NULL,
											const AmEvent* lookahead = NULL);
	virtual AmEvent*	HandleBatchToolEvents(	AmEvent* event,
												const am_filter_params* params = NULL,
												const am_tool_filter_params* toolParams = NULL,
												const AmEvent* lookahead = NULL);
	virtual AmEvent*	HandleBatchRealtimeEvents(	AmEvent* event,
													const am_filter_params* params = NULL,
													const am_realtime_filter_params* realtimeParams = NULL,
													const AmEvent* lookahead = NULL);

	/* Yes, ANOTHER filter mechanism.  This gets called only if the
	 * OSCILLATOR_FLAG is set.  It's used by filters that generate events,
	 * regardless of whether they've received one.  It might get generalized
	 * into an existing part of the system, or it might be it doesn't work at
	 * all.  We'll see.
	 */
	virtual AmEvent*	OscPulse(	AmTime start, AmTime end,
									const am_filter_params* params = NULL);

	/* If FLUSH_FLAG is set, these functions are called at the start and end
	 * of a chain of events, respectively().  Generally, you will use Ready()
	 * to initialize your filter's state, and Flush() to return any remaining
	 * events it has.
	 */
	virtual	void		Ready(const am_filter_params* params);
	virtual	AmEvent*	Flush(const am_filter_params* params);
	
	/* Filters are notified when their config window is opened or closed.
	 * The default implementation is empty.
	 */
	virtual void		ConfigWindowOpened();
	virtual void		ConfigWindowClosed();

	/* OK, major hack here.  It's for the multi filter and the AmProducerFilter.
	 * I was being caught by code that is old and I don't have a clue what it's
	 * doing.
	 */
	virtual void		TurnOffWtfHack()	{ }

protected:
	void				SetLabel(const char* label);

private:
	virtual	void		_ReservedAmFilterI1();
	virtual	void		_ReservedAmFilterI2();
	virtual	void		_ReservedAmFilterI3();
	virtual	void		_ReservedAmFilterI4();
	virtual	void		_ReservedAmFilterI5();
	virtual	void		_ReservedAmFilterI6();
	virtual	void		_ReservedAmFilterI7();
	virtual	void		_ReservedAmFilterI8();
	virtual	void		_ReservedAmFilterI9();
	virtual	void		_ReservedAmFilterI10();
	virtual	void		_ReservedAmFilterI11();
	virtual	void		_ReservedAmFilterI12();
	virtual	void		_ReservedAmFilterI13();
	virtual	void		_ReservedAmFilterI14();
	virtual	void		_ReservedAmFilterI15();
	virtual	void		_ReservedAmFilterI16();
	
	ArpRef<AmFilterAddOn>	mAddOn;
	uint32					mFlags;
	BString					mLabel;
	size_t					_reserved_data[12];
};

// interface
typedef AmFilterAddOn* (*make_nth_filter_type)(int32 n, image_id you,
												const void* cookie, uint32 flags, ...);
extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id you,
												  const void* cookie, uint32 flags, ...);


/*****************************************************************************
 * Filter utilities
 * Convenience functions for filter writers.
 *****************************************************************************/
/* Fill out a signature event at the supplied time -- the outSig will have
 * the correct measure number, time, etc.
 */
status_t am_get_measure(		AmTime time, const AmSignature* sigList,
								AmSignature& outMeasure,
								const AmSignature** outSignature = NULL);
/* This is the new, simplified motion system -- we're beginning the
 * change to sweeps.  Given a time and a motion (and params for the
 * signature info), answer the Y value at that time.  Interpolate
 * based on the type of motion -- progressions get no interpolation
 * (and might answer with an error), rhythms have a flat interpolation,
 * and envelopes a smooth.
 */
status_t am_get_motion_y(	AmTime time, const AmMotionI* motion,
							const am_filter_params* params, float* outY);
/* Supply a Y value for the current time, given the motion chain, motion
 * offset, and measure.  The Y value will be zero if no hit was found,
 * or the value of the hit -- either an actual hit for rhythm and progression
 * motions, or an interpolated one for envelope motions.
 *
 * Clients should do a little preparation before calling this method:
 *		Set motionMeasureOffset to an initial value of zero
 *		The measure argument should be created like this:
 *					AmSignature		measure(*(params->cur_signature) );
 *		The curMotionEvent should be the appropriate AmMotionChange in
 *					the am_filter_params.
 * With these items done, this method can be called efficiently on multiple
 * events.
 */
status_t am_motion_hit_at_time(	AmTime time, AmMotionChange** curMotionEvent,
								int32* motionMeasureOffset,
								AmSignature& measure,
								float* outHitY);

/* Answer the hit and next hit (which will be identical to the hit if there
 * is no next hit) at the given time.  The params are required for the
 * measure info.
 */
status_t am_get_motion_hits(AmTime time, const AmMotionI* motion,
							const am_filter_params* params,
							BPoint* outHitPt, float* outEnd = NULL,
							BPoint* outNextPt = NULL,
							AmRange* outHitRange = NULL);
/* Answer the hit and next hit (which will be identical to the hit if there
 * is no next hit) at the given time.  The supplied measure object can
 * be found by using the am_get_measure() function.
 * If the outHitRange argument is supplied, this value is filled with the
 * time range for the outHitPt.
 */
status_t am_get_motion_hits(AmTime time, const AmMotionI* motion,
							const AmSignature& measure,
							BPoint* outHitPt, float* outEnd = NULL,
							BPoint* outNextPt = NULL,
							AmRange* outHitRange = NULL);
/* Answer with a value between 0 and 1 based on where the curTime falls in
 * relation to the start and end.  If curTime occurs at centerTime it will
 * be 1, then it falls to zero as it approaches the start and end.
 */
float am_x_amount(	AmTime centerTime, AmTime curTime,
					AmTime start, AmTime end);
float am_x_amount(const am_tool_filter_params* params, AmTime curTime);


/*****************************************************************************
 * AM-STATIC-RESOURCES
 * A class to make resource accessing more convenient.  See any of the
 * example filters for usage.
 *****************************************************************************/
class AmStaticResources 
{ 
public:
	AmStaticResources() 
		:  fInitResources(0) 
	{ }
	~AmStaticResources() { } 
 
	BResourceSet& Resources()
	{
		if (atomic_or(&fInitResources, 1) == 0) {
			fResources.AddResources(this); 
			atomic_or(&fInitResources, 2);
		} else {
			while ((fInitResources&2) == 0) snooze(20000);
		}
		return fResources;
	}
 
private: 
	BResourceSet fResources;
	int32 fInitResources;
};

#endif
