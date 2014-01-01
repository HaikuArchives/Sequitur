#include "GlPublic/GlDefs.h"
#include "GlPublic/GlNodeData.h"
#include "GlPublic/GlAlgoNbr.h"
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlParamTypeList.h"
#include "GlKernel/GlDefs.h"

float					glTable256[256];

void gl_init_defs()
{
	ArpASSERT(GlAlgoNbr::RegisterKey(GL_FEEDER_INPUT_KEY));
	for (uint32 k = 0; k < 256; k++) glTable256[k] = k / 255.0f;
}

const char*		GL_NODE_CREATOR_STR		= "NC";
const char*		GL_NODE_KEY_STR			= "NK";
//extern const char*			GL_ARP_CREATOR;
//const char*		GL_ARP_CREATOR			= "arp";

//extern const BString16*		GL_ARP_CREATOR;
//static const BString16					gArp("arp");
//const BString16*	GL_ARP_CREATOR		= &gArp;

// Pattern 'ATxx' where A means A (for ARP), T can
// be m (map), s (surface), i (image), t (text) or n (number), p (plane),
// and xx is any two uint8 values.
const int32		GL_1D_IN_KEY			= '1in0';
const int32		GL_2D_IN_KEY			= '2in0';
const int32		GL_BINARY_NBR_KEY		= '#Bin';
const int32		GL_COMPOSITE_KEY		= 'ICm0';
const int32		GL_CONSTANT_KEY			= 'AmCo';
const int32		GL_CLOSE_PAREN_2D_KEY	= 'As)_';
const int32		GL_CLOSE_PAREN_NBR_KEY	= '#)__';
const int32		GL_ELLIPSE_KEY			= 'PEl0';
const int32		GL_ENVELOPE_KEY			= 'AmEn';
const int32		GL_GRID_KEY				= 'Grid';
const int32		GL_IMAGE_IN_KEY			= 'Iin0';
const int32		GL_INVERT_KEY			= 'PInv';
const int32		GL_LAYER_KEY			= 'AiLy';
const int32		GL_NEW_IMAGE_KEY		= 'NewI';
const int32		GL_NUMBER_KEY			= 'NNb0';
const int32		GL_NUMBER_IN_KEY		= '#in0';
const int32		GL_OPEN_PAREN_2D_KEY	= 'As(_';
const int32		GL_OPEN_PAREN_NBR_KEY	= '#(__';
const int32		GL_PINK_NOISE_KEY		= 'AmPi';
const int32		GL_PLACE_KEY			= 'IPl0';
const int32		GL_PROMOTE_KEY			= 'AsPr';
const int32		GL_RESIZE_KEY			= 'AiRs';
const int32		GL_SAWTOOTH_KEY			= 'AmSt';
const int32		GL_SINE_KEY				= 'AmSi';
const int32		GL_SQUARE_KEY			= 'AmSq';
const int32		GL_SUM_KEY				= '#sum';
const int32		GL_STAIR_KEY			= 'AmQn';
const int32		GL_STRETCH_KEY			= 'AiSt';
const int32		GL_TEXT_IN_KEY			= 'Tin0';
const int32		GL_TEXT_TO_IMAGE_KEY	= 'Tx2I';
const int32		GL_TRIANGLE_KEY			= 'AmTr';

const int32		GL_NUMBER_INPUT_KEY		= 'nI';
const int32		GL_FEEDER_INPUT_KEY		= 'fI';

const int32		GL_ADD_PARAM_KEY		= 'add_';
const int32		GL_BG_RGBA				= 'bg_c';
const int32		GL_BG_Z					= 'bg_z';
const int32		GL_BINARY_OP_PARAM_KEY	= 'op2_';
const int32		GL_BOTTOM_KEY			= 'bttm';
const int32		GL_COMPOSITE_ALIGN_SOURCES_KEY = 'algn';
const int32		GL_CYCLE_KEY			= 'cycl';
const int32		GL_DIV_PARAM_KEY		= 'div_';
const int32		GL_FG_RGBA				= 'fg_c';
const int32		GL_FG_Z					= 'fg_z';
const int32		GL_HRZ_KEY				= 'hrz_';
const int32		GL_KEY_PARAM			= 'key_';
const int32		GL_INDEX_PARAM_KEY		= 'indx';
const int32		GL_LEFT_KEY				= 'left';
const int32		GL_MAX_PARAM_KEY		= 'max_';
const int32		GL_MIN_PARAM_KEY		= 'min_';
const int32		GL_MODE_PARAM_KEY		= 'mode';
const int32		GL_MULT_PARAM_KEY		= 'mult';
const int32		GL_NAME_PARAM_KEY		= 'name';
const int32		GL_PHASE_KEY			= 'phas';
const int32		GL_RIGHT_KEY			= 'rght';
const int32		GL_SUB_PARAM_KEY		= 'sub_';
const int32		GL_TOP_KEY				= 'top_';
const int32		GL_VALUE_PARAM_KEY		= 'valu';
const int32		GL_VRT_KEY				= 'vrt_';

const BString16 gSzi[] = {
	"arp"
};

const BString16*	SZI = gSzi;

/***************************************************************************
 * gl_param_key
 ****************************************************************************/
gl_param_key::gl_param_key(gl_node_id n, int32 k, int32 i)
		: nid(n), key(k), index(i)
{
}

gl_param_key& gl_param_key::operator=(const gl_param_key& o)
{
	nid = o.nid;
	key = o.key;
	index = o.index;
	return *this;
}

