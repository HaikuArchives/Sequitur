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
 * ArpRunningBar.h
 *
 * A layout class that organizes its children as a horizontal
 * or vertical row.  Each child object can be assigned a weight,
 * which is used to determine how to space the objects along
 * the horizontal or vertical axis.
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
 * • Add "make equal size" option.
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

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#define ARPLAYOUT_ARPRUNNINGBAR_H

#ifndef ARPLAYOUT_ARPLAYOUTVIEW_H
#include <ArpLayout/ArpLayoutView.h>
#endif

class _EXPORT ArpRunningBar : public ArpLayoutView {
private:
	typedef ArpLayoutView inherited;

public:
	ArpRunningBar(const char* name = 0, uint32 flags = 0);
	ArpRunningBar(BMessage* data, bool final=true);
	virtual ~ArpRunningBar();
	
	static ArpRunningBar*	Instantiate(BMessage* archive);
	virtual status_t		Archive(BMessage* data, bool deep=true) const;

	/* Parameters:
	 * "Orientation" (int32)  Orientation of the object --
	 * that is, one of B_HORIZONTAL or B_VERTICAL.
	 * B_HORIZONTAL means the children are placed side-by-side
	 * next to each other; B_VERTICAL places them on top of
	 * each other.
	 *
	 * "InsetLeft", "InsetRight", "InsetTop", "InsetBottom"
	 * (float or int32) Space to place on sides, between the
	 * children and this object's frame.  Floats specify the
	 * dimensions in character units (font height, width of a
	 * single "W" character), while int32s are absolute pixel
	 * dimensions.  All defaults are zero.
	 *
	 * "IntraSpace" (float or int32)  The amount of space to
	 * put between children; dimensions are as above, and the
	 * default is also zero.
	 *
	 * NOTE: Absolute dimensioning (int32) is not implemented.
	 */
	static const char*	OrientationP;
	static const char*	InsetLeftP;
	static const char*	InsetRightP;
	static const char*	InsetTopP;
	static const char*	InsetBottomP;
	static const char*	IntraSpaceP;
	
	/* Constraints:
	 * "Weight" (float) The weight of an object, relative to
	 * its other siblings.  This determines how much extra space
	 * is assigned to that object.
	 *
	 * "Fill" (int32) How to place a space-limited child in a
	 * larger area allocated to it.  Valid values are the ArpGravity
	 * flags.
	 *
	 * "AlignLabels" (bool) Indicate whether the labels of this
	 * object should be aligned with the position of its sibling's
	 * labels.
	 */
	static const char* WeightC;
	static const char* FillC;
	static const char* AlignLabelsC;
	
	virtual int 	LayoutChildSpace() const	{ return INT_MAX; }
	virtual BRect	HintLayoutChild(ArpBaseLayout* before = NULL) const;
	
protected:
  
	virtual void GetConstraintSuites(BMessage *data);
	virtual status_t QueryConstraint(const char* name,
									 BMessage* data) const;
									 
	virtual void	ComputeDimens(ArpDimens& dimens);
	virtual void	LayoutView(void);
	
private:
	void		initialize();
	
	ArpGlobalParam<int32>	PV_Orientation;
	ArpGlobalParam<float>	PV_InsetLeft;
	ArpGlobalParam<float>	PV_InsetRight;
	ArpGlobalParam<float>	PV_InsetTop;
	ArpGlobalParam<float>	PV_InsetBottom;
	ArpGlobalParam<float>	PV_IntraSpace;
	
	float		total_weight;		// WeightC constraint of all chidren
	float		min_weight;			// pref-min size of all children
	
	float		char_w, char_h;
	float		inset_l, inset_r, inset_t, inset_b;
	float		intra_s;
};

#endif
