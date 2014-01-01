/*
	
	ArpScrollArea.c
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	A layout class that places scroll bars around an interior
	area.  Only one child is allowed, and it must possess its
	own BView to which the scrollers can be attached.
*/

#ifndef ARPLAYOUT_ARPSCROLLAREA_H
#include <ArpLayout/ArpScrollArea.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
#endif

#ifndef _SCROLLBAR_H
#include <be/interface/ScrollBar.h>
#endif

#include <algobase.h>
#include <string.h>

ArpMOD();

const char* ArpScrollArea::ScrollHorizontalP = "ScrollHorizontal";
const char* ArpScrollArea::ScrollVerticalP = "ScrollVertical";
const char* ArpScrollArea::InsetCornerP = "InsetCorner";
const char* ArpScrollArea::BorderStyleP = "BorderStyle";

enum {
	ScrollHorizontalIdx = 0,
	ScrollVerticalIdx,
	InsetCornerIdx,
	BorderStyleIdx
};

enum {
	SCROLL_BARS_CHANGED = 1<<0
};

static property_info properties[] = {
	{ const_cast<char*>(ArpScrollArea::ScrollHorizontalP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Include horizontal scroll bar?", 0,
	  { B_BOOL_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpScrollArea::ScrollVerticalP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Include vertical scroll bar?", 0,
	  { B_BOOL_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpScrollArea::InsetCornerP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Leave corner inset for window size control?", 0,
	  { B_BOOL_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpScrollArea::BorderStyleP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Kind of border to draw around scroll area.", 0,
	  { B_INT32_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	0
};

static const parameter_info parameters[] = {
	{ sizeof(parameter_info), &properties[ScrollHorizontalIdx],
	  SCROLL_BARS_CHANGED, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[ScrollVerticalIdx],
	  SCROLL_BARS_CHANGED, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[InsetCornerIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[BorderStyleIdx],
	  0, ARP_INVALIDATE_DIMENS, arp_border_style_values,
	},
	{ 0, 0, 0, 0, 0 }
};

static const parameter_suite suite = {
	sizeof(suite),
	"suite/vnd.ARP.ArpScrollArea",
	properties
};

/* ------ ArpScrollArea constructor and destructor ------
 *
 * The various ways to create and destroy ArpScrollArea
 * objects.
 */
 
ArpScrollArea::ArpScrollArea(const char* name, uint32 flags)
		: ArpLayoutView(name, flags)
{
	initialize();
}

ArpScrollArea::ArpScrollArea(BMessage* data, bool final)
	: ArpLayoutView(data, false)
{
	initialize();
	
	ParamSet()->InstantiateParams(data, parameters);
	if( final ) InstantiateParams(data);
	
	ArpBaseLayout* child = LayoutChildAt(0);
	if( child ) attach_child(child);
}

void ArpScrollArea::initialize()
{
	if( ParamSet() ) {
		ParamSet()->AddParameters(parameters, &suite);
		PV_ScrollHorizontal.Init(ParamSet(), ScrollHorizontalP, false);
		PV_ScrollVertical.Init(ParamSet(), ScrollVerticalP, false);
		PV_InsetCorner.Init(ParamSet(), InsetCornerP, false);
		PV_BorderStyle.Init(ParamSet(), BorderStyleP, B_FANCY_BORDER);
	}
	
	SetBodyFill(ArpCenter);
	
	scroll_h = new BScrollBar(BRect(0,0,100,B_H_SCROLL_BAR_HEIGHT),
							"hscroll", NULL, 0, 0, B_HORIZONTAL);
	scroll_v = new BScrollBar(BRect(0,0,B_V_SCROLL_BAR_WIDTH,100),
							"vscroll", NULL, 0, 0, B_VERTICAL);
	if( scroll_h ) {
		scroll_h->Hide();
		scroll_h->SetResizingMode(B_FOLLOW_NONE);
	}
	if( scroll_v ) {
		scroll_v->Hide();
		scroll_v->SetResizingMode(B_FOLLOW_NONE);
	}
	do_vert = do_horiz = inset_corner = false;
	border = 0;
	s_width = s_height = 0;
}

ArpScrollArea::~ArpScrollArea()
{
	if( ParamSet() ) ParamSet()->RemoveParameters(parameters);
}

/* ------------ ArpScrollArea archiving ------------
 *
 * Archiving and retrieving ArpBaseLayout objects.
 */
ArpScrollArea* ArpScrollArea::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpScrollArea") ) 
		return new ArpScrollArea(archive); 
	return NULL;
}

status_t ArpScrollArea::Archive(BMessage* data, bool deep) const
{
	status_t status = inherited::Archive(data,deep);
	//if( status == B_NO_ERROR )
	//	status = data->AddString("class","ArpScrollArea");
	return status;
}

BView* ArpScrollArea::get_child(void)
{
	ArpBaseLayout* childl = LayoutChildAt(0);
	if( childl ) return childl->OwnerView();
	return NULL;
}

void ArpScrollArea::update_scroll_bars(void)
{
	BView* childv = get_child();
	bool changed = false;
	if( PV_ScrollHorizontal && scroll_h && scroll_h->IsHidden() ) {
		ArpD(cdb << ADH << "ArpScrollArea: Showing hscroll" << endl);
		if( childv ) scroll_h->SetTarget(childv);
		scroll_h->Show();
		changed = true;
	} else if( !PV_ScrollHorizontal && scroll_h && !scroll_h->IsHidden() ) {
		ArpD(cdb << ADH << "ArpScrollArea: Hiding hscroll" << endl);
		scroll_h->SetTarget((BView*)NULL);
		scroll_h->Hide();
		changed = true;
	}
	if( PV_ScrollVertical && scroll_v && scroll_v->IsHidden() ) {
		ArpD(cdb << ADH << "ArpScrollArea: Showing vscroll" << endl);
		if( childv ) scroll_v->SetTarget(childv);
		scroll_v->Show();
		changed = true;
	} else if( !PV_ScrollVertical && scroll_v && !scroll_v->IsHidden() ) {
		ArpD(cdb << ADH << "ArpScrollArea: Hiding vscroll" << endl);
		scroll_v->SetTarget((BView*)NULL);
		scroll_v->Hide();
		changed = true;
	}
	if( changed ) InvalidateDimens();
}

void ArpScrollArea::ParametersChanged(const ArpParamSet* params)
{
	if( params->GetChanges(parameters)&SCROLL_BARS_CHANGED ) {
		update_scroll_bars();
	}
	inherited::ParametersChanged(params);
}
	
BScrollBar* ArpScrollArea::HScrollBar(void)
{
	if(this && PV_ScrollHorizontal) return scroll_h;
	return NULL;
}

BScrollBar* ArpScrollArea::VScrollBar(void)
{
	if(this && PV_ScrollVertical) return scroll_v;
	return NULL;
}

ArpBaseLayout* ArpScrollArea::AddLayoutChild(ArpBaseLayout* v,
						const BMessage& c, ArpBaseLayout* before)
{
	ArpASSERT(CountLayoutChildren() == 0);
	ArpBaseLayout* ret = inherited::AddLayoutChild(v,c,before);
	attach_child(v);
	return ret;
}

void ArpScrollArea::attach_child(ArpBaseLayout* v)
{
	if( v ) {
		ArpVALIDATE(v->OwnerView() != NULL, return);
		if( PV_ScrollVertical && scroll_v )
			scroll_v->SetTarget(v->OwnerView());
		if( PV_ScrollHorizontal && scroll_h )
			scroll_h->SetTarget(v->OwnerView());
	}
}

bool ArpScrollArea::RemoveLayoutChild(ArpBaseLayout* child)
{
	bool ret = inherited::RemoveLayoutChild(child);
	if( ret && child ) {
		if( scroll_v ) scroll_v->SetTarget((BView*)NULL);
		if( scroll_h ) scroll_h->SetTarget((BView*)NULL);
	}
	return ret;
}

BRect ArpScrollArea::HintLayoutChild(ArpBaseLayout* before) const
{
	BRect frm = LayoutBounds();
	
	float dimw=0, dimh=0;
	if( (PV_ScrollHorizontal || PV_InsetCorner) && scroll_h )
		scroll_h->GetPreferredSize(&dimw,&dimh);
	frm.bottom -= dimh - 1;
	
	dimw = dimh = 0;
	if( (PV_ScrollVertical || PV_InsetCorner) && scroll_v )
		scroll_v->GetPreferredSize(&dimw,&dimh);
	frm.right -= dimw - 1;
	
	int border = 0;
	if( PV_BorderStyle == B_FANCY_BORDER ) {
		border = 2;
	} else if( PV_BorderStyle == B_PLAIN_BORDER ) {
		border = 1;
	}
	frm.left += border;
	frm.right -= border;
	frm.top += border;
	frm.bottom -= border;
	
	return frm;
}

void ArpScrollArea::SetFocusShown(bool state, bool andParent)
{
	bool old = FocusShown();
	inherited::SetFocusShown(state, false);
	if( InView() && old != state ) DrawLayout(InView(), InView()->Bounds());
}


void ArpScrollArea::SetLayoutActivated(bool state)
{
	bool old = LayoutActivated();
	inherited::SetLayoutActivated(state);
	if( InView() && old != state && FocusShown() ) {
		DrawLayout(InView(), InView()->Bounds());
	}
}

void ArpScrollArea::AttachView(BView* par_view, BView* before)
{
	ArpD(cdb << ADH << "*** ArpScrollArea::AttachView()" << endl);
	inherited::AttachView(par_view, before);
	if( par_view && InView() ) {
		par_view = InView();
		if( scroll_h ) par_view->AddChild(scroll_h);
		if( scroll_v ) par_view->AddChild(scroll_v);
	} else {
		if( scroll_h && scroll_h->Parent() ) scroll_h->RemoveSelf();
		if( scroll_v && scroll_v->Parent() ) scroll_v->RemoveSelf();
	}
}

void ArpScrollArea::ComputeDimens(ArpDimens& cur_dimens)
{
	inherited::ComputeDimens(cur_dimens);
	
	ArpUniDimens& curx(cur_dimens.X());
	ArpUniDimens& cury(cur_dimens.Y());
	
	curx.MakeAllBody();
	cury.MakeAllBody();
	
	if( curx.TotalMax() <= 0 ) curx.SetTo(0, 30, 30, 30, 0);
	if( cury.TotalMax() <= 0 ) cury.SetTo(0, 30, 30, 30, 0);
	
	float dimw=0, dimh=0;
	if( (PV_ScrollHorizontal || PV_InsetCorner) && scroll_h )
		scroll_h->GetPreferredSize(&dimw,&dimh);
	cury.AddBody(dimh+1);
	curx.SetMinTotal((dimh*3)+4);
	s_height = dimh;
	
	ArpD(cdb << ADH << "Computed hscroll dimens:" << endl
				<< "  dimw=" << dimw << " dimh=" << dimh
				<< " min_w=" << curx.MinBody()
				<< " min_h=" << cury.MinBody() << endl);
	
	dimw = dimh = 0;
	if( (PV_ScrollVertical || PV_InsetCorner) && scroll_v )
		scroll_v->GetPreferredSize(&dimw,&dimh);
	curx.AddBody(dimw+1);
	cury.SetMinTotal((dimw*3)+4);
	s_width = dimw;

	ArpD(cdb << ADH << "Computed vscroll dimens:" << endl
				<< "  dimw=" << dimw << " dimh=" << dimh
				<< " min_w=" << curx.MinBody()
				<< " min_h=" << cury.MinBody() << endl);
	
	if( (PV_ScrollHorizontal && PV_ScrollVertical)
		|| (PV_InsetCorner && (PV_ScrollHorizontal || PV_ScrollVertical)) ) {
		curx.SetMinTotal(s_height + s_width + 4);
		cury.SetMinTotal(s_height + s_width + 4);
	}
	
	if( PV_BorderStyle == B_FANCY_BORDER ) {
		curx.AddBody(4);
		cury.AddBody(4);
	} else if( PV_BorderStyle == B_PLAIN_BORDER ) {
		curx.AddBody(2);
		cury.AddBody(2);
	}
	
	ArpD(cdb << ADH << "final min_width = " << curx.MinBody()
			<< ", min_height = " << cury.MinBody() << endl);
	ArpD(cdb << ADH << "final pref_width = " << curx.PrefBody()
			<< ", pref_height = " << cury.PrefBody() << endl);
}

void ArpScrollArea::LayoutView(void)
{
	if( PV_BorderStyle != B_NO_BORDER && InView() ) {
		// If we are drawing a frame around the children, we need to
		// invalidate our containing view to be sure the frame is
		// completely redrawn.  Curiously, calling Invalidate() with
		// an explicit rectangle seems to cause EVERYTHING to be redrawn...
		// including all the parents!!!  So it's better to just invalidate
		// the whole view, it seems.  *sigh*
		ArpD(cdb << ADH << "Invalidating parent view: " << InView()->Name()
				<< " (" << InView() << ")" << endl);
		if( mLastFrame.IsValid() ) {
			ArpD(cdb << ADH << "Previous rectangle: " << mLastFrame << endl);
			//InView()->Invalidate(mLastFrame);
		}
		mLastFrame = LayoutFrame();
		ArpD(cdb << ADH << "New rectangle: " << mLastFrame << endl);
		//InView()->Invalidate(mLastFrame);
		InView()->Invalidate();
	}
	
	BRect frm = LayoutBounds();
	int32 num = CountLayoutChildren();

	ArpD(cdb << ADH << "Frame=" << frm << " s_width=" << s_width
				<< " s_height=" << s_height << endl);
	
	if( PV_BorderStyle == B_FANCY_BORDER ) {
		frm.left += 2;
		frm.top += 2;
		frm.right -= 2;
		frm.bottom -= 2;
	} else if( PV_BorderStyle == B_PLAIN_BORDER ) {
		frm.left += 1;
		frm.top += 1;
		frm.right -= 1;
		frm.bottom -= 1;
	}
	
	if( PV_ScrollHorizontal && scroll_h ) frm.bottom -= s_height;
	if( PV_ScrollVertical && scroll_v ) frm.right -= s_width;

	if( PV_ScrollHorizontal && scroll_h && frm.Width() > 0 ) {
		ArpD(cdb << ADH << "Moving hscroll to (" << frm.right << ","
					<< frm.top << ") size=(" << s_width
					<< "," << frm.Height() << ")" << endl);
		ArpD(cdb << ADH << "Hscroll parent="
					<< (void*)scroll_h->Parent()
					<< " ishidden=" << scroll_h->IsHidden() << endl);
		float inset = (PV_InsetCorner && !PV_ScrollVertical) ? s_width : 0;
		float width = frm.Width()-inset;
		if( width < s_height+4 ) width = s_height+4;
		if( PV_BorderStyle == B_PLAIN_BORDER ) {
			scroll_h->MoveTo(frm.left,frm.bottom);
			scroll_h->ResizeTo(width,s_height);
			frm.bottom--;
		} else {
			scroll_h->MoveTo(frm.left-1,frm.bottom+1);
			scroll_h->ResizeTo(width+2,s_height);
		}
	}
	if( PV_ScrollVertical && scroll_v && frm.Height() > 0 ) {
		ArpD(cdb << ADH << "Moving vscroll to (" << frm.left << ","
					<< frm.bottom << ") size=(" << frm.Width()
					<< "," << s_height << ")" << endl);
		ArpD(cdb << ADH << "Vscroll parent="
					<< (void*)scroll_v->Parent()
					<< " ishidden=" << scroll_v->IsHidden() << endl);
		float inset = (PV_InsetCorner && !PV_ScrollHorizontal) ? s_height : 0;
		float height = frm.Height()-inset;
		if( height < s_width+4 ) height = s_width+4;
		if( PV_BorderStyle == B_PLAIN_BORDER ) {
			scroll_v->MoveTo(frm.right,frm.top);
			scroll_v->ResizeTo(s_width,height+1);
			frm.right--;
		} else {
			scroll_v->MoveTo(frm.right+1,frm.top-1);
			scroll_v->ResizeTo(s_width,height+2);
		}
	}
	
	if( num >= 1 ) {
		ArpBaseLayout* child = LayoutChildAt(0);
		ArpD(cdb << ADH << "Moving child to " << frm << endl);
		if( child ) child->SetViewLayout(frm);
	}
	
	ArpD(cdb << ADH << "cframe=" << (get_child() ? get_child()->Frame():BRect())
				<< " vframe=" << (scroll_v ? scroll_v->Frame():BRect())
				<< " hframe=" << (scroll_h ? scroll_h->Frame():BRect())
				<< endl);
}

static void
draw_area_frame(BView* inside, const BRect& rect, const BPoint& inset,
				rgb_color shine, rgb_color shadow, bool isInset)
{
	if( isInset ) {
		inside->AddLine(BPoint(rect.right, rect.top),
						BPoint(rect.right, inset.y), shadow);
		inside->AddLine(BPoint(rect.right, inset.y),
						BPoint(inset.x, inset.y), shadow);
		inside->AddLine(BPoint(inset.x, inset.y),
						BPoint(inset.x, rect.bottom), shadow);
		inside->AddLine(BPoint(inset.x, rect.bottom),
						BPoint(rect.left, rect.bottom), shadow);
		inside->AddLine(BPoint(rect.left, rect.bottom),
						BPoint(rect.left, rect.top), shine);
		inside->AddLine(BPoint(rect.left, rect.top),
						BPoint(rect.right, rect.top), shine);
	} else {
		inside->AddLine(BPoint(rect.right,rect.top),
						BPoint(rect.right,rect.bottom), shadow);
		inside->AddLine(BPoint(rect.left,rect.bottom),
						BPoint(rect.right,rect.bottom), shadow);
		inside->AddLine(BPoint(rect.left,rect.top),
						BPoint(rect.left,rect.bottom), shine);
		inside->AddLine(BPoint(rect.left,rect.top),
						BPoint(rect.right,rect.top), shine);
	}
}

void ArpScrollArea::DrawLayout(BView* inside, BRect region)
{
	if( !inside ) return;
	inside->PushState();
	inside->SetPenSize(1.0);
	
	BRect rect(LayoutBounds());
	BPoint inset(rect.right-s_width, rect.bottom-s_height);
	ArpD(cdb << ADH << "Drawing frame in bounds: " << rect << endl);
	
	bool show_focus = LayoutActivated() ? FocusShown() : false;
	
	if( PV_BorderStyle == B_FANCY_BORDER ) {
		inside->BeginLineArray(12);
		if( show_focus ) {
			draw_area_frame(inside, rect, inset,
							ui_color(B_KEYBOARD_NAVIGATION_COLOR),
							ui_color(B_KEYBOARD_NAVIGATION_COLOR),
							PV_InsetCorner);
		} else {
			draw_area_frame(inside, rect, inset,
							tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
											B_DARKEN_1_TINT),
							tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
											B_LIGHTEN_MAX_TINT),
							PV_InsetCorner);
		}
		inside->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
										B_DARKEN_2_TINT));
		rect.left += 1;
		rect.top += 1;
		rect.right -= 1;
		rect.bottom -= 1;
		inset.x -= 1;
		inset.y -= 1;
		draw_area_frame(inside, rect, inset,
						tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
										B_DARKEN_2_TINT),
						tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
										B_DARKEN_2_TINT),
						PV_InsetCorner);
		inside->EndLineArray();
	} else if( PV_BorderStyle == B_PLAIN_BORDER ) {
		inside->BeginLineArray(12);
		if( show_focus ) {
			draw_area_frame(inside, rect, inset,
							ui_color(B_KEYBOARD_NAVIGATION_COLOR),
							ui_color(B_KEYBOARD_NAVIGATION_COLOR),
							PV_InsetCorner);
		} else {
			draw_area_frame(inside, rect, inset,
							tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
											B_DARKEN_2_TINT),
							tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
											B_DARKEN_2_TINT),
							PV_InsetCorner);
		}
		inside->EndLineArray();
	}
	inside->PopState();
}
