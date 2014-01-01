/*
	
	ViewStubs.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	Interfaces to various Be UI objects that need a bit
	of extra help...
*/

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include "ArpLayout/ViewStubs.h"
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include "ArpKernel/ArpColor.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef _MENUITEM_H
#include <be/interface/MenuItem.h>
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
#endif

#include <float.h>
#include <algobase.h>
#include <string.h>

ArpMOD();

static BRect InitFrame(-10000,-10000,-9800,-9800);
//static BRect InitFrame(0,0,1,1);

/* ------ ArpBox object ------
 */
 
ArpBox::ArpBox(const char* name, const char* label,
				border_style border)
	: BBox(InitFrame, name, B_FOLLOW_NONE,
			B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP,
			border)
{
	SetGlobalParam(BasicFontP, "BoldFont");
	initialize();
	SetLabel(label);
}

void ArpBox::initialize()
{
	indent_l = indent_t = indent_r = indent_b = 0;
}

ArpBox* ArpBox::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpBox") ) 
		return new ArpBox(archive); 
	return NULL;
}

BRect ArpBox::HintLayoutChild(ArpBaseLayout* before) const
{
	const_cast<ArpBox*>(this)->ComputeIndents();
	BRect frm = LayoutBounds();
	frm.left += indent_l;
	frm.right -= indent_r;
	frm.top += indent_t;
	frm.bottom -= indent_b;
	return frm;
}

static property_info properties_box[] = {
	{ "Label",
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Textual label for box.", 1
	},
	{ "BorderStyle",
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Appearance of box border.", 2
	},
	0
};

void ArpBox::MessageReceived(BMessage *message)
{
	status_t err = B_OK;
	
	int32 index = 0;
	BMessage spec;
	const char* property = 0;
	int32 what=0;
	if( message->GetCurrentSpecifier(&index, &spec, &what, &property) == B_OK ) {
		BPropertyInfo prop_info(properties_box);
		int32 propi = prop_info.FindMatch(message, index, &spec, what, property);
		if( propi >= 0 ) {
			if( properties_box[propi].extra_data == 1 ) {
				if( message->what == B_SET_PROPERTY ) {
					const char* str = 0;
					err = message->FindString("data", &str);
					if( !err ) {
						SetLabel(str);
						InvalidateDimens();
						return;
					}
				} else if( message->what == B_GET_PROPERTY ) {
					BMessage ret(B_REPLY);
					err = ret.AddString("result", Label());
					ret.AddInt32("error", err);
					message->SendReply(&ret);
					return;
				}
			} else if( properties_box[propi].extra_data == 2 ) {
				if( message->what == B_SET_PROPERTY ) {
					int32 value = 0;
					err = message->FindInt32("data", &value);
					if( !err ) {
						SetBorder((border_style)value);
						InvalidateDimens();
						return;
					}
				} else if( message->what == B_GET_PROPERTY ) {
					BMessage ret(B_REPLY);
					err = ret.AddInt32("result", Border());
					ret.AddInt32("error", err);
					message->SendReply(&ret);
					return;
				}
			}
		}
	}

	if( LayoutMessageReceived(message) == B_OK ) return;
	BBox::MessageReceived(message);
}

