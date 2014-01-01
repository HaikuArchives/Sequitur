#include "GlPublic/GlParam.h"
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlParamWrap.h"

/***************************************************************************
 * GL-BOOL-WRAP
 ****************************************************************************/
GlBoolWrap::GlBoolWrap(bool in)
		: v(in)
{
}

GlBoolWrap::~GlBoolWrap()
{
}
	
status_t GlBoolWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlBoolParam*)param)->b;
	return B_OK;
}

void GlBoolWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlBoolParam*)param)->b = v;
}

status_t GlBoolWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return msg.AddBool(name, v);
}

// #pragma mark -

/***************************************************************************
 * GL-COLOR-WRAP
 ****************************************************************************/
GlColorWrap::GlColorWrap(const ArpVoxel& in)
		: v(in)
{
}

GlColorWrap::GlColorWrap(uint8 r, uint8 g, uint8 b, uint8 a, uint8 z)
		: v(r, g, b, a, z)
{
}

GlColorWrap::~GlColorWrap()
{
}
	
status_t GlColorWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlColorParam*)param)->v;
	return B_OK;
}

void GlColorWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlColorParam*)param)->v = v;
}

status_t GlColorWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	ArpASSERT(false);
	return B_ERROR;
}

// #pragma mark -

/***************************************************************************
 * GL-FLOAT-WRAP
 ****************************************************************************/
GlFloatWrap::GlFloatWrap(float in)
		: v(in)
{
}

GlFloatWrap::~GlFloatWrap()
{
}
	
status_t GlFloatWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlFloatParam*)param)->f;
	return B_OK;
}

void GlFloatWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlFloatParam*)param)->f = v;
}

status_t GlFloatWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return msg.AddFloat(name, v);
}

// #pragma mark -

/***************************************************************************
 * GL-FONT-WRAP
 ****************************************************************************/
GlFontWrap::GlFontWrap(const ArpFont* in)
{
	if (in) v = *in;
}

GlFontWrap::~GlFontWrap()
{
}
	
status_t GlFontWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlFontParam*)param)->f;
	return B_OK;
}

void GlFontWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlFontParam*)param)->f = v;
}

status_t GlFontWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	BMessage		fMsg;
	status_t		err;
	if ((err = v.WriteTo(fMsg)) != B_OK) return err;
	return msg.AddMessage(name, &fMsg);
}

// #pragma mark -

/***************************************************************************
 * GL-INT32-WRAP
 ****************************************************************************/
GlInt32Wrap::GlInt32Wrap(int32 in)
		: v(in)
{
}

GlInt32Wrap::~GlInt32Wrap()
{
}
	
status_t GlInt32Wrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlInt32Param*)param)->i32;
	return B_OK;
}

void GlInt32Wrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlInt32Param*)param)->i32 = v;
}

status_t GlInt32Wrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return msg.AddInt32(name, v);
}

// #pragma mark -

/***************************************************************************
 * GL-POINT-WRAP
 ****************************************************************************/
GlPointWrap::GlPointWrap()
{
}

GlPointWrap::GlPointWrap(float x, float y)
		: v(x, y)
{
}

GlPointWrap::GlPointWrap(const BPoint& in)
		: v(in)
{
}

GlPointWrap::~GlPointWrap()
{
}
	
status_t GlPointWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlPointParam*)param)->pt;
	return B_OK;
}

void GlPointWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlPointParam*)param)->pt = v;
}

status_t GlPointWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return msg.AddPoint(name, v);
}

// #pragma mark -

/***************************************************************************
 * GL-POINT-3D-WRAP
 ****************************************************************************/
GlPoint3dWrap::GlPoint3dWrap()
{
}

GlPoint3dWrap::GlPoint3dWrap(float x, float y, float z)
		: v(x, y, z)
{
}

GlPoint3dWrap::GlPoint3dWrap(const ArpPoint3d& in)
		: v(in)
{
}

GlPoint3dWrap::~GlPoint3dWrap()
{
}
	
status_t GlPoint3dWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlPoint3dParam*)param)->pt;
	return B_OK;
}

void GlPoint3dWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlPoint3dParam*)param)->pt = v;
}

status_t GlPoint3dWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return v.WriteTo(msg, name);
}

// #pragma mark -

/***************************************************************************
 * GL-REL-ABS-WRAP
 ****************************************************************************/
GlRelAbsWrap::GlRelAbsWrap(float inRel, int32 inAbs)
		: v(inRel, inAbs)
{
}

GlRelAbsWrap::GlRelAbsWrap(const GlRelAbs& in)
		: v(in)
{
}

GlRelAbsWrap::~GlRelAbsWrap()
{
}
	
status_t GlRelAbsWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlRelAbsParam*)param)->v;
	return B_OK;
}

void GlRelAbsWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlRelAbsParam*)param)->v = v;
}

status_t GlRelAbsWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return v.WriteTo(msg, name);
}

// #pragma mark -

/***************************************************************************
 * GL-TEXT-WRAP
 ****************************************************************************/
GlTextWrap::GlTextWrap()
{
}

GlTextWrap::GlTextWrap(const char* in)
{
	if (in) v = in;
}

GlTextWrap::GlTextWrap(const BString16& in)
		: v(in)
{
}

GlTextWrap::~GlTextWrap()
{
}

status_t GlTextWrap::GetValue(const GlParam* param)
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return B_ERROR);
	v = ((GlTextParam*)param)->text;
	return B_OK;
}

void GlTextWrap::SetValue(GlParam* param) const
{
	ArpVALIDATE(param->ParamType()->AcceptType(Type()), return);
	((GlTextParam*)param)->text = v;
}

status_t GlTextWrap::AddValue(BMessage& msg, const char* name) const
{
	ArpASSERT(name);
	return msg.AddString16(name, v);
}
