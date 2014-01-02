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
 * ArpBaseLayout.h
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * • Integrate ArpViewWrapper with the ArpBaseLayout base
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

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#define ARPLAYOUT_ARPBASELAYOUT_H

#ifndef ARPLAYOUT_ARPPARAMSET_H
#include <ArpLayout/ArpParamSet.h>
#endif

#ifndef ARPLAYOUT_ARPPARAMTEMPLATE_H
#include <ArpLayout/ArpParamTemplate.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTDEFS_H
#include <ArpLayout/ArpLayoutDefs.h>
#endif

#ifndef ARPLAYOUT_ARPDIMENS_H
#include <ArpLayout/ArpDimens.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

// Forward reference parameter interfaces.
class ArpParamSet;
class ArpGlobalSetI;

// These are standard global parameter flags that you can use
// in the base layout's parameter set.  Note that all other
// global flags are reserved.
enum {
	ARP_INVALIDATE_VIEW		= 1<<0,
	ARP_INVALIDATE_DIMENS	= 1<<1
};

/** -----------------------------------------------------------------

This is the base class for the layout architecture, defining
the interaction between objects in a hierarchy.  It is not intended
to be directly instantiated.  To use it, you will need to inherit
from one of the pre-build subclasses (ArpLayoutView for a full
BView object, or ArpLayout for a view-less container class).
Alternatively, you can mix-in ArpBaseLayout with an existing
BView-based class.

@bug	I'm not very happy with the parameters and globals stuff.
		Possibly, it should just disappear -- that would shave
		about 1/3 off the size of this class...
@bug	The SetError() stuff should go away -- it hasn't been
		useful ever since the post-R8 days of exceptions.
		This also implies that performing a series of nested
		allocations will cause a leak if one of the allocations
		throws an exception.  This needs to be fixed.
@bug	The cur_dimens member variable should be made private
		to the class.
 
    ----------------------------------------------------------------- */

class _EXPORT ArpBaseLayout {

  public:
    /** Default constructor and destructor.  Note that the archiving
	 	constructor has an additional non-standard parameter, 'final'.
	 	This is used for restoring the object's parameters.  When you
	 	implement your own constructor, you should call
	 	InstantiateParams() on 'data' if 'final' is true.  Otherwise,
	 	you should ignore the parameters.  In addition, when you call
	 	your superclass's constructor, you should pass in 'final' as
	 	false.  In this way, the parameters are not applied until the
	 	lowest subclass, ensuring that when the DoParams() method is
	 	called the entire object has been initialized.
     */
						ArpBaseLayout();
	virtual				~ArpBaseLayout();

	//* @see ArpBaseLayout()
						ArpBaseLayout(BMessage* data, bool final=true);
						
	/**	Standard archiving interface.
	 */
	virtual status_t	Archive(BMessage* data, bool deep=true) const;

	/**	Error control.  These functions are used to set and get
	 	the error state of an object, i.e. if a NULL child object
	 	was added, meaning a child somewhere ran out of memory.
	 	These are safe to call on NULL objects.
	 */
	ArpBaseLayout*		SetError(status_t err);
	//* @see SetError()
	status_t			Error();
	
	/** Quick and easy access to the associated BHandler's name.
	 */
	ArpBaseLayout*		SetLayoutName(const char* inname);
	//* @see SetLayoutName()
	const char*			LayoutName(void) const;
	
	/**	Parameters are values that apply to a specific view object.
	 	This is primarily modifiable state information, such as
	 	the background color, font to use, various control
	 	flags, etc.  Their interpretation is derived entirely
	 	from the particular view object in which they are set.
	 
	 	The parameter interface is through a standard NMessage.
	 	When setting parameters, a reference to an NMessage
	 	object is passed in, and the ArpBaseLayout object retrieves
	 	all the values contained in it that it understands.
	 	The message is no longer referenced by the object after
	 	the function returns, and can be safely deleted.  The
	 	object usually stores the values it finds in its own
	 	members, rather than another BMessage, so that they
	 	are easy to access.
	 
	 	When retrieving an object's parameters, a BMessage is
	 	filled in.  The caller can either specify a particular
	 	parameter name it is interested in, and only that one will
	 	be placed in the return message, or it can specify NULL
	 	to have the object return all of its parameters.
	 
	 	The ArpBaseLayout base class defines these common
	 	parameters:
	 
	 	@begindl
	 	@term	BackgroundColor (char*).
	 	@definition
	 		Default: "BackgroundColor".
	 		The name of the background pen the view should use.
	 		This is actually the name of a variable in the
	 		global name space that contains an rgb_color value.
	 	@term	ForegroundColor (char*).
	 	@definition
	 		Default: "ForegroundColor".
	 		The name of the foreground color of the view, i.e.
	 		the color text is drawn in on top of the background.
	 	@term	BasicFont (char*).
	 	@definition
	 		Default: "PlainFont".
	 		The name of the font that normal text should be
	 		drawn in.
	 	@enddl
	 	
	 	@see SetGlobals()
	 */
	virtual ArpBaseLayout*		SetParams(const BMessage& p);
	//* @see SetParams()
	virtual status_t			GetParams(BMessage* p, const char* name=0) const;
	
