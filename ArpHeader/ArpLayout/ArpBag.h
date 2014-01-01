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
 * ArpBag.h
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
 * NOT IMPLEMENTED!
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

#ifndef ARPLAYOUT_ARPBAG_H
#define ARPLAYOUT_ARPBAG_H

#ifndef ARPLAYOUT_ARPLAYOUTVIEW_H
#include <ArpLayout/ArpLayoutView.h>
#endif

class _EXPORT ArpBag : public ArpLayoutView {
private:
	typedef ArpLayoutView inherited;

public:
	ArpBag(const char* name = 0, uint32 flags = 0);
	ArpBag(BMessage* data, bool final=true);
	virtual ~ArpBag();
	
	static ArpBag*			Instantiate(BMessage* archive);
	virtual status_t		Archive(BMessage* data, bool deep=true) const;

	/* Parameters:
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
	static const char*	InsetLeftP;
	static const char*	InsetRightP;
	static const char*	InsetTopP;
	static const char*	InsetBottomP;
	static const char*	IntraSpaceP;
	
	/* Constraints:
	 * "X" (int32) The column this child should be placed in the bag.
	 *
	 * "Y" (int32) The row this child should be place in the bag.
	 *
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
	static const char* XC;
	static const char* YC;
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
	virtual void	Layout(void);
	
private:
	void		initialize();
	
	ArpGlobalParam<float>	PV_InsetLeft;
	ArpGlobalParam<float>	PV_InsetRight;
	ArpGlobalParam<float>	PV_InsetTop;
	ArpGlobalParam<float>	PV_InsetBottom;
	ArpGlobalParam<float>	PV_IntraSpace;
	
	struct totals {
		float		total_weight;		// WeightC constraint of all chidren
		float		min_weight;			// pref-min size of all children
	};
	vector<ArpDimens>	fColumns;
	vector<ArpDimens>	fRows;
	totals				fColumnTotals;
	totals				fRowTotals;
	
	float		char_w, char_h;
	float		inset_l, inset_r, inset_t, inset_b;
	float		intra_s;
};

#endif