bool gl_param_key::operator==(const gl_param_key& o) const
{
	return (nid == o.nid && key == o.key && index == o.index);
}

// #pragma mark -

/***************************************************************************
 * gl_process_args
 ****************************************************************************/
gl_process_args::gl_process_args()
		: size(sizeof(gl_process_args)),
		  flags(FIRST_ITERATION_F | LAST_ITERATION_F), stopNid(0),
		  status(0), paramList(0)
{
}

gl_process_args::gl_process_args(const gl_process_args& o)
		: size(sizeof(gl_process_args)),
		  flags(o.flags), stopNid(o.stopNid), status(o.status),
		  paramList(o.paramList)
{
}

gl_process_args::~gl_process_args()
{
}

gl_process_args& gl_process_args::operator=(const gl_process_args& o)
{
	size = sizeof(gl_process_args);
	flags = o.flags;
	stopNid = o.stopNid;
	status = o.status;
	paramList = o.paramList;
	return *this;
}

// #pragma mark -

/*************************************************************************
 * GL-GENERATE-ARGS
 *************************************************************************/

gl_generate_args::gl_generate_args()
	: size(sizeof(gl_generate_args)), flags(0)
{
}

gl_generate_args::gl_generate_args(const gl_generate_args& o)
	: size(sizeof(gl_generate_args)), flags(o.flags)
{
}

gl_generate_args::~gl_generate_args()
{
}

gl_generate_args& gl_generate_args::operator=(const gl_generate_args& o)
{
	if (this != &o) {
		size = sizeof(gl_generate_args);
		flags = o.flags;
	}
	return *this;
}

/***************************************************************************
 * Misc
 ****************************************************************************/
void gl_scale_to_dimens(int32 oldW, int32 oldH, int32 boundW, int32 boundH,
						int32* outW, int32* outH)
{
	float		s1W = boundW / float(oldW), s1H = boundH / float(oldH);
	if (s1H * oldW > boundW) {
		*outW = int32(s1W * oldW);
		*outH = int32(s1W * oldH);
	} else if (s1W * oldH > boundH) {
		*outW = int32(s1H * oldW);
		*outH = int32(s1H * oldH);
	} else {
		if (s1W > s1H) {
			*outW = int32(s1W * oldW);
			*outH = int32(s1W * oldH);
		} else {
			*outW = int32(s1H * oldW);
			*outH = int32(s1H * oldH);
		}
	}
	ArpASSERT(*outW <= boundW && *outH <= boundH);
	if (*outW > boundW) *outW = boundW;
	if (*outH > boundH) *outH = boundH;
}

static const BString16 gMidiLabels[] = {
		"A",	"B",	"C",	"D",	"E",	"F",	"G",	"H",
		"I",	"J",	"K",	"L",	"M",	"N",	"O",	"P",
		"Q",	"R",	"S",	"T",	"U",	"V",	"W",	"X",
		"Y",	"Z",	"Off",	"Unknown"
};

const BString16* gl_midi_label(int32 midi)
{
	if (midi < 0) return &gMidiLabels[26];
	if (midi > 25) return &gMidiLabels[27];
	return &gMidiLabels[midi];
}

static const char* gMidiLabelsOld[] = {
		"A",	"B",	"C",	"D",	"E",	"F",	"G",	"H",
		"I",	"J",	"K",	"L",	"M",	"N",	"O",	"P",
		"Q",	"R",	"S",	"T",	"U",	"V",	"W",	"X",
		"Y",	"Z"
};
static const char*		gMidiOffOld		= "Off";
static const char*		gMidiUnknownOld	= "Unknown";

const char* gl_midi_label_old(int32 midi)
{
	if (midi < 0) return gMidiOffOld;
	if (midi > 25) return gMidiUnknownOld;
	return gMidiLabelsOld[midi];
}

const char*		GL_NODE_CATEGORY_STR	= "CAT";
const char*		GL_NODE_LABEL_STR		= "LBL";
const char*		GL_NODE_ID_STR			= "nid";
const char*		GL_CHAIN_ID_STR			= "cid";
const char*		GL_SENDER_STR			= "sender";

/***************************************************************************
 * From GlKernel/GlDefs.h
 ****************************************************************************/
const int32		GL_ROOT_KEY						= 'Root';
const int32		GL_WRAPPER_KEY					= 'Wrap';

const int32		GL_PARAM_ROOT_CREATOR			= 'RCrt';
const int32		GL_PARAM_ROOT_KEY				= 'RKey';
const int32		GL_PARAM_ROOT_CATEGORY			= 'RCat';
const int32		GL_PARAM_ROOT_LABEL				= 'RLbl';

const int32		GL_UNSET_MIDI					= -9999;

const char*		GL_ROOT_CREATOR_STR				= "crt";
const char*		GL_ROOT_CATEGORY_STR			= "cat";
const char*		GL_ROOT_LABEL_STR				= "lbl";
const char*		GL_ROOT_KEY_STR					= "key";

const char*		GL_NI_STR						= "ni";
const char*		GL_NMSG_STR						= "nmsg";
const char*		GL_RH_STR						= "rh";

void gl_print_key(int32 key)
{
	printf("%c", uint8((key >> 24) &0xFF));
	printf("%c", uint8((key >> 16) &0xFF));
	printf("%c", uint8((key >> 8) &0xFF));
	printf("%c", uint8(key&0xFF));
}
