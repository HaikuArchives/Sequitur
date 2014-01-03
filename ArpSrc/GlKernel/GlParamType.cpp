#include <app/Message.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamWrap.h>

static const BString16		gX("x");
static const BString16		gY("y");
static const BString16		gZ("z");

/*******************************************************
 * GL-PARAM-TYPE
 *******************************************************/
GlParamType::~GlParamType()
{
}

bool GlParamType::AcceptType(int32 type) const
{
	return type == Type();
}

int32 GlParamType::Key() const
{
	return mKey;
}

const BString16* GlParamType::Label() const
{
	return &mLabel;
}

void GlParamType::SetLabel(const BString16* label)
{
	mLabel.WinRelease();
	if (label) mLabel = *label;
}

void GlParamType::SetLabel(const char* label)
{
	mLabel.WinRelease();
	if (label) mLabel = label;
}

uint32 GlParamType::StateFlags() const
{
	return mStateFlags;
}

int32 GlParamType::Row() const
{
	return mRow;
}

status_t GlParamType::GetInit(GlParamWrap& wrap) const
{
	/* If this condition is being hit it's not a terribly
	 * big deal, still, off hand it doesn't seem like it should.
	 */
	ArpASSERT(false);
	return B_ERROR;
}

bool GlParamType::Matches(int32 type, int32 key) const
{
	return key == mKey && type == Type();
}

GlParamType::GlParamType(	int32 key, const BString16* label, int32 row,
							uint32 stateFlags)
		: mKey(key), mRow(row), mStateFlags(stateFlags)
{
	if (label) SetLabel(label);
}

GlParamType::GlParamType(	int32 key, const char* label, int32 row,
							uint32 stateFlags)
		: mKey(key), mRow(row), mStateFlags(stateFlags)
{
	if (label) SetLabel(label);
}

void GlParamType::Print() const
{
	char*		lbl = mLabel.AsAscii();
	printf("GlParamType %s (key %ld) (%p)\n", lbl, mKey, this);
	delete[] lbl;
}

// #pragma mark -

/***************************************************************************
 * GL-BOOL-PARAM-TYPE
 ****************************************************************************/
GlBoolParamType::GlBoolParamType(	int32 key, const BString16* label,
									bool inInit, int32 row)
		: inherited(key, label, row), mInit(inInit)
{
}

GlBoolParamType::GlBoolParamType(const GlBoolParamType& o)
		: inherited(o), mInit(o.mInit)
{
}

GlParam* GlBoolParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlBoolParam(this, id, mInit, config, StateFlags());
}

status_t GlBoolParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_BOOL_TYPE) return B_ERROR;
	((GlBoolWrap&)wrap).v = mInit;
	return B_OK;
}

bool GlBoolParamType::Init() const
{
	return mInit;
}

void GlBoolParamType::Print() const
{
	printf("GlBoolParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-COLOR-PARAM-TYPE
 ****************************************************************************/
GlColorParamType::GlColorParamType(	int32 key, const BString16* label,
									const ArpVoxel* init, int32 row)
		: inherited(key, label, row)
{
	if (init) mInit = *init;
}

GlColorParamType::GlColorParamType(const GlColorParamType& o)
		: inherited(o), mInit(o.mInit)
{
}

GlParam* GlColorParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlColorParam(this, id, mInit, config, StateFlags());
}

status_t GlColorParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_COLOR_TYPE) return B_ERROR;
	((GlColorWrap&)wrap).v = mInit;
	return B_OK;
}

ArpVoxel GlColorParamType::Init() const
{
	return mInit;
}