	ArpParamSet* ParamSet();
	const ArpParamSet* ParamSet() const;
	
	virtual void ParametersChanged(const ArpParamSet* params);
	
	static const char*	BackgroundColorP;
	static const char*	ForegroundColorP;
	static const char*	BasicFontP;
	
	/** Set the standard font used by this control.  Note that
		while BasicFont is stored as a separate variable from
		the view's own font, it is copied to the view before
		drawing.  Thus setting this is typically the equivalent
		of calling SetFont().
	 **/
	void SetBasicFont(const BFont* font);
	const BFont* BasicFont() const;
	
	/** Set the standard background color used by this control.
		Note that while BackgroundColor is stored in a separate
		variable from the view color, it is copied into the
		view color before drawing.  This setting this is typically
		the equivalent of calling SetViewColor().
	 **/
	 void SetBackgroundColor(rgb_color c);
	 rgb_color BackgroundColor() const;
	 
	/** Set the standard foreground color used by this control.
		Note that while ForegroundColor is stored in a separate
		variable from the view's high color, it is copied into the
		high color before drawing.  This setting this is typically
		the equivalent of calling SetHighColor().
	 **/
	 void SetForegroundColor(rgb_color c);
	 rgb_color ForegroundColor() const;
	 
	/**	Globals are values that apply to an entire tree of
	 	layout objects.  They are used primarily to define
	 	common attributes that apply across the layout; this
	 	is intended to make it easy to create visually
	 	consistent interfaces that are easy to dynamically
	 	modify as the program executes.
	 
	 	The global interface is, like parameters, through a
	 	BMessage object.  Unlike parameters, however, the
	 	layout object does not have to make a copy of the values
	 	contained in the message.  Instead, it just stores
	 	a reference to the message.  Since it always has a
	 	reference to the message, it can look up values when
	 	needed.  This is important because the parameters
	 	above usually specify a global variable that contains
	 	the parameter's value, rather than directly specifying
	 	the value itself.
	 
	 	By indirectly defining parameter values through this
	 	global name space, a change in a global parameter value
	 	can easily be propagated into all of the objects that
	 	it is used in.  This is made easier by the second
	 	function, RefreshGlobals().  The function is called with
	 	a BMessage that contains only the changed global
	 	values -- the object can then determine what changed,
	 	and limit the computation that it needs to do.  Note
	 	that this only informs the object about a change in
	 	global values; the application itself needs to make
	 	sure the BMessage set by SetGlobals() has actually
	 	changed.
	 
	 	In addition, changes to the global data space are
	 	propagated to all of a layout object's children: both
	 	SetGlobals() and RefreshGlobals() call the same function
	 	on their children, and a child retrieves the global
	 	values from its parent when it is attached.
	 
	 	The last method, AddGlobals(), is used by subclasses to
	 	add new values to the global parameter space.  The default
	 	implementation propagates up the view hierarchy, until the
	 	ArpBaseLayout that contains the global data is reached.  It
	 	returns an error if it couldn't add the value(s) -- for
	 	example, if one already exists of a different type.
	 	
	 	@see SetParams()
	 */
	virtual void		SetGlobals(ArpGlobalSetI* gl);
	//* @see SetGlobals()
	ArpGlobalSetI*		Globals(void) const;
	//* @see SetGlobals()
	void				SetGlobalParam(const char* name, const char* value);
	
	/** Unlike parameters and globals, constraints are
	 	not defined by an object itself.  Instead, they are
	 	used by its parent object to hold child-specific
	 	information about how it is to be layed out.  A subclass
	 	implementation needs to worry little about this:
	 	The ArpBaseLayout class takes care of storing and
	 	retrieving the values, by simply copying them into
	 	a local BMessage object and returning that object
	 	when requested.
	 */
	ArpBaseLayout*		SetConstraints(const BMessage& c);
	//* @see SetConstraints()
	BMessage&			Constraints(void);
	
