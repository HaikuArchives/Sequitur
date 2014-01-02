//#include <AppKit.h>
#include <StorageKit.h>
#include <ArpKernel/ArpDebug.h>
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlRootNode.h"
#include "GlKernel/GlUserNodeAddOn.h"
#include <GlKernel/GlWrapperNode.h>
#include <Glasslike/GlApp.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlGlobalsImpl.h>
#include <Glasslike/SZ.h>

#include "GlNodes/GlBevelSrf.h"
#include <GlNodes/GlBinaryOp1d.h>
#include <GlNodes/GlBinaryOp2d.h>
#include <GlNodes/GlBinaryOpNbr.h>
#include <GlNodes/GlBlend1d.h>
#include "GlNodes/GlCompositeImg.h"
#include "GlNodes/GlConstant1d.h"
#include <GlNodes/GlConvolveSum.h>
#include "GlNodes/GlEarthquakeIm.h"
#include <GlNodes/GlEdge.h>
#include "GlNodes/GlEllipse.h"
#include "GlNodes/GlEnvelope1d.h"
#include "GlNodes/GlFeather.h"
#include "GlNodes/GlFractalMap.h"
#include "GlNodes/GlFreezeMap.h"
#include "GlNodes/GlGrainMap.h"
#include "GlNodes/GlImageNodes.h"
#include "GlNodes/GlInNodes.h"
#include <GlNodes/GlInvert.h>
#include <GlNodes/GlInvert1d.h>
#include <GlNodes/GlIteratorNodes.h>
#include <GlNodes/GlLayerIm.h>
#include <GlNodes/GlMedianNbr.h>
#include <GlNodes/GlMirror.h>
#include "GlNodes/GlOutlineSrf.h"
#include "GlNodes/GlParenNodes.h"
#include <GlNodes/GlPinkNoise1d.h>
#include "GlNodes/GlPivot1d.h"
#include "GlNodes/GlPromote.h"
#include "GlNodes/GlReplicate.h"
#include "GlNodes/GlReplicateMap.h"
#include "GlNodes/GlResizeIm.h"
#include "GlNodes/GlSawtooth1d.h"
#include "GlNodes/GlScaleMap.h"
#include "GlNodes/GlScanner.h"
#include "GlNodes/GlSequenceMap.h"
#include <GlNodes/GlSine1d.h>
#include "GlNodes/GlSquare1d.h"
#include "GlNodes/GlStairMap.h"
#include "GlNodes/GlStretchIm.h"
#include <GlNodes/GlSumNbr.h>
#include <GlNodes/GlTriangle1d.h>
#include <GlNodes/GlValue.h>

#if 0
#include "GlNodes/GlAppend.h"
#include "GlNodes/GlBilinearCoons.h"
#include "GlNodes/GlCellularAutomata.h"
#include "GlNodes/GlColorToText.h"
#include "GlNodes/GlDelay1d.h"
#include "GlNodes/GlDenoise.h"
#include "GlNodes/GlDisplace1d.h"
#include "GlNodes/GlEnvelope1d.h"
#include "GlNodes/GlFloodFill.h"
#include "GlNodes/GlFluidIm.h"
#include "GlNodes/GlImage2d.h"
#include "GlNodes/GlImgToNbr.h"
#include "GlNodes/GlIridescent.h"
#include "GlNodes/GlLine2dNodes.h"
#include "GlNodes/GlLocalLight.h"
#include "GlNodes/GlMakeGrowthPattern.h"
#include "GlNodes/GlMaxNmb.h"
#include "GlNodes/GlMinNmb.h"
#include "GlNodes/GlMossIm.h"
#include "GlNodes/GlMorphingIm.h"
#include "GlNodes/GlMulti1d.h"
#include "GlNodes/GlMulti2d.h"
#include "GlNodes/GlNewText.h"
#include "GlNodes/GlOrder.h"
#include "GlNodes/GlPassThrough.h"
#include "GlNodes/GlPerlinNoise1d.h"
#include "GlNodes/GlPlaceIm.h"
#include "GlNodes/GlPreviewers.h"
#include "GlNodes/GlRand1d.h"
#include "GlNodes/GlRect.h"
#include "GlNodes/GlRipple.h"
#include "GlNodes/GlSaveFile.h"
#include "GlNodes/GlScale1d.h"
#include "GlNodes/GlScript.h"
#include "GlNodes/GlSetColorIm.h"
#include "GlNodes/GlSpreadIm.h"
#include "GlNodes/GlStrokeIm.h"
#include "GlNodes/GlSunLight.h"
#include "GlNodes/GlTextToImage.h"
#include "GlNodes/GlTurbulence.h"
#include "GlNodes/GlVolume2d.h"
#include "GlNodes/GlWhirl.h"
/* The kernel nodes.
 */
