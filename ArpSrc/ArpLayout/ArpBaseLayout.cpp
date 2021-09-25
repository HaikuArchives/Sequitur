/*
	
	ArpBaseLayout.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	The layout engine's base class, which encapsulates
	information about a rectangular UI area and how it
	can be dimensioned.
*/

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#include <ArpLayout/ArpGlobalSet.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#if __GNUC < 3 
#include <algobase.h>
#include 

#include <float.h>
#include <cstring>

ArpMOD();

static BMessage ArpNoParamsMutable;

static const char* ChildParam = "arp:child";
static const char* ConstraintsParam = "arp:constraints";

const char* ArpBaseLayout::BackgroundColorP = "BackgroundColor";
const char* ArpBaseLayout::ForegroundColorP = "ForegroundColor";
const char* ArpBaseLayout::BasicFontP = "BasicFont";

enum {
	BackgroundColorIdx = 0,
	ForegroundColorIdx,
	BasicFontIdx,
	ConstraintsIdx,
	GlobalsIdx,
};

static property_info properties[] = {
	// Properties for parameters.
	{ const_cast<char*>(ArpBaseLayout::BackgroundColorP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Standard background color of object.", 0,
	  { B_RGB_COLOR_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpBaseLayout::ForegroundColorP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Standard foreground color of object.", 0,
	  { B_RGB_COLOR_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpBaseLayout::BasicFontP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Standard font of object.", 0,
	  { FFont::FONT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	
	// Other properties.
	{ "Constraints", { 0 }, { B_DIRECT_SPECIFIER },
	  "Parent layout constraints applied to this child.", 0
	},
	{ "Globals", { B_GET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER },
	  "Nearest root object's global values for its children.", 0
	},
	0
};

enum {
	BackgroundChange = 0x0001,
	ForegroundChange = 0x0002,
	BasicFontChange = 0x0004
};

static const parameter_info parameters[] = {
	{ sizeof(parameter_info), &properties[BackgroundColorIdx],
	  BackgroundChange, ARP_INVALIDATE_VIEW, 0
	},
	{ sizeof(parameter_info), &properties[ForegroundColorIdx],
	  ForegroundChange, ARP_INVALIDATE_VIEW, 0
	},
	{ sizeof(parameter_info), &properties[BasicFontIdx],
	  BasicFontChange, ARP_INVALIDATE_DIMENS, 0
	},
	{ 0, 0, 0, 0, 0 }
};

static const parameter_suite suite = {
	sizeof(parameter_suite),
	"suite/vnd/ARP.ArpBaseLayout",
	properties
};

/* ------ ArpBaseLayout constructor and destructor ------
 *
 * The various ways to create and destroy ArpBaseLayout
 * objects.
 */
 
ArpBaseLayout::ArpBaseLayout()
	: constraints(B_SIMPLE_DATA)
{
	do_construct();
}

ArpBaseLayout::ArpBaseLayout(BMessage* data, bool final)
	: constraints(B_SIMPLE_DATA)
{
	do_construct();
	if( data ) {
		constraints.MakeEmpty();
		data->FindMessage(ConstraintsParam,&constraints);
		
		mParams.InstantiateParams(data, parameters);
		if( final ) InstantiateParams(data);
	}
}

status_t ArpBaseLayout::InstantiateParams(BMessage* data)
{
	ArpVALIDATE(data != 0, return B_ERROR);
	
	status_t status = B_OK;

	BMessage childArch;
	int32 i=0;
	while( data->FindMessage(ChildParam, i, &childArch) == B_OK ) {
		BArchivable* arch = instantiate_object(&childArch);
		if( arch ) {
			ArpBaseLayout* child = dynamic_cast<ArpBaseLayout*>(arch);
			if( child ) AddLayoutChild(child);
			else printf("ArpBaseLayout: Child is not an ArpBaseLayout.\n");
		} else printf("ArpBaseLayout: Unable to instantiate child.\n");
		i++;
	}
	
	return status;
}

void ArpBaseLayout::do_construct(void)
{
	mParams.AddParameters(parameters, &suite);
	PV_BackgroundColor.Init(&mParams, BackgroundColorP,
							ui_color(B_PANEL_BACKGROUND_COLOR),
							"StdBackColor");
	rgb_color black;
	black.red = black.green = black.blue = 0;
	black.alpha = 255;
	PV_ForegroundColor.Init(&mParams, ForegroundColorP,
							black,
							"StdTextColor");
	PV_BasicFont.Init(&mParams, BasicFontP, *be_plain_font, "PlainFont");
	
	mParent = mPrevSibling = mNextSibling = mFirstChild = mLastChild = 0;
	owner = 0;
	in_view = 0;
	mBodyFill = ArpFillAll;
	dimens_changed = true;
	inhibit_layout = true;
	focus_shown = false;
	layout_activated = false;
	
	cur_frame = body_frame =
		BRect(-99999.5,-99999.5,-99998.5,-99998.5);
	last_frame = BRect(-99997.5,-99997.5,-99996.5,-99996.5);
	error = B_NO_ERROR;
}

ArpBaseLayout::~ArpBaseLayout()
{
	ArpD(cdb << ADH << "Destroying ArpBaseLayout: " << LayoutName() << std::endl);
	
	if( ParamSet() ) ParamSet()->RemoveParameters(parameters);
	
	LayoutRemoveSelf();
	if( !LayoutInhibit() ) SetLayoutInhibit(true);
	
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		ArpBaseLayout* next = child->NextLayoutSibling();
		if( !child->OwnerView() ) {
			child->LayoutRemoveSelf();
			delete child;
		}
		child = next;
	}
}

/* ------------ ArpBaseLayout archiving ------------
 *
 * Archiving and retrieving ArpBaseLayout objects.
 */
status_t ArpBaseLayout::Archive(BMessage* data, bool deep) const
{
	status_t status = data->AddMessage(ConstraintsParam, &constraints);
	if( status == B_NO_ERROR ) {
		status = mParams.ArchiveParams(data);
	}
	if( deep ) {
		ArpBaseLayout* mychild = LayoutChildAt(0);
		while( mychild ) {
			BMessage childarch(B_ARCHIVED_OBJECT);
			if( status == B_NO_ERROR )
				status = mychild->Archive(&childarch, deep);
			if( status == B_NO_ERROR )
				status = data->AddMessage(ChildParam, &childarch);
			mychild = mychild->NextLayoutSibling();
		}
	}
	// Add this class name, even though it is abstract.  This is
	// so that people probing the BMessage archive can easily tell
	// if an object is a valid ArpLayout component.
	//if( status == B_NO_ERROR )
	//	status = data->AddString("class","ArpBaseLayout");
	return status;
}

/* ------------ ArpBaseLayout error control ------------
 *
 * Get and set the error state of an ArpBaseLayout object.
 * These functions are safe to call on NULL objects.
 */
 
ArpBaseLayout* ArpBaseLayout::SetError(status_t err)
{
	if( this ) error = err;
	return this;
}

status_t ArpBaseLayout::Error()
{
	if( this ) return error;
	return B_NO_MEMORY;
}

/* ------------ ArpBaseLayout parameters ------------
 *
 * Getting and setting object parameters.  Functions to
 * set parameters can be called on NULL objects.
 */
 
ArpBaseLayout* ArpBaseLayout::SetLayoutName(const char* inname)
{
	if( this ) {
		ArpVALIDATE(LayoutHandler() != 0, return this);
		LayoutHandler()->SetName(inname);
	}
	return this;
}

const char* ArpBaseLayout::LayoutName() const
{
	ArpVALIDATE(LayoutHandler() != 0, return "*bad*");
	return LayoutHandler()->Name();
}

ArpBaseLayout* ArpBaseLayout::SetParams(const BMessage& p)
{
	mParams.SetParams(&p);
	PerformChanges();
	return this;
}

status_t ArpBaseLayout::GetParams(BMessage* p, const char* name) const
{
	return mParams.GetParams(p, name);
}

ArpParamSet* ArpBaseLayout::ParamSet()
{
	return &mParams;
}

const ArpParamSet* ArpBaseLayout::ParamSet() const
{
	return &mParams;
}

void ArpBaseLayout::SetBasicFont(const BFont* font)
{
	PV_BasicFont = *font;
}

const BFont* ArpBaseLayout::BasicFont() const
{
	return &PV_BasicFont;
}

void ArpBaseLayout::SetBackgroundColor(rgb_color c)
{
	PV_BackgroundColor = c;
}

rgb_color ArpBaseLayout::BackgroundColor() const
{
	return (rgb_color)PV_BackgroundColor;
}

void ArpBaseLayout::SetForegroundColor(rgb_color c)
{
	PV_ForegroundColor = c;
}

rgb_color ArpBaseLayout::ForegroundColor() const
{
	return (rgb_color)PV_ForegroundColor;
}

void ArpBaseLayout::PerformChanges()
{
	ParametersChanged(&mParams);
	mParams.ClearChanges();
}

void ArpBaseLayout::ParametersChanged(const ArpParamSet* params)
{
	uint32 locals = params->GetChanges(parameters);
	uint32 globals = params->GetGlobalChanges();
	
	if( OwnerView() ) {
		if( locals&BackgroundChange ) {
			OwnerView()->SetViewColor(PV_BackgroundColor);
			OwnerView()->SetLowColor(PV_BackgroundColor);
		}
		if( locals&ForegroundChange ) {
			OwnerView()->SetHighColor(PV_ForegroundColor);
		}
		if( locals&BasicFontChange ) {
			OwnerView()->SetFont(&PV_BasicFont);
		}
	}
	
	if( globals&(ARP_INVALIDATE_VIEW|ARP_INVALIDATE_DIMENS) ) {
		ArpD(cdb << ADH << "Layout " << LayoutName()
				<< ": Invalidating View." << std::endl);
		InvalidateView();
	}
	
	if( globals&ARP_INVALIDATE_DIMENS ) {
		ArpD(cdb << ADH << "Layout " << LayoutName()
				<< ": Invalidating Dimensions." << std::endl);
		InvalidateDimens();
	}
}

void ArpBaseLayout::SetGlobals(ArpGlobalSetI* gl)
{
	bool inhibit = inhibit_layout;
	if( !inhibit ) SetLayoutInhibit(true);
	#if defined(ArpDEBUG)
	if( gl ) {
		ArpD(cdb << ADH << "Setting globals for "
					<< LayoutName() << ": " << *(gl->GlobalValues()) << std::endl);
	} else {
		ArpD(cdb << ADH << "Clearing globals for "
					<< LayoutName() << "." << std::endl);
	}
	#endif
	mParams.SetGlobals(gl);
	PerformChanges();
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		child->SetGlobals(gl);
		child = child->NextLayoutSibling();
	}
	if( !inhibit ) SetLayoutInhibit(false);
}

ArpGlobalSetI* ArpBaseLayout::Globals(void) const
{
	return mParams.Globals();
}

void ArpBaseLayout::SetGlobalParam(const char* name, const char* value)
{
	BMessage set;
	if( set.AddData(name, ARP_INDIRECT_TYPE, value, strlen(value)+1) == B_OK ) {
		SetParams(set);
	}
}

ArpBaseLayout* ArpBaseLayout::SetConstraints(const BMessage& c)
{
	if( this ) {
		arp_update_message(constraints, c);
		ArpD(cdb << ADH << "Set constraints for "
					<< LayoutName() << ": " << constraints << std::endl);
		InvalidateDimens();
	}
	return this;
}

BMessage& ArpBaseLayout::Constraints(void)
{
	if( this ) return constraints;
	return ArpNoParamsMutable;
}

void ArpBaseLayout::do_inhibit_layout(bool state)
{
	inhibit_layout = state;
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		child->do_inhibit_layout(state);
		child = child->NextLayoutSibling();
	}
}

ArpBaseLayout* ArpBaseLayout::SetLayoutInhibit(bool state)
{
	if( this ) {
		ArpD(cdb << ADH << "Setting layout inhibit: " << state << std::endl);
		inhibit_layout = state;
		ArpBaseLayout* root = this;
		while( root->LayoutParent() ) {
			root = root->LayoutParent();
		}
		root->do_inhibit_layout(state);
		if( !state && dimens_changed ) {
			ArpD(cdb << ADH << "dimens_changed; changing layout." << std::endl);
			root->LayoutChanged(false);
		}
	}
	return this;
}

bool ArpBaseLayout::LayoutInhibit() const
{
	return inhibit_layout;
}

void ArpBaseLayout::DrawLayout(BView* inside, BRect region)
{
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		if( !child->OwnerView()) child->DrawLayout(inside, region);
		child = child->NextLayoutSibling();
	}
}

status_t ArpBaseLayout::LayoutMessageReceived(BMessage *message)
{
	if( !message ) return B_ERROR;
	
	int32 idx = 0;
	BMessage spec;
	const char* property = 0;
	int32 what=0;
	if( message->GetCurrentSpecifier(&idx, &spec, &what, &property) != B_OK ) {
		return B_ERROR;
	}
	
	if( strcmp(property, "Constraints") == 0 && what == B_DIRECT_SPECIFIER ) {
		if( message->what == B_GET_SUPPORTED_SUITES ) {
			BMessage result(B_REPLY);
			if( LayoutParent() ) {
				LayoutParent()->GetConstraintSuites(&result);
			}
			result.AddInt32("error", B_OK);
			message->SendReply(&result);
			return B_OK;
		}
		
		BMessage result(B_REPLY);
		status_t error = B_OK;
		
		error = message->PopSpecifier();
		if( error == B_OK ) {
			error = message->GetCurrentSpecifier(&idx, &spec, &what, &property);
		}
		if( error == B_OK && what != B_DIRECT_SPECIFIER ) error = B_ERROR;
		if( error == B_OK ) {
			switch( message->what ) {
				case B_GET_PROPERTY: {
					if( strcmp(property, "Suites") == 0 ) {
						if( LayoutParent() ) {
							LayoutParent()->GetConstraintSuites(&result);
						}
						break;
					}
					
					type_code type=0;
					bool fixed_size=false;
					error = constraints.GetInfo(property, &type, &fixed_size);
					if( error != B_OK ) {
						if( LayoutParent() ) {
							error = LayoutParent()->QueryConstraint(property, &result);
						} else {
							error = B_NAME_NOT_FOUND;
						}
						break;
					}
					
					const void* data=0;
					ssize_t numBytes=0;
					error = constraints.FindData(property, type, &data, &numBytes);
					if( error != B_OK ) break;
					
					error = result.AddData("result", type, data, numBytes, fixed_size);
				} break;
				case B_SET_PROPERTY: {
					type_code type=0;
					bool fixed_size=false;
					error = message->GetInfo("data", &type, &fixed_size);
					if( error != B_OK ) break;
					
					const void* data=0;
					ssize_t numBytes=0;
					error = message->FindData("data", type, &data, &numBytes);
					if( error != B_OK ) break;
					
					BMessage set(B_SET_PROPERTY);
					error = set.AddData(property, type, data, numBytes, fixed_size);
					if( error != B_OK ) break;
					
					SetConstraints(set);
				} break;
				default:
					error = B_BAD_SCRIPT_SYNTAX;
					break;
			}
		}
		
		result.AddInt32("error", error);
		message->SendReply(&result);
		return B_OK;
	}
	
	if( strcmp(property, "Globals") == 0 && what == B_DIRECT_SPECIFIER ) {
		if( message->what == B_GET_PROPERTY ) {
			BMessage result(B_REPLY);
			if( Globals() ) {
				if( result.AddMessage("result", Globals()->GlobalValues()) == B_OK
						&& result.AddInt32("error", B_OK) == B_OK ) {
					message->SendReply(&result);
					return B_OK;
				}
			}
		} else if( message->what == B_SET_PROPERTY ) {
			// XXX TO DO: Update global values.
			return B_OK;
		}
		return B_ERROR;
	}
	
	if( mParams.ParamMessageReceived(message) == B_OK ) {
		PerformChanges();
		return B_OK;
	}
	
	return B_ERROR;
}

status_t ArpBaseLayout::LayoutResolveSpecifier(BHandler** result,
										BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property)
{
	status_t err = mParams.ParamResolveSpecifier(msg, index, specifier,
												 form, property);
	if( err == B_OK ) {
		if( result ) *result = LayoutHandler();
	}
	
	return err;
}

status_t ArpBaseLayout::LayoutGetSupportedSuites(BMessage *data)
{
	return mParams.ParamGetSupportedSuites(data);
}

void ArpBaseLayout::GetConstraintSuites(BMessage *data)
{
}
	
status_t ArpBaseLayout::QueryConstraint(const char* name,
										BMessage* data) const
{
	return B_NAME_NOT_FOUND;
}

BWindow* ArpBaseLayout::LayoutWindow() const
{
	return owner;
}

void ArpBaseLayout::set_layout_window(BWindow* window)
{
	if( window == owner ) return;
	if( window != 0 && owner != 0 ) AttachLayoutWindow(0);
	
	if( LayoutHandler() && !OwnerView() ) {
		if( window ) {
			window->AddHandler(LayoutHandler());
			BHandler* next = LayoutParent()
						   ? LayoutParent()->LayoutHandler() : 0;
			LayoutHandler()->SetNextHandler(next);
		} else if( owner ) {
			owner->RemoveHandler(LayoutHandler());
		}
	}
	
	owner = window;
	
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		if( !child->OwnerView() ) child->set_layout_window(window);
		child = child->NextLayoutSibling();
	}
}

