/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * ----------------------------------------------------------------------
 *
 * ArpLayoutable.h
 *
 * The layout engine's base class, which encapsulates
 * information about a rectangular UI area and how it
 * can be dimensioned.  It may be used either as a stand-alone
 * object, for simple containers that don't need to interact
 * with the user, or combined with a BView class [or subclass]
 * to create a GUI object that can be placed in the layout
 * system.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * • Archiving is not yet completely implemented.
 *   At the least, it needs to be fixed to never deep copy
 *   an attached BView object, and make sure to archive all
 *   ArpLayoutable children.  It would also be nice to be
 *   able to automatically archive all of an objects Params().
 * • I'm not very happy with the parameters and globals stuff.
 *   Possibly, it should just disappear -- that would shave
 *   about 1/3 off the size of this class...
 * • The SetError() stuff should go away -- it hasn't been
 *   useful ever since the post-R8 days of exceptions.
 *   This also implies that performing a series of nested
 *   allocations will cause a leak if one of the allocations
 *   throws an exception.  This needs to be fixed.
 * • The cur_dimens member variable should be made private
 *   to the class.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * • Integrate ArpViewWrapper with the ArpLayoutable base
 *   object?
 * • Improve name handling: always change an associated view
 *   when the layoutable name is changed, return the associated
 *   view's name instead of the layout object's if possible.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * Dec 6, 1998:
 *	First public release.
 *
 * May 5, 1999:
 *	- Added "ArpAnySize" constant.
 *
 */

#ifndef ARPLAYOUT_ARPLAYOUTABLE_H
#define ARPLAYOUT_ARPLAYOUTABLE_H

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef _LIST_H
#include <support/List.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

/*
 * This class is used to pass information about the basic
 * display dimensions of a user interface element.  This
 * is an extension to the existing GetPreferredSize() function
 * call, which provides more detailed information about how
 * the object can be dimensioned.
 */

// Place this value into the maximum width/height to allow
// unbounded expansion.
extern const float ArpAnySize;

typedef struct arp_layout_dimens {
	// The absolute minimum size the object can be, in pixels.
	float min_width, min_height;
	
	// The minimum size the object would prefer to be, in pixels.
	float pref_width, pref_height;
	
	// The absolute maximum size the object can use, in pixels.
	float max_width, max_height;
} ArpLayoutDimens;

// Quick way to pass no (a.k.a. default) paramters to a function.
extern ArpMessage ArpNoParams;

class ArpLayoutable {

  public:
	ArpLayoutable(const char* name = 0);
	ArpLayoutable(BMessage* data);
	virtual ~ArpLayoutable();

	// Standard archiving interface.
	static ArpLayoutable*	Instantiate(BMessage* archive);
	virtual status_t		Archive(BMessage* data, bool deep=true);

	/* Error control.  These functions are used to set and get
	 * the error state of an object, i.e. if a NULL child object
	 * was added, meaning a child somewhere ran out of memory.
	 * These are safe to call on NULL objects.
	 */
	ArpLayoutable*	SetError(status_t err);
	status_t		Error();
											
	/* Initialization functions.  These can be called on NULL
	 * object pointers, and won't behave poorly.
	 */
	ArpLayoutable*	SetLayoutName(const char* inname);
	const char*	LayoutName(void) const	{ return name; }
	
	/* Parameters.
	 * These are values that apply to a specific view object.
	 * This is primarily modifiable state information, such as
	 * the background color, font to use, various control
	 * flags, etc.  Their interpretation is derived entirely
	 * from the particular view object in which they are set.
	 *
	 * The parameter interface is through a standard ArpMessage.
	 * When setting parameters, a reference to an ArpMessage
	 * object is passed in, and the ArpLayoutable object retrieves
	 * all the values contained in it that it understands.
	 * The message is no longer referenced by the object after
	 * the function returns, and can be safely deleted.  The
	 * object usually stores the values it finds in its own
	 * members, rather than another ArpMessage, so that they
	 * are easy to access.
	 *
	 * When retrieving an object's parameters, an ArpMessage
	 * is returned.  The caller can either specify a particular
	 * parameter name it is interested in, and only that one will
	 * be placed in the return message, or it can specify NULL
	 * to have the object return all of its parameters.
	 *
	 * The ArpLayoutable base class defines these common
	 * parameters:
	 *
	 * BackgroundColor (char*).	Default: "BackgroundColor".
	 *		The name of the background pen the view should use.
	 *		This is actually the name of a variable in the
	 *		global name space that contains an rgb_color value.
	 * ForegroundColor (char*).	Default: "ForegroundColor".
	 *		The name of the foreground color of the view, i.e.
	 *		the color text is drawn in on top of the background.
	 * BasicFont (char*).		Default: "PlainFont".
	 *		The name of the font that normal text should be
	 *		drawn in.
	 */
	ArpLayoutable*		SetParams(const ArpMessage& p);
	const ArpMessage	Params(const char* name);
	