	/** Set this to true to keep the view from refreshing during
	 	partial updates.  If false, it will perform a new layout
	 	any time its (or one of its childrens') dimensions change.
	 	When changed to false from being true, any stored-up
	 	changes will occur then and there.
	 */
	virtual ArpBaseLayout*	SetLayoutInhibit(bool state);
	//* @see SetLayoutInhibit()
	bool	LayoutInhibit(void) const;
	
	/** Child manipulation.  Work just like BView's respective
	 	functions.  These should -always- be called instead
	 	those in BView -- they will take care of doing the
	 	correct things with the layoutable's associated BView
	 	object and parent/children.
	 */
	virtual ArpBaseLayout* AddLayoutChild(ArpBaseLayout* v,
								   const BMessage& c = ArpNoParams,
								   ArpBaseLayout* before = NULL);
	//* @see AddLayoutChild()
	virtual	bool		RemoveLayoutChild(ArpBaseLayout* child);
	//* @see AddLayoutChild()
	ArpBaseLayout*		LayoutParent(void) const;
	//* @see AddLayoutChild()
	int32				CountLayoutChildren(void) const;
	//* @see AddLayoutChild()
	ArpBaseLayout*		LayoutChildAt(int32 index) const;
	//* @see AddLayoutChild()
	ArpBaseLayout*		NextLayoutSibling(void);
	//* @see AddLayoutChild()
	ArpBaseLayout*		PreviousLayoutSibling(void);
	//* @see AddLayoutChild()
	bool				LayoutRemoveSelf(void);
	//* @see AddLayoutChild()
	ArpBaseLayout*		FindLayoutable(const char* name);
	
	/** Window attachment.  These work just like the corresponding
	 	BView functions.
	 */
	virtual BWindow* LayoutWindow() const;
	//* @see LayoutWindow()
	virtual void LayoutAttachedToWindow();
	//* @see LayoutWindow()
	virtual void LayoutAllAttached();
	//* @see LayoutWindow()
	virtual void LayoutDetachedFromWindow();
	//* @see LayoutWindow()
	virtual void LayoutAllDetached();
	
	/**	Check to see if there is space in the container for children.
	 	This function should return the number of children it can
	 	take, in addition to those currently in it.
	 	If your object does not take children, always return 0 (the
	 	default case); if it can take one child, return 1 if it
	 	currently has none and 0 if it does; if it can take any
	 	number of children, always return INT_MAX; etc.
	 */
	virtual int 		LayoutChildSpace() const;
	
	/**	Predict where a child will appear in its container.  The
	 	arguments define where the child will be inserted, just
	 	like AddLayoutChild().  The return value is a rectangle
	 	identifying the location in the container that the child
	 	will be placed.  Note that the size and shape of this
	 	rectangle should not be taken as gospel -- it is only
	 	intended as a guide, for showing a cursor for an insertion
	 	point in the layout.
	 */
	virtual BRect		HintLayoutChild(ArpBaseLayout* before = NULL) const;
	
	/**	Associated BHandler information.  An ArpBaseLayout object
	 	should always have a BHandler -- either its own, or that
	 	of the BView class a subclass has mixed in.
	 */
	virtual BHandler* LayoutHandler()				= 0;
	virtual const BHandler* LayoutHandler()	const	= 0;
	
	/** Associated BView information.  If this ArpBaseLayout
	 	is associated with an actual BView, OwnerView() must
	 	return a pointer to it before AddLayoutChild() can be
	 	called on this object.  The BView must then remain
	 	constant for the entire time the layoutable has a parent
	 	or any children.
	 	InView() returns the actual view the layoutable exists
	 	in -- this is either OwnerView() if there is one, or the
	 	closest parents' OwnerView().
	 */
	virtual BView*	OwnerView();
	//* @see OwnerView()
	BView*	InView();

	/**	Dimensioning.
	 	LayoutDimens() is called by others to retrieve this
	 	object's dimensioning information.  It, in turn, will
	 	call ComputeDimens() if they need to be recomputed.
	 	
	 	@see ComputeDimens()
	 */
	const ArpDimens& LayoutDimens(void);

	/**	These are called to position the object.  If the call
		results in a change, all of the children will automatically be
	 	updated as well.
	 */
	virtual void	SetViewLayout(BRect frame, BRect body = BRect(),
							  bool force = false);
	//* @see SetViewLayout()
	virtual void	RequestLayout(bool force = false);
	
