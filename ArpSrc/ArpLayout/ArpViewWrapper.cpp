/*
	
	ArpViewWrapper.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	An ArpLayoutable that can be used to wrap around another
	BView, without requiring multiple inheritance or subclassing.
*/

#ifndef ARPLAYOUT_ARPVIEWWRAPPER_H
#include "ArpLayout/ArpViewWrapper.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#include <float.h>
//
#if __GNUC__ < 3 
#include <algobase.h>
#endif 



#include <support/Autolock.h>

ArpMOD();

static const char* ViewParam = "arp:view";
static const char* OldViewParam = "ArpViewWrapper::View";

const char* ArpViewWrapper::HorridSizeKludgeP = "HorridSizeKludge";

enum {
	SizeKludgeIdx = 0
};

static property_info properties[] = {
	{ const_cast<char*>(ArpViewWrapper::HorridSizeKludgeP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Perform hideous vile things to try to extract size information from Be controls?", 0,
	  { B_BOOL_TYPE, 0 }
	},
	0
};

static const parameter_info parameters[] = {
	{ sizeof(parameter_info), &properties[SizeKludgeIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ 0, 0, 0, 0, 0 }
};

static const parameter_suite suite = {
	sizeof(parameter_suite),
	"suite/vnd.ARP.ArpViewWrapper",
	properties
};

/* ------ ArpViewWrapper constructor and destructor ------
 *
 * The various ways to create and destroy ArpViewWrapper
 * objects.
 */
 
ArpViewWrapper::ArpViewWrapper(BView* wrapview)
	: ArpLayout(wrapview ? wrapview->Name() : "<unnamed>"),
	  view(wrapview)
{
	initialize();
	ArpVALIDATE(view != 0, SetError(B_NO_MEMORY));
}

ArpViewWrapper::ArpViewWrapper(BMessage* data, bool final)
	: ArpLayout(data, false), view(0)
{
	initialize();
	if( data ) {
		BMessage viewarch;
		if( data->FindMessage(ViewParam,&viewarch) == B_NO_ERROR ) {
			view = dynamic_cast<BView*>(instantiate_object(&viewarch));
			if( view == 0 ) printf("ArpViewWrapper: Error instantiating wrapped child.\n");
		
		// Backwards compatibility...
		} else if( data->FindMessage(OldViewParam,&viewarch) == B_NO_ERROR ) {
			view = dynamic_cast<BView*>(instantiate_object(&viewarch));
			if( view == 0 ) printf("ArpViewWrapper: Error instantiating wrapped child.\n");
			
		}
		ArpVALIDATE(view != 0, SetError(B_NO_MEMORY));
		ParamSet()->InstantiateParams(data, parameters);
		if( final ) InstantiateParams(data);
	}
}

void ArpViewWrapper::initialize()
{
	if( ParamSet() ) {
		ParamSet()->AddParameters(parameters, &suite);
		PV_HorridSizeKludge.Init(ParamSet(), HorridSizeKludgeP, false);
	}
}

ArpViewWrapper::~ArpViewWrapper()
{
	if( ParamSet() ) ParamSet()->RemoveParameters(parameters);
}

/* ------------ ArpViewWrapper archiving ------------
 *
 * Archiving and retrieving ArpViewWrapper objects.
 */
ArpViewWrapper* ArpViewWrapper::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpViewWrapper") ) 
		return new ArpViewWrapper(archive); 
	return NULL;
}

status_t ArpViewWrapper::Archive(BMessage* data, bool deep) const
{
	status_t status = inherited::Archive(data, true);
	//if( status == B_NO_ERROR )
	//	status = data->AddString("class","ArpViewWrapper");
	if( status == B_NO_ERROR && view ) {
		BMessage viewarch(B_ARCHIVED_OBJECT);
		status = view->Archive(&viewarch,true);
		if( status == B_NO_ERROR )
			status = data->AddMessage(ViewParam,&viewarch);
	}
	return status;
}

void ArpViewWrapper::MessageReceived(BMessage *message)
{
	if( LayoutMessageReceived(message) == B_OK ) return;
	
	if( message ) {
		int32 idx = 0;
		BMessage spec;
		const char* property = 0;
		int32 what=0;
		if( message->GetCurrentSpecifier(&idx, &spec, &what, &property) == B_OK ) {
			if( what == B_DIRECT_SPECIFIER ) {
				if( strcmp(property, "Suites") == 0
						|| strcmp(property, "Messenger") == 0 ) {
					inherited::MessageReceived(message);
				}
			}
		}
	}
	
	view->MessageReceived(message);
	
	if( message->what == B_SET_PROPERTY ) InvalidateDimens();
}

BHandler*
ArpViewWrapper::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
								 int32 form, const char *property)
{
	BHandler* ret = 0;
	if( LayoutResolveSpecifier(&ret, msg, index, specifier,
							   form, property) == B_OK ) {
		return ret;
	}
	if( strcmp(property, "Suites") == 0 || strcmp(property, "Messenger") == 0 ) {
		return inherited::ResolveSpecifier(msg, index, specifier, form, property);
	}
	return view->ResolveSpecifier(msg, index, specifier, form, property);
}

status_t ArpViewWrapper::GetSupportedSuites(BMessage *data)
{
        LayoutGetSupportedSuites(data);
        return view->GetSupportedSuites(data);
}

BHandler* ArpViewWrapper::LayoutHandler()
{
	return this;
}
	
const BHandler* ArpViewWrapper::LayoutHandler() const
{
	return this;
}
	
BView* ArpViewWrapper::OwnerView(void)
{
	return view;
}

void ArpViewWrapper::ComputeDimens(ArpDimens& cur_dimens)
{
	if( view ) {
		if( PV_HorridSizeKludge ) get_view_dimens(&cur_dimens, view);
		else {
			float vw=0;
			float vh=0;
			view->GetPreferredSize(&vw,&vh);
			vw++;
			vh++;
			cur_dimens.X().SetTo(0, vw, vw, vw, 0);
			cur_dimens.Y().SetTo(0, vh, vh, vh, 0);
		}
	} else {
		inherited::ComputeDimens(cur_dimens);
	}
}