	static const char*	BackgroundColor;
	static const char*	ForegroundColor;
	static const char*	BasicFont;
	 
	/* Globals.
	 * These are values that apply to an entire tree of
	 * layout objects.  They are used primarily to define
	 * common attributes that apply across the layout; this
	 * is intended to make it easy to create visually
	 * consistent interfaces that are easy to dynamically
	 * modify as the program executes.
	 *
	 * The global interface is, like parameters, through an
	 * ArpMessage object.  Unlike parameters, however, the
	 * layout object does not have to make a copy of the values
	 * contained in the message.  Instead, it just stores
	 * a reference to the message.  Since it always has a
	 * reference to the message, it can look up values when
	 * needed.  This is important because the parameters
	 * above usually specify a global variable that contains
	 * the parameter's value, rather than directly specifying
	 * the value itself.
	 *
	 * By indirectly defining parameter values through this
	 * global name space, a change in a global parameter value
	 * can easily be propagated into all of the objects that
	 * it is used in.  This is made easier by the second
	 * function, RefreshGlobals().  The function is called with
	 * an ArpMessage that contains only the changed global
	 * values -- the object can then determine what changed,
	 * and limit the computation that it needs to do.  Note
	 * that this only informs the object about a change in
	 * global values; the application itself needs to make
	 * sure the ArpMessage set by SetGlobals() has actually
	 * changed.
	 *
	 * In addition, changes to the global data space are
	 * propagated to all of a layout object's children: both
	 * SetGlobals() and RefreshGlobals() call the same function
	 * on their children, and a child retrieves the global
	 * values from its parent when it is attached.
	 *
	 * The last method, AddGlobals(), is used by subclasses to
	 * add new values to the global parameter space.  The default
	 * implementation propagates up the view hierarchy, until the
	 * ArpLayoutable that contains the global data is reached.  It
	 * returns an error if it couldn't add the value(s) -- for
	 * example, if one already exists of a different type.
	 */
	ArpLayoutable*		SetGlobals(const ArpMessage* gl);
	bool				RefreshGlobals(const ArpMessage* gl);
	const ArpMessage*	Globals(void);
	virtual status_t	AddGlobals(const ArpMessage* gl);
	
	/* Constraints.
	 * Unlike the previous two data spaces, constraints are
	 * not defined by an object itself.  Instead, they are
	 * used by its parent object to hold child-specific
	 * information about how it is to be layed out.  A subclass
	 * implementation needs to worry little about this:
	 * The ArpLayoutable class takes care of storing and
	 * retrieving the values, by simply copying them into
	 * a local ArpMessage object and returning that object
	 * when requested.
	 */
	ArpLayoutable*		SetConstraints(const ArpMessage& c);
	ArpMessage&			Constraints(void);
	
	/* Set this to true to keep the view from refreshing during
	 * partial updates.  If false, it will perform a new layout
	 * any time its (or one of its childrens') dimensions change.
	 * When changed to false from being true, any stored-up
	 * changes will occur then and there.
	 */
	virtual ArpLayoutable*	SetLayoutInhibit(bool state);
	bool	LayoutInhibit(void) const	{ return inhibit_layout; }
	
	/* Child manipulation.  Work just like BView's respective
	 * functions.  These should -always- be called instead
	 * those in BView -- they will take care of doing the
	 * correct things with the layoutable's associated BView
	 * object and parent/children.
	 */
	virtual ArpLayoutable* AddLayoutChild(ArpLayoutable* v,
								   const BMessage& c = ArpNoParams,
								   ArpLayoutable* before = NULL);
	virtual	bool		RemoveLayoutChild(ArpLayoutable* child);
	ArpLayoutable*		LayoutParent(void) const { return parent; }
	int32				CountLayoutChildren(void) const;
	ArpLayoutable*		LayoutChildAt(int32 index) const;
	ArpLayoutable*		NextLayoutSibling(void);
	ArpLayoutable*		PreviousLayoutSibling(void);
	bool				LayoutRemoveSelf(void);
	ArpLayoutable*		FindLayoutable(const char* name);
	
