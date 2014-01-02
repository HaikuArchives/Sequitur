#include <ArpCore/ArpChar.h>
#include "GlPublic/GlParam.h"
#include "GlPublic/GlParamType.h"

static const char*		VAL_STR			= "val";

/*******************************************************
 * GL-PARAM
 *******************************************************/
GlParam::GlParam(	const GlParamType* type, gl_id id,
					const BMessage* config, uint32 stateFlags)
		: mType(type), mOwner(id), mStateFlags(stateFlags)
{
}

GlParam::GlParam(const GlParam& o, gl_id id)
		: mType(o.mType), mOwner(id), mStateFlags(o.mStateFlags)
{
}

GlParam::~GlParam()
{
}

bool GlParam::IsValid() const
{
	if (!mType || !mOwner) return false;
	if (mType->Label() == 0) return false;
	return true;
}

gl_id GlParam::Owner() const
{
	return mOwner;
}

uint32 GlParam::StateFlags() const
{
	return mStateFlags;
}

const GlParamType* GlParam::ParamType() const
{
	return mType;
}

status_t GlParam::ReadFrom(const BMessage& msg)
{
	return B_OK;
}

status_t GlParam::WriteTo(BMessage& msg) const
{
	if (!mType) return B_ERROR;
	return B_OK;
}

void GlParam::Print() const
{
	printf("GlParam (someone should be overriding Print()\n");
}

// #pragma mark -

/***************************************************************************
 * GL-BOOL-PARAM
 ****************************************************************************/
GlBoolParam::GlBoolParam(	const GlBoolParamType* type, gl_node_id nid,
							bool inB, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), b(inB)
{
	if (config) ReadFrom(*config);
}

GlBoolParam::GlBoolParam(const GlBoolParam& o, gl_node_id nid)
		: inherited(o, nid), b(o.b)
{
}

GlParam* GlBoolParam::Clone(gl_node_id nid) const
{
	return new GlBoolParam(*this, nid);
}

status_t GlBoolParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	bool			arg;
	if (msg.FindBool(VAL_STR, &arg) == B_OK) b = arg;
	return B_OK;
}

status_t GlBoolParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	if ((err = msg.AddBool(VAL_STR, b)) != B_OK) return err;
	return B_OK;
}

void GlBoolParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlBoolParam %s: %d (nid %p)\n", label, b, Owner());
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-COLOR-PARAM
 ****************************************************************************/
GlColorParam::GlColorParam(	const GlColorParamType* type, gl_node_id nid,
							const ArpVoxel& inV, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), v(inV)
{
	if (config) ReadFrom(*config);
}

GlColorParam::GlColorParam(const GlColorParam& o, gl_node_id nid)
		: inherited(o, nid), v(o.v)
{
}

GlParam* GlColorParam::Clone(gl_node_id nid) const
{
	return new GlColorParam(*this, nid);
}

status_t GlColorParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	BMessage		cMsg;
	if (msg.FindMessage(VAL_STR, &cMsg) == B_OK) v.ReadFrom(cMsg);
	return B_OK;
}

status_t GlColorParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	BMessage		cMsg;
	if ((err = v.WriteTo(cMsg)) != B_OK) return err;
	if ((err = msg.AddMessage(VAL_STR, &cMsg)) != B_OK) return err;
	return B_OK;
}

void GlColorParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlColorParam %s: R %d G %d B %d A %d (nid %p)\n", label, v.r, v.g, v.b, v.a, Owner());
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-INT32-PARAM
 ****************************************************************************/
GlInt32Param::GlInt32Param(	const GlParamType* type, gl_node_id nid,
							int32 init, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), i32(init)
{
	if (config) ReadFrom(*config);
}

GlInt32Param::GlInt32Param(const GlInt32Param& o, gl_node_id nid)
		: inherited(o, nid), i32(o.i32)
{
}

GlParam* GlInt32Param::Clone(gl_node_id nid) const
{
	return new GlInt32Param(*this, nid);
}

status_t GlInt32Param::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	int32			v;
	if (msg.FindInt32(VAL_STR, &v) == B_OK) i32 = v;
	return B_OK;
}

status_t GlInt32Param::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	if ((err = msg.AddInt32(VAL_STR, i32)) != B_OK) return err;
	return B_OK;
}

void GlInt32Param::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlInt32Param %s: %ld (nid %p)\n", label, i32, Owner());
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-FLOAT-PARAM
 ****************************************************************************/
GlFloatParam::GlFloatParam(	const GlFloatParamType* type, gl_node_id nid,
							float inF, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), f(inF), mFlags(0)
{
	if (config) ReadFrom(*config);
}

GlFloatParam::GlFloatParam(const GlFloatParam& o, gl_node_id nid)
		: inherited(o, nid), f(o.f), mFlags(o.mFlags),
		  mMin(o.mMin), mMax(o.mMax)
{
}

GlParam* GlFloatParam::Clone(gl_node_id nid) const
{
	return new GlFloatParam(*this, nid);
}

status_t GlFloatParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	float			val;
	if (msg.FindFloat(VAL_STR, &val) == B_OK) f = val;
	return B_OK;
}

status_t GlFloatParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	if ((err = msg.AddFloat(VAL_STR, f)) != B_OK) return err;
	return B_OK;
}

status_t GlFloatParam::GetRange(float* min, float* max) const
{
	if (mFlags&HAS_RANGE_F) {
		if (min) *min = mMin;
		if (max) *max = mMax;
		return B_OK;
	} else {
		const GlParamType*	pt = ParamType();
		if (pt && pt->Type() == GL_FLOAT_TYPE) {
			const GlFloatParamType*	fpt = (GlFloatParamType*)pt;
			if (min) *min = fpt->Min();
			if (max) *max = fpt->Max();
			return B_OK;
		}
	}
	return B_ERROR;
}