static void send_layout_attach(ArpBaseLayout* o, bool attach)
{
	if( !o ) return;
	
	if( attach ) o->LayoutAttachedToWindow();
	else o->LayoutDetachedFromWindow();
	
	ArpBaseLayout* child = o->LayoutChildAt(0);
	while( child ) {
		if( !child->OwnerView() ) send_layout_attach(child, attach);
		child = child->NextLayoutSibling();
	}
	
	if( attach ) o->LayoutAllAttached();
	else o->LayoutAllDetached();
}

void ArpBaseLayout::AttachLayoutWindow(BWindow* window)
{
	if( window ) {
		set_layout_window(window);
		send_layout_attach(this, true);
	} else {
		send_layout_attach(this, false);
		set_layout_window(window);
	}
}

/* ------------ ArpBaseLayout child manipulation ------------
 *
 * Adding, examining, removing children.  This operates in
 * parallel with the BView mechanism; we need to do our own
 * implementation since we can have ArpBaseLayout objects without
 * associated BView objects.
 */

BView* ArpBaseLayout::FindClosestView(int32 index)
{
	ArpBaseLayout* child = LayoutChildAt(0);
	
	while( child ) {
		BView* view = child->OwnerView();
		if( view ) return view;
		view = child->FindClosestView(0);
		if( view ) return view;
		child = child->NextLayoutSibling();
	}
	
	return 0;
}