	/* Window attachment.  These work just like the corresponding
	 * BView functions.
	 */
	virtual void LayoutAttachedToWindow()			{ }
	virtual void LayoutAllAttached()				{ }
	virtual void LayoutDetachedFromWindow()			{ }
	virtual void LayoutAllDetached()				{ }
	virtual BWindow* LayoutWindow() const;
	
	/* Check to see if there is space in the container for children.
	 * This function should return the number of children it can
	 * take, in addition to those currently in it.
	 * If your object does not take children, always return 0 (the
	 * default case); if it can take one child, return 1 if it
	 * currently has none and 0 if it does; if it can take any
	 * number of children, always return INT_MAX; etc.
	 */
	virtual int 		LayoutChildSpace() const	{ return 0; }
	
	/* Predict where a child will appear in its container.  The
	 * arguments define where the child will be inserted, just
	 * like AddLayoutChild().  The return value is a rectangle
	 * identifying the location in the container that the child
	 * will be placed.  Note that the size and shape of this
	 * rectangle should not be taken as gospel -- it is only
	 * intended as a guide, for showing a cursor for an insertion
	 * point in the layout.
	 */
	virtual BRect		HintLayoutChild(ArpLayoutable* before = NULL) const;
	
	/* Associated BHandler information.  An ArpLayoutable object
	 * should always have a BHandler -- either its own, or that
	 * of the BView class a subclass has mixed in.
	 */
	virtual BHandler* LayoutHandler()	{ return OwnerView(); }
	
	/* Associated BView information.  If this ArpLayoutable
	 * is associated with an actual BView, OwnerView() must
	 * return a pointer to it before AddLayoutChild() can be
	 * called on this object.  The BView must then remain
	 * constant for the entire time the layoutable has a parent
	 * or any children.
	 * InView() returns the actual view the layoutable exists
	 * in -- this is either OwnerView() if there is one, or the
	 * closest parents' OwnerView().
	 */
	virtual BView*	OwnerView()			{ return NULL; }
	BView*	InView()					{ return in_view ? in_view : (in_view=OwnerView()); }

	/* Dimensioning.
	 * LayoutDimens() is called by others to retrieve this
	 * object's dimensioning information.  It, in turn, will
	 * call ComputeDimens() if they need to be recomputed.
	 */
	const ArpLayoutDimens& LayoutDimens(void);

	/* Layout.
	 * These are called to position the object; if they make
	 * a change, all of its children will automatically be
	 * updated as well.
	 */
	virtual void	ResizeLayout(float width, float height, bool force = false);
	virtual void	MoveLayout(float left, float top, bool force = false);
	virtual void	SetLayout(const BRect& frame, bool force = false);
	virtual void	RequestLayout(bool force = false);
	
	/* Display.  Call this function to cause the view to be
	 * redrawn.
	 */
	void InvalidateView(void);
	
	/* Dimensioning.  Call this function to cause a recomputation
	 * the next time dimension information is requested.
	 */
	void InvalidateDimens(void);
	
	/* Retrieve the layoutables's frame, relative to its parent.
	 * This is the size of the frame, with left and top being
	 * relative to the closest parent BView's coordinate system
	 * (That is, -not- necessarily relative to the BView it is
	 * in; see below for that.)
	 * This is the frame that should be used when calling
	 * LayoutTo() et. al.
	 */
	BRect	LayoutFrame() const			{ return cur_frame; }
		
	/* Retrieve layoutable's frame, from interior coordinate
	 * space -- that is, the rendering frame relative to its
	 * nearest parent BView.
	 * For objects that have their own associated BView, this
	 * frame is always positioned at (0,0), as you would expect.
	 * Otherwise, 'left' and 'top' are the offsets needed to
	 * draw at the appropriate place in InView().
	 * This is the region to use, for example, when drawing and
	 * positioning children inside the object.
	 *
	 * (Note: this implies that the ArpLayoutable coordiate
	 *  system is not directly compatible with BView's
	 *  ScrollTo() et al.  I'm not sure what will need to be
	 *  done to make these interact together correctly.)
	 */
	BRect	LayoutBounds() const		{ return cur_bounds; }

