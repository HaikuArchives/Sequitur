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
 * ArpScrollArea.h
 *
 * A layout class that places scroll bars around an interior
 * area.  Only one child is allowed, and it must possess its
 * own BView to which the scroller bars can be attached.
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
 *
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef ARPLAYOUT_ARPSCROLLAREA_H
#define ARPLAYOUT_ARPSCROLLAREA_H

#ifndef ARPLAYOUT_ARPLAYOUTVIEW_H
#include <ArpLayout/ArpLayoutView.h>
#endif

class _EXPORT ArpScrollArea : public ArpLayoutView {
private:
	typedef	ArpLayoutView inherited;
	
public:
	ArpScrollArea(const char* name = 0, uint32 flags = B_WILL_DRAW);
	ArpScrollArea(BMessage* data, bool final=true);
	virtual ~ArpScrollArea();
	
	static ArpScrollArea*	Instantiate(BMessage* archive);
	virtual status_t		Archive(BMessage* data, bool deep=true) const;

	/* Parameters:
	 * "ScrollHorizontal" and "ScrollVertical"
	 * (bool)  Indicate whether a horizontal and/or vertical
	 * scroll bar should be shown to control the interior view.
	 * A true value indicates that respective scroll bar will
	 * be shown.  Both defaults are false.
	 *
	 * "InsetCorner" (bool)  Indicate whether space should
	 * be reserved for a control in the bottom-right corner.
	 * Set this to true for scroll views placed in the
	 * bottom-right corner of a document window.
	 * Default is false.
	 *
	 * "BorderStyle" (int32)  Indicate type of frame to draw
	 * around scroll area, as per BScrollView: B_PLAIN_BORDER,
	 * B_FANCY_BORDER, or B_NO_BORDER.  Default is B_FANCY_BORDER.
	 */
	static const char* ScrollHorizontalP;
	static const char* ScrollVerticalP;
	static const char* InsetCornerP;
	static const char* BorderStyleP;
	
	/* Retrieve scroll bars being used.
	 */
	BScrollBar*		VScrollBar(void);
	BScrollBar*		HScrollBar(void);
	
	/* Need to attach scroll bars to child view, when it is added.
	 */
	virtual ArpBaseLayout* AddLayoutChild(ArpBaseLayout* v,
								   const BMessage& c = ArpNoParams,
								   ArpBaseLayout* before = NULL);
	virtual	bool		RemoveLayoutChild(ArpBaseLayout* child);
	virtual int 		LayoutChildSpace() const	{ return CountLayoutChildren() <= 0 ? 1 : 0; }
	virtual BRect		HintLayoutChild(ArpBaseLayout* before = NULL) const;
	
	virtual void		SetFocusShown(bool state, bool andParent);
	virtual void		SetLayoutActivated(bool state);
	
	virtual void		ParametersChanged(const ArpParamSet* params);
	
protected:
  
	virtual void	ComputeDimens(ArpDimens& dimens);
	virtual void	Layout(void);
	virtual void	AttachView(BView* par_view, BView* before);
	virtual void	DrawLayout(BView* inside, BRect region);
	
private:
	void			initialize();
	void			attach_child(ArpBaseLayout* v);
	void			update_scroll_bars(void);
	
	// Returns the child view that is being targetted.
	BView*			get_child(void);
	
	// The last location of the scroll area in its parent.  This
	// is stored so that we can invalidate that area when moved or
	// resized.
	BRect			mLastFrame;
	
	ArpGlobalParam<bool>	PV_ScrollHorizontal;
	ArpGlobalParam<bool>	PV_ScrollVertical;
	ArpGlobalParam<bool>	PV_InsetCorner;
	ArpGlobalParam<int32>	PV_BorderStyle;
	
	// The vertical and horizontal scrollers.  These are created
	// when the object is instantiated, and are valid for its
	// entire existance.
	BScrollBar*		scroll_h;
	BScrollBar*		scroll_v;
	
	// Current on/off status of scrollers.
	bool			do_vert, do_horiz;
	
	// If true, there is something in the corner (most likely the
	// window's size control), so scroll bars should always be
	// inset from it.
	bool			inset_corner;
	
	// Type of border to draw around view
	int32			border;
	
	// The scrollers' preferred width (vertical scroller) and
	// height (horizontal scroller)
	float			s_width, s_height;
};

#endif