ArpBaseLayout* ArpBaseLayout::AddLayoutChild(ArpBaseLayout* v,
											   const BMessage& c,
											   ArpBaseLayout* before)
{
	// Sanity checks.
	if( !this ) return NULL;
	if( !v ) {
		SetError(B_NO_MEMORY);
		return this;
	}
	
	ArpD(cdb << ADH << "Adding child: " << v->LayoutName() << std::endl);
	
	bool old_inhibit = LayoutInhibit();
	SetLayoutInhibit(true);
	
	if( v->LayoutParent() != 0 ) {
		debugger("AddLayoutChild() failed -- the added layout already has a parent.");
	}
	
	// Add the given child to this ArpBaseLayout.
	int bindex = -1;
	if( before ) {
		if( before->LayoutParent() != this ) {
			debugger("AddLayoutChild() failed -- the \"before\" layout and \"this\" don't match.");
		} else {
			v->mPrevSibling = before->mPrevSibling;
			if( before->mPrevSibling ) before->mPrevSibling->mNextSibling = v;
			else mFirstChild = v;
			before->mPrevSibling = v;
			v->mNextSibling = before;
		}
	} else {
		if( mLastChild ) mLastChild->mNextSibling = v;
		v->mPrevSibling = mLastChild;
		v->mNextSibling = 0;
		mLastChild = v;
		if( !mFirstChild ) mFirstChild = v;
	}
	if( &c != &ArpNoParams ) v->constraints = c;
	v->mParent = this;
	if( !(in_view=OwnerView()) && LayoutParent() ) {
		in_view = LayoutParent()->InView();
	}
	
	// Figure out the 'before' object in the BView space,
	// and add the child to our parent BView.
	BView* beforeView = 0;
	if( bindex >= 0 ) beforeView = FindClosestView(bindex+1);
	ArpD(cdb << ADH << "Attaching view: parent=" << in_view
					<< ", before=" << beforeView << std::endl);
	v->AttachView(in_view, beforeView);
	
	// Update globals and make sure the dimensions are re-evaluated.
	if( Globals() != v->Globals() ) v->SetGlobals(Globals());
	ArpD(cdb << ADH << "Invalidating dimens; changed="
					<< dimens_changed << std::endl);
	InvalidateDimens();
	
	v->SetLayoutInhibit(old_inhibit);
	
	return this;
}