void GlFloatParam::SetRange(float min, float max)
{
	mFlags |= HAS_RANGE_F;
	mMin = min;
	mMax = max;
}

void GlFloatParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlFloatParam %s: %f (nid %p me %p)\n", label, f, Owner(), this);
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-FONT-PARAM
 ****************************************************************************/
GlFontParam::GlFontParam(	const GlFontParamType* type, gl_node_id nid,
							const ArpFont* inF, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags)
{
	if (inF) f = *inF;
	if (config) ReadFrom(*config);
}

GlFontParam::GlFontParam(const GlFontParam& o, gl_node_id nid)
		: inherited(o, nid), f(o.f)
{
}

GlParam* GlFontParam::Clone(gl_node_id nid) const
{
	return new GlFontParam(*this, nid);
}

status_t GlFontParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	BMessage		fMsg;
	if (msg.FindMessage(VAL_STR, &fMsg) == B_OK) f.ReadFrom(fMsg);
	return B_OK;
}

status_t GlFontParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	BMessage		fMsg;
	if ((err = f.WriteTo(fMsg)) != B_OK) return err;
	if ((err = msg.AddMessage(VAL_STR, &fMsg)) != B_OK) return err;
	return B_OK;
}

void GlFontParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlFontParam %s: %ld pt %s (nid %p me %p)\n", label,
			int32(f.Size()), f.Family().String(), Owner(), this);
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-POINT-PARAM
 ****************************************************************************/
GlPointParam::GlPointParam(	const GlPointParamType* type, gl_node_id nid,
							const BPoint& inPt, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), pt(inPt)
{
	if (config) ReadFrom(*config);
}

GlPointParam::GlPointParam(const GlPointParam& o, gl_node_id nid)
		: inherited(o, nid), pt(o.pt)
{
}

GlParam* GlPointParam::Clone(gl_node_id nid) const
{
	return new GlPointParam(*this, nid);
}

status_t GlPointParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	BPoint			val;
	if (msg.FindPoint(VAL_STR, &val) == B_OK) pt = val;
	return B_OK;
}

status_t GlPointParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	if ((err = msg.AddPoint(VAL_STR, pt)) != B_OK) return err;
	return B_OK;
}

void GlPointParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlPointParam %s: (%f, %f) (nid %p)\n", label, pt.x, pt.y, Owner());
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-POINT-3D-PARAM
 ****************************************************************************/
GlPoint3dParam::GlPoint3dParam(	const GlPoint3dParamType* type, gl_node_id nid,
								const ArpPoint3d& inPt, const BMessage* config,
								uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), pt(inPt)
{
	if (config) ReadFrom(*config);
}

GlPoint3dParam::GlPoint3dParam(const GlPoint3dParam& o, gl_node_id nid)
		: inherited(o, nid), pt(o.pt)
{
}

GlParam* GlPoint3dParam::Clone(gl_node_id nid) const
{
	return new GlPoint3dParam(*this, nid);
}

status_t GlPoint3dParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	return pt.ReadFrom(msg, VAL_STR);
}

status_t GlPoint3dParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	return pt.WriteTo(msg, VAL_STR);
}

void GlPoint3dParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlPointParam %s: (%f, %f, %f) (nid %p)\n", label, pt.x, pt.y, pt.z, Owner());
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-REL-ABS-PARAM
 ****************************************************************************/
GlRelAbsParam::GlRelAbsParam(	const GlParamType* type, gl_node_id nid,
								const GlRelAbs& ra, const BMessage* config,
								uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), v(ra)
{
	if (config) ReadFrom(*config);
}

GlRelAbsParam::GlRelAbsParam(const GlRelAbsParam& o, gl_node_id nid)
		: inherited(o, nid), v(o.v)
{
}

GlParam* GlRelAbsParam::Clone(gl_node_id nid) const
{
	return new GlRelAbsParam(*this, nid);
}

status_t GlRelAbsParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	return v.ReadFrom(msg, VAL_STR);
}

status_t GlRelAbsParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	return v.WriteTo(msg, VAL_STR);
}

void GlRelAbsParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlRelAbsParam %s: rel %f abs %ld (nid %p me %p)\n", label,
			v.rel, v.abs, Owner(), this);
	delete[] label;
}

// #pragma mark -

/***************************************************************************
 * GL-TEXT-PARAM
 ****************************************************************************/
GlTextParam::GlTextParam(	const GlParamType* type, gl_node_id nid,
							const BString16& t, const BMessage* config,
							uint32 stateFlags)
		: inherited(type, nid, config, stateFlags), text(t)
{
	if (config) ReadFrom(*config);
}

GlTextParam::GlTextParam(const GlTextParam& o, gl_node_id nid)
		: inherited(o, nid), text(o.text)
{
}

GlParam* GlTextParam::Clone(gl_node_id nid) const
{
	return new GlTextParam(*this, nid);
}

status_t GlTextParam::ReadFrom(const BMessage& msg)
{
	status_t		err = inherited::ReadFrom(msg);
	if (err != B_OK) return err;
	const char*		str;
	if (msg.FindString(VAL_STR, &str) == B_OK) text = str;
	return B_OK;
}

status_t GlTextParam::WriteTo(BMessage& msg) const
{
	status_t		err = inherited::WriteTo(msg);
	if (err != B_OK) return err;
	if ((err = msg.AddString(VAL_STR, text)) != B_OK) return err;
	return B_OK;
}

void GlTextParam::Print() const
{
	const char*		label = NULL;
	if (ParamType() && ParamType()->Label()) label = ParamType()->Label()->AsAscii();
	printf("GlTextParam %s: '%s' (nid %p)\n", label, text.String(), Owner());
	delete[] label;
}
