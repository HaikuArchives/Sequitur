/*
	ArpLayoutTools.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	Miscellaneous things used by the ArpLayout library.
*/

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTDEFS_H
#include <ArpLayout/ArpLayoutDefs.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _MESSAGE_H
#include <Message.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#include <float.h>
//#include <algobase.h>

#include <support/Autolock.h>

ArpMOD();

const param_value_item arp_orientation_values[] = {
	{ "Horizontal", ARP_ENUM_VALUE, B_HORIZONTAL },
	{ "Vertical", ARP_ENUM_VALUE, B_VERTICAL },
	{ 0, ARP_ENUM_VALUE, 0 }
};

const param_value_item arp_border_style_values[] = {
	{ "Plain", ARP_ENUM_VALUE, B_PLAIN_BORDER },
	{ "Fancy", ARP_ENUM_VALUE, B_FANCY_BORDER },
	{ "None", ARP_ENUM_VALUE, B_NO_BORDER },
	{ 0, ARP_ENUM_VALUE, 0 }
};

const param_value_item arp_alignment_values[] = {
	{ "Left", ARP_ENUM_VALUE, B_ALIGN_LEFT },
	{ "Right", ARP_ENUM_VALUE, B_ALIGN_RIGHT },
	{ "Center", ARP_ENUM_VALUE, B_ALIGN_CENTER },
	{ 0, ARP_ENUM_VALUE, 0 }
};

const param_value_item arp_vertical_alignment_values[] = {
	{ "Top", ARP_ENUM_VALUE, B_ALIGN_TOP },
	{ "Middle", ARP_ENUM_VALUE, B_ALIGN_MIDDLE },
	{ "Bottom", ARP_ENUM_VALUE, B_ALIGN_BOTTOM },
	{ "None", ARP_ENUM_VALUE, B_ALIGN_NO_VERTICAL },
	{ 0, ARP_ENUM_VALUE, 0 }
};

const param_value_item arp_gravity_values[] = {
	{ "Center", ARP_ENUM_VALUE, ArpCenter },
	{ "FillAll", ARP_ENUM_VALUE, ArpFillAll },
	{ "North", ARP_MASK_VALUE, ArpNorth },
	{ "South", ARP_MASK_VALUE, ArpSouth },
	{ "East", ARP_MASK_VALUE, ArpEast },
	{ "West", ARP_MASK_VALUE, ArpWest },
	{ 0, ARP_ENUM_VALUE, 0 }
};
	
const BMessage ArpNoParams;

BMessage& arp_update_message(BMessage& to, const BMessage& msg)
{
	char* name;
	type_code type;
	int32 count;
	for( int32 i=0; !msg.GetInfo(B_ANY_TYPE,i,&name,&type,&count);
		i++ ) {
		bool all_gone = false;
		for( int32 j=0; j<count; j++ ) {
			const void* data;
			ssize_t size;
			if( !msg.FindData(name,type,j,&data,&size) ) {
				if( !all_gone ) {
					if( type == B_MESSAGE_TYPE ) {
						BMessage oldMsg;
						BMessage newMsg;
						if( !to.FindMessage(name,j,&oldMsg) &&
							!msg.FindMessage(name,j,&newMsg) ) {
							arp_update_message(oldMsg, newMsg);
							to.ReplaceMessage(name,j,&oldMsg);
						} else {
							all_gone = true;
						}
					}
					if( to.ReplaceData(name,type,j,data,size) ) {
						int32 cnt=0;
						type_code mtype = type;
						if( !to.GetInfo(name,&mtype,&cnt) ) {
							for( int32 k=cnt-1; k>=j; k-- ) {
								to.RemoveData(name,k);
							}
						}
						all_gone = true;
					}
				}
				if( all_gone ) {
					to.AddData(name,type,data,size);
				}
			}
		}
	}
	return to;
}

void get_view_dimens(ArpDimens* dimens, BView* view, bool sizekludge)
{
	ArpASSERT(dimens != 0 && view != 0);
	
	ArpD(cdb << ADH << "**** arp_get_view_dimens(): " << view->Name() << std::endl);
	ArpD(cdb << ADH << "orig min_width = " << dimens->X().MinBody()
				<< ", min_height = " << dimens->Y().MinBody() << std::endl);
	ArpD(cdb << ADH << "orig pref_width = " << dimens->X().PrefBody()
				<< ", pref_height = " << dimens->Y().PrefBody() << std::endl);
	float vw=0;
	float vh=0;
	if( view ) {
		if( sizekludge ) {
			// XXX This is an UGLY, UGLY hack!!  But it seems we
			// have to do this to get many views to return reasonable
			// dimensions to us...
			BRect frm = view->Frame();
			ArpD(cdb << ADH << "Got current frame: " << frm << std::endl);
			view->ResizeTo(1,1);
			ArpD(cdb << ADH << "Resized to (1,1)" << std::endl);
			view->Draw(view->Bounds());
			ArpD(cdb << ADH << "Called Draw() using bounds dimensions." << std::endl);
			//view->Invalidate();
			view->ResizeToPreferred();
			ArpD(cdb << ADH << "Resized to preferred." << std::endl);
			view->GetPreferredSize(&vw,&vh);
			ArpD(cdb << ADH << "retrieved preferred size." << std::endl);
			vw++;
			vh++;
			view->ResizeTo(frm.Width(),frm.Height());
			ArpD(cdb << ADH << "vwidth = " << vw << ", vheight = " << vh << std::endl);
		} else {
			view->GetPreferredSize(&vw,&vh);
			ArpD(cdb << ADH << "retrieved preferred size." << std::endl);
			vw++;
			vh++;
		}
	}
	dimens->X().SetTo(0, vw, vw, vw, 0);
	dimens->Y().SetTo(0, vh, vh, vh, 0);
	ArpD(cdb << ADH << "min_width = " << dimens->X().MinBody()
				<< ", min_height = " << dimens->Y().MinBody() << std::endl);
	ArpD(cdb << ADH << "pref_width = " << dimens->X().PrefBody()
				<< ", pref_height = " << dimens->Y().PrefBody() << std::endl);
}