#include "GlKernel/GlBinaryOpNbr.h"
#include "GlKernel/GlElementNode.h"
#include "GlKernel/GlInOutNodes.h"
#include "GlKernel/GlIteratorNode.h"
#include "GlKernel/GlListNode.h"
#include "GlKernel/GlMatrixNode.h"
#include "GlKernel/GlNumberNmb.h"		// Here 'cause everyone uses it so mucb
#include "GlKernel/GlUnaryOpNbr.h"
#include "GlKernel/GlSwitchNodes.h"
#include "GlKernel/GlWarpNodes.h"
#endif

//static const char*		USER_STR		= "User";

static status_t _get_app_path(BPath& appPath);

/**********************************************************************
 * _TOOL-BAR-INIT
 **********************************************************************/
class _ToolBarInit
{
public:
	_ToolBarInit();
	~_ToolBarInit();

	status_t	Install(GlNodeAddOn* addon, const BString16& creator,
						const BString16& category);

private:
	vector<GlToolBar*>	mBars;

	status_t	Install(GlNodeAddOn* addon, const BString16& creator,
						int32 type, const BString16* lbl);
};

/**********************************************************************
 * GL-GLOBALS-I
 **********************************************************************/
static GlGlobalsI* gGlobals = NULL;

GlGlobalsI& GlGlobals()
{
	if (!gGlobals) ArpASSERT(false);
	return *gGlobals;
}

void SetGlobals(GlGlobalsI& globals)
{
	if (gGlobals) ArpASSERT(false);
	gGlobals = &globals;
}

/***************************************************************************
 * GL-GLOBALS-IMPL
 ***************************************************************************/
GlGlobalsImpl::GlGlobalsImpl()
{
	SetGlobals(*this);
}

GlGlobalsImpl::~GlGlobalsImpl()
{
}

void GlGlobalsImpl::Initialize()
{
	InitializeParamTypes();
	InitializeAddOns();
}

status_t GlGlobalsImpl::GetAddOnInfo(	uint32 index, gl_node_add_on_id* outId,
										uint32* outIo,
										BString16* outCreator, int32* outKey,
										BString16* outCategory, BString16* outLabel,
										const ArpBitmap** outImage) const
{
	return mNodes.GetAddOnInfo(	index, outId, outIo, outCreator, outKey,
								outCategory, outLabel, outImage);
}

const GlNodeAddOn* GlGlobalsImpl::GetAddOn(gl_node_add_on_id id) const
{
	return mNodes.GetAddOn(id);
}

const GlNodeAddOn* GlGlobalsImpl::GetAddOn(	const BString16& creator,
											int32 key) const
{
	return mNodes.GetAddOn(creator, key);
}

GlNode* GlGlobalsImpl::NewNode(	const BString16& creator, int32 key,
								const BMessage* config) const
{
	const GlNodeAddOn*		addon = mNodes.GetAddOn(creator, key);
	if (!addon) return 0;
	return addon->NewInstance(config);
}