	/** This sets the gravity to determine how the frame and body
		rectangles are mapped to the BView frame rectangle.  The
		default fill is ArpFillAll, meaning that it is expanded to
		fill its entire frame rectangle.  Use the gravity flags to
		move the sides of the view between the body and frame
		rectangles.  For example, ArpCenter will cause the view
		to be placed entirely in the body rectangle; ArpWest will
		cause the left side of the view to match the frame rectangle,
		and the other sides to align with the body rectangle.
		
		@see SetViewLayout()
		@see ::ArpGravity
	 **/
	void SetBodyFill(ArpGravity fill);
	//* @see SetBodyFill()
	ArpGravity BodyFill() const;
	
	/**	Call this function to cause the view to be redrawn.
	 */
	void InvalidateView(void);
	
	/** Call this function to cause a recomputation
	 	the next time dimension information is requested.
	 	
	 	@see LayoutDimens()
	 */
	void InvalidateDimens(void);
	
	/**	Retrieve the layoutables's frame, relative to its parent.
	 	This is the size of the frame, with left and top being
	 	relative to the closest parent BView's coordinate system
	 	(That is, -not- necessarily relative to the BView it is
	 	in; see LayoutBounds() for that.)
	 	This is the frame that should be used when calling
	 	LayoutTo() et. al.
	 	
	 	@see LayoutBounds()
	 */
	BRect	LayoutFrame() const;
		
	/** Retrieve layoutable's frame for its body, relative to
		its parent.
	 	
	 	@see LayoutFrame()
	 */
	BRect	BodyFrame() const;

	/** Retrieve layoutable's frame, from interior coordinate
	 	space -- that is, the rendering frame relative to its
	 	nearest parent BView.
	 	For objects that have their own associated BView, this
	 	frame is always positioned at (0,0), as you would expect.
	 	Otherwise, 'left' and 'top' are the offsets needed to
	 	draw at the appropriate place in InView().
	 	This is the region to use, for example, when drawing and
	 	positioning children inside the object.
	
	 	@warning This implies that the ArpBaseLayout coordiate
	 	system is not directly compatible with BView's
	 	ScrollTo() et al.  I'm not sure what will need to be
	 	done to make these interact together correctly.
	 	
	 	@see LayoutFrame()
	 */
	BRect	LayoutBounds() const;

	/** Retrieve layoutable's bounds for its body, from the
		interior coordinate space -- that is, the rendering
		frame relative to its nearest parent BView.
	 	
	 	@see BodyFrame()
	 */
	BRect	BodyBounds() const;

	/** Focus rendering control.  This is intended as a more
	 	general replacement of BScrollView's mechanism for
	 	informing its child that it is attached.  When a child
	 	has received focus but can not display this itself, it
	 	should call its SetFocusShown() function with true; the
	 	default ArpBaseLayout implementation of this function
	 	simply calls its parent with the same value -- thus the
	 	need to show focus will move up the instance hierarchy
	 	until it reaches a parent that can take care of it.
	 	In this way, any container object can show its children's
	 	focus.
	 	
	 	(The basic need for this is that ArpLayout implements its own
	 	BScrollView-type object that works better with the
	 	rest of the ArpBaseLayout system; however, it can't
	 	register itself with things like BListView because it
	 	isn't really a BScrollView.)
	 */
	virtual void SetFocusShown(bool state, bool andParent=true);
	//* @see SetFocusShown()
	bool FocusShown(void) const;
	
	/** Window activation.  This is the same as BView::WindowActivated(),
	 	but we need to do additional tracking of that state for
	 	ArpBaseLayout objects without an attached BView.  Setting this
	 	flag also sets it for all the children; if you override this,
	 	be sure to also call the superclass.
	 */
	virtual void SetLayoutActivated(bool state);
	//* @see LayoutActivated()
	bool LayoutActivated(void) const;
	
  protected:

	/** Subclass layout.  This function is called whenever the
	 	object size and/or position has changed, and it needs
	 	reposition its children.  By default, it just places
	 	the first child within its entire frame.
	 */
	virtual void LayoutView();
	
	/** Dimensioning.  When a recomputation of the object is
	 	needed (indicated with InvalidateDimens() above),
	 	ComputeDimens() will be called and should fill
	 	in dimens with the appropriate information.  The
	 	default implementation sets this object's dimensions
	 	to the same as its first child, or zero.
	 	
	 	@see LayoutDimens()
	 	@see InvalidateDimens()
	 */
	virtual void ComputeDimens(ArpDimens& dimens);
	
