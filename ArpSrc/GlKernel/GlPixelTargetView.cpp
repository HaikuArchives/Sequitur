#include <be/interface/Window.h>
#include <ArpSupport/ArpUniversalStringMachine.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpPrefs.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlRootNode.h>
#include <GlKernel/GlPixelTargetView.h>

static const char*		RED_STR			= "Red";
static const char*		GREEN_STR		= "Green";
static const char*		BLUE_STR		= "Blue";
static const char*		ALPHA_STR		= "Alpha";
static const char*		DEPTH_STR		= "Depth";
static const char*		DIFFUSION_STR	= "Diffusion";
static const char*		SPECULARITY_STR	= "Specularity";
static const char*		DENSITY_STR		= "Density";
static const char*		COHESION_STR	= "Cohesion";
static const char*		FLUIDITY_STR	= "Fluidity";

static const uint32		CHANGE_MSG		= WM_USER + 1;

/***************************************************************************
 * GL-PIXEL-TARGET-VIEW
 ***************************************************************************/
GlPixelTargetView::GlPixelTargetView(	const BRect& frame,
										const GlRootRef& ref,
										const GlPlaneNode& node)
		: inherited(frame), mRef(ref), mNid(node.Id()),
		  mR(0), mG(0), mB(0), mA(0), mZ(0),
		  mDiff(0), mSpec(0), mD(0), mC(0), mF(0)
{
	float			padX = float(Prefs().GetInt32(ARP_PADX)),
					padY = float(Prefs().GetInt32(ARP_PADY));
	float			fh = ViewFontHeight();
	uint32			targets = node.PixelTargets();

	BString16		lab(node.AddOn()->Label());
	lab << " Targets";
	BRect			rect(padX, padY, 200, padY + fh);
	AddLabel(rect, "l", lab);

	/* Ugly convenience to deal with string issues.  Hopefully
	 * this gets cleaned up.
	 */
	ArpUniversalStringMachine	usm;

	/* ~Color~
	 */	
	rect.top = rect.bottom + padY;
	rect.bottom = rect.top + fh;
	AddLabel(rect, "l", "~Color~");
	ShiftCheckBoxDown(rect);
	mR = AddCheckBox(rect, "r", usm.String16(RED_STR), CHANGE_MSG, (targets&GL_PIXEL_R_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mG = AddCheckBox(rect, "g", usm.String16(GREEN_STR), CHANGE_MSG, (targets&GL_PIXEL_G_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mB = AddCheckBox(rect, "b", usm.String16(BLUE_STR), CHANGE_MSG, (targets&GL_PIXEL_B_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mA = AddCheckBox(rect, "a", usm.String16(ALPHA_STR), CHANGE_MSG, (targets&GL_PIXEL_A_MASK) != 0);

	/* ~Light~
	 */	
	rect.top = rect.bottom + padY;
	rect.bottom = rect.top + fh;
	AddLabel(rect, "l", "~Light~");
	ShiftCheckBoxDown(rect);
	mZ = AddCheckBox(rect, "z", usm.String16(DEPTH_STR), CHANGE_MSG, (targets&GL_PIXEL_Z_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mDiff = AddCheckBox(rect, "diff", usm.String16(DIFFUSION_STR), CHANGE_MSG, (targets&GL_PIXEL_DIFF_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mSpec = AddCheckBox(rect, "spec", usm.String16(SPECULARITY_STR), CHANGE_MSG, (targets&GL_PIXEL_SPEC_MASK) != 0);

	/* ~Material~
	 */	
	rect.top = rect.bottom + padY;
	rect.bottom = rect.top + fh;
	AddLabel(rect, "l", "~Material~");
	ShiftCheckBoxDown(rect);
	mD = AddCheckBox(rect, "d", usm.String16(DENSITY_STR), CHANGE_MSG, (targets&GL_PIXEL_D_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mC = AddCheckBox(rect, "c", usm.String16(COHESION_STR), CHANGE_MSG, (targets&GL_PIXEL_C_MASK) != 0);
	ShiftCheckBoxDown(rect);
	mF = AddCheckBox(rect, "f", usm.String16(FLUIDITY_STR), CHANGE_MSG, (targets&GL_PIXEL_F_MASK) != 0);
}

status_t GlPixelTargetView::ControlMessage(uint32 what)
{
	if (what != CHANGE_MSG) return B_ERROR;
	uint32			targets = MakeTargets();
	GlRootNode*		root = mRef.WriteLock();
	if (root) {
		GlNode*		n = root->FindNode(0, mNid);
		ArpASSERT(n);
		if (n) n->SetProperty(GL_PIXEL_TARGET_PROP, GlInt32Wrap(targets));
	}
	mRef.WriteUnlock(root);
	return B_OK;
}

uint32 GlPixelTargetView::MakeTargets() const
{
	uint32		targets = 0;
	if (mR && mR->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_R_MASK;
	if (mG && mG->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_G_MASK;
	if (mB && mB->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_B_MASK;
	if (mA && mA->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_A_MASK;
	if (mZ && mZ->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_Z_MASK;
	if (mDiff && mDiff->Value() == B_CONTROL_ON)	targets |= GL_PIXEL_DIFF_MASK;
	if (mSpec && mSpec->Value() == B_CONTROL_ON)	targets |= GL_PIXEL_SPEC_MASK;
	if (mD && mD->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_D_MASK;
	if (mC && mC->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_C_MASK;
	if (mF && mF->Value() == B_CONTROL_ON)			targets |= GL_PIXEL_F_MASK;
	return targets;
}