GlAlgo* GlGlobalsImpl::NewAlgo(const BString16& creator, int32 key) const
{
	const GlNodeAddOn*		addon = mNodes.GetAddOn(creator, key);
	if (!addon) return 0;
	gl_generate_args		args;
	return addon->Generate(args);
}

GlParamTypeList& GlGlobalsImpl::ParamTypes()
{
	return mParamTypes;
}

gl_image_id GlGlobalsImpl::AcquireImage(const BString16& filename)
{
	return mImagePool.Acquire(filename);
}

gl_image_id GlGlobalsImpl::AcquireImage(GlImage* image)
{
	return mImagePool.Acquire(image);
}

gl_image_id GlGlobalsImpl::AcquireImage(gl_image_id id)
{
	return mImagePool.Acquire(id);
}

status_t GlGlobalsImpl::ReleaseImage(gl_image_id id)
{
	return mImagePool.Release(id);
}

GlImage* GlGlobalsImpl::CloneImage(gl_image_id id)
{
	return mImagePool.Clone(id);
}

ArpBitmap* GlGlobalsImpl::CloneBitmap(gl_image_id id)
{
	return mImagePool.CloneBitmap(id);
}

const GlImage* GlGlobalsImpl::SourceImage(gl_image_id id)
{
	return mImagePool.Source(id);
}

// #pragma mark -

/***************************************************************************
 * Initializing
 ***************************************************************************/
void GlGlobalsImpl::InitializeParamTypes()
{
#if 0
	mParamTypes.Add(new GlInt32ParamType(GL_SUBTYPE_ANN, "Subtype", 0, LONG_MAX, GL_UNKNOWN_SUBTYPE));
	mParamTypes.Add(new GlPointParamType(GL_ORIGIN_ANN, "Origin x", "y", BPoint(-9999, -9999), BPoint(9999, 9999), BPoint(0, 0), 1));
	mParamTypes.Add(new GlPointParamType(GL_EXTENT_ANN, "Extent x", "y", BPoint(0, 0), BPoint(9999, 9999), BPoint(0, 0), 1));
#endif
}