BHandler*
ArpBox::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
						 int32 form, const char *property)
{
	BPropertyInfo prop_info(properties_box);
	if( prop_info.FindMatch(msg, index, specifier, form, property) >= 0 ) return this;
	BHandler* ret = 0;
	if( LayoutResolveSpecifier(&ret, msg, index, specifier,
							   form, property) == B_OK ) {
		return ret;
	}
	return BBox::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t ArpBox::GetSupportedSuites(BMessage *data)
{
	data->AddString("suites", "suite/vnd.ARP.ArpBox");
	BPropertyInfo prop_info(properties_box);
	data->AddFlat("messages", &prop_info);
	LayoutGetSupportedSuites(data);
	return BBox::GetSupportedSuites(data);
}

void ArpBox::ComputeIndents(void)
{
	float fw = BasicFont()->StringWidth("W");
	font_height fhs;
	BasicFont()->GetHeight(&fhs);
	float fh = fhs.ascent+fhs.descent+fhs.leading;
	float ps = PenSize();
	if( ps < 1 ) ps = 1;
	indent_l = indent_r = floor( (fw/2) + (ps*2) + .5 );
	indent_b = floor( (fh/2) + (ps*2) + .5 );

	const char* lab = Label();
	min_w = 0;
	if( lab && *lab ) {
		min_w = BasicFont()->StringWidth(lab)
				+ indent_l + indent_r;
		indent_t = floor( fh + ps*2 + .5 );
	} else {
		indent_t = floor( (fh/2) + ps*2 + .5 );
	}
}

void ArpBox::ComputeDimens(ArpDimens& cur_dimens)
{
	ArpBaseLayout::ComputeDimens(cur_dimens);
	
	ArpUniDimens& curx(cur_dimens.X());
	ArpUniDimens& cury(cur_dimens.Y());
	
	if( curx.TotalMax() <= 0 ) curx.SetTo(0, 30, 30, 30, 0);
	if( cury.TotalMax() <= 0 ) cury.SetTo(0, 30, 30, 30, 0);
	
	ComputeIndents();
	curx.AddLabel(indent_l,indent_r);
	cury.AddLabel(indent_t,indent_b);
}

void ArpBox::Layout(void)
{
	BRect frm = LayoutBounds();
	BRect bdy = BodyBounds();

	frm.left += indent_l;
	frm.top += indent_t;
	frm.right -= indent_r;
	frm.bottom -= indent_b;
	
	ArpBaseLayout* child = LayoutChildAt(0);
	if( child ) {
		ArpD(cdb << ADH << "Box moving child to Lab:" << frm
				<< " / Bod:" << bdy << endl);
		child->SetLayout(frm, bdy);
	}	
}

/* ------ ArpButton object ------
 */
 
ArpButton::ArpButton(const char* name, const char* label,
						BMessage* message)
	:BButton(InitFrame, name, label, message, B_FOLLOW_NONE,
				B_WILL_DRAW|B_NAVIGABLE|B_FULL_UPDATE_ON_RESIZE)
{
	initialize();
}

void ArpButton::initialize()
{
	SetBodyFill(ArpCenter);
}

ArpButton* ArpButton::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpButton") ) 
		return new ArpButton(archive); 
	return NULL;
}

void ArpButton::ComputeDimens(ArpDimens& cur_dimens)
{
	SetLabel(Label());
	get_view_dimens(&cur_dimens, this, false);
}

/* ------ ArpListView object ------
 */
 
ArpListView::ArpListView(const char* name, list_view_type type)
	:BListView(InitFrame, name, type, B_FOLLOW_NONE)
{
	SetGlobalParam(BackgroundColorP, "FillBackColor");
	SetGlobalParam(ForegroundColorP, "FillTextColor");
	initialize();
}

void ArpListView::initialize()
{
	SetBodyFill(ArpCenter);
}

ArpListView* ArpListView::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpListView") ) 
		return new ArpListView(archive); 
	return NULL;
}

void ArpListView::GetPreferredSize(float* w, float* h)
{
	if( w ) *w = 0;
	if( h ) *h = 0;
}

void ArpListView::ComputeDimens(ArpDimens& cur_dimens)
{
	float min_w=0, min_h=0;
	float pref_w=0, pref_h=0;
	
	int32 num = CountItems();
	int32 max_num = 4;
	for( int32 i=0; i<num; i++ ) {
		BListItem* item = ItemAt(i);
		if( item ) {
			item->Update(this,BasicFont());
			float w = item->Width();
			float h = item->Height();
			min_h = max(min_h,h);
			pref_w = max(pref_w,w);
			if( max_num > 0 ) pref_h += h;
			max_num--;
		}
	}
	pref_h+=2;
	float fw = BasicFont()->StringWidth("WWWW");
	font_height fhs;
	BasicFont()->GetHeight(&fhs);
	float fh = fhs.ascent+fhs.descent+fhs.leading;
	min_w = max(min_w,fw);
	min_h = max(min_h,fh);
	pref_w = max(pref_w,min_w);
	pref_h = max(pref_h,min_h);
	
	cur_dimens.X().SetTo(0, min_w, pref_w, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, min_h, pref_h, ArpAnySize, 0);
}

