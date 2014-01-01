/*
	
	ArpLayoutable.cpp
	
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

#ifndef ARPLAYOUT_ARPLAYOUTABLE_H
#include <ArpLayout/ArpLayoutable.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _WINDOW_H
#include <be/interface/Window.h>
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
#endif

#include <algobase.h>
#include <float.h>
#include <string.h>

ArpMOD();

const float ArpAnySize = FLT_MAX/2;

ArpMessage ArpNoParams;

static const char* NameParam = "ArpLayoutable:Name";
static const char* ChildParam = "ArpLayoutable:Child";
static const char* ConstraintsParam = "ArpLayoutable:Constraints";

const char* ArpLayoutable::BackgroundColor = "BackgroundColor";
const char* ArpLayoutable::ForegroundColor = "ForegroundColor";
const char* ArpLayoutable::BasicFont = "BasicFont";

/* ------ ArpLayoutable constructor and destructor ------
 *
 * The various ways to create and destroy ArpLayoutable
 * objects.
 */
 
ArpLayoutable::ArpLayoutable(const char* name = 0) : children(5)
{
	do_construct();
	if( name ) SetLayoutName(name);
}

ArpLayoutable::ArpLayoutable(BMessage* data) : children(5)
{
	do_construct();
	if( data ) {
		const char* newname = NULL;
		if( data->FindString(NameParam,&newname) && newname ) {
			SetLayoutName(newname);
		}
		constraints.MakeEmpty();
		data->FindMessage(ConstraintsParam,&constraints);
	}
}

void ArpLayoutable::do_construct(void)
{
	dimens_changed = true;
	inhibit_layout = true;
	focus_shown = false;
	layout_activated = false;
	owner = NULL;
	parent = NULL;
	in_view = NULL;
	globals = NULL;
	PG_BackgroundColor = "StdBackColor";
	PG_ForegroundColor = "StdTextColor";
	PG_BasicFont = "BasicFont";
	PV_BackgroundColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	PV_ForegroundColor.red = 0x00;
	PV_ForegroundColor.green = 0x00;
	PV_ForegroundColor.blue = 0x00;
	PV_BasicFont = *be_plain_font;
	cur_frame = cur_bounds = BRect(-99999.5,-99999.5,-99998.5,-99998.5);
	last_frame = BRect(-99997.5,-99997.5,-99996.5,-99996.5);
	error = B_NO_ERROR;
	SetLayoutName("<unnamed>");
}

ArpLayoutable::~ArpLayoutable()
{
	ArpD(cdb << ADH << "Destroying ArpLayoutable: " << LayoutName() << endl);
	
	ArpLayoutable* child = 0;
	int32 index=0;
	while( (child=(ArpLayoutable*)children.ItemAt(index)) != 0 ) {
		if( !child->OwnerView() ) delete child;
		index++;
	}
}

/* ------------ ArpLayoutable archiving ------------
 *
 * Archiving and retrieving ArpLayoutable objects.
 */
ArpLayoutable* ArpLayoutable::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpLayoutable") ) 
		return new ArpLayoutable(archive); 
	return NULL;
}

status_t ArpLayoutable::Archive(BMessage* data, bool deep)
{
	status_t status = data->AddString("class","ArpLayoutable");
	if( status != B_NO_ERROR )
		status = data->AddMessage(ConstraintsParam,&constraints);
	if( deep ) {
		int i;
		for( i=0; i<CountLayoutChildren(); i++ ) {
			ArpLayoutable* mychild = LayoutChildAt(i);
			if( mychild ) {
				BMessage childarch;
				if( status != B_NO_ERROR )
					status = mychild->Archive(&childarch,deep);
				if( status != B_NO_ERROR )
					status = data->AddMessage(ChildParam,&childarch);
			}
		}
	}
	return status;
}

/* ------------ ArpLayoutable error control ------------
 *
 * Get and set the error state of an ArpLayoutable object.
 * These functions are safe to call on NULL objects.
 */
 