void GlGlobalsImpl::InitializeAddOns()
{
	_ToolBarInit*		tb = new _ToolBarInit();
	
	Install(new GlRootNodeAddOn(), tb);
	Install(new GlWrapperNodeAddOn(), tb);

	Install(new GlBevelSrfAddOn(), tb);
	Install(new GlBinaryOp1dAddOn(), tb);
	Install(new GlBinaryOp2dAddOn(), tb);
	Install(new GlBinaryOpNbrAddOn(), tb);
	Install(new GlBlend1dAddOn(), tb);
	Install(new GlCompositeImgAddOn(), tb);
	Install(new GlConstant1dAddOn(), tb);
	Install(new GlConvolveSumAddOn(), tb);
	Install(new GlEarthquakeImAddOn(), tb);
	Install(new GlEdgeAddOn(), tb);
	Install(new GlEllipseAddOn(), tb);
	Install(new GlEnvelope1dAddOn(), tb);
	Install(new GlFeatherAddOn(), tb);
	Install(new GlFractalMapAddOn(), tb);
	Install(new GlFreezeMapAddOn(), tb);
	Install(new GlGrainMapAddOn(), tb);
	// GlImageNodes.h ...
	Install(new GlLoadImageAddOn(), tb);
	Install(new GlNewImageAddOn(), tb);
	// ... end GlImageNodes.h
	// GlInNodes.h ...
//	Install(new GlInAddOn(GL_1D_IO), tb);
//	Install(new GlInAddOn(GL_2D_IO), tb);
	Install(new GlInAddOn(GL_IMAGE_IO), tb);
//	Install(new GlInAddOn(GL_TEXT_IO), tb);
	Install(new GlInNbrAddOn(), tb);
	// ... end GlInNodes.h
	Install(new GlInvertAddOn(), tb);
	Install(new GlInvert1dAddOn(), tb);
	Install(new GlIteratorImAddOn(), tb);
	Install(new GlLayerImAddOn(), tb);
	Install(new GlMedianNbrAddOn(), tb);
	Install(new GlMirrorAddOn(), tb);
	Install(new GlOutlineSrfAddOn(), tb);
	// GlParenNodes.h ...
	Install(new GlParenSrfAddOn(GL_OPEN_PAREN_2D_KEY, SZ(SZ_Open), GL_OPEN_PAREN), tb);
	Install(new GlParenSrfAddOn(GL_CLOSE_PAREN_2D_KEY, SZ(SZ_Close), GL_CLOSE_PAREN), tb);
	Install(new GlParenNbrAddOn(GL_OPEN_PAREN_NBR_KEY, SZ(SZ_Open), GL_OPEN_PAREN), tb);
	Install(new GlParenNbrAddOn(GL_CLOSE_PAREN_NBR_KEY, SZ(SZ_Close), GL_CLOSE_PAREN), tb);
	// ... end GlParenNodes.h
	Install(new GlPinkNoise1dAddOn(), tb);
	Install(new GlPivot1dAddOn(), tb);
	Install(new GlPromoteAddOn(), tb);
	Install(new GlReplicateAddOn(), tb);
	Install(new GlReplicateMapAddOn(), tb);
	Install(new GlResizeImAddOn(), tb);
	Install(new GlSawtooth1dAddOn(), tb);
	Install(new GlScaleMapAddOn(), tb);
	Install(new GlScannerAddOn(), tb);
	Install(new GlSequenceMapAddOn(), tb);
	Install(new GlSine1dAddOn(), tb);
	Install(new GlSquare1dAddOn(), tb);
	Install(new GlStairMapAddOn(), tb);
	Install(new GlStretchImAddOn(), tb);
	Install(new GlSumNbrAddOn(), tb);
	Install(new GlTriangle1dAddOn(), tb);
	Install(new GlValueAddOn(), tb);

	/* Get all the nodes stored under my directory.
	 */
	BPath				appPath;
	if (_get_app_path(appPath) != B_OK) return;

	InitializeUserNodes(appPath, tb);


#if 0
	mNodeAddOns.Install(new GlAppendAddOn());
	mNodeAddOns.Install(new GlBilinearCoonsAddOn());
	mNodeAddOns.Install(new GlCellularAutomataAddOn());
	mNodeAddOns.Install(new GlColorToTextAddOn());
	mNodeAddOns.Install(new GlCompositeAddOn());
	mNodeAddOns.Install(new GlDelay1dAddOn());
	mNodeAddOns.Install(new GlDenoiseAddOn());
	mNodeAddOns.Install(new GlDisplace1dAddOn());
	mNodeAddOns.Install(new GlFloodFillAddOn());
	mNodeAddOns.Install(new GlFluidImAddOn());
	mNodeAddOns.Install(new GlImage2dAddOn());
	mNodeAddOns.Install(new GlImgToNbrAddOn());
	mNodeAddOns.Install(new GlIridescentAddOn());
	mNodeAddOns.Install(new GlLine2dAddOn());
	mNodeAddOns.Install(new GlLoadImageAddOn());
	mNodeAddOns.Install(new GlLocalLightAddOn());
	mNodeAddOns.Install(new GlMakeGrowthPatternAddOn());
	mNodeAddOns.Install(new GlMaxNmbAddOn());
	mNodeAddOns.Install(new GlMinNmbAddOn());
	mNodeAddOns.Install(new GlMorphingImAddOn());
	mNodeAddOns.Install(new GlMossImAddOn());
	mNodeAddOns.Install(new GlMulti1dAddOn());
	mNodeAddOns.Install(new GlMulti2dAddOn());
	mNodeAddOns.Install(new GlNewImageAddOn());
	mNodeAddOns.Install(new GlNewTextAddOn());
	mNodeAddOns.Install(new GlNumberNmbAddOn());
	mNodeAddOns.Install(new GlOrderAddOn());
	mNodeAddOns.Install(new GlPassThroughAddOn());
	mNodeAddOns.Install(new GlPerlinNoise1dAddOn());
	mNodeAddOns.Install(new GlPlaceImAddOn());
	mNodeAddOns.Install(new GlPreview2dAddOn());
	mNodeAddOns.Install(new GlRand1dAddOn());
	mNodeAddOns.Install(new GlRectAddOn());
	mNodeAddOns.Install(new GlRippleAddOn());
	mNodeAddOns.Install(new GlSaveFileAddOn());
	mNodeAddOns.Install(new GlScratch2dAddOn());
	mNodeAddOns.Install(new GlScriptAddOn());
	mNodeAddOns.Install(new GlSetColorImAddOn());
	mNodeAddOns.Install(new GlSpreadImAddOn());
	mNodeAddOns.Install(new GlStrokeImAddOn());
	mNodeAddOns.Install(new GlSunLightAddOn());
	mNodeAddOns.Install(new GlTextToImageAddOn());
	mNodeAddOns.Install(new GlTurbulenceAddOn());
	mNodeAddOns.Install(new GlUnaryOpNbrAddOn(GL_COS_UNARY_OP_KEY));
	mNodeAddOns.Install(new GlUnaryOpNbrAddOn(GL_SIN_UNARY_OP_KEY));
	mNodeAddOns.Install(new GlVolume2dAddOn());
	mNodeAddOns.Install(new GlWhirlAddOn());
	/* The IO nodes
	 */
	mNodeAddOns.Install(new GlInAddOn(	GL_1D_IO,		GL_1D_IN_KEY,		GL_1D_IN_NODE,		"1D In"));
	mNodeAddOns.Install(new GlOutAddOn(	GL_1D_IO,		GL_1D_OUT_KEY,		GL_1D_OUT_NODE,		"1D Out"));
	mNodeAddOns.Install(new GlInAddOn(	GL_2D_IO,		GL_2D_IN_KEY,		GL_2D_IN_NODE,		"2D In"));
	mNodeAddOns.Install(new GlOutAddOn(	GL_2D_IO,		GL_2D_OUT_KEY,		GL_2D_OUT_NODE,		"2D Out"));
	mNodeAddOns.Install(new GlInAddOn(	GL_IMAGE_IO,	GL_IMAGE_IN_KEY,	GL_IMAGE_IN_NODE,	"Image In"));
	mNodeAddOns.Install(new GlOutAddOn(	GL_IMAGE_IO,	GL_IMAGE_OUT_KEY,	GL_IMAGE_OUT_NODE,	"Image Out"));
	mNodeAddOns.Install(new GlInAddOn(	GL_NUMBER_IO,	GL_NUMBER_IN_KEY,	GL_NUMBER_IN_NODE,	"Number In"));
	mNodeAddOns.Install(new GlOutAddOn(	GL_NUMBER_IO,	GL_NUMBER_OUT_KEY,	GL_NUMBER_OUT_NODE,	"Number Out"));
	mNodeAddOns.Install(new GlInAddOn(	GL_TEXT_IO, 	GL_TEXT_IN_KEY,		GL_TEXT_IN_NODE,	"Text In"));
	mNodeAddOns.Install(new GlOutAddOn(	GL_TEXT_IO, 	GL_TEXT_OUT_KEY,	GL_TEXT_OUT_NODE,	"Text Out"));
	/* The switch nodes
	 */
	mNodeAddOns.Install(new GlSwitchAddOn(	GL_1D_IO,		GL_SWITCH_1D_KEY,		"sw1d",		"Switch 1D"));
	mNodeAddOns.Install(new GlSwitchAddOn(	GL_2D_IO,		GL_SWITCH_2D_KEY,		"sw2d",		"Switch 2D"));
	mNodeAddOns.Install(new GlSwitchAddOn(	GL_IMAGE_IO,	GL_SWITCH_IMAGE_KEY,	"swImg",	"Switch Image"));
	mNodeAddOns.Install(new GlSwitchAddOn(	GL_NUMBER_IO,	GL_SWITCH_NUMBER_KEY,	"swNbr",	"Switch Number"));
	mNodeAddOns.Install(new GlSwitchAddOn(	GL_TEXT_IO,		GL_SWITCH_TEXT_KEY,		"swTxt",	"Switch Text"));
	/* The warp nodes
	 */
	mNodeAddOns.Install(new GlWarpInAddOn(	GL_1D_IO,		GL_WARP_1D_IN_KEY,		"w1din",	"Warp 1D In"));
	mNodeAddOns.Install(new GlWarpOutAddOn(	GL_1D_IO,		GL_WARP_1D_OUT_KEY,		"w1dout",	"Warp 1D Out"));
	mNodeAddOns.Install(new GlWarpInAddOn(	GL_2D_IO,		GL_WARP_2D_IN_KEY,		"w2din",	"Warp 2D In"));
	mNodeAddOns.Install(new GlWarpOutAddOn(	GL_2D_IO,		GL_WARP_2D_OUT_KEY,		"w2dout",	"Warp 2D Out"));
	mNodeAddOns.Install(new GlWarpInAddOn(	GL_IMAGE_IO,	GL_WARP_IMAGE_IN_KEY,	"2imgin",	"Warp Image In"));
	mNodeAddOns.Install(new GlWarpOutAddOn(	GL_IMAGE_IO,	GL_WARP_IMAGE_OUT_KEY,	"wimgout",	"Warp Image Out"));
	mNodeAddOns.Install(new GlWarpInAddOn(	GL_NUMBER_IO,	GL_WARP_NUMBER_IN_KEY,	"w#in",		"Warp Number In"));
	mNodeAddOns.Install(new GlWarpOutAddOn(	GL_NUMBER_IO,	GL_WARP_NUMBER_OUT_KEY,	"w#out",	"Warp Number Out"));
	mNodeAddOns.Install(new GlWarpInAddOn(	GL_TEXT_IO, 	GL_WARP_TEXT_IN_KEY,	"wtxtin",	"Warp Text In"));
	mNodeAddOns.Install(new GlWarpOutAddOn(	GL_TEXT_IO, 	GL_WARP_TEXT_OUT_KEY,	"wtxtout",	"Warp Text Out"));
	/* The kernel nodes.
	 */
	mNodeAddOns.Install(new GlElementNodeAddOn());
	mNodeAddOns.Install(new GlMatrixNodeAddOn());
	mNodeAddOns.Install(new GlIteratorNodeAddOn());
	mNodeAddOns.Install(new GlListNodeAddOn());
#endif

	delete tb;
}

