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
 * ViewStubs.h
 *
 * Interfaces to various Be UI objects that need a bit
 * of extra help...
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * • ArpListView and ArpOutlineListView currently set their
 *   preferred height to the total height of the text in the
 *   list view.  This is wrong -- it should probably place some
 *   limit on this height, such as no more than six lines.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * • All of these should provide an ArpLayout parameters
 *   interface for setting their associated object's values.
 *
 * • It would probably be good to split this out into separate
 *   files.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * May 5, 1999:
 *	- ArpBox was not correctly propagating its child's maximum
 *	  dimensions up to itself.
 *	- Added special magic to ArpMenuBar to correctly redraw the
 *	  menu when resizing, without using B_FULL_UPDATE_ON_RESIZE.
 *
 * Dec 6, 1998:
 *	First public release.
 *
 */

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_VIEWSTUBS_H
#define ARPLAYOUT_VIEWSTUBS_H

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTHOOKS_H
#include <ArpLayout/ArpLayoutHooks.h>
#endif

#ifndef _VIEW_H
#include <be/interface/View.h>
#endif

#ifndef _MENUBAR_H
#include <be/interface/MenuBar.h>
#endif

#ifndef _BOX_H
#include <be/interface/Box.h>
#endif

#ifndef _BUTTON_H
#include <be/interface/Button.h>
#endif

#ifndef _COLORCONTROL_H
#include <be/interface/ColorControl.h>
#endif

#ifndef _LISTVIEW_H
#include <be/interface/ListView.h>
#endif

#ifndef _OUTLINELISTVIEW_H
#include <be/interface/OutlineListView.h>
#endif

#ifndef _MENUBAR_H
#include <be/interface/MenuBar.h>
#endif

#ifndef _MENUFIELD_H
#include <be/interface/MenuField.h>
#endif

#ifndef _TEXTCONTROL_H
#include <be/interface/TextControl.h>
#endif

/** -----------------------------------------------------------
 	ArpLayout interface to BBox.
 
 	@description
 	Addresses the following problems:
 
 	Need to place proper space between box and child.
 */
class _EXPORT ArpBox : public BBox, public ArpBaseLayout
{
private:
	typedef BBox inherited;

public:
	ArpBox(const char* name, const char* label,
			border_style border = B_FANCY_BORDER);
  	
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
  	void LayoutView(void);
  	
private:
	void initialize();
  	void ComputeIndents(void);
  	
	float indent_l, indent_t, indent_r, indent_b;
	float min_w;
};

/**	-----------------------------------------------------------
 	ArpLayout interface to BButton.
 	
 	@description
 	Addresses the following problems:

 	Needs to have B_FULL_UPDATE_ON_RESIZE set to correctly
 	resize itself.
 	
 	BButton does not completely see a font change until
 	its SetLabel() function is called.
 */
class _EXPORT ArpButton : public BButton, public ArpBaseLayout
{
private:
	typedef ArpButton inherited;
	
public:
	ArpButton(const char* name, const char* label,
				BMessage* message);
  	
	static ArpButton*	Instantiate(BMessage* archive);

	ARPLAYOUT_VIEWHOOKS(BButton);
	ARPLAYOUT_HANDLERHOOKS(BButton);
	ARPLAYOUT_SUITEHOOKS(BButton);
	ARPLAYOUT_ARCHIVEHOOKS(ArpButton, BButton, false);
	virtual void SetFocusShown(bool state, bool andParent=true) { }
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);

private:
	void initialize();

};

/**	-----------------------------------------------------------
 	ArpLayout interface to BListView.

 	@description
 	Addresses the following problems:

 	BListView does not return any useful preferred size.
 */
class _EXPORT ArpListView : public BListView, public ArpBaseLayout
{
private:
	typedef ArpBaseLayout inherited;

public:
	ArpListView(const char* name,
				list_view_type type=B_SINGLE_SELECTION_LIST);
	
	static ArpListView*	Instantiate(BMessage* archive);

	ARPLAYOUT_VIEWHOOKS(BListView);
	ARPLAYOUT_HANDLERHOOKS(BListView);
	ARPLAYOUT_SUITEHOOKS(BListView);
	ARPLAYOUT_ARCHIVEHOOKS(ArpListView, BListView, true);
	
	void	GetPreferredSize(float* w, float* h);
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);

private:
	void initialize();
};

/**	-----------------------------------------------------------
 	ArpLayout interface to BOutlineListView.
 	
 	@description
 	Addresses the following problems:
 	
 	BOutlineListView does not return any useful preferred size.
 	
 	@bug Font changes do not change the spacing of its items --
 		but the items in a sub-hierarchy will be re-spaced if
 		they are collapsed and then expanded again.
 */
class _EXPORT ArpOutlineListView : public BOutlineListView,
									public ArpBaseLayout
{
private:
	typedef ArpBaseLayout inherited;

public:
	ArpOutlineListView(const char* name,
				list_view_type type=B_SINGLE_SELECTION_LIST);
	
	static ArpOutlineListView*	Instantiate(BMessage* archive);

	ARPLAYOUT_VIEWHOOKS(BOutlineListView);
	ARPLAYOUT_HANDLERHOOKS(BOutlineListView);
	ARPLAYOUT_SUITEHOOKS(BOutlineListView);
	ARPLAYOUT_ARCHIVEHOOKS(ArpOutlineListView, BOutlineListView, true);
	
	void	GetPreferredSize(float* w, float* h);
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);

