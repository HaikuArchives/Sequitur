/*
	
	ArpRunningBar.c
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	A layout class that organizes its children as a horizontal
	or vertical row.
*/

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include "ArpLayout/ArpRunningBar.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

//
#if __GNUC__ < 3 
#include <algobase.h>
#endif 


#include <float.h>
#include <cstring>

ArpMOD();

const char* ArpRunningBar::WeightC = "Weight";
const char* ArpRunningBar::FillC = "Fill";
const char* ArpRunningBar::AlignLabelsC = "AlignLabels";
const char* ArpRunningBar::OrientationP = "Orientation";
const char* ArpRunningBar::InsetLeftP = "InsetLeft";
const char* ArpRunningBar::InsetRightP = "InsetRight";
const char* ArpRunningBar::InsetTopP = "InsetTop";
const char* ArpRunningBar::InsetBottomP = "InsetBottom";
const char* ArpRunningBar::IntraSpaceP = "IntraSpace";

enum {
	OrientationIdx = 0,
	InsetLeftIdx,
	InsetRightIdx,
	InsetTopIdx,
	InsetBottomIdx,
	IntraSpaceIdx
};

static property_info properties[] = {
	{ const_cast<char*>(ArpRunningBar::OrientationP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Direction of layout: B_HORIZONTAL or B_VERTICAL.", 0,
	  { B_INT32_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpRunningBar::InsetLeftP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Space (in characters) to leave on left side.", 0,
	  { B_FLOAT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpRunningBar::InsetRightP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Space (in characters) to leave on right side.", 0,
	  { B_FLOAT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpRunningBar::InsetTopP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Space (in characters) to leave on top side.", 0,
	  { B_FLOAT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpRunningBar::InsetBottomP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Space (in characters) to leave on bottom side.", 0,
	  { B_FLOAT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	{ const_cast<char*>(ArpRunningBar::IntraSpaceP),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Space (in characters) to place between children.", 0,
	  { B_FLOAT_TYPE, ARP_INDIRECT_TYPE, 0 }
	},
	0
};

static const parameter_info parameters[] = {
	{ sizeof(parameter_info), &properties[OrientationIdx],
	  0, ARP_INVALIDATE_DIMENS, arp_orientation_values
	},
	{ sizeof(parameter_info), &properties[InsetLeftIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[InsetRightIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[InsetTopIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[InsetBottomIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ sizeof(parameter_info), &properties[IntraSpaceIdx],
	  0, ARP_INVALIDATE_DIMENS, 0
	},
	{ 0, 0, 0, 0, 0 }
};

static const parameter_suite suite = {
	sizeof(parameter_suite),
	"suite/vnd.ARP.ArpRunningBar",
	properties
};

/* ------ ArpRunningBar constructor and destructor ------
 *
 * The various ways to create and destroy ArpRunningBar
 * objects.
 */
 
ArpRunningBar::ArpRunningBar(const char* name, uint32 flags)
		: ArpLayoutView(name, flags)
{
	initialize();
}

ArpRunningBar::ArpRunningBar(BMessage* data, bool final)
	: ArpLayoutView(data, false)
{
	initialize();
	if( final ) InstantiateParams(data);
	ParamSet()->InstantiateParams(data, parameters);
}

void ArpRunningBar::initialize()
{
	if( ParamSet() ) {
		ParamSet()->AddParameters(parameters, &suite);
		PV_Orientation.Init(ParamSet(), OrientationP, B_HORIZONTAL);
		PV_InsetLeft.Init(ParamSet(), InsetLeftP, 0);
		PV_InsetRight.Init(ParamSet(), InsetRightP, 0);
		PV_InsetTop.Init(ParamSet(), InsetTopP, 0);
		PV_InsetBottom.Init(ParamSet(), InsetBottomP, 0);
		PV_IntraSpace.Init(ParamSet(), IntraSpaceP, 0);
	}
	total_weight = 0;
	char_w = char_h = 0;
	inset_l = inset_r = inset_t = inset_b = 0;
	intra_s = 0;
}

ArpRunningBar::~ArpRunningBar()
{
	if( ParamSet() ) ParamSet()->RemoveParameters(parameters);
}

/* ------------ ArpRunningBar archiving ------------
 *
 * Archiving and retrieving ArpBaseLayout objects.
 */
ArpRunningBar* ArpRunningBar::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpRunningBar") ) 
		return new ArpRunningBar(archive); 
	return NULL;
}

status_t ArpRunningBar::Archive(BMessage* data, bool deep) const
{
	status_t status = inherited::Archive(data,deep);
	//if( status == B_NO_ERROR )
	//	status = data->AddString("class","ArpRunningBar");
	return status;
}

int ArpRunningBar::LayoutChildSpace() const
{
	return INT_MAX;
}

BRect ArpRunningBar::HintLayoutChild(ArpBaseLayout* before) const
{
	float inset_l = ceil(PV_InsetLeft*char_w);
	float inset_r = ceil(PV_InsetRight*char_w);
	float inset_t = ceil(PV_InsetTop*char_h);
	float inset_b = ceil(PV_InsetBottom*char_h);
	
	BRect bnd = LayoutBounds();
	BRect frm = bnd;
	frm.left += inset_l;
	frm.right -= inset_r;
	frm.top += inset_t;
	frm.bottom -= inset_b;
	
	if( CountLayoutChildren() <= 0 ) return frm;
	
	if( PV_Orientation == B_HORIZONTAL ) {
		float x;
		if( before ) x = before->LayoutFrame().left;
		else x = bnd.right;
		if( x < bnd.left+1 ) x = bnd.left+1;
		if( x > bnd.right-1 ) x = bnd.right-1;
		frm.left = x-1;
		frm.right = x+1;
	} else {
		float y;
		if( before ) y = before->LayoutFrame().top;
		else y = bnd.bottom;
		if( y < bnd.top+1 ) y = bnd.top+1;
		if( y > bnd.bottom-1 ) y = bnd.bottom-1;
		frm.top = y-1;
		frm.bottom = y+1;
	}
	
	return frm;
}

static property_info constraints_info[] = {
	{ const_cast<char*>(ArpRunningBar::WeightC),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Relative weight of this child.", 0 
	},
	{ const_cast<char*>(ArpRunningBar::FillC),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Bit mask specifying gravity to fill unneeded space.", 0 
	},
	{ const_cast<char*>(ArpRunningBar::AlignLabelsC),
	  { B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	  { B_DIRECT_SPECIFIER, 0 },
	  "Align this child's labels with its siblings' labels?", 0 
	},
	0
};

void ArpRunningBar::GetConstraintSuites(BMessage *data)
{
	data->AddString("suites", "suite/vnd.ARP.ArpRunningBarC");
	BPropertyInfo prop_info(constraints_info);
	data->AddFlat("messages", &prop_info);
	return inherited::GetConstraintSuites(data);
}

status_t ArpRunningBar::QueryConstraint(const char* name,
								 BMessage* data) const
{
	if( strcmp(name, WeightC) == 0 ) {
		if( data ) {
			data->AddFloat("result", 1.0);
		}
		return B_OK;
	}
	if( strcmp(name, FillC) == 0 ) {
		if( data ) {
			data->AddInt32("result", ArpCenter);
		}
		return B_OK;
	}
	if( strcmp(name, AlignLabelsC) == 0 ) {
		if( data ) {
			data->AddBool("result", true);
		}
		return B_OK;
	}
	
	return B_NAME_NOT_FOUND;
}

void ArpRunningBar::ComputeDimens(ArpDimens& cur_dimens)
{
	// Initialize dimensions of this component.
	ArpUniDimens& curx(cur_dimens.X());
	ArpUniDimens& cury(cur_dimens.Y());
	curx.SetTo(0,0,0,0,0);
	cury.SetTo(0,0,0,0,0);
	total_weight = 0;
	min_weight = 0;
	
	// Minimum size of all children whose labels are not
	// being aligned.
	float min_size = 0, pref_size = 0, max_size = 0;
	
	// Retrieve font metrics.
	char_w = BasicFont()->StringWidth("W");
	font_height fhs;
	BasicFont()->GetHeight(&fhs);
	char_h = fhs.ascent+fhs.descent+fhs.leading;
	
	ArpD(cdb << ADH << "charleft=" << PV_InsetLeft
				<< " charright=" << PV_InsetRight
				<< " chartop=" << PV_InsetTop
				<< " charbot=" << PV_InsetBottom
				<< " charspace=" << PV_IntraSpace << std::endl);
	
	// Compute inset pixel values based on font metrics.
	inset_l = ceil(PV_InsetLeft*char_w);
	inset_r = ceil(PV_InsetRight*char_w);
	inset_t = ceil(PV_InsetTop*char_h);
	inset_b = ceil(PV_InsetBottom*char_h);
	intra_s = ceil(PV_IntraSpace *
					((PV_Orientation==B_VERTICAL) ? char_w:char_h));
					
	ArpD(cdb << ADH << "pixeft=" << inset_l
				<< " pixright=" << inset_r
				<< " pixtop=" << inset_t
				<< " pixbot=" << inset_b
				<< " pixspace=" << intra_s << std::endl);
				
	for( ArpBaseLayout* child = LayoutChildAt(0);
		 child != 0;
		 child = child->NextLayoutSibling() ) {
		 	
		// Retrieve dimensions and weight of this child.
		const ArpDimens& dim(child->LayoutDimens());
		float weightc;
		if( child->Constraints().FindFloat(WeightC, &weightc) != B_OK ) {
			weightc = 1;
		}
		bool alignc;
		if( child->Constraints().FindBool(AlignLabelsC, &alignc) != B_OK ) {
			alignc = true;
		}
		total_weight += weightc;

#if 0
		ArpDimens& curspread =
			PV_Orientation == B_HORIZONTAL ? curx : cury;
		ArpDimens& curalign =
			PV_Orientation == B_HORIZONTAL ? cury : curx;
#endif

		const ArpUniDimens& dimx(dim.X());
		const ArpUniDimens& dimy(dim.Y());
		
		// For horizontal orientation...
		if( PV_Orientation == B_HORIZONTAL ) {
		
			// The weight when our size is less than the preferred
			// size is the difference between all of the preferred
			// sizes and the minimum sizes.  This allows us to later
			// smoothly scale between the preferred size layout to
			// the minimum size layout.
			min_weight += dimx.PrefBody() - dimx.MinBody();
			
			// Add in label areas.  If this is the first child, its
			// pre-label is assigned to the container's pre-label;
			// if it is the last child, its post-label is assigned
			// to the container's post-label.  All other space is
			// placed in the body.
			float bodySpace = 0;
			if( !child->PreviousLayoutSibling() ) {
				curx.SetPreLabel(dimx.PreLabel());
			} else {
				bodySpace += dimx.PreLabel();
			}
			if( !child->NextLayoutSibling() ) {
				curx.SetPostLabel(dimx.PostLabel());
			} else {
				bodySpace += dimx.PostLabel();
			}
			
			// Add intra-child space if there is more than one.
			if( child->PreviousLayoutSibling() ) bodySpace += intra_s;
			
			// Add in collected body space.
			curx.AddBody(bodySpace);
			
			// Add in running body dimensions of child.
			curx.AddBody(dimx);
			
			// Add in alined dimensions of the child.  If not aligning
			// this child, update the total minimum size needed.
			if( alignc ) cury.SetMinDimens(dimy);
			else {
				min_size = std::max(min_size, dimy.TotalMin());
				pref_size = std::max(pref_size, dimy.TotalPref());
				max_size = std::max(max_size, dimy.TotalMax());
			}
			
		// For vertical dimension, similar to above...
		} else {
			min_weight += dimy.PrefBody() - dimy.MinBody();
			float bodySpace = 0;
			if( !child->PreviousLayoutSibling() ) {
				cury.SetPreLabel(dimy.PreLabel());
			} else {
				bodySpace += dimy.PreLabel();
			}
			if( !child->NextLayoutSibling() ) {
				cury.SetPostLabel(dimy.PostLabel());
			} else {
				bodySpace += dimy.PostLabel();
			}
			if( child->PreviousLayoutSibling() ) bodySpace += intra_s;
			cury.AddBody(bodySpace);
			cury.AddBody(dimy);
			if( alignc ) curx.SetMinDimens(dimx);
			else {
				min_size = std::max(min_size, dimx.TotalMin());
				pref_size = std::max(pref_size, dimx.TotalPref());
				max_size = std::max(max_size, dimx.TotalMax());
			}
		}
	}
	
	// Make sure our dimensions are large enough to contain any
	// unaligned children.
	if( PV_Orientation == B_HORIZONTAL ) {
		if( max_size > cury.TotalMax() ) {
			if( max_size >= ArpAnySize ) cury.SetMaxBody(ArpAnySize);
			else cury.SetMaxBody(cury.MaxBody()+max_size-cury.TotalMax());
		}
		if( pref_size > cury.TotalPref() ) {
			cury.SetPrefBody(cury.PrefBody()+pref_size-cury.TotalPref());
		}
		if( min_size > cury.TotalMin() ) {
			cury.SetMinBody(cury.MinBody()+min_size-cury.TotalMin());
		}
	} else {
		if( max_size > curx.TotalMax() ) {
			if( max_size >= ArpAnySize ) curx.SetMaxBody(ArpAnySize);
			else curx.SetMaxBody(curx.MaxBody()+max_size-curx.TotalMax());
		}
		if( pref_size > curx.TotalPref() ) {
			curx.SetPrefBody(curx.PrefBody()+pref_size-curx.TotalPref());
		}
		if( min_size > curx.TotalMin() ) {
			curx.SetMinBody(curx.MinBody()+min_size-curx.TotalMin());
		}
	}
	
	if( curx.TotalMax() <= 0 ) curx.SetTo(0, 30, 30, 30, 0);
	if( cury.TotalMax() <= 0 ) cury.SetTo(0, 30, 30, 30, 0);
	
	// Add in space around frame of our group.
	curx.AddLabel(inset_l,inset_r);
	cury.AddLabel(inset_t,inset_b);
}

// Scale a number <x> by a factor of <y>/<z>, approximating up.
#define APPROX_SCALE(x,y,z) floor( (z >= 1) ? \
					(((ceil(x))*(y))+((z)-1))/(z) : 0 )

void ArpRunningBar::LayoutView(void)
{
	// Retrieve current dimensions.
	BRect frm = LayoutBounds();
	BRect bdy = BodyBounds();
	const ArpDimens& cur_dimens(LayoutDimens());
	const ArpUniDimens& curx(cur_dimens.X());
	const ArpUniDimens& cury(cur_dimens.Y());
	
	// Initialize variables used to spread children across our space.
	float extra_spc;
	float leftwgt = total_weight;
	float left = frm.left, top = frm.top;
	float width = frm.Width()+1 - inset_l - inset_r;
	float height = frm.Height()+1 - inset_t - inset_b;

	// Find amount of pre-label and post-label space is used in
	// the spread direction.
	float prex=0, postx=0;
	float prey=0, posty=0;
	prex = std::max(curx.PreLabel(), bdy.left-frm.left) - inset_l;
	postx = std::max(curx.PostLabel(), frm.right-bdy.right) - inset_r;
	prey = std::max(cury.PreLabel(), bdy.top-frm.top) - inset_t;
	posty = std::max(cury.PostLabel(), frm.bottom-bdy.bottom) - inset_b;
#if 0
	if( PV_Orientation == B_HORIZONTAL ) {
		prex = std::max(curx.PreLabel(), bdy.left-frm.left);
		postx = std::max(curx.PostLabel(), frm.right-bdy.right);
		prey = cury.PreLabel();
		posty = cury.PostLabel();
	} else {
		prey = std::max(cury.PreLabel(), bdy.top-frm.top);
		posty = std::max(cury.PostLabel(), frm.bottom-bdy.bottom);
		prex = curx.PreLabel();
		postx = curx.PostLabel();
	}
#endif
	
	// Check if either direction is less than the preferred size.
	bool upw = true, uph = true;
	if( width < (curx.PrefBody()+prex+postx) ) upw = false;
	if( height < (cury.PrefBody()+prey+posty) ) uph = false;
	
	// Move by top/left insets.
	left += inset_l;
	top += inset_t;
	
	BRect cfrm;
	cfrm.left = left;
	cfrm.top = top;
	
	// Spreading across X dimension...
	if( PV_Orientation == B_HORIZONTAL ) {
		// If less then preferred size, our base is min_width and we
		// weight by the difference between preferred width and min width.
		if( !upw ) leftwgt = min_weight;
		// This is the amount of extra space we have to use -- either
		// additional over preferred width, or over the minimum width.
		extra_spc = width - (upw ? curx.PrefBody():curx.MinBody()) - prex - postx + inset_l + inset_r;
		// Place the bottom of all children.
		cfrm.bottom = top+height-1;
		
	// Spreading across Y dimension, similar to above...
	} else {
		if( !uph ) leftwgt = min_weight;
		extra_spc = height - (uph ? cury.PrefBody():cury.MinBody()) - prey - posty + inset_t + inset_b;
		cfrm.right = left+width-1;
	}
	
	ArpD(cdb << ADH << "ArpRunningBar::LayoutView() -- " << LayoutName()
				<< " = " << frm << std::endl);
	ArpD(cdb << ADH << "  width=" << width << ", height=" << height
				<< ", extra_spc=" << extra_spc
				<< ", weight=" << leftwgt << std::endl);

	for( ArpBaseLayout* child = LayoutChildAt(0);
		 child != 0;
		 child = child->NextLayoutSibling() ) {
		 	
		// Retrieve dimensions and constraints of this child.
		const ArpDimens& dim(child->LayoutDimens());
		const ArpUniDimens& dimx(dim.X());
		const ArpUniDimens& dimy(dim.Y());
		
		const bool first = child->PreviousLayoutSibling() ? false : true;
		const bool last = child->NextLayoutSibling() ? false : true;
		
		float thiswgt;
		if( child->Constraints().FindFloat(WeightC, &thiswgt) != B_OK ) {
			thiswgt = 1;
		}
		int32 thisfill;
		if( child->Constraints().FindInt32(FillC, &thisfill) != B_OK ) {
			thisfill = ArpCenter;
		}
		bool thisalign;
		if( child->Constraints().FindBool(AlignLabelsC, &thisalign) != B_OK ) {
			thisalign = true;
		}
		
		// If our spread axis is less than its preferred size, the
		// child's weight is instead the difference between its
		// minimum and preferred sizes.
		if( PV_Orientation == B_HORIZONTAL && !upw ) {
			thiswgt = dimx.PrefBody() - dimx.MinBody();
		} else if( PV_Orientation == B_VERTICAL && !uph ) {
			thiswgt = dimy.PrefBody() - dimy.MinBody();
		}
		
		// Compute the amount of additional space for this child.
		if( extra_spc < 0 ) extra_spc = 0;
		float spc = APPROX_SCALE(extra_spc,thiswgt,leftwgt);
		
		ArpD(cdb << ADH << "Positioning " << child->LayoutName()
					<< " left=" << left << ", top=" << top
					<< ", extra=" << spc << std::endl);
		
		// This is the body frame rectangle.
		BRect cbod;
		
		// Spreading across X so place left and right of rect...
		if( PV_Orientation == B_HORIZONTAL ) {
			// Currently at left position.
			cfrm.left = left;
			// Set right position to the base size (either min or
			// preferred), plus additional space computed above.
			cfrm.right = floor( left+spc-1 + 
						(upw ? dimx.PrefBody():dimx.MinBody()) );
			cfrm.right += first ? prex : dimx.PreLabel();
			cfrm.right += last ? postx : dimx.PostLabel();
			// Set left position for next child.
			left = cfrm.right+intra_s+1;
			
			// Set the top and bottom of the body rectangle.
			if( thisalign ) {
				cbod.top = cfrm.top + prey; //cury.PreLabel();
				cbod.bottom = cfrm.bottom - posty; //cury.PostLabel();
			} else {
				cbod.top = cfrm.top + dimy.PreLabel();
				cbod.bottom = cfrm.bottom - dimy.PostLabel();
			}
			
			// Set the left and right edge of body rectangle.
			if( thisalign ) {
				cbod.left = cfrm.left + (first ? prex : dimx.PreLabel());
				cbod.right = cfrm.right - (last ? postx : dimx.PostLabel());
			} else {
				cbod.left = cfrm.left + dimx.PreLabel();
				cbod.right = cfrm.right - dimx.PostLabel();
			}
			
		// Place top and bottom of rect, similarily to above...
		} else {
			cfrm.top = top;
			cfrm.bottom = floor( top+spc-1 + 
						(uph ? dimy.PrefBody():dimy.MinBody()) );
			cfrm.bottom += first ? prey : dimy.PreLabel();
			cfrm.bottom += last ? posty : dimy.PostLabel();
			top = cfrm.bottom+intra_s+1;
			if( thisalign ) {
				cbod.left = cfrm.left + prex; //curx.PreLabel();
				cbod.right = cfrm.right - postx; //curx.PostLabel();
			} else {
				cbod.left = cfrm.left + dimx.PreLabel();
				cbod.right = cfrm.right - dimx.PostLabel();
			}
			if( thisalign ) {
				cbod.top = cfrm.top + (first ? prey : dimy.PreLabel());
				cbod.bottom = cfrm.bottom - (last ? posty : dimy.PostLabel());
			} else {
				cbod.top = cfrm.top + dimy.PreLabel();
				cbod.bottom = cfrm.bottom - dimy.PostLabel();
			}
		}
		
		ArpD(cdb << ADH << "Child frame=" << cfrm
				<< ", body=" << cbod << std::endl);
				
		// Compute the final frame of the child, based on its
		// gravity constraint.  If there is more space assigned to
		// the child than it can use, this determines how it is
		// placed within that space.
		BRect ffrm(cfrm);
		const float outw = (!thisalign) // || PV_Orientation == B_HORIZONTAL)
						 ? floor((ffrm.right-ffrm.left+1)-dimx.TotalMax())
						 : floor((cbod.right-cbod.left+1)-dimx.MaxBody());
		if( outw > 0 ) {
			ArpD(cdb << ADH << "Filling outw=" << outw << ", init frame="
					<< ffrm << std::endl);
			ArpD(cdb << ADH << "Max width=" << dimx.TotalMax()
						<< ", fill mode=" << thisfill << std::endl);
			switch( thisfill&ArpEastWest ) {
				case ArpCenter: {
					const float leftw = floor(outw/2);
					ffrm.left += leftw;
					ffrm.right -= (outw-leftw);
					cbod.left += leftw;
					cbod.right -= (outw-leftw);
					break;
				}
				case ArpWest: {
					ffrm.right -= outw;
					cbod.right -= outw;
					break;
				}
				case ArpEast: {
					ffrm.left += outw;
					cbod.left += outw;
					break;
				}
			}
			ArpD(cdb << ADH << "Final frame=" << ffrm << std::endl);
		}
		const float outh = (!thisalign) // || PV_Orientation == B_VERTICAL)
						 ? floor((ffrm.bottom-ffrm.top+1)-dimy.TotalMax())
						 : floor((cbod.bottom-cbod.top+1)-dimy.MaxBody());
		if( outh > 0 ) {
			ArpD(cdb << ADH << "Filling outh=" << outh << ", init frame="
					<< ffrm << std::endl);
			ArpD(cdb << ADH << "Max height=" << dimy.TotalMax()
						<< ", fill mode=" << thisfill << std::endl);
			switch( thisfill&ArpNorthSouth ) {
				case ArpCenter: {
					const float toph = floor(outh/2);
					ffrm.top += toph;
					ffrm.bottom -= (outh-toph);
					cbod.top += toph;
					cbod.bottom -= (outh-toph);
					break;
				}
				case ArpNorth: {
					ffrm.bottom -= outh;
					cbod.bottom -= outh;
					break;
				}
				case ArpSouth: {
					ffrm.top += outh;
					cbod.top += outh;
					break;
				}
			}
			ArpD(cdb << ADH << "Final frame=" << ffrm << std::endl);
		}
		
		// Finally, actually place the child.
		child->SetViewLayout(ffrm, cbod);
		
		// And subtract the space and weight it used.
		extra_spc -= spc;
		leftwgt -= thiswgt;
	}
}