bool ArpBaseLayout::RemoveLayoutChild(ArpBaseLayout* child)
{
	if( !child ) return false;
	
	ArpVALIDATE( child->LayoutParent() == this, return false );
	
	bool old_inhibit = LayoutInhibit();
	if( !old_inhibit ) SetLayoutInhibit(true);
	
	if( child->mPrevSibling ) {
		child->mPrevSibling->mNextSibling = child->mNextSibling;
	}
	if( child->mNextSibling ) {
		child->mNextSibling->mPrevSibling = child->mPrevSibling;
	}
	if( mFirstChild == child ) mFirstChild = child->mNextSibling;
	if( mLastChild == child ) mLastChild = child->mPrevSibling;
	child->mNextSibling = child->mPrevSibling = child->mParent = 0;
	
	ArpD(cdb << ADH << "Removed child " << child->LayoutName()
					<< " from " << LayoutName() << std::endl);
					
	child->AttachView(0);
	InvalidateDimens();
	
	if( !old_inhibit ) SetLayoutInhibit(old_inhibit);
	
	return true;
}

ArpBaseLayout* ArpBaseLayout::LayoutParent() const
{
	return mParent;
}

void ArpBaseLayout::AttachView(BView* par_view, BView* before)
{
	ArpD(cdb << ADH << "AttachView: par_view=" << par_view
				<< ", before=" << before << ", owner=" << OwnerView()
				<< ", in=" << InView() << std::endl);
				
	// If this ArpBaseLayout does not have an associated BView,
	// we need to run through all of its children looking for BViews
	// that instead need to be attached.
	if( !OwnerView() ) {
		in_view = par_view;
		ArpBaseLayout* child = LayoutChildAt(0);
		while( child ) {
			child->AttachView(par_view, before);
			child = child->NextLayoutSibling();
		}
		
	// If we are adding to a parent, do so.
	} else if( par_view ) {
		par_view->AddChild(OwnerView(), before);
	
	// If we are removing from a parent, do so.
	} else if( OwnerView()->Parent() ) {
		OwnerView()->RemoveSelf();
	}
}

