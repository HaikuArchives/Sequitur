/*
 * Copyright (c)2000 by Angry Red Planet.
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
 * ViewStubs.h
 *
 * This is an addendum to ArpLayout/ViewStubs.h, created specifically
 * for using the knob control in the layout routines.  When I understand
 * how all this works better, hopefully this can be merged in with
 * the original ViewStubs.h
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * - None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * - Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * August 22, 2000:
 * Created this file
 */

/**
@author Eric Hackborn
@package ArpViews
**/
 
#ifndef ARPVIEWS_VIEWSTUBS_H
#define ARPVIEWS_VIEWSTUBS_H

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPVIEWS_ARPINTCONTROL_H
#include <ArpViews/ArpIntControl.h>
#endif

#ifndef ARPVIEWS_ARPKNOBCONTROL_H
#include <ArpViews/ArpKnobControl.h>
#endif

/** -----------------------------------------------------------
 	ArpLayoutKnob macro for ArpKnobControl
 
 	@description
	This class is intended to function as a container for the
	knob and any adornments it might have, like a label or
	an associated int control.
 */
class _EXPORT ArpLayoutKnob : public BView, public ArpBaseLayout
{
private:
	typedef BView inherited;

public:
	ArpLayoutKnob(	const char* name, const char* label,
					int32 minValue, int32 maxValue,
					bool showInt);

	virtual	void	GetPreferredSize(float* width, float* height);

#if 0
	static ArpBox*		Instantiate(BMessage* archive);

	virtual int 		LayoutChildSpace() const	{ return CountLayoutChildren() <= 0 ? 1 : 0; }
	virtual BRect		HintLayoutChild(ArpBaseLayout* before = NULL) const;
	
	ARPLAYOUT_VIEWHOOKS(BBox);
	ARPLAYOUT_ARCHIVEHOOKS(ArpBox, BBox, false);
	
	virtual void MessageReceived(BMessage *message);
	virtual BHandler*
	ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
					 int32 form, const char *property);
	virtual status_t GetSupportedSuites(BMessage *data);
	
	virtual BHandler* LayoutHandler() { return this; }
	virtual const BHandler* LayoutHandler() const { return this; }
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);
  	void Layout(void);
  	
private:
	void initialize();
  	void ComputeIndents(void);
  	
	float indent_l, indent_t, indent_r, indent_b;
	float min_w;
#endif
};

#endif