ArpLayoutable* ArpLayoutable::SetError(status_t err)
{
	if( this ) error = err;
	return this;
}

status_t ArpLayoutable::Error()
{
	if( this ) return error;
	return B_NO_MEMORY;
}

status_t ArpLayoutable::InterpretType(const ArpMessage& msg,
	const char* name, type_code type, void* data)
{
	switch(type) {
		case B_STRING_TYPE: {
			const char* str = NULL;
			status_t res = msg.FindString(name,&str);
			if( res == B_NO_ERROR ) *(ArpString*)data = str;
			return res;
		}
		case B_BOOL_TYPE:
			return msg.FindBool(name,(bool*)data);
		case B_INT8_TYPE:
			return msg.FindInt8(name,(int8*)data);
		case B_INT16_TYPE:
			return msg.FindInt16(name,(int16*)data);
		case B_INT32_TYPE:
			return msg.FindInt32(name,(int32*)data);
		case B_INT64_TYPE:
			return msg.FindInt64(name,(int64*)data);
		case B_FLOAT_TYPE:
			return msg.FindFloat(name,(float*)data);
		case B_DOUBLE_TYPE:
			return msg.FindDouble(name,(double*)data);
		case B_POINT_TYPE:
			return msg.FindPoint(name,(BPoint*)data);
		case B_RECT_TYPE:
			return msg.FindRect(name,(BRect*)data);
		case B_REF_TYPE:
			return msg.FindRef(name,(entry_ref*)data);
		case B_RGB_COLOR_TYPE:
			return msg.FindRGBColor(name,(rgb_color*)data);
		case B_PATTERN_TYPE: {
			const pattern* val;
			ssize_t size=0;
			status_t res = msg.FindData(name,type,(const void**)&val,&size);
			if( !res ) *((pattern*)data) = *val;
			return res;
		}
		case B_MESSAGE_TYPE:
			return msg.FindMessage(name,(BMessage*)data);
		case B_MESSENGER_TYPE:
			return msg.FindMessenger(name,(BMessenger*)data);
		case B_POINTER_TYPE:
			return msg.FindPointer(name,(void**)data);
		case FFont::FONT_TYPE:
			return msg.FindFont(name,(BFont*)data);
		default:
			return B_BAD_TYPE;
	}
	return B_NO_ERROR;
}

status_t ArpLayoutable::ExtractParam(
	const ArpMessage* params, const char* name, type_code type,
	const ArpMessage* globs, ArpString* gName, void* result)
{
	status_t res = B_NAME_NOT_FOUND;
	if( name && params ) {
		ArpD(cdb << ADH << "Looking for param " << name << endl);
		if( params->HasData(name,type) ) {
			res = InterpretType(*params,name,type,result);
			if( gName ) *gName = (const char*)0;
			ArpD(cdb << ADH << "Result = " << res << endl);
		} else if( gName ) {
			ArpD(cdb << ADH << "Not found." << endl);
			const char* extname = NULL;
			extname = params->GetIndirect(name,NULL);
			if( extname ) {
				res = B_NO_ERROR;
				*gName = *extname ? extname : (const char*)0;
				ArpD(cdb << ADH << "Found global reference: "
								<< gName << endl);
			}
		}
	}
	
	if( gName && !gName->IsNull() && globs && (!res || !name || !params) ) {
		status_t gres = B_NAME_NOT_FOUND;
		ArpD(cdb << ADH << "Retrieving global " << *gName
					<< " for " << name << endl);
		if( globs->HasData(*gName,type) ) {
			gres = InterpretType(*globs,*gName,type,result);
			
		// If the global wasn't found, and we are being updated from the
		// complete global set, add our current value to the global params.
		} else if( globs == Globals() ) {
			ArpD(cdb << ADH << "Global not found; adding." << endl);
			// This is wretched...  but fortunately, it shouldn't
			// happen very often.
			ArpMessage msg = Params(name);
			status_t res;
			type_code type;
			int32 count;
			if( (res=msg.GetInfo(name,&type,&count)) == B_OK ) {
				const void* data;
				ssize_t numBytes;
				if( (res=msg.FindData(name,type,&data,&numBytes)) == B_OK ) {
					ArpMessage glob;
					if( (res=glob.AddData(*gName,type,data,numBytes))
							== B_OK ) {
						res = AddGlobals(&glob);
					}
				}
			}
			return res;
		}
		if( !gres ) res = B_NO_ERROR;
		if( !res ) {
			ArpD(cdb << ADH << "Found global value." << endl);
		}
	}
	return res;
}