void GlColorParamType::Print() const
{
	printf("GlColorParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-FILE-NAME-PARAM-TYPE
 ****************************************************************************/
GlFileNameParamType::GlFileNameParamType(int32 key, const BString16* label, int32 row)
		: inherited(key, label, row)
{
}

GlFileNameParamType::GlFileNameParamType(const GlFileNameParamType& o)
		: inherited(o)
{
}

bool GlFileNameParamType::AcceptType(int32 type) const
{
	return type == GL_TEXT_TYPE;
}

GlParam* GlFileNameParamType::NewParam(gl_id id, const BMessage* config) const
{
	BString16		str;
	return new GlTextParam(this, id, str, config, StateFlags());
}

void GlFileNameParamType::Print() const
{
	printf("GlFileNameParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-FLOAT-PARAM-TYPE
 ****************************************************************************/
GlFloatParamType::GlFloatParamType(	int32 key, const BString16* label,
									float inMin, float inMax, float inInit,
									float inSteps, int32 row, uint32 stateFlags)
		: inherited(key, label, row, stateFlags), mMin(inMin), mMax(inMax),
		  mInit(inInit), mSteps(inSteps)
{
}

GlFloatParamType::GlFloatParamType(const GlFloatParamType& o)
		: inherited(o), mMin(o.mMin), mMax(o.mMax), mInit(o.mInit), mSteps(o.mSteps)
{
}

GlParam* GlFloatParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlFloatParam(this, id, mInit, config, StateFlags());
}

status_t GlFloatParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	((GlFloatWrap&)wrap).v = mInit;
	return B_OK;
}

float GlFloatParamType::Min() const
{
	return mMin;
}

float GlFloatParamType::Max() const
{
	return mMax;
}

float GlFloatParamType::Init() const
{
	return mInit;
}

float GlFloatParamType::Steps() const
{
	return mSteps;
}

void GlFloatParamType::Print() const
{
	printf("GlFloatParamType %s %ld (min %f max %f) (%p)\n", Label(), Key(), mMin, mMax, this);
}

// #pragma mark -

/***************************************************************************
 * GL-FONT-PARAM-TYPE
 ****************************************************************************/
GlFontParamType::GlFontParamType(	int32 key, const BString16* label,
									ArpFont* init, int32 row)
		: inherited(key, label, row)
{
	if (init) mInit = *init;
}

GlFontParamType::GlFontParamType(const GlFontParamType& o)
		: inherited(o), mInit(o.mInit)
{
}

GlParam* GlFontParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlFontParam(this, id, &mInit, config, StateFlags());
}

status_t GlFontParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FONT_TYPE) return B_ERROR;
	((GlFontWrap&)wrap).v = mInit;
	return B_OK;
}

const ArpFont* GlFontParamType::Init() const
{
	return &mInit;
}

void GlFontParamType::Print() const
{
	printf("GlFontParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-INT32-PARAM-TYPE
 ****************************************************************************/
GlInt32ParamType::GlInt32ParamType(	int32 key, const BString16* label,
									int32 inMin, int32 inMax, int32 inInit,
									int32 row, uint32 stateFlags)
		: inherited(key, label, row, stateFlags),
		  mMin(inMin), mMax(inMax), mInit(inInit)
{
}

GlInt32ParamType::GlInt32ParamType(const GlInt32ParamType& o)
		: inherited(o), mMin(o.mMin), mMax(o.mMax), mInit(o.mInit) 
{
}

GlParam* GlInt32ParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlInt32Param(this, id, mInit, config, StateFlags());
}

status_t GlInt32ParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_INT32_TYPE) return B_ERROR;
	((GlInt32Wrap&)wrap).v = mInit;
	return B_OK;
}

int32 GlInt32ParamType::Min() const
{
	return mMin;
}

int32 GlInt32ParamType::Max() const
{
	return mMax;
}

int32 GlInt32ParamType::Init() const
{
	return mInit;
}

void GlInt32ParamType::Print() const
{
	printf("GlInt32ParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-MENU-PARAM-TYPE
 ****************************************************************************/
GlMenuParamType::GlMenuParamType(	int32 key, const BString16* label,
									const BMessage& items, int32 inInit,
									int32 row)
		: inherited(key, label, row), mItems(items), mInit(inInit)
{
}

GlMenuParamType::GlMenuParamType(const GlMenuParamType& o)
		: inherited(o), mItems(o.mItems), mInit(o.mInit)
{
}

bool GlMenuParamType::AcceptType(int32 type) const
{
	return type == GL_INT32_TYPE;
}

GlParam* GlMenuParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlInt32Param(this, id, mInit, config, StateFlags());
}

status_t GlMenuParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_INT32_TYPE) return B_ERROR;
	((GlInt32Wrap&)wrap).v = mInit;
	return B_OK;
}