private:
	void initialize();
};

/**	-----------------------------------------------------------
 	ArpLayout interface to BMenuBar.
 	
 	@description
 	Addresses the following problems:

 	BMenuBar does not always return sane GetPreferredSize()
 	values, unless some special magic is performed.
 	
 	Need to distribute font changes to menu children.
 */
class _EXPORT ArpMenuBar : public BMenuBar, public ArpBaseLayout
{
private:
	typedef ArpBaseLayout inherited;

public:
	ArpMenuBar(const char* title,
				menu_layout layout = B_ITEMS_IN_ROW,
				bool resizeToFit = true);
	
	static ArpMenuBar*	Instantiate(BMessage* archive);

	ARPLAYOUT_VIEWHOOKS(BMenuBar);
	ARPLAYOUT_HANDLERHOOKS(BMenuBar);
	ARPLAYOUT_SUITEHOOKS(BMenuBar);
	ARPLAYOUT_ARCHIVEHOOKS(ArpMenuBar, BMenuBar, true);
	virtual void SetFocusShown(bool state, bool andParent=true) { }
	
	virtual void FrameResized(float new_width, float new_height);
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);

private:
	void initialize();
	void copy_font(BMenu* menu, BFont* font);
	BRect mLastBounds;
};

/**	-----------------------------------------------------------
 	ArpLayout interface to BMenuField.

 	@description
	Addresses the following problems:

	BMenuField does not seem to return anything useful
	from GetPreferredSize().
	
	Need to correctly manage SetDivider().
	
	Need to distribute font changes to menu children.
	
	Add parameters to control font of menu.
 */
class _EXPORT ArpMenuField : public BMenuField, public ArpBaseLayout
{
private:
	typedef ArpBaseLayout inherited;

public:
	ArpMenuField(const char* name, const char* label,
				BMenu* menu, bool fixed_size=false);
	virtual ~ArpMenuField();

	static ArpMenuField*	Instantiate(BMessage* archive);

	ARPLAYOUT_VIEWHOOKS(BMenuField);
	ARPLAYOUT_HANDLERHOOKS(BMenuField);
	ARPLAYOUT_SUITEHOOKS(BMenuField);
	ARPLAYOUT_ARCHIVEHOOKS(ArpMenuField, BMenuField, true);
	virtual void SetFocusShown(bool state, bool andParent=true) { }
	
	virtual void ParametersChanged(const ArpParamSet* params);
	
	static const char*	MenuFontP;
	static const char*	MenuBackColorP;
	static const char*	MenuForeColorP;
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);
	virtual void LayoutView();
  	
	ArpGlobalParam<rgb_color>	PV_MenuBackColor;
	ArpGlobalParam<rgb_color>	PV_MenuForeColor;
	ArpGlobalParam<BFont>		PV_MenuFont;
	
private:
	void initialize();
	void copy_attrs(BMenu* menu);
};

/**	-----------------------------------------------------------
	ArpLayout interface to BTextControl.

 	@description
	Addresses the following problems:

	Need to distribute font changes to text child.
	
	Add parameters to control fill color of child.
	
	Need more control over min/pref/max dimensions.
 */
class _EXPORT ArpTextControl : public BTextControl, public ArpBaseLayout
{
private:
	typedef ArpBaseLayout inherited;

public:
	ArpTextControl(const char* name, const char* label,
					const char* text, BMessage* message);
	virtual ~ArpTextControl();
	
	static ArpTextControl*	Instantiate(BMessage* archive);
	virtual void LayoutAttachedToWindow(void);
	
	ARPLAYOUT_VIEWHOOKS(BTextControl);
	ARPLAYOUT_HANDLERHOOKS(BTextControl);
	ARPLAYOUT_SUITEHOOKS(BTextControl);
	ARPLAYOUT_ARCHIVEHOOKS(ArpTextControl, BTextControl, true);
	
	virtual void ParametersChanged(const ArpParamSet* params);
	
	virtual void FrameResized(float new_width, float new_height);
	
	static const char*	FillBackColorP;
	static const char*	FillForeColorP;
	static const char*	FillFontP;
	static const char*	MinTextStringP;
	static const char*	PrefTextStringP;
	static const char*	MaxTextStringP;
	
protected:
	virtual void ComputeDimens(ArpDimens& dimens);
	virtual void LayoutView();
  	
	ArpGlobalParam<rgb_color>	PV_FillBackColor;
	ArpGlobalParam<rgb_color>	PV_FillForeColor;
	ArpGlobalParam<BFont>		PV_FillFont;
	ArpParam<BString>			PV_MinTextString;
	ArpParam<BString>			PV_PrefTextString;
	ArpParam<BString>			PV_MaxTextString;
	
private:
	void initialize();
	void copy_colors(BView* v);
	void copy_font(BView* v);
};

#endif