/* ------------ ArpLayoutable parameters ------------
 *
 * Getting and setting object parameters.  Functions to
 * set parameters can be called on NULL objects.
 */
 
ArpLayoutable* ArpLayoutable::SetLayoutName(const char* inname)
{
	if( this ) {
		name = inname;
		if( LayoutHandler() ) LayoutHandler()->SetName(inname);
	}
	return this;
}

ArpLayoutable* ArpLayoutable::SetParams(const ArpMessage& p)
{
	if( this ) {
		ArpD(cdb << ADH << "Setting parameters for "
					<< LayoutName() << ": " << p << endl);
		DoSetParams(globals,&p);
	}
	return this;
}

const ArpMessage ArpLayoutable::Params(const char* name)
{
	if( this ) {
		ArpMessage p;
		DoParams(p,name);
		ArpD(cdb << ADH << "Retrieved paramters for "
					<< LayoutName() << ": " << p << endl);
		return p;
	}
	return ArpNoParams;
}

int32 ArpLayoutable::DoSetParams(const ArpMessage* g,
								const ArpMessage* p)
{
	ArpD(cdb << ADH << "Doing set params, globals = " << g << endl);
	ArpD(if( p ) cdb << ADH << "Params = " << *p << endl);
	
	int32 count=0;
	
	status_t res;
	res = ExtractParam(p,BackgroundColor,B_RGB_COLOR_TYPE,
						g,&PG_BackgroundColor,&PV_BackgroundColor);
	if( !res ) {
		ArpD(cdb << ADH << "Invalidating view: view color" << endl);
		if(OwnerView()) {
			OwnerView()->SetViewColor(PV_BackgroundColor);
			OwnerView()->SetLowColor(PV_BackgroundColor);
		}
		InvalidateView();
		count++;
	}
	
	res = ExtractParam(p,ForegroundColor,B_RGB_COLOR_TYPE,
						g,&PG_ForegroundColor,&PV_ForegroundColor);
	if( !res ) {
		ArpD(cdb << ADH << "Invalidating view: high color" << endl);
		if(OwnerView()) OwnerView()->SetHighColor(PV_ForegroundColor);
		InvalidateView();
		count++;
	}
	
	res = ExtractParam(p,BasicFont,FFont::FONT_TYPE,
						g,&PG_BasicFont,&PV_BasicFont);
	if( !res ) {
		ArpD(cdb << ADH << "Invalidating dimens: font" << endl);
		if(OwnerView()) OwnerView()->SetFont(&PV_BasicFont);
		InvalidateDimens();
		count++;
	}
	
	return count;
}

bool ArpLayoutable::DoParams(ArpMessage& p, const char* name)
{
	bool res = true;
	if( !name || !strcmp(name,BackgroundColor) ) {
		if( p.AddRGBColor(BackgroundColor,&PV_BackgroundColor) ) {
			res = false;
		}
		if( name ) return res;
	}
	if( !name || !strcmp(name,ForegroundColor) ) {
		if( p.AddRGBColor(ForegroundColor,&PV_ForegroundColor) ) {
			res = false;
		}
		if( name ) return res;
	}
	if( !name || !strcmp(name,BasicFont) ) {
		if( p.AddFont(BasicFont,&PV_BasicFont) ) {
			res = false;
		}
		if( name ) return res;
	}
	return res;
}