/* ------ ArpOutlineListView object ------
 */
 
ArpOutlineListView::ArpOutlineListView(const char* name, list_view_type type)
	:BOutlineListView(InitFrame, name, type, B_FOLLOW_NONE)
{
	SetGlobalParam(BackgroundColorP, "FillBackColor");
	SetGlobalParam(ForegroundColorP, "FillTextColor");
	initialize();
}

void ArpOutlineListView::initialize()
{
	SetBodyFill(ArpCenter);
}

ArpOutlineListView* ArpOutlineListView::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpOutlineListView") ) 
		return new ArpOutlineListView(archive); 
	return NULL;
}

void ArpOutlineListView::GetPreferredSize(float* w, float* h)
{
	if( w ) *w = 0;
	if( h ) *h = 0;
}

void ArpOutlineListView::ComputeDimens(ArpDimens& cur_dimens)
{
	float min_w=0, min_h=0;
	float pref_w=0, pref_h=0;
	
	int32 num = CountItems();
	int32 max_num = 4;
	for( int32 i=0; i<num; i++ ) {
		BListItem* item = ItemAt(i);
		if( item ) {
			item->Update(this,BasicFont());
			float w = item->Width();
			float h = item->Height();
			min_h = max(min_h,h);
			pref_w = max(pref_w,w);
			if( max_num > 0 ) pref_h += h;
			max_num--;
		}
	}
	pref_h+=2;
	float fw = BasicFont()->StringWidth("WWWW");
	font_height fhs;
	BasicFont()->GetHeight(&fhs);
	float fh = fhs.ascent+fhs.descent+fhs.leading;
	min_w = max(min_w,fw);
	min_h = max(min_h,fh);
	pref_w = max(pref_w,min_w);
	pref_h = max(pref_h,min_h);
	
	cur_dimens.X().SetTo(0, min_w, pref_w, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, min_h, pref_h, ArpAnySize, 0);
}

/* ------ ArpMenuBar object ------
 */
 
ArpMenuBar::ArpMenuBar(const char* title, menu_layout layout,
						bool resizeToFit)
	:BMenuBar(InitFrame, title, B_FOLLOW_NONE,
				layout, resizeToFit)
{
	SetGlobalParam(BackgroundColorP, "MenuBackColor");
	SetGlobalParam(ForegroundColorP, "MenuTextColor");
	SetGlobalParam(BasicFontP, "MenuFont");
	initialize();
}

void ArpMenuBar::initialize()
{
	mLastBounds = Bounds();
	
	SetBodyFill(ArpCenter);
	
	SetFlags(Flags() | B_FRAME_EVENTS);
}

ArpMenuBar* ArpMenuBar::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpMenuBar") ) 
		return new ArpMenuBar(archive); 
	return NULL;
}

void ArpMenuBar::FrameResized(float new_width, float new_height)
{
#if 0
	const BRect bounds(Bounds());
	if( Window() ) {
		ArpD(cdb << ADH << "Bounds new: " << bounds
				<< ", old: " << mLastBounds << endl);
		if( bounds.Width() < mLastBounds.Width() ) {
			Invalidate(BRect(bounds.right-1, bounds.top,
							 bounds.right, bounds.bottom));
		} else if( bounds.Width() > mLastBounds.Width() ) {
			Invalidate(BRect(mLastBounds.right-1, bounds.top,
							 mLastBounds.right, bounds.bottom));
		}
		if( bounds.Height() < mLastBounds.Height() ) {
			Invalidate(BRect(bounds.left, bounds.bottom-1,
							 bounds.right, bounds.bottom));
		} else if( bounds.Height() > mLastBounds.Height() ) {
			Invalidate(BRect(bounds.left, mLastBounds.bottom-1,
							 bounds.right, mLastBounds.bottom));
		}
	}
	mLastBounds = bounds;
#endif
	BMenuBar::FrameResized(new_width, new_height);
}

