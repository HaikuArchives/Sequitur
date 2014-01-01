/* GlParamWrap.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.02.19			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLPARAMWRAP_H
#define GLPUBLIC_GLPARAMWRAP_H

#include <ArpCore/String16.h>
#include <be/app/Message.h>
#include <ArpMath/ArpPoint3d.h>
#include <ArpSupport/ArpVoxel.h>
#include <ArpInterface/ArpFont.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlSupportTypes.h>
class GlParam;

/***************************************************************************
 * GL-PARAM-WRAP
 * I abstract getting and setting operations on parameters.  This is
 * just a convenience, so the APIs aren't riddled with calls to get and
 * set each type of data.
 ****************************************************************************/
class GlParamWrap
{
public:
	virtual int32		Type() const = 0;

	virtual status_t	GetValue(const GlParam* param) = 0;
	virtual void		SetValue(GlParam* param) const = 0;

	virtual status_t	AddValue(BMessage& msg, const char* name) const = 0;
};

// GL-BOOL-WRAP
class GlBoolWrap : public GlParamWrap
{
public:
	bool				v;

	GlBoolWrap(bool in);
	virtual ~GlBoolWrap();

	virtual int32		Type() const	{ return GL_BOOL_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-COLOR-WRAP
class GlColorWrap : public GlParamWrap
{
public:
	ArpVoxel			v;

	GlColorWrap(const ArpVoxel& in);
	GlColorWrap(uint8 r, uint8 g, uint8 b, uint8 a = 255, uint8 z = 0);
	virtual ~GlColorWrap();
	
	virtual int32		Type() const	{ return GL_COLOR_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-FLOAT-WRAP
class GlFloatWrap : public GlParamWrap
{
public:
	float				v;

	GlFloatWrap(float in = 0.0);
	virtual ~GlFloatWrap();
	
	virtual int32		Type() const	{ return GL_FLOAT_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-FONT-WRAP
class GlFontWrap : public GlParamWrap
{
public:
	ArpFont				v;

	GlFontWrap(const ArpFont* in = 0);
	virtual ~GlFontWrap();
	
	virtual int32		Type() const	{ return GL_FONT_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-INT-32-WRAP
class GlInt32Wrap : public GlParamWrap
{
public:
	int32				v;

	GlInt32Wrap(int32 in = 0);
	virtual ~GlInt32Wrap();
	
	virtual int32		Type() const	{ return GL_INT32_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-POINT-WRAP
class GlPointWrap : public GlParamWrap
{
public:
	BPoint				v;

	GlPointWrap();
	GlPointWrap(float x, float y);
	GlPointWrap(const BPoint& in);
	virtual ~GlPointWrap();
	
	virtual int32		Type() const	{ return GL_POINT_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-POINT-3D-WRAP
class GlPoint3dWrap : public GlParamWrap
{
public:
	ArpPoint3d			v;

	GlPoint3dWrap();
	GlPoint3dWrap(float x, float y, float z);
	GlPoint3dWrap(const ArpPoint3d& in);
	virtual ~GlPoint3dWrap();
	
	virtual int32		Type() const	{ return GL_POINT_3D_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-REL-ABS-WRAP
class GlRelAbsWrap : public GlParamWrap
{
public:
	GlRelAbs			v;

	GlRelAbsWrap(float inRel = 0.0, int32 inAbs = 0);
	GlRelAbsWrap(const GlRelAbs& in);
	virtual ~GlRelAbsWrap();
	
	virtual int32		Type() const	{ return GL_REL_ABS_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

// GL-TEXT-WRAP
class GlTextWrap : public GlParamWrap
{
public:
	BString16			v;

	GlTextWrap();
	GlTextWrap(const char* in);
	GlTextWrap(const BString16& in);
	virtual ~GlTextWrap();
	
	virtual int32		Type() const	{ return GL_TEXT_TYPE; }

	virtual status_t	GetValue(const GlParam* param);
	virtual void		SetValue(GlParam* param) const;

	virtual status_t	AddValue(BMessage& msg, const char* name) const;
};

#endif