ArpLayoutable* ArpLayoutable::SetGlobals(const ArpMessage* gl)
{
	if( this ) {
		bool inhibit = inhibit_layout;
		if( !inhibit ) SetLayoutInhibit(true);
		globals = gl;
		#if defined(ArpDEBUG)
		if( gl ) {
			ArpD(cdb << ADH << "Setting globals for "
						<< LayoutName() << ": " << *globals << endl);
		} else {
			ArpD(cdb << ADH << "Clearing globals for "
						<< LayoutName() << "." << endl);
		}
		#endif
		DoSetParams(globals,NULL);
		int32 num = CountLayoutChildren();
		for( int32 i=0; i<num; i++ ) {
			ArpLayoutable* child = LayoutChildAt(i);
			if( child ) child->SetGlobals(globals);
		}
		if( !inhibit ) SetLayoutInhibit(false);
	}
	return this;
}

bool ArpLayoutable::RefreshGlobals(const ArpMessage* gl)
{
	if( this ) {
		if( !gl ) gl = globals;
		ArpD(cdb << ADH << "Refreshing globals for "
					<< LayoutName() << " with: " << *gl << endl);
		DoSetParams(gl,NULL);
		int32 num = CountLayoutChildren();
		for( int32 i=0; i<num; i++ ) {
			ArpLayoutable* child = LayoutChildAt(i);
			if( child ) child->RefreshGlobals(gl);
		}
		return true;
	}
	return this;
}

const ArpMessage* ArpLayoutable::Globals(void)
{
	if( this ) return globals;
	return 0;
}

status_t ArpLayoutable::AddGlobals(const ArpMessage* gl)
{
	if( LayoutParent() ) return LayoutParent()->AddGlobals(gl);
	return B_NO_ERROR;
}

ArpLayoutable* ArpLayoutable::SetConstraints(const ArpMessage& c)
{
	if( this ) {
		constraints.Update(c);
		ArpD(cdb << ADH << "Set constraints for "
					<< LayoutName() << ": " << constraints << endl);
		InvalidateDimens();
	}
	return this;
}

ArpMessage& ArpLayoutable::Constraints(void)
{
	if( this ) return constraints;
	return ArpNoParams;
}

void ArpLayoutable::do_inhibit_layout(bool state)
{
	inhibit_layout = state;
	int32 num = CountLayoutChildren();
	for( int32 i=0; i<num; i++ ) {
		ArpLayoutable* child = LayoutChildAt(i);
		if( child ) child->do_inhibit_layout(state);
	}
}

ArpLayoutable* ArpLayoutable::SetLayoutInhibit(bool state)
{
	if( this ) {
		ArpD(cdb << ADH << "Setting layout inhibit: " << state << endl);
		inhibit_layout = state;
		ArpLayoutable* root = this;
		while( root->LayoutParent() ) {
			root = root->LayoutParent();
		}
		root->do_inhibit_layout(state);
		if( !state && dimens_changed ) {
			ArpD(cdb << ADH << "dimens_changed; changing layout." << endl);
			root->LayoutChanged(false);
		}
	}
	return this;
}

void ArpLayoutable::DrawLayout(BView* inside, BRect region)
{
	int32 num = CountLayoutChildren();
	for( int32 i=0; i<num; i++ ) {
		ArpLayoutable* child = LayoutChildAt(i);
		if( child && !child->OwnerView()) child->DrawLayout(inside, region);
	}
}

status_t ArpLayoutable::LayoutMessageReceived(BMessage *message)
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
					
					ArpMessage set(B_SET_PROPERTY);
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
	
	switch( message->what ) {
		case B_GET_PROPERTY: {
			ArpMessage result(B_REPLY);
			if( DoParams(result, property) ) {
				if( result.Rename(property, "result") == B_OK
						&& result.AddInt32("error", B_OK) == B_OK ) {
					message->SendReply(&result);
					return B_OK;
				}
			}
		} break;
		case B_SET_PROPERTY: {
			type_code type=0;
			bool fixed_size=false;
			if( message->GetInfo("data", &type, &fixed_size) == B_OK ) {
				const void* data=0;
				ssize_t numBytes=0;
				if( message->FindData("data", type, &data, &numBytes) == B_OK ) {
					ArpMessage set(B_SET_PROPERTY);
					set.AddData(property, type, data, numBytes, fixed_size);
					if( DoSetParams(globals, &set) > 0 ) return B_OK;
				}
			}
		} break;
	}
	
	return B_ERROR;
}

