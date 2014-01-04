/* GlDefs.h
 * Global definitions for Glasslike.
 */
#ifndef GL_DEFS_H
#define GL_DEFS_H

#include <stdio.h>
#include <support/Errors.h>
#include <support/SupportDefs.h>
#include <ArpCore/String16.h>
#include <ArpInterface/ArpPrefs.h>
class GlStrainedParamList;
class GlParamTypeList;
class GlProcessStatus;

/* NOTE!  THE DEFS MUST BE INITIALIZED BEFORE USING CERTAIN FUNCTIONS!
 */
void		gl_init_defs();

/* Io() types
 */
enum {
	GL_1D_IN				= 0x00010000,
	GL_2D_IN				= 0x00020000,
	GL_IMAGE_IN				= 0x00040000,
	GL_TEXT_IN				= 0x00080000,
	GL_NUMBER_IN			= 0x00100000,

	GL_1D_OUT				= 0x00000001,
	GL_2D_OUT				= 0x00000002,
	GL_IMAGE_OUT			= 0x00000004,
	GL_TEXT_OUT				= 0x00000008,
	GL_NUMBER_OUT			= 0x00000010
};
#define GL_1D_IO			(GL_1D_IN		| GL_1D_OUT)
#define GL_2D_IO			(GL_2D_IN		| GL_2D_OUT)
#define GL_IMAGE_IO			(GL_IMAGE_IN	| GL_IMAGE_OUT)
#define GL_TEXT_IO			(GL_TEXT_IN		| GL_TEXT_OUT)
#define GL_NUMBER_IO		(GL_NUMBER_IN	| GL_NUMBER_OUT)

typedef		void*			gl_id;				// a universal ID
typedef		void*			gl_image_id;
typedef		void*			gl_node_add_on_id;
typedef		void*			gl_node_id;
typedef		void*			gl_chain_id;

extern const char*			GL_NODE_CREATOR_STR;
extern const char*			GL_NODE_KEY_STR;

/* Communication.
 */
enum {
	GL_STATUS_MSG				= 'Gsta',		// The status bar has changed
		// float "f"			0 - 1, how far along we are
};

/* Keys for the system nodes.
 */
extern const int32		GL_1D_IN_KEY;
extern const int32		GL_2D_IN_KEY;
extern const int32		GL_BINARY_NBR_KEY;
extern const int32		GL_CLOSE_PAREN_2D_KEY;
extern const int32		GL_CLOSE_PAREN_NBR_KEY;
extern const int32		GL_COMPOSITE_KEY;
extern const int32		GL_CONSTANT_KEY;
extern const int32		GL_ELLIPSE_KEY;
extern const int32		GL_ENVELOPE_KEY;
extern const int32		GL_GRID_KEY;
extern const int32		GL_IMAGE_IN_KEY;
extern const int32		GL_INVERT_KEY;
extern const int32		GL_LAYER_KEY;
extern const int32		GL_NEW_IMAGE_KEY;
extern const int32		GL_NUMBER_KEY;
extern const int32		GL_NUMBER_IN_KEY;
extern const int32		GL_OPEN_PAREN_2D_KEY;
extern const int32		GL_OPEN_PAREN_NBR_KEY;
extern const int32		GL_PINK_NOISE_KEY;
extern const int32		GL_PLACE_KEY;
extern const int32		GL_PROMOTE_KEY;
extern const int32		GL_RESIZE_KEY;
extern const int32		GL_SAWTOOTH_KEY;
extern const int32		GL_SINE_KEY;
extern const int32		GL_SQUARE_KEY;
extern const int32		GL_SUM_KEY;
extern const int32		GL_STRETCH_KEY;
extern const int32		GL_STAIR_KEY;
extern const int32		GL_TEXT_IN_KEY;
extern const int32		GL_TEXT_TO_IMAGE_KEY;
extern const int32		GL_TRIANGLE_KEY;

/* Keys for the system number algos.
 */
extern const int32		GL_NUMBER_INPUT_KEY;
extern const int32		GL_FEEDER_INPUT_KEY;

/* Some nodes are utilized by the UI, and so have params and
 * values that have been exposed.
 */