	/* Focus rendering control.  This is intended as a more
	 * general replacement of BScrollView's mechanism for
	 * informing its child that it is attached.  When a child
	 * has received focus but can not display this itself, it
	 * should call its SetFocusShown() function with true; the
	 * default ArpLayoutable implementation of this function
	 * simply calls its parent with the same value -- thus the
	 * need to show focus will move up the instance hierarchy
	 * until it reaches a parent that can take care of it.
	 * In this way, any container object can show its children's
	 * focus.
	 * (The basic need for this is that we implement our own
	 *  BScrollView-type object that works better with the
	 *  rest of the ArpLayoutable system; however, it can't
	 *  register itself with things like BListView because it
	 *  isn't really a BScrollView.)
	 */
	virtual void SetFocusShown(bool state, bool andParent=true);
	bool FocusShown(void) const;
	
	/* Window activation.  This is the same as BView::WindowActivated(),
	 * but we need to do additional tracking of that state for
	 * ArpLayoutable objects without an attached BView.  Setting this
	 * flag also sets it for all the children; if you override this,
	 * be sure to also call the superclass.
	 */
	virtual void SetLayoutActivated(bool state);
	bool LayoutActivated(void) const;
	
	/* Subclasses should override this to return true if they are the
	 * root of a layout heirarchy.  This tells ArpLayoutable that their
	 * associated BView should never be positioned or resized.  For the
	 * most part, only ArpRootLayout need to be concerned about this.
	 */
	virtual bool IsLayoutRoot(void) const { return false; }
	
  protected:

	/* Parameter retrieval.
	 */
	status_t
		ExtractParam(const ArpMessage* params, const char* name,
					 type_code type, const ArpMessage* globs,
					 ArpString* gName, void* data);
	
	/* This is called by the above function to actually pull
	 * the particular data from a message.  It undertands the
	 * following types:
	 *
	 * type_code type			void* data
	 * B_STRING_TYPE			ArpString
	 * B_BOOL_TYPE				bool
	 * B_INT8_TYPE				int8
	 * B_INT16_TYPE				int16
	 * B_INT32_TYPE				int32
	 * B_INT64_TYPE				int64
	 * B_FLOAT_TYPE				float
	 * B_DOUBLE_TYPE			double
	 * B_POINT_TYPE				BPoint
	 * B_RECT_TYPE				BRect
	 * B_REF_TYPE				entry_ref
	 * B_RGB_COLOR_TYPE			rgb_color
	 * B_PATTERN_TYPE			pattern
	 * B_MESSAGE_TYPE			BMessage
	 * B_MESSENGER_TYPE			BMessenger
	 * B_POINTER_TYPE			void*
	 * FFont::FONT_TYPE			BFont
	 *
	 * The 'data' you supply MUST match 'type', or Bad Things
	 * will certainly happen.
	 *
	 * If you want to use other types, you will need to
	 * override this function to handle them yourself.
	 */
	virtual status_t
		InterpretType(const ArpMessage& msg, const char* name,
						type_code type, void* data);
	
	/* Parameter setting.  This function is called whenever
	 * SetParam() is called on the object.  It should look
	 * for the parameters it understands and update them
	 * appropriately.  Be sure to call InvalidateDimens()
	 * or InvalidateView() as appropriate.
	 *
	 * This function is also called whenever the global values
	 * changed -- either because SetGlobals() or RefreshGlobals()
	 * was called.  You can differentiate this from a SetParam()
	 * call because 'p' will be NULL.  The 'g' ArpMessage is
	 * the message that was passed into either of those
	 * functions; in either case, the entire global data space
	 * can be retrieved with Globals().  As with parameters,
	 * be sure to call InvalidateDimens() and InvalidateView()
	 * as needed.
	 *
	 * The function should return the number of parameters set,
	 * or at least zero if none and non-zero if at least one was.
	 */
	virtual int32	DoSetParams(const ArpMessage* g,
								const ArpMessage* p);
	
	/* Parameter retrieval.  This function is called whenever
	 * paremeter values are requested with Params().  It
	 * should place its parameters into the given ArpMessage,
	 * as follows:
	 * - If 'name' is NULL, place all parameters into message.
	 * - If 'name' is non-NULL, place this parameter into
	 *   message if it is one you understand, otherwise call
	 *   the superclass.
	 * Return 'true' if all requested parameters were successfully
	 * placed into the BMessage.
	 */
	virtual bool	DoParams(ArpMessage& p, const char* name);
	
	/* Subclass layout.  This function is called whenever the
	 * object size and/or position has changed, and it needs
	 * reposition its children.  By default, it just places
	 * the first child within its entire frame.
	 */
	virtual void Layout();
	