int32 ArpBaseLayout::CountLayoutChildren() const
{
	int32 count=0;
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		count++;
		child = child->NextLayoutSibling();
	}
	return count;
}

ArpBaseLayout* ArpBaseLayout::LayoutChildAt(int32 index) const
{
	if( index >= 0 ) {
		ArpBaseLayout* child = mFirstChild;
		while( index > 0 && child ) {
			index--;
			child = child->NextLayoutSibling();
		}
		return child;
	} else {
		ArpBaseLayout* child = mLastChild;
		while( index < -1 && child ) {
			index++;
			child = child->PreviousLayoutSibling();
		}
		return child;
	}
}

ArpBaseLayout* ArpBaseLayout::NextLayoutSibling()
{
	return mNextSibling;
}

ArpBaseLayout* ArpBaseLayout::PreviousLayoutSibling()
{
	return mPrevSibling;
}

bool ArpBaseLayout::LayoutRemoveSelf()
{
	if( LayoutParent() ) return LayoutParent()->RemoveLayoutChild(this);
	return false;
}

ArpBaseLayout* ArpBaseLayout::FindLayoutable(const char* name)
{
	if( !name ) return 0;
	
	if( LayoutName() && strcmp(LayoutName(), name) == 0 ) return this;
	
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		ArpBaseLayout* find = child->FindLayoutable(name);
		if( find ) return find;
		child = child->NextLayoutSibling();
	}
	
	return 0;
}