status_t GlGlobalsImpl::Install(GlNodeAddOn* addon, _ToolBarInit* tb)
{
	ArpVALIDATE(addon, return B_ERROR);
	status_t		err = mNodes.Install(addon);
	if (err != B_OK) {
		delete addon;
		return err;
	}
	if (!tb) return B_OK;
	return tb->Install(addon, addon->mCreator, addon->mCategory);
}

status_t GlGlobalsImpl::InitializeUserNodes(BPath path, _ToolBarInit* tb)
{
	status_t			err;
	if ((err = path.Append("UserNodes")) != B_OK) return err;
	BDirectory			dir(path.Path());
	if ((err = dir.InitCheck()) != B_OK) return err;
	
	BEntry			entry;
	while (dir.GetNextEntry(&entry) == B_OK){
		BMessage	groupMsg;
		BFile		file(&entry, B_READ_ONLY);
		if (file.InitCheck() == B_OK && groupMsg.Unflatten(&file) == B_OK) {
			BMessage		msg;
			int32			msgIndex = 0;
			while (groupMsg.FindMessage(GL_ROOT_STR, msgIndex, &msg) == B_OK) {
				AddUserNode(msg, tb);
				msg.MakeEmpty();
				msgIndex++;
			}
		}
	}
	return B_OK;
}