extern const int32		GL_ADD_PARAM_KEY;
extern const int32		GL_BG_RGBA;
extern const int32		GL_BG_Z;
extern const int32		GL_BINARY_OP_PARAM_KEY;
extern const int32		GL_BOTTOM_KEY;
extern const int32		GL_COMPOSITE_ALIGN_SOURCES_KEY;
#define	GL_COMPOSITE_MODE_REPLACE		'repl'
#define	GL_COMPOSITE_MODE_AVERAGE		'avg_'
extern const int32		GL_CYCLE_KEY;
extern const int32		GL_DIV_PARAM_KEY;
extern const int32		GL_FG_RGBA;
extern const int32		GL_FG_Z;
extern const int32		GL_HRZ_KEY;
extern const int32		GL_INDEX_PARAM_KEY;
extern const int32		GL_KEY_PARAM;
extern const int32		GL_LEFT_KEY;
extern const int32		GL_MAX_PARAM_KEY;
extern const int32		GL_MIN_PARAM_KEY;
extern const int32		GL_MODE_PARAM_KEY;
extern const int32		GL_MULT_PARAM_KEY;
extern const int32		GL_NAME_PARAM_KEY;
extern const int32		GL_PHASE_KEY;
extern const int32		GL_RIGHT_KEY;
extern const int32		GL_SUB_PARAM_KEY;
extern const int32		GL_TOP_KEY;
extern const int32		GL_VALUE_PARAM_KEY;
extern const int32		GL_VRT_KEY;

/* Coordinate manipulation
 */
#define GL_IN_BOUNDS(x, y, w, h)	((x) >= 0 && (x) < (w) && (y) >= 0 && (y) < (h))
#define GL_COORD_INC(x, y, w) \
	x++; \
	if (x >= w) { x = 0; y++; }

/* These are the possible GL_SUBTYPE_ANN values.
 * Obviously, all ARP-defined values start with A.
 * Except for no and unknown, I think all these
 * only apply to Element nodes -- at least, I can't
 * imagine how they would be valid otherwise.  But
 * for now, I'm enforcing that in the UI.
 * OK, OBSOLETE NOW
 */
enum {
	GL_NO_SUBTYPE			= 0,
	GL_UNKNOWN_SUBTYPE		= 'A000',
	GL_ELEMENT_LIST			= 'AElL',
	GL_ELEMENT				= 'AEl_',
	GL_GENERATOR			= 'AGe_',
	GL_GENERATOR_CORE		= 'AGeC',
	GL_TRANSFORMER_LIST		= 'ATrL',
	GL_TRANSFORMER			= 'ATr_',
	GL_TRANSFORMER_CORE		= 'ATrC',
	GL_2D_GEN_LIST			= 'A2GL'
};

enum {
	GL_NODE_IMAGE_X			= ARP_INT32_PREF_SIZE,
	GL_NODE_IMAGE_Y			= ARP_INT32_PREF_SIZE + 1,
	GL_NODE_CONTROL_X		= ARP_INT32_PREF_SIZE + 2,
	GL_NODE_CONTROL_Y		= ARP_INT32_PREF_SIZE + 3
};

/* Flow control for Actions (which are generally pluggable objects
 * used for traversing graphs).
 */
enum {
	GL_STOP_OPERATION		= 0,	// Completely halt the traversal
	GL_STOP_BRANCH			= 1,	// Don't investigage the current toNode's out links
	GL_CONTINUE				= 2		// Keep goin'
};


enum GlFillType {
	GL_FILL_NONE			= 'none',
	GL_FILL_BLACK			= 'blck',
	GL_FILL_COLORWHEEL		= 'clrw'
};

enum GlCompositeOp {
	GL_ACCUM_OP				= -1,
	GL_AVG_OP				= 1,	// Average the values together
	GL_MIN_OP				= 2,	// Take the min value
	GL_MAX_OP				= 3,	// Take the max value
	GL_ADD_OP				= 4,	// Add the values
	GL_SUB_OP				= 5,	// Subtract the values
	GL_MULT_OP				= 6,	// Multiply the values
	GL_DIV_OP				= 7		// Divide the values
};

class gl_param_key
{
public:
	gl_node_id		nid;
	int32			key, index;

	gl_param_key(gl_node_id n, int32 k, int32 i = 0);

	gl_param_key&	operator=(const gl_param_key& o);
	bool			operator==(const gl_param_key& o) const;
};

