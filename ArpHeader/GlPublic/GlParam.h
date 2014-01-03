/* GlParam.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.09.16			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLPARAM_H
#define GLPUBLIC_GLPARAM_H

#include <interface/Point.h>
#include <support/SupportDefs.h>
#include <ArpSupport/ArpVoxel.h>
#include <ArpCore/String16.h>
#include <ArpInterface/ArpFont.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlSupportTypes.h>
class GlNode;
class GlParamTypeList;

/***************************************************************************
 * GL-PARAM
 * I am a node's instance of a param type.
 ****************************************************************************/
class GlParam
{
protected:
	GlParam(const GlParamType* type, gl_id id,
			const BMessage* config = 0, uint32 stateFlags = 0);
	GlParam(const GlParam& o, gl_id id);

public:
	virtual ~GlParam();

	/* A catch all -- I have my param type, owner, and paramtype is valid.
	 */
	bool					IsValid() const;

	gl_id					Owner() const;
	const GlParamType*		ParamType() const;
	uint32					StateFlags() const;
		
	virtual GlParam*		Clone(gl_node_id nid) const = 0;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	const GlParamType*		mType;
	gl_id					mOwner;
	uint32					mStateFlags;
	
public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-BOOL-PARAM
 ****************************************************************************/
class GlBoolParam : public GlParam
{
public:
	bool					b;

	GlBoolParam(	const GlBoolParamType* type, gl_node_id nid,
					bool b, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlBoolParam(const GlBoolParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-COLOR-PARAM
 ****************************************************************************/
class GlColorParam : public GlParam
{
public:
	ArpVoxel				v;

	GlColorParam(	const GlColorParamType* type, gl_node_id nid,
					const ArpVoxel& inV, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlColorParam(const GlColorParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-INT32-PARAM
 ****************************************************************************/
class GlInt32Param : public GlParam
{
public:
	int32			i32;
	
	GlInt32Param(	const GlParamType* type, gl_node_id nid,
					int32 i32, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlInt32Param(const GlInt32Param& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-FLOAT-PARAM
 ****************************************************************************/
class GlFloatParam : public GlParam
{
public:
	float					f;

	GlFloatParam(	const GlFloatParamType* type, gl_node_id nid,
					float inF, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlFloatParam(const GlFloatParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

	status_t				GetRange(float* min, float* max) const;
	void					SetRange(float min, float max);
	
private:
	typedef GlParam			inherited;
	enum {
		HAS_RANGE_F			= 0x00000001
	};
	uint32					mFlags;
	float					mMin, mMax;
	
public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-FONT-PARAM
 ****************************************************************************/
class GlFontParam : public GlParam
{
public:
	ArpFont					f;

	GlFontParam(const GlFontParamType* type, gl_node_id nid,
				const ArpFont* inF, const BMessage* config = 0,
				uint32 stateFlags = 0);
	GlFontParam(const GlFontParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;
	
private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-POINT-PARAM
 ****************************************************************************/
class GlPointParam : public GlParam
{
public:
	BPoint					pt;

	GlPointParam(	const GlPointParamType* type, gl_node_id nid,
					const BPoint& inPt, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlPointParam(const GlPointParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-POINT-3D-PARAM
 ****************************************************************************/
class GlPoint3dParam : public GlParam
{
public:
	ArpPoint3d				pt;

	GlPoint3dParam(	const GlPoint3dParamType* type, gl_node_id nid,
					const ArpPoint3d& inPt, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlPoint3dParam(const GlPoint3dParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-REL-ABS-PARAM
 ****************************************************************************/
class GlRelAbsParam : public GlParam
{
public:
	GlRelAbs		v;
	
	GlRelAbsParam(	const GlParamType* type, gl_node_id nid,
					const GlRelAbs& ra, const BMessage* config = 0,
					uint32 stateFlags = 0);
	GlRelAbsParam(const GlRelAbsParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-TEXT-PARAM
 ****************************************************************************/
class GlTextParam : public GlParam
{
public:
	BString16				text;

	GlTextParam(const GlParamType* type, gl_node_id nid,
				const BString16& t, const BMessage* config = 0,
				uint32 stateFlags = 0);
	GlTextParam(const GlTextParam& o, gl_node_id nid);

	virtual GlParam*		Clone(gl_node_id nid) const;
	virtual status_t		ReadFrom(const BMessage& msg);
	virtual status_t		WriteTo(BMessage& msg) const;

private:
	typedef GlParam			inherited;

public:
	virtual void			Print() const;
};

#endif