void ArpBaseLayout::LayoutAttachedToWindow()
{
}

void ArpBaseLayout::LayoutAllAttached()
{
}

void ArpBaseLayout::LayoutDetachedFromWindow()
{
}

void ArpBaseLayout::LayoutAllDetached()
{
}

BRect ArpBaseLayout::HintLayoutChild(ArpBaseLayout* before) const
{
	return LayoutBounds();
}

int ArpBaseLayout::LayoutChildSpace() const
{
	return 0;
}

void ArpBaseLayout::ComputeDimens(ArpDimens& cur_dimens)
{
	ArpD(cdb << ADH << "ArpBaseLayout::ComputeDimens()" << std::endl);
	ArpBaseLayout* child = LayoutChildAt(0);
	if( child ) {
		cur_dimens = child->LayoutDimens();
	} else {
		cur_dimens.Init();
	}
}

const ArpDimens& ArpBaseLayout::LayoutDimens(void)
{
	if( dimens_changed ) {
		ComputeDimens(cur_dimens);
		dimens_changed = false;
	}
	return cur_dimens;
}

BRect ArpBaseLayout::LayoutFrame() const
{
	return cur_frame;
}

BRect ArpBaseLayout::BodyFrame() const
{
	const_cast<ArpBaseLayout*>(this)->ensure_body_frame();
	return body_frame;
}