status_t ArpLayoutable::LayoutResolveSpecifier(BHandler** result,
										BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property)
{
	if( form == B_DIRECT_SPECIFIER ) {
		if( strcmp(property, "Constraints") == 0 ) {
			if( result ) *result = LayoutHandler();
			return B_OK;
		}
		ArpMessage reply(B_REPLY);
		if( DoParams(reply, property) ) {
			if( result ) *result = LayoutHandler();
			return B_OK;
		}
	}
	
	return B_ERROR;
}

static property_info properties[] = {
	{ "Constraints", { 0 }, { B_DIRECT_SPECIFIER },
	  "Parent layout constraints applied to this child.", 0
	},
	{ const_cast<char*>(ArpLayoutable::BackgroundColor),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Standard background color of object.", 0 
	},
	{ const_cast<char*>(ArpLayoutable::ForegroundColor),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Standard foreground color of object.", 0 
	},
	{ const_cast<char*>(ArpLayoutable::BasicFont),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Standard font of object.", 0 
	},
	0
};

status_t ArpLayoutable::LayoutGetSupportedSuites(BMessage *data)
{
	data->AddString("suites", "suite/vnd.ARP.ArpLayoutable");
	BPropertyInfo prop_info(properties);
	data->AddFlat("messages", &prop_info);
	return B_OK;
}

void ArpLayoutable::GetConstraintSuites(BMessage *data)
{
}
	
status_t ArpLayoutable::QueryConstraint(const char* name,
										BMessage* data) const
{
	return B_NAME_NOT_FOUND;
}

BWindow* ArpLayoutable::LayoutWindow() const
{
	return owner;
}

void ArpLayoutable::set_layout_window(BWindow* window)
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
	
	ArpLayoutable* child = LayoutChildAt(0);
	while( child ) {
		if( !child->OwnerView() ) child->set_layout_window(window);
		child = child->NextLayoutSibling();
	}
}

static void send_layout_attach(ArpLayoutable* o, bool attach)
{
	if( !o ) return;
	
	if( attach ) o->LayoutAttachedToWindow();
	else o->LayoutDetachedFromWindow();
	
	ArpLayoutable* child = o->LayoutChildAt(0);
	while( child ) {
		if( !child->OwnerView() ) send_layout_attach(child, attach);
		child = child->NextLayoutSibling();
	}
	
	if( attach ) o->LayoutAllAttached();
	else o->LayoutAllDetached();
}

void ArpLayoutable::AttachLayoutWindow(BWindow* window)
{
	if( window ) {
		set_layout_window(window);
		send_layout_attach(this, true);
	} else {
		send_layout_attach(this, false);
		set_layout_window(window);
	}
}

/* ------------ ArpLayoutable child manipulation ------------
 *
 * Adding, examining, removing children.  This operates in
 * parallel with the BView mechanism; we need to do our own
 * implementation since we can have ArpLayoutable objects without
 * associated BView objects.
 */

BView* ArpLayoutable::FindClosestView(int32 index)
{
	ArpLayoutable* child = 0;
	
	while( (child=(ArpLayoutable*)children.ItemAt(index)) != 0 ) {
		BView* view = child->OwnerView();
		if( view ) return view;
		view = child->FindClosestView(0);
		if( view ) return view;
		index++;
	}
	
	return 0;
}