enum {
	GL_BOOL_TYPE			= 'bool',
	GL_COLOR_TYPE			= 'colr',
	GL_FILENAME_TYPE		= 'flnm',
	GL_FLOAT_TYPE			= 'flot',
	GL_FONT_TYPE			= 'font',
	GL_INT32_TYPE			= 'in32',
	GL_MENU_TYPE			= 'menu',
	GL_POINT_TYPE			= 'pt2d',
	GL_POINT_3D_TYPE		= 'pt3d',
	GL_REL_ABS_TYPE			= 'rlab',
	GL_TEXT_TYPE			= 'text'
};

struct gl_generate_args
{
	size_t					size;				// amount of data in structure
	uint32					flags;			// state flags

	gl_generate_args();
	gl_generate_args(const gl_generate_args& o);
	~gl_generate_args();
	gl_generate_args& operator=(const gl_generate_args& o);

private:
	bool operator==(const gl_generate_args& o) const;
	bool operator!=(const gl_generate_args& o) const;
};

/* gl_process_args flags
 */
enum {
	GL_NODE_ICON_F				= 0x00000001,	// Set when the processing is for the
												// node icon -- a hack, but most nodes
												// need different settings for large and
												// small images.
	GL_PREVIEW_F				= 0x00000002,	// Set when the processing the preview.
	FIRST_ITERATION_F			= 0x00000004,	// Set on the first and last iteration.
	LAST_ITERATION_F			= 0x00000008	// If not iterating, both are set.
};

class gl_process_args
{
public:
	size_t						size;			// amount of data in structure
	uint32						flags;			// state flags
	gl_node_id					stopNid;		// processing will stop after this node
	GlProcessStatus*			status;
	const GlStrainedParamList*	paramList;		// Some nodes provide a list of params
												// that will override someone's internals

	gl_process_args();
	gl_process_args(const gl_process_args& o);
	~gl_process_args();
	gl_process_args& operator=(const gl_process_args& o);

private:
	bool				operator==(const gl_process_args& o) const;
	bool				operator!=(const gl_process_args& o) const;
};

/* A table for turning uint8's into floats of 0 - 1, because it's done
 * frequently and it provides a performance benefit.  The gl_init_defs()
 * must be called before this is used.
 */
extern float			glTable256[256];

/* EXTERNAL STRINGS
 * Miscellaneous text that appears in the interface (i.e. localization
 * candidates).  Some text is only used in 1 place -- for example, a lot
 * of the node text, which technically is coming from an addon anyway.
 * For those cases, I'm keeping a running total of how many times I'm
 * using it, since I'm really not sure what to do about it.
 */