	/* Dimensioning.  When a recomputation of the object is
	 * needed (indicated with InvalidateDimens() above),
	 * ComputeDimens() will be called and should fill
	 * in cur_dimens with the appropriate information.  By
	 * default, this sets its dimensions to be the same as its
	 * first child, or zero.
	 */
	virtual void ComputeDimens();	
	ArpLayoutDimens	cur_dimens;

	/* BView management.  This function is called whenever the
	 * layoutable's parent BView changes -- the parameter passed
	 * in will either be a valid BView to which this object
	 * should attach itself, or NULL if it should detach itself
	 * from its current parent.  It is GUARANTEED not to be called
	 * with a valid BView if the object is already attached to
	 * another BView.
	 * The default implementation of this function takes care of
	 * three cases:
	 * (1) If this object does not itself have an owner BView,
	 *     it calls AttachView() on all of its children.
	 * (2) Otherwise, if par_view is non-NULL, it adds its owner
	 *     BView to par_view.
	 * (3) Otherwise, if InView() is currently valid, it removes
	 *     its owner BView that.
	 * You can override this function to, for example, attach
	 * any extra BViews created by your subclass, which do not
	 * have their own ArpLayoutable object (that is, they are not
	 * a visible part of the layout system).  In such a case,
	 * though, you will almost certainly still want to call the
	 * inherited form of this function.
	 */
	virtual void AttachView(BView* par_view, BView* before = 0);

	/* Object rendering.  This function is called whenever the
	 * object should draw itself.  It is not useful to override
	 * in layout objects that are incorporated with actual
	 * BViews, which should be overriding the BView Draw()
	 * function.  Conversely, a subclass of BView should always
	 * call its ArpLayoutable DrawLayout() function to be sure
	 * its children are drawn.
	 * You should also always call the superclass when overriding
	 * this function to be sure your children are drawn.
	 */
	virtual void DrawLayout(BView* inside, BRect region);

	/* These methods are echos of the corresponding BHandler
	 * methods.  A subclass of ArpLayoutable that actually
	 * includes the BHandler object should override the
	 * associated BHandler method, call these first, and if
	 * they return an error use the regular BHandler method.
	 */
	virtual status_t LayoutMessageReceived(BMessage *message);
	virtual status_t LayoutResolveSpecifier(BHandler** result,
											BMessage *msg,
											int32 index,
											BMessage *specifier,
											int32 form,
											const char *property);
	virtual status_t LayoutGetSupportedSuites(BMessage *data);

	/* This method fills in a standard suite BMessage, but one
	 * that describes the properties this ArpLayoutable parent
	 * object understands in its child constraints.
	 */
	virtual void GetConstraintSuites(BMessage *data);
	
	/* Return the default value (and possibly other information)
	 * of a particular constraint this parent understands.
	 */
	virtual status_t QueryConstraint(const char* name,
									 BMessage* data) const;
	
	/* Current object parameters.  PG_* parameters are
	 * the of the value in the global data space; they may
	 * be NULL if not supplied.  PV_* values are the actual
	 * value and are guaranteed to always be valid.
	 */
	ArpString	PG_BackgroundColor;
	ArpString	PG_ForegroundColor;
	ArpString	PG_BasicFont;
	
	rgb_color	PV_BackgroundColor;
	rgb_color	PV_ForegroundColor;
	BFont		PV_BasicFont;
	
	/* This method is called by mixed-in BView classes when they
	 * are attached to or detached from a window.
	 */
	void AttachLayoutWindow(BWindow* window);
	
  private:

	void LayoutChanged(bool force);
	BView* FindClosestView(int32 startIndex);
	void do_construct(void);
	void do_inhibit_layout(bool state);
	status_t lookup_param(const ArpMessage& msg, const char* name,
							type_code type, void* data);
	void set_layout_window(BWindow* window);
	
	ArpString			name;
	BList				children;
	BWindow*			owner;
	ArpLayoutable*		parent;
	BView*				in_view;
	bool				dimens_changed;	
	bool				inhibit_layout;
	bool				focus_shown;
	bool				layout_activated;
	
	const ArpMessage*	globals;
	
	/* Current constraints.  This is used to hold parent-defined
	 * state information about the object; it is not for use
	 * by this implementor.
	 */
	ArpMessage constraints;
	
	// Current LayoutFrame() and LayoutBounds(), as
	// described for those functions above.
	BRect			cur_frame, cur_bounds;
	
	// The last frame this object/BView was sized to, used to
	// avoid unnessasary exchanges with the app_server.
	BRect			last_frame;
	status_t		error;
	
};

#endif