ArpLayoutable* ArpLayoutable::AddLayoutChild(ArpLayoutable* v,
											   const BMessage& c,
											   ArpLayoutable* before)
{
	// Sanity checks.
	if( !this ) return NULL;
	if( !v ) {
		SetError(B_NO_MEMORY);
		return this;
	}
	
	ArpD(cdb << ADH << "Adding child: " << v->LayoutName() << endl);
	
	bool old_inhibit = LayoutInhibit();
	SetLayoutInhibit(true);
	
	// Add the given child to this ArpLayoutable.
	int bindex = -1;
	if( before ) {
		bindex = children.IndexOf(before);
		if( bindex < 0 ) {
			debugger("AddLayoutChild() failed -- the \"before\" layout and \"this\" don't match.");
			bindex = 0;
		}
		children.AddItem(v, bindex);
	} else {
		children.AddItem(v);
	}
	if( &c != &ArpNoParams ) v->constraints = c;
	v->parent = this;
	if( !(in_view=OwnerView()) && LayoutParent() ) {
		in_view = LayoutParent()->InView();
	}
	
	// Figure out the 'before' object in the BView space,
	// and add the child to our parent BView.
	BView* beforeView = 0;
	if( bindex >= 0 ) beforeView = FindClosestView(bindex+1);
	ArpD(cdb << ADH << "Attaching view: parent=" << in_view
					<< ", before=" << beforeView << endl);
	v->AttachView(in_view, beforeView);
	
	// Update globals and make sure the dimensions are re-evaluated.
	if( globals && globals != v->globals ) v->SetGlobals(globals);
	ArpD(cdb << ADH << "Invalidating dimens; changed="
					<< dimens_changed << endl);
	InvalidateDimens();
	
	v->SetLayoutInhibit(old_inhibit);
	
	return this;
}

bool ArpLayoutable::RemoveLayoutChild(ArpLayoutable* child)
{
	if( !child ) return false;
	
	bool old_inhibit = LayoutInhibit();
	SetLayoutInhibit(true);
	
	bool res = children.RemoveItem(child);
	ArpD(cdb << ADH << "Removed child " << child->LayoutName()
					<< " from " << LayoutName() << "; result="
					<< res << endl);
	if( res ) {
		child->AttachView(0);
		child->parent = 0;
	}
	InvalidateDimens();
	
	SetLayoutInhibit(old_inhibit);
	
	return res;
}

void ArpLayoutable::AttachView(BView* par_view, BView* before)
{
	ArpD(cdb << ADH << "AttachView: par_view=" << par_view
				<< ", before=" << before << ", owner=" << OwnerView()
				<< ", in=" << InView() << endl);
				
	// If this ArpLayoutable does not have an associated BView,
	// we need to run through all of its children looking for BViews
	// that instead need to be attached.
	if( !OwnerView() ) {
		in_view = par_view;
		for( int32 i=0; i<CountLayoutChildren(); i++ ) {
			ArpLayoutable* child = LayoutChildAt(i);
			if( child ) child->AttachView(par_view, before);
		}
		
	// If we are adding to a parent, do so.
	} else if( par_view ) {
		par_view->AddChild(OwnerView(), before);
	
	// If we are removing from a parent, do so.
	} else {
		OwnerView()->RemoveSelf();
	}
}

int32 ArpLayoutable::CountLayoutChildren() const
{
	return children.CountItems();
}

ArpLayoutable* ArpLayoutable::LayoutChildAt(int32 index) const
{
	ArpLayoutable *child = (ArpLayoutable*)children.ItemAt(index);
	return child;
}

ArpLayoutable* ArpLayoutable::NextLayoutSibling()
{
	if( parent ) {
		int newindex = parent->children.IndexOf(this)+1;
		if( newindex < parent->CountLayoutChildren() ) {
			return parent->LayoutChildAt(newindex);
		}
	}
	return NULL;
}

ArpLayoutable* ArpLayoutable::PreviousLayoutSibling()
{
	if( parent ) {
		int newindex = parent->children.IndexOf(this)-1;
		if( newindex >= 0 ) {
			return parent->LayoutChildAt(newindex);
		}
	}
	return NULL;
}

bool ArpLayoutable::LayoutRemoveSelf()
{
	if( parent ) return parent->RemoveLayoutChild(this);
	return false;
}