	/** BView management.  This function is called whenever the
	 	layoutable's parent BView changes -- the parameter passed
	 	in will either be a valid BView to which this object
	 	should attach itself, or NULL if it should detach itself
	 	from its current parent.  It is GUARANTEED not to be called
	 	with a valid BView if the object is already attached to
	 	another BView.
	 	
	 	The default implementation of this function takes care of
	 	three cases:
	 	
	 	(1)	If this object does not itself have an owner BView,
			it calls AttachView() on all of its children.
	 
	 	(2)	Otherwise, if par_view is non-NULL, it adds its owner
	 		BView to par_view.
	 		
	 	(3)	Otherwise, if InView() is currently valid, it removes
	 		its owner BView that.
	 		
	 	You can override this function to, for example, attach
	 	any extra BViews created by your subclass, which do not
	 	have their own ArpBaseLayout object (that is, they are not
	 	a visible part of the layout system).  In such a case,
	 	though, you will almost certainly still want to call the
	 	inherited form of this function.
	 */
	virtual void AttachView(BView* par_view, BView* before = 0);

	/** Object rendering.  This function is called whenever the
	 	object should draw itself.  It is not useful to override
	 	in layout objects that are incorporated with actual
	 	BViews, which should be overriding the BView Draw()
	 	function.  Conversely, a subclass of BView should always
	 	call its ArpBaseLayout DrawLayout() function to be sure
	 	its children are drawn.
	 	You should also always call the superclass when overriding
	 	this function to be sure your children are drawn.
	 */
	virtual void DrawLayout(BView* inside, BRect region);

	/** These methods are echoes of the corresponding BHandler
	 	methods.  A subclass of ArpBaseLayout that actually
	 	includes the BHandler object should override the
	 	associated BHandler method, call these first, and if
	 	they return an error use the regular BHandler method.
	 */
	virtual status_t LayoutMessageReceived(BMessage *message);
	//* @see LayoutMessageReceived()
	virtual status_t LayoutResolveSpecifier(BHandler** result,
											BMessage *msg,
											int32 index,
											BMessage *specifier,
											int32 form,
											const char *property);
	//* @see LayoutMessageReceived()
	virtual status_t LayoutGetSupportedSuites(BMessage *data);

	/** This method fills in a standard suite BMessage, but one
	 	that describes the properties this ArpBaseLayout parent
	 	object understands in its child constraints.
	 	
	 	@see QueryConstraint()
	 */
	virtual void GetConstraintSuites(BMessage *data);
	
	/** Return the default value (and possibly other information)
	 	of a particular constraint that this parent understands.
	 	
	 	@see GetConstraintSuites()
	 */
	virtual status_t QueryConstraint(const char* name,
									 BMessage* data) const;
	
	/**	This function extracts the parameter information out
	 	of a BMessage archive and applies it to the object.
	 	You should call it in your constructor that takes
	 	a BMessage archive when its 'final' parameter is true.
	 */
	status_t InstantiateParams(BMessage* data);
	
	/**	This method is called by mixed-in BView classes when they
	 	are attached to or detached from a window.
	 */
	void AttachLayoutWindow(BWindow* window);
	
  private:

	void PerformChanges();
	void LayoutChanged(bool force);
	BView* FindClosestView(int32 startIndex);
	void do_construct(void);
	void do_inhibit_layout(bool state);
	void set_layout_window(BWindow* window);
	void ensure_body_frame();
	
	ArpParamSet			mParams;
	
	ArpGlobalParam<rgb_color>	PV_BackgroundColor;
	ArpGlobalParam<rgb_color>	PV_ForegroundColor;
	ArpGlobalParam<BFont>		PV_BasicFont;
	
	ArpBaseLayout*		mParent;
	ArpBaseLayout*		mPrevSibling;
	ArpBaseLayout*		mNextSibling;
	ArpBaseLayout*		mFirstChild;
	ArpBaseLayout*		mLastChild;
	BWindow*			owner;
	BView*				in_view;
	
	ArpDimens			cur_dimens;
	ArpGravity			mBodyFill;
	
	bool				dimens_changed;	
	bool				inhibit_layout;
	bool				focus_shown;
	bool				layout_activated;
	
	/* Current constraints.  This is used to hold parent-defined
	 * state information about the object; it is not for use
	 * by this implementor.
	 */
	BMessage			constraints;
	
	// Current LayoutFrame() and LayoutBounds(), as
	// described for those functions above.
	BRect				cur_frame, body_frame;
	
	// The last frame this object/BView was sized to, used to
	// avoid unnessasary exchanges with the app_server.
	BRect				last_frame;
	status_t			error;
};

#endif