status_t GlGlobalsImpl::AddUserNode(const BMessage& msg, _ToolBarInit* tb)
{
	GlUserNodeAddOn*	addon = new GlUserNodeAddOn(msg);
	if (!addon && addon->InitCheck() != B_OK) {
		delete addon;
		return B_NO_MEMORY;
	}
	return Install(addon, tb);
}

// #pragma mark -

/***************************************************************************
 * Miscellaneous functions
 ***************************************************************************/
static status_t _get_app_path(BPath& appPath)
{
ArpPOLISH("Not there yet");
// OK, where to store settings files is a great big platform
// dependent piece, and I have no idea how it's done in win.
return B_ERROR;
/*
	if (!be_app) return B_ERROR;
	app_info		ai;
	status_t		err = be_app->GetAppInfo(&ai);
	if (err != B_OK) return err;
	BEntry			entry(&ai.ref, true);
	if (entry.InitCheck() != B_OK) return B_ERROR;
	if ((err = entry.GetPath(&appPath)) != B_OK) return err;
	return appPath.GetParent(&appPath);
*/
}

// #pragma mark -

/**********************************************************************
 * _TOOL-BAR-INIT
 **********************************************************************/
_ToolBarInit::_ToolBarInit()
{
	/* Just a debugging aid -- this step must always be done before
	 * the bars are read in, so make sure they're empty.
	 */
	ArpASSERT(gl_app->ToolBars().Size() == 0);
}