ArpLayoutable* ArpLayoutable::FindLayoutable(const char* name)
{
	if( !name ) return 0;
	
	if( LayoutName() && strcmp(LayoutName(), name) == 0 ) return this;
	
	for( int32 i=0; i<CountLayoutChildren(); i++ ) {
		ArpLayoutable* child = LayoutChildAt(i);
		if( child ) child = child->FindLayoutable(name);
		if( child ) return child;
	}
	
	return 0;
}

BRect ArpLayoutable::HintLayoutChild(ArpLayoutable* before) const
{
	return LayoutBounds();
}

void ArpLayoutable::ComputeDimens(void)
{
	ArpD(cdb << ADH << "ArpLayoutable::ComputeDimens()" << endl);
	ArpLayoutable* child = (ArpLayoutable*)children.FirstItem();
	if( child ) {
		cur_dimens = child->LayoutDimens();
	} else {
		cur_dimens.min_width = cur_dimens.min_height
			= cur_dimens.pref_width = cur_dimens.pref_height = 0;
		cur_dimens.max_width = cur_dimens.max_height = ArpAnySize;
	}
}

const ArpLayoutDimens& ArpLayoutable::LayoutDimens(void)
{
	if( dimens_changed ) {
		ComputeDimens();
		ArpD(cdb << ADH << "Uncorrected dimensions for "
					<< LayoutName() << endl);
		ArpD(cdb << ADH << "  min  width=" << cur_dimens.min_width
					<< " height=" << cur_dimens.min_height << endl);
		ArpD(cdb << ADH << "  pref width=" << cur_dimens.pref_width
					<< " height=" << cur_dimens.pref_height << endl);
		ArpD(cdb << ADH << "  max  width=" << cur_dimens.max_width
					<< " height=" << cur_dimens.max_height << endl);
		cur_dimens.min_width = ceil(max(cur_dimens.min_width,(float)0));
		cur_dimens.min_height = ceil(max(cur_dimens.min_height,(float)0));
		cur_dimens.pref_width = ceil(max(cur_dimens.min_width,
										cur_dimens.pref_width));
		cur_dimens.pref_height = ceil(max(cur_dimens.min_height,
										cur_dimens.pref_height));
		cur_dimens.max_width = floor(max(cur_dimens.max_width,(float)0));
		cur_dimens.max_height = floor(max(cur_dimens.max_height,(float)0));
		dimens_changed = false;
	}
	return cur_dimens;
}

void ArpLayoutable::ResizeLayout(float width, float height,
								  bool force)
{
	ArpD(cdb << ADH << "ArpLayoutable::LayoutResize(" << width
				 << "," << height << "): " << LayoutName() << endl);
	cur_frame.right = cur_frame.left + width;
	cur_frame.bottom = cur_frame.top + height;
	LayoutChanged(force);
}

void ArpLayoutable::MoveLayout(float left, float top, bool force)
{
	ArpD(cdb << ADH << "ArpLayoutable::LayoutMove(" << left
				 << "," << top << "): " << LayoutName() << endl);
	cur_frame.OffsetTo(left,top);
	LayoutChanged(force);
}

void ArpLayoutable::SetViewLayout(const BRect& frame, bool force)
{
	ArpD(cdb << ADH << "ArpLayoutable::SetViewLayout(" << frame << "): "
				  << LayoutName() << endl);
	cur_frame = frame;
	LayoutChanged(force);
}

void ArpLayoutable::RequestLayout(bool force)
{
	ArpD(cdb << ADH << "ArpLayoutable::RequestLayout()" << endl);
	LayoutChanged(force);
}

void ArpLayoutable::InvalidateDimens(void)
{
	if( dimens_changed ) return;
	ArpLayoutable* root = this;
	ArpLayoutable* last = this;
	while( root ) {
		ArpD(cdb << ADH << "Invalidating dimensions for: "
					<< root->LayoutName() << endl);
		root->dimens_changed = true;
		root->last_frame.left = root->last_frame.top = -1000;
		root->last_frame.right = root->last_frame.bottom = -2000;
		root->InvalidateView();
		last = root;
		root = root->LayoutParent();
	}
	if( !inhibit_layout && last ) last->LayoutChanged(false);
}