const BString16*	SZ(uint32 i);
enum {
	SZ_1D		= 0,
	SZ_2D,
	SZ_Add,
	SZ_Alpha,
	SZ_Amount,
	SZ_Angle,
	SZ_Attenuation,
	SZ_Average,
	SZ_Bevel,			// 1
	SZ_Blend,
	SZ_Blue,
	SZ_Blur,			// category
	SZ_Binary,			// 3
	SZ_Bottom,
	SZ_Category,
	SZ_Chain,
	SZ_Close,
	SZ_Cohesion,
	SZ_Color,			// category
	SZ_Combine,			// category
	SZ_Composite,		// 1
	SZ_Constant,		// 1
	SZ_Control,
	SZ_Creator,
	SZ_Curve,
	SZ_Cycle,
	SZ_Density,
	SZ_Depth,
	SZ_Destination_Mask,
	SZ_Diffusion,
	SZ_Direction,
	SZ_Distance,
	SZ_Distort,			// category
	SZ_Dot,
	SZ_Earthquake,		// 1
	SZ_Edge,			// 1
	SZ_Ellipse,			// 1
	SZ_End,
	SZ_Envelope,		// 1
	SZ_Feather,			// 1
	SZ_File,
	SZ_Filter,
	SZ_Flow,			// category
	SZ_Fluidity,
	SZ_Fractal,			// 1
	SZ_Frames,
	SZ_Freeze,			// 1
	SZ_Freeze_morphing,
	SZ_Freeze_random,
	SZ_from,
	SZ_Fulcrum,
	SZ_Grain,			// 1
	SZ_Green,
	SZ_Height,
	SZ_High,
	SZ_Horizontal,
	SZ_Hue,
	SZ_Image,
	SZ_Image_In,		// 1
	SZ_Images,			// category
	SZ_Invert,			// 2
	SZ_Iterator,		// 1
	SZ_Key,
	SZ_Label,
	SZ_Layer,
	SZ_Large,
	SZ_Left,
	SZ_Load_Image,		// 1
	SZ_Low,
	SZ_Map,
	SZ_Map_In,			// 1
	SZ_Mask,
	SZ_Max,
	SZ_median,			// 1
	SZ_Midi,
	SZ_Min,
	SZ_Mirror,			// 1
	SZ_Mode,
	SZ_New_Image,		// 1
	SZ_Number_In,		// 1
	SZ_Numbers,			// category
	SZ_of,
	SZ_Off,
	SZ_On,
	SZ_Open,
	SZ_Outline,			// 1
	SZ_Parameter,
	SZ_Phase,
	SZ_Pink_Noise,		// 1
	SZ_Pivot,			// 1
	SZ_Plates,
	SZ_Promote,			// 1
	SZ_Quality,
	SZ_Random,
	SZ_Recorders,		// category
	SZ_Red,
	SZ_Replicate,		// 2
	SZ_Resize,			// 1
	SZ_Resolution,
	SZ_Right,
	SZ_Root,
	SZ_Ruggedness,
	SZ_Saturation,
	SZ_Sawtooth,		// 1
	SZ_Scale,
	SZ_Scanner,			// 1
	SZ_Sequence,		// 1
	SZ_Shapes,			// category
	SZ_Sine,			// 1
	SZ_Size,
	SZ_Small,
	SZ_Smoothing,
	SZ_Source,
	SZ_Source_Image,
	SZ_Source_Mask,
	SZ_Specularity,
	SZ_Square,			// 1
	SZ_Stage,
	SZ_Stair,			// 1
	SZ_Start,
	SZ_Step,
	SZ_Steps,
	SZ_Stretch,			// 1
	SZ_Subtract,
	SZ_sum,				// 1
	SZ_Surface_In,		// 1
	SZ_Text_In,			// 1
	SZ_Tie,
	SZ_to,
	SZ_Top,
	SZ_Triangle,		// 1
	SZ_Value,
	SZ_Vertical,
	SZ_Warp,
	SZ_Width,
	SZ_Wrapper,			// 1, possibly internal
	SZ_Wraps,			// category
	SZ_x,
	SZ_X,
	SZ_y,
	SZ_Y,
	SZ_Z,
	// Probably tmp
	SZ_Abs,
	SZ_Abs_length,
	SZ_Abs_radius,
	SZ_Align_sources,
	SZ_BG,
	SZ_BG_depth,
	SZ_Detail,
	SZ_Erase_background,
	SZ_FG,
	SZ_FG_depth,
	SZ_H,
	SZ_LT,
	SZ_Maintain_origin,
	SZ_RB,
	SZ_Rel,
	SZ_Rel_length,
	SZ_Rel_radius,
	SZ_W,
	
	_GL_SZ_END
};

/* INTERNAL STRINGS
 * Miscellaneous text that persists, so can't be localized.
 */
extern const BString16*	SZI;
enum {
	SZI_arp			= 0,

	_GL_SZI_END
};

/* Translate oldW and oldH into something that fits into boundW
 * and boundH but maintains the same aspect ratio.
 */
void gl_scale_to_dimens(int32 oldW, int32 oldH, int32 boundW, int32 boundH,
						int32* outW, int32* outH);

#define GL_MIDI_OFF			(-1)
#define GL_MIDI_A			(0)
#define GL_MIDI_Z			(25)
#define GL_MIDI_SIZE		(26)

/* Answer a label for a MIDI number.  0 - 25 is A - Z.  -1 is off.
 */
const BString16*		gl_midi_label(int32 midi);
const char*		gl_midi_label_old(int32 midi);

/* INTERNAL STRINGS
 * Primarily for message passing -- the point is, the never appear
 * in the interface and wouldn't need to be localized.
 */
extern const char*		GL_NODE_CATEGORY_STR;
extern const char*		GL_NODE_LABEL_STR;
extern const char*		GL_NODE_ID_STR;
extern const char*		GL_CHAIN_ID_STR;
extern const char*		GL_SENDER_STR;

#endif