_ToolBarInit::~_ToolBarInit()
{
	/* Install all my bars on the tool bar roster.
	 */
	if (gl_app) {
		GlToolBarRoster&		r = gl_app->ToolBars();
		for (uint32 k = 0; k < mBars.size(); k++) r.AddToolBar(mBars[k]);
	}
	
	for (uint32 k = 0; k < mBars.size(); k++) mBars[k]->DecRefs();
//	gl_app->ToolBars().Print();
}

status_t _ToolBarInit::Install(	GlNodeAddOn* addon, const BString16& creator,
								const BString16& category)
{
	const BString16*		cat = SZ(SZ_User);
	if (category.Length() > 0) *cat = category.String();

	status_t		err = B_OK;

	if (addon->Io()&GL_1D_IO) err = Install(addon, creator, GL_1D_IO, cat);
	if (err != B_OK) return err;

	if (addon->Io()&GL_2D_IO) err = Install(addon, creator, GL_2D_IO, cat);
	if (err != B_OK) return err;

	if (addon->Io()&GL_IMAGE_IO) err = Install(addon, creator, GL_IMAGE_IO, cat);
	if (err != B_OK) return err;

	if (addon->Io()&GL_TEXT_IO) err = Install(addon, creator, GL_TEXT_IO, cat);
	if (err != B_OK) return err;

	if (addon->Io()&GL_NUMBER_IO) err = Install(addon, creator, GL_NUMBER_IO, cat);
	if (err != B_OK) return err;

	return B_OK;
}


status_t _ToolBarInit::Install(	GlNodeAddOn* addon, const BString16& creator,
								int32 type, const BString16* lbl)
{
	GlToolBar*		bar = 0;
	for (uint32 k = 0; k < mBars.size(); k++) {
		ArpASSERT(mBars[k]);
		if (mBars[k]->Matches(type, lbl)) {
			bar = mBars[k];
			break;
		}
	}

	if (!bar) {
		bar = new GlToolBar(type, lbl);
		if (!bar) return B_NO_MEMORY;
		bar->IncRefs();
		mBars.push_back(bar);
	}
	
	return bar->AddTool(creator, addon->Key());
}