void ArpLayoutable::InvalidateView(void)
{
	//printf("Invalidate view %s: in=%x, owner=%x\n",
	//		LayoutName(), InView(), OwnerView());
	if( InView() ) {
		InView()->Invalidate();
	}
}

void ArpLayoutable::SetFocusShown(bool state, bool andParent)
{
	focus_shown = state;
	if( andParent && LayoutParent() ) LayoutParent()->SetFocusShown(state);
}

bool ArpLayoutable::FocusShown(void) const
{
	return focus_shown;
}

void ArpLayoutable::SetLayoutActivated(bool state)
{
	layout_activated = state;
	int32 num = CountLayoutChildren();
	for( int32 i=0; i<num; i++ ) {
		ArpLayoutable* child = LayoutChildAt(i);
		if( child ) child->SetLayoutActivated(state);
	}
}

bool ArpLayoutable::LayoutActivated(void) const
{
	return layout_activated;
}

void ArpLayoutable::LayoutChanged(bool force)
{
	ArpD(cdb << ADH << "ArpLayoutable::LayoutChanged(" << force
				<< ")" << ": " << LayoutName() << endl);
	//ArpLayoutable* parent = LayoutParent();
	BView* view = OwnerView();
	bool changed = false;
	
	// Make sure this object's dimension information has been
	// computed by requesting it.
	LayoutDimens();
	
	// Special case for the root of a layout hierarchy -- we don't
	// tell it where to go; it tells us.
	if( IsLayoutRoot() ) {
		if( force || last_frame != cur_frame ) {
			last_frame = cur_frame;
			cur_bounds = cur_frame;
			if( view ) cur_bounds.OffsetTo(0,0);
			LayoutView();
		}
		return;
	}
	
	// Compute the interior drawing frame of this object.
	// If there is no BView associated with it, this is simply
	// the real offset we were given -- we use the same
	// coordinate system as our parent.
	// Otherwise, we have our own coordinate system and the
	// inner frame is simply placed at (0,0).
	cur_bounds = cur_frame;
	if( view ) cur_bounds.OffsetTo(0,0);
	
	//printf("ArpLayoutable::LayoutChanged() of %s\n",LayoutName());
	if( force || cur_frame.left != last_frame.left
			  || cur_frame.top != last_frame.top ) {
		ArpD(cdb << ADH << "The frame position has changed..." << endl);
		if( view ) {
			ArpD(cdb << ADH << "ArpLayoutable::SetFrame "
						<< LayoutName() << " -- moving." << endl);
			//printf("Move %s to (%f,%f)\n",LayoutName(),
			//	cur_frame.left,cur_frame.top);
			view->MoveTo(cur_frame.left, cur_frame.top);
		} else {
			// Only need to re-layout children if we have no
			// BView, so their own offsets will change.
			changed = true;
		}
	}
	if( force || cur_frame.Width() != last_frame.Width()
			  || cur_frame.Height() != last_frame.Height() ) {
		ArpD(cdb << ADH << "The frame size has changed..." << endl);
		changed = true;
		if( view ) {
			ArpD(cdb << ADH << "ArpLayoutable::SetFrame "
						<< LayoutName() << " -- resizing." << endl);
			//printf("Resize %s to (%f-%f)\n",LayoutName(),
			//	cur_frame.Width(),cur_frame.Height());
			view->ResizeTo(cur_frame.Width(), cur_frame.Height());
		}
	}
	last_frame = cur_frame;
	if( changed ) {
		LayoutView();
	}
}

void ArpLayoutable::LayoutView()
{
	BRect frm = LayoutBounds();
	ArpD(cdb << ADH << "ArpLayoutable::LayoutView() -- " << LayoutName()
			<< " " << frm << endl);
	ArpLayoutable* child = (ArpLayoutable*)children.FirstItem();
	if( child ) {
		child->SetViewLayout(frm);
	}
}