BRect ArpBaseLayout::LayoutBounds() const
{
	// Compute the interior drawing frame of this object.
	// If there is no BView associated with it, this is simply
	// the real offset we were given -- we use the same
	// coordinate system as our parent.
	// Otherwise, we have our own coordinate system and the
	// inner frame is simply placed at (0,0).
	BRect bounds(cur_frame);
	if( const_cast<ArpBaseLayout*>(this)->OwnerView() ) {
		bounds.OffsetTo(0,0);
	}
	return bounds;
}

BRect ArpBaseLayout::BodyBounds() const
{
	// Compute the interior body frame of this object.
	// If there is no BView associated with it, this is simply
	// the real offset we were given -- we use the same
	// coordinate system as our parent.
	// Otherwise, we have our own coordinate system and the
	// inner frame is simply placed at (0,0).
	const_cast<ArpBaseLayout*>(this)->ensure_body_frame();
	BRect bounds(body_frame);
	if( const_cast<ArpBaseLayout*>(this)->OwnerView() ) {
		bounds.OffsetBy(-cur_frame.left,-cur_frame.top);
	}
	return bounds;
}

void ArpBaseLayout::SetViewLayout(BRect frame, BRect body, bool force)
{
	ArpD(cdb << ADH << "ArpBaseLayout::SetViewLayout(" << frame << "): "
				  << LayoutName() << std::endl);
	cur_frame = frame;
	body_frame = body;
	if( body.IsValid() ) {
		ArpVALIDATE( cur_frame.left <= body_frame.left,
					 body_frame.left = cur_frame.left );
		ArpVALIDATE( cur_frame.top <= body_frame.top,
					 body_frame.top = cur_frame.top );
		ArpVALIDATE( cur_frame.right >= body_frame.right,
					 body_frame.right = cur_frame.right );
		ArpVALIDATE( cur_frame.bottom >= body_frame.bottom,
					 body_frame.bottom = cur_frame.bottom );
	}
	LayoutChanged(force);
}

void ArpBaseLayout::RequestLayout(bool force)
{
	ArpD(cdb << ADH << "ArpBaseLayout::RequestLayout()" << std::endl);
	LayoutChanged(force);
}

void ArpBaseLayout::SetBodyFill(ArpGravity fill)
{
	if( fill != mBodyFill ) {
		mBodyFill = fill;
		LayoutChanged(false);
	}
}

ArpGravity ArpBaseLayout::BodyFill() const
{
	return mBodyFill;
}

BView* ArpBaseLayout::OwnerView()
{
	return NULL;
}

BView* ArpBaseLayout::InView()
{
	return in_view ? in_view : (in_view = OwnerView());
}

void ArpBaseLayout::InvalidateDimens(void)
{
	if( dimens_changed ) return;
	ArpBaseLayout* root = this;
	ArpBaseLayout* last = this;
	while( root ) {
		ArpD(cdb << ADH << "Invalidating dimensions for: "
					<< root->LayoutName() << std::endl);
		root->dimens_changed = true;
		root->last_frame.left = root->last_frame.top = -1000;
		root->last_frame.right = root->last_frame.bottom = -2000;
		root->InvalidateView();
		last = root;
		root = root->LayoutParent();
	}
	if( !inhibit_layout && last ) last->LayoutChanged(false);
}

void ArpBaseLayout::InvalidateView(void)
{
	//printf("Invalidate view %s: in=%x, owner=%x\n",
	//		LayoutName(), InView(), OwnerView());
	if( InView() ) {
		InView()->Invalidate();
	}
}