void ArpMenuBar::copy_font(BMenu* menu, BFont* font)
{
	if( !menu ) return;
	int32 num = menu->CountItems();
	for( int32 i=0; i<num; i++ ) {
		BMenu* child = menu->SubmenuAt(i);
		if( child ) {
			copy_font(child,font);
			child->SetFont(font);
			child->InvalidateLayout();
		}
	}
}

void ArpMenuBar::ComputeDimens(ArpDimens& cur_dimens)
{
	InvalidateLayout();
	// Copy my font to all children
	BFont font;
	GetFont(&font);
	copy_font((BMenu*)this,&font);
	get_view_dimens(&cur_dimens, this, false);
	cur_dimens.X().SetMaxBody(ArpAnySize);
}

/* ------ ArpMenuField object ------
 */
 
const char* ArpMenuField::MenuFontP = "MenuFont";
const char* ArpMenuField::MenuBackColorP = "MenuBackColor";
const char* ArpMenuField::MenuForeColorP = "MenuForeColor";

enum {
	MenuFieldBackColorIdx = 0,
	MenuFieldForeColorIdx,
	MenuFieldFontIdx
};

enum {
	MENU_PROPS_CHANGED = 1<<0
};

static property_info properties_menufield[] = {
	{ const_cast<char*>(ArpMenuField::MenuBackColorP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Menu background color of object.", 0,
	  { B_RGB_COLOR_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpMenuField::MenuForeColorP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Menu foreground color of object.", 0,
	  { B_RGB_COLOR_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpMenuField::MenuFontP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Menu font of object.", 0,
	  { FFont::FONT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	0
};

static const parameter_info parameters_menufield[] = {
	{ sizeof(parameter_info), &properties_menufield[MenuFieldBackColorIdx],
	  MENU_PROPS_CHANGED, ARP_INVALIDATE_VIEW, 0
	},
	{ sizeof(parameter_info), &properties_menufield[MenuFieldForeColorIdx],
	  MENU_PROPS_CHANGED, ARP_INVALIDATE_VIEW, 0
	},
	{ sizeof(parameter_info), &properties_menufield[MenuFieldFontIdx],
	  MENU_PROPS_CHANGED, ARP_INVALIDATE_DIMENS, 0
	},
	{ 0, 0, 0, 0, 0 }
};

static const parameter_suite suite_menufield = {
	sizeof(parameter_suite),
	"suite/vnd/ARP.ArpMenuField",
	properties_menufield
};

ArpMenuField::ArpMenuField(const char* name, const char* label,
						   BMenu* menu, bool fixed_size)
	:BMenuField(InitFrame, name, label, menu, fixed_size, B_FOLLOW_NONE,
				B_WILL_DRAW|B_NAVIGABLE|B_FRAME_EVENTS)
{
	initialize();
}

void ArpMenuField::initialize()
{
	if( ParamSet() ) {
		ParamSet()->AddParameters(parameters_menufield, &suite_menufield);
		PV_MenuBackColor.Init(ParamSet(), MenuBackColorP,
							  ui_color(B_PANEL_BACKGROUND_COLOR),
							  "MenuBackColor");
		rgb_color black;
		black.red = black.green = black.blue = 0;
		black.alpha = 255;
		PV_MenuForeColor.Init(ParamSet(), MenuForeColorP,
							  black, "MenuTextColor");
		PV_MenuFont.Init(ParamSet(), MenuFontP,
						 *BasicFont(), "PlainFont");
	}
	
	SetBodyFill(ArpWest);
}

ArpMenuField::~ArpMenuField()
{
	if( ParamSet() ) ParamSet()->RemoveParameters(parameters_menufield);
}

ArpMenuField* ArpMenuField::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpMenuField") ) 
		return new ArpMenuField(archive); 
	return NULL;
}

void ArpMenuField::copy_attrs(BMenu* menu)
{
	if( !menu ) return;
	int32 num = menu->CountItems();
	for( int32 i=0; i<num; i++ ) {
		BMenu* child = menu->SubmenuAt(i);
		if( child ) {
			copy_attrs(child);
			// the menu doesn't seem to see this.
			child->SetFont(&PV_MenuFont);
			child->SetHighColor(PV_MenuForeColor);
			child->SetLowColor(PV_MenuBackColor);
			child->SetViewColor(PV_MenuBackColor);
			child->InvalidateLayout();
		}
	}
}

void ArpMenuField::ParametersChanged(const ArpParamSet* params)
{
	if( params->GetChanges(parameters_menufield)&MENU_PROPS_CHANGED ) {
		if( MenuBar() ) {
			copy_attrs(MenuBar());
			MenuBar()->SetFont(&PV_MenuFont);
			MenuBar()->SetHighColor(PV_MenuForeColor);
			MenuBar()->SetLowColor(PV_MenuBackColor);
			MenuBar()->SetViewColor(PV_MenuBackColor);
		}
		if( Menu() ) {
			copy_attrs(Menu());
		}
		InvalidateView();
	}
	inherited::ParametersChanged(params);
}

void ArpMenuField::ComputeDimens(ArpDimens& cur_dimens)
{
	BMenu* menu = MenuBar();
	BMenu* popup = Menu();
	
	if( menu ) {
		copy_attrs(menu);
		copy_attrs(popup);
		menu->SetFont(&PV_MenuFont);
		menu->InvalidateLayout();
	}
	
	get_view_dimens(&cur_dimens, this, false);
	
	font_height fhs;
	BasicFont()->GetHeight(&fhs);
	const float fh = fhs.ascent+fhs.descent+fhs.leading;
	float fw = BasicFont()->StringWidth("WWWW");
	
	float pref_w=0;
	if( popup ) {
		int32 num = popup->CountItems();
		for( int32 i=0; i<num; i++ ) {
			BMenuItem* item = popup->ItemAt(i);
			if( item ) {
				const float w=BasicFont()->StringWidth(item->Label());
				ArpD(cdb << ADH << "Dimensions for popup label "
							<< item->Label() << ": " << w << endl);
				if( w > pref_w ) pref_w = w;
			}
		}
	}
	
	cur_dimens.Y().SetTo(0, fh+12, fh+12, fh+12, 0);
	
	float labelWidth = (Label() && *Label())
		? BasicFont()->StringWidth(Label())	+ BasicFont()->StringWidth(" ")
		: 0;
	
	cur_dimens.X().SetTo(labelWidth,
						 (fw < pref_w ? fw : pref_w) + 20,
						 pref_w + 20, pref_w + 20,
						 0);
}

void ArpMenuField::Layout()
{
	SetDivider(BodyFrame().left - LayoutFrame().left);
}

/* ------ ArpTextControl object ------
 */
 
const char* ArpTextControl::FillBackColorP = "FillBackColor";
const char* ArpTextControl::FillForeColorP = "FillForeColor";
const char* ArpTextControl::FillFontP = "FillFont";
const char* ArpTextControl::MinTextStringP = "MinTextString";
const char* ArpTextControl::PrefTextStringP = "PrefTextString";
const char* ArpTextControl::MaxTextStringP = "MaxTextString";

enum {
	TextCtrlBackColorIdx,
	TextCtrlForeColorIdx,
	TextCtrlFillFontIdx,
	TextCtrlMinTextIdx,
	TextCtrlPrefTextIdx,
	TextCtrlMaxTextIdx,
};

enum {
	TEXTCTRL_COLOR_CHANGED = 1<<0,
	TEXTCTRL_FONT_CHANGED = 1<<1
};

static property_info properties_textcontrol[] = {
	{ const_cast<char*>(ArpTextControl::FillBackColorP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Text fill background color of object.", 0,
	  { B_RGB_COLOR_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpTextControl::FillForeColorP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Text fill foreground color of object.", 0,
	  { B_RGB_COLOR_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpTextControl::FillFontP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Text fill font of object.", 0,
	  { FFont::FONT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpTextControl::MinTextStringP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "String defining minimum space for control.", 0,
	  { B_STRING_TYPE, 0 }
	},
	{ const_cast<char*>(ArpTextControl::PrefTextStringP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "String defining preferred space for control.", 0,
	  { B_STRING_TYPE, 0 }
	},
	{ const_cast<char*>(ArpTextControl::MaxTextStringP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "String defining maximum space for control.", 0,
	  { B_STRING_TYPE, 0 }
	},
	0
};

static const parameter_info parameters_textcontrol[] = {
	{ sizeof(parameter_info), &properties_textcontrol[TextCtrlBackColorIdx],
	  TEXTCTRL_COLOR_CHANGED, ARP_INVALIDATE_VIEW, 0
	},
	{ sizeof(parameter_info), &properties_textcontrol[TextCtrlForeColorIdx],
	  TEXTCTRL_COLOR_CHANGED, ARP_INVALIDATE_VIEW, 0
	},
	{ sizeof(parameter_info), &properties_textcontrol[TextCtrlFillFontIdx],
	  TEXTCTRL_FONT_CHANGED, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties_textcontrol[TextCtrlMinTextIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties_textcontrol[TextCtrlPrefTextIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties_textcontrol[TextCtrlMaxTextIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ 0, 0, 0, 0, 0 }
};

static const parameter_suite suite_textcontrol = {
	sizeof(parameter_suite),
	"suite/vnd.ARP.ArpTextControl",
	properties_textcontrol
};

ArpTextControl::ArpTextControl(const char* name, const char* label,
								const char* text, BMessage* message)
	:BTextControl(InitFrame, name, label, text, message,
					B_FOLLOW_NONE)
{
	SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	initialize();
}

void ArpTextControl::initialize()
{
	if( ParamSet() ) {
		ParamSet()->AddParameters(parameters_textcontrol, &suite_textcontrol);
		PV_FillBackColor.Init(ParamSet(), FillBackColorP,
							  tint_color( ui_color(B_PANEL_BACKGROUND_COLOR),
										  B_LIGHTEN_MAX_TINT ),
							  "FillBackColor");
		rgb_color black;
		black.red = black.green = black.blue = 0;
		black.alpha = 255;
		PV_FillForeColor.Init(ParamSet(), FillForeColorP,
							  black, "FillTextColor");
		PV_FillFont.Init(ParamSet(), FillFontP,
						 *BasicFont(), "PlainFont");
		PV_MinTextString.Init(ParamSet(), MinTextStringP, "");
		PV_PrefTextString.Init(ParamSet(), PrefTextStringP, "");
		PV_MaxTextString.Init(ParamSet(), MaxTextStringP, "");
	}
	
	SetBodyFill(ArpWest);
}

ArpTextControl::~ArpTextControl()
{
	if( ParamSet() ) ParamSet()->RemoveParameters(parameters_textcontrol);
}

ArpTextControl* ArpTextControl::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpTextControl") ) 
		return new ArpTextControl(archive); 
	return NULL;
}

void ArpTextControl::LayoutAttachedToWindow(void)
{
	copy_colors(this);
	copy_font(this);
}

void ArpTextControl::copy_colors(BView* v)
{
	// Make no assumptions about the structure of the children.
	if( !v ) return;
	int32 num = v->CountChildren();
	for( int32 i=0; i<num; i++ ) {
		BView* child = v->ChildAt(i);
		if( child ) {
			copy_colors(child);
			BTextView* text = dynamic_cast<BTextView*>(child);
			if( text ) {
				text->SetFontAndColor(NULL, B_FONT_ALL, &PV_FillForeColor);
			}
			child->SetViewColor(PV_FillBackColor);
			child->SetLowColor(PV_FillBackColor);
			child->SetHighColor(PV_FillForeColor);
			child->Invalidate();
		}
	}
}

void ArpTextControl::ParametersChanged(const ArpParamSet* params)
{
	if( params->GetChanges(parameters_textcontrol)&TEXTCTRL_COLOR_CHANGED ) {
		copy_colors((BView*)this);
		InvalidateView();
	}
	if( params->GetChanges(parameters_textcontrol)&TEXTCTRL_FONT_CHANGED ) {
		copy_font((BView*)this);
		InvalidateDimens();
	}
	inherited::ParametersChanged(params);
}

void ArpTextControl::FrameResized(float new_width, float new_height)
{
	ArpD(cdb << ADH << "ArpTextControl: FrameResized(" << new_width
			<< ", " << new_height << ")" << endl);
	BTextView* text = dynamic_cast<BTextView*>(ChildAt(0));
	if( !text ) ArpD(cdb << ADH << "!!! No BTextView !!!" << endl);
	if( text ) {
		ArpD(cdb << ADH << "BTextView: Initial Bounds=" << text->Bounds()
			<< ", TextRect=" << text->TextRect() << endl);
	}
	BTextControl::FrameResized(new_width, new_height);
	BRect textBounds(text->Bounds());
	BRect textRect(text->TextRect());
	textRect.right = textRect.left + text->LineWidth(0)-1;
	if( textRect.Width() < textBounds.Width() ) {
		textRect.right = textRect.left + textBounds.Width()-1;
	}
	text->SetTextRect(textRect);
	if( text ) {
		ArpD(cdb << ADH << "BTextView: Finish Bounds=" << text->Bounds()
			<< ", TextRect=" << text->TextRect() << endl);
	}
}

void ArpTextControl::copy_font(BView* v)
{
	// Make no assumptions about the structure of the children.
	if( !v ) return;
	int32 num = v->CountChildren();
	for( int32 i=0; i<num; i++ ) {
		BView* child = v->ChildAt(i);
		if( child ) {
			copy_font(child);
			BTextView* text = dynamic_cast<BTextView*>(child);
			if( text ) {
				text->SetFontAndColor(&PV_FillFont);
			} else {
				child->SetFont(&PV_FillFont);
			}
		}
	}
}

void ArpTextControl::ComputeDimens(ArpDimens& cur_dimens)
{
	// Copy fill font to all children
	copy_font((BView*)this);

	float vw=0, vh=0;
	GetPreferredSize(&vw,&vh);
	
	const float divw = BasicFont()->StringWidth(Label())
					 + BasicFont()->StringWidth("  ");
	vw = divw+12+PV_FillFont.Value().StringWidth("WWWWWW");
	
	cur_dimens.X().SetTo(divw, vw-divw-12, vw-divw-12, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, vh, vh, vh, 0);
	
	//printf("Left: %f, Width: %f, Divider: %f\n",
	//		Frame().left, Frame().Width(), Divider());
	ArpUniDimens& dx = cur_dimens.X();
	float minb = dx.MinBody();
	float prefb = dx.PrefBody();
	float maxb = dx.MaxBody();
	if( PV_MinTextString.Value().Length() > 0 ) {
		minb = PV_FillFont.Value().StringWidth(PV_MinTextString.Value().String());
	}
	if( PV_PrefTextString.Value().Length() > 0 ) {
		prefb = PV_FillFont.Value().StringWidth(PV_PrefTextString.Value().String());
	}
	if( PV_MaxTextString.Value().Length() > 0 ) {
		maxb = PV_FillFont.Value().StringWidth(PV_MaxTextString.Value().String());
	}
	
	dx.SetBody(minb, prefb, maxb);
	dx.AddBody(12);
}

void ArpTextControl::Layout()
{
	SetDivider(BodyFrame().left - LayoutFrame().left);
}