int32 GlMenuParamType::Init() const
{
	return mInit;
}

const BMessage& GlMenuParamType::Items() const
{
	return mItems;
}

int32 GlMenuParamType::ItemSize() const
{
	type_code		tc;
	int32			size;
	if (mItems.GetInfo("item", &tc, &size) != B_OK) return 0;
	ArpASSERT(tc == B_STRING_TYPE);
	return size;
}

int32 GlMenuParamType::Selection(int32 i) const
{
	type_code		tc;
	int32			itemSize, iSize;
	if (mItems.GetInfo("item", &tc, &itemSize) != B_OK) return i;
	ArpASSERT(tc == B_STRING_TYPE);
	if (mItems.GetInfo("i", &tc, &iSize) != B_OK) return i;
	ArpASSERT(tc == B_INT32_TYPE);
	ArpVALIDATE(i >= 0 && i < iSize, return i);
	int32			ans;
	if (mItems.FindInt32("i", i, &ans) != B_OK) return i;
	return ans;
}

void GlMenuParamType::Print() const
{
	printf("GlMenuParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-POINT-PARAM-TYPE
 ****************************************************************************/
GlPointParamType::GlPointParamType(	int32 key, const BString16* label,
									const BString16* xLabel, const BString16* yLabel,
									const BPoint& inMin, const BPoint& inMax,
									const BPoint& inInit, float inSteps,
									int32 row, uint32 stateFlags)
		: inherited(key, label, row, stateFlags),
		  mMin(inMin), mMax(inMax), mInit(inInit), mSteps(inSteps)
{
	if (xLabel) mXLabel = *xLabel;
	if (yLabel) mYLabel = *yLabel;
}

GlPointParamType::GlPointParamType(const GlPointParamType& o)
		: inherited(o), mXLabel(o.mXLabel), mYLabel(o.mYLabel),
		  mMin(o.mMin), mMax(o.mMax), mInit(o.mInit), mSteps(o.mSteps)
{
}

GlPointParamType::~GlPointParamType()
{
}

GlParam* GlPointParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlPointParam(this, id, mInit, config, StateFlags());
}

status_t GlPointParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_POINT_TYPE) return B_ERROR;
	((GlPointWrap&)wrap).v = mInit;
	return B_OK;
}

const BString16* GlPointParamType::XLabel() const
{
	if (mXLabel.WinString()) return &mXLabel;
	return &gX;
}

const BString16* GlPointParamType::YLabel() const
{
	if (mYLabel.WinString()) return &mYLabel;
	return &gY;
}

BPoint GlPointParamType::Min() const
{
	return mMin;
}

BPoint GlPointParamType::Max() const
{
	return mMax;
}

BPoint GlPointParamType::Init() const
{
	return mInit;
}

float GlPointParamType::Steps() const
{
	return mSteps;
}

void GlPointParamType::Print() const
{
	printf("GlPointParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-POINT-3D-PARAM-TYPE
 ****************************************************************************/
GlPoint3dParamType::GlPoint3dParamType(	int32 key, const BString16* label, const BString16* xLabel,
										const BString16* yLabel, const BString16* zLabel,
										const ArpPoint3d& inMin, const ArpPoint3d& inMax,
										const ArpPoint3d& inInit, float inSteps,
										int32 row, uint32 stateFlags)
		: inherited(key, label, row, stateFlags),
		  mMin(inMin), mMax(inMax), mInit(inInit), mSteps(inSteps)
{
	if (xLabel) mXLabel = *xLabel;
	if (yLabel) mYLabel = *yLabel;
	if (zLabel) mZLabel = *zLabel;
}

GlPoint3dParamType::GlPoint3dParamType(const GlPoint3dParamType& o)
		: inherited(o), mXLabel(o.mXLabel), mYLabel(o.mYLabel),
		  mZLabel(o.mZLabel), mMin(o.mMin), mMax(o.mMax), mInit(o.mInit),
		  mSteps(o.mSteps)
{
}

GlPoint3dParamType::~GlPoint3dParamType()
{
}

GlParam* GlPoint3dParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlPoint3dParam(this, id, mInit, config, StateFlags());
}

status_t GlPoint3dParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_POINT_3D_TYPE) return B_ERROR;
	((GlPoint3dWrap&)wrap).v = mInit;
	return B_OK;
}