void ArpBaseLayout::SetFocusShown(bool state, bool andParent)
{
	focus_shown = state;
	if( andParent && LayoutParent() ) LayoutParent()->SetFocusShown(state);
}

bool ArpBaseLayout::FocusShown(void) const
{
	return focus_shown;
}

void ArpBaseLayout::SetLayoutActivated(bool state)
{
	layout_activated = state;
	ArpBaseLayout* child = LayoutChildAt(0);
	while( child ) {
		child->SetLayoutActivated(state);
		child = child->NextLayoutSibling();
	}
}

bool ArpBaseLayout::LayoutActivated(void) const
{
	return layout_activated;
}

void ArpBaseLayout::ensure_body_frame()
{
	// If the body frame has not been explicitly set, automatically
	// compute an appropriate one from this object's dimensions.
	if( !body_frame.IsValid() ) {
		body_frame = LayoutFrame();
		ArpVALIDATE( !LayoutInhibit(), return );
		
		const ArpDimens& dimens = LayoutDimens();
		body_frame.left += dimens.X().PreLabel();
		body_frame.right -= dimens.X().PostLabel();
		body_frame.top += dimens.Y().PreLabel();
		body_frame.bottom -= dimens.Y().PostLabel();
	}
}

void ArpBaseLayout::LayoutChanged(bool force)
{
	ArpD(cdb << ADH << "ArpBaseLayout::LayoutChanged(" << force
				<< ")" << ": " << LayoutName() << std::endl);
	//ArpBaseLayout* parent = LayoutParent();
	BView* view = OwnerView();
	bool changed = false;
	
	if( LayoutInhibit() ) return;
	
	// Make sure this object's dimension information has been
	// computed by requesting it.
	LayoutDimens();
	
	// Special case for the root of a layout hierarchy -- we don't
	// tell it where to go; it tells us.
	if( LayoutParent() == 0 ) {
		if( force || last_frame != cur_frame ) {
			body_frame = last_frame = cur_frame;
			LayoutView();
		}
		return;
	}
	
	BRect view_frame(BodyFrame());
	if( mBodyFill&ArpNorth ) view_frame.top = cur_frame.top;
	if( mBodyFill&ArpSouth ) view_frame.bottom = cur_frame.bottom;
	if( mBodyFill&ArpEast ) view_frame.right = cur_frame.right;
	if( mBodyFill&ArpWest ) view_frame.left = cur_frame.left;
	
//	printf("ArpBaseLayout::LayoutChanged() of %s\n",LayoutName());
	if( force || view_frame.left != last_frame.left
			  || view_frame.top != last_frame.top ) {
		ArpD(cdb << ADH << "The frame position has changed..." << std::endl);
		if( view ) {
			ArpD(cdb << ADH << "ArpBaseLayout::SetFrame "
						<< LayoutName() << " -- moving." << std::endl);
//			printf("Move %s to (%f,%f)\n",LayoutName(),
//				cur_frame.left,cur_frame.top);
			view->MoveTo(view_frame.left, view_frame.top);
		} else {
			// Only need to re-layout children if we have no
			// BView, so their own offsets will change.
			changed = true;
		}
	}
	if( force || view_frame.Width() != last_frame.Width()
			  || view_frame.Height() != last_frame.Height() ) {
		ArpD(cdb << ADH << "The frame size has changed..." << std::endl);
		changed = true;
		if( view ) {
			ArpD(cdb << ADH << "ArpBaseLayout::SetFrame "
						<< LayoutName() << " -- resizing." << std::endl);
//			printf("Resize %s to (%f,%f)\n",LayoutName(),
//				cur_frame.Width(),cur_frame.Height());
			view->ResizeTo(view_frame.Width(), view_frame.Height());
		}
	}
	last_frame = view_frame;
	if( changed ) {
		LayoutView();
	}
}

void ArpBaseLayout::LayoutView()
{
	BRect frm = LayoutBounds();
	BRect body = BodyBounds();
	ArpD(cdb << ADH << "ArpBaseLayout::LayoutView() -- " << LayoutName()
			<< " " << frm << std::endl);
	ArpBaseLayout* child = LayoutChildAt(0);
	if( child ) {
		child->SetViewLayout(frm, body);
	}
}