const BString16* GlPoint3dParamType::XLabel() const
{
	if (mXLabel.WinString()) return &mXLabel;
	return &gX;
}

const BString16* GlPoint3dParamType::YLabel() const
{
	if (mYLabel.WinString()) return &mYLabel;
	return &gY;
}

const BString16* GlPoint3dParamType::ZLabel() const
{
	if (mZLabel.WinString()) return &mZLabel;
	return &gZ;
}

ArpPoint3d GlPoint3dParamType::Min() const
{
	return mMin;
}

ArpPoint3d GlPoint3dParamType::Max() const
{
	return mMax;
}

ArpPoint3d GlPoint3dParamType::Init() const
{
	return mInit;
}

float GlPoint3dParamType::Steps() const
{
	return mSteps;
}

void GlPoint3dParamType::Print() const
{
	printf("GlPoint3dParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-REL-ABS-PARAM-TYPE
 ****************************************************************************/
GlRelAbsParamType::GlRelAbsParamType(	int32 key, const BString16* label,
										const GlRelAbs& min, const GlRelAbs& max,
										const GlRelAbs& init, float steps,
										int32 row, uint32 stateFlags)
		: inherited(key, label, row, stateFlags),
		  mMin(min), mMax(max), mInit(init), mSteps(steps)
{
}

GlRelAbsParamType::GlRelAbsParamType(const GlRelAbsParamType& o)
		: inherited(o), mMin(o.mMin), mMax(o.mMax), mInit(o.mInit) 
{
}

GlParam* GlRelAbsParamType::NewParam(gl_id id, const BMessage* config) const
{
	return new GlRelAbsParam(this, id, mInit, config, StateFlags());
}

status_t GlRelAbsParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_REL_ABS_TYPE) return B_ERROR;
	((GlRelAbsWrap&)wrap).v = mInit;
	return B_OK;
}

GlRelAbs GlRelAbsParamType::Min() const
{
	return mMin;
}

GlRelAbs GlRelAbsParamType::Max() const
{
	return mMax;
}

GlRelAbs GlRelAbsParamType::Init() const
{
	return mInit;
}

float GlRelAbsParamType::Steps() const
{
	return mSteps;
}

void GlRelAbsParamType::Print() const
{
	printf("GlRelAbsParamType %s (%p)\n", Label(), this);
}

// #pragma mark -

/***************************************************************************
 * GL-TEXT-PARAM-TYPE
 ****************************************************************************/
GlTextParamType::GlTextParamType(	int32 key, const BString16* label, int32 row,
									uint32 stateFlags)
		: inherited(key, label, row, stateFlags), mInit(0)
{
}

GlTextParamType::GlTextParamType(	int32 key, const BString16* label,
									const char* inInit, int32 row,
									uint32 stateFlags)
		: inherited(key, label, row, stateFlags), mInit(inInit)
{
}

GlTextParamType::GlTextParamType(const GlTextParamType& o)
		: inherited(o)
{
}

GlParam* GlTextParamType::NewParam(gl_id id, const BMessage* config) const
{
	BString16		str;
	return new GlTextParam(this, id, str, config, StateFlags());
}

status_t GlTextParamType::GetInit(GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_TEXT_TYPE) return B_ERROR;
	if (mInit) ((GlTextWrap&)wrap).v = mInit;
	else ((GlTextWrap&)wrap).v = "";
	return B_OK;
}

const char* GlTextParamType::Init() const
{
	return mInit;
}

void GlTextParamType::Print() const
{
	printf("GlTextParamType %s (%p)\n", Label(), this);
}
