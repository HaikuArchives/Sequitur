/* GlParamType.h
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
 * 2002.09.18			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLPARAMTYPE_H
#define GLPUBLIC_GLPARAMTYPE_H

#include <be/app/Message.h>
#include <be/support/SupportDefs.h>
#include <ArpMath/ArpPoint3d.h>
#include <ArpSupport/ArpVoxel.h>
#include <ArpCore/String16.h>
#include <ArpInterface/ArpFont.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlSupportTypes.h>
class BMessage;
class GlNode;
class GlParam;
class GlParamWrap;

/* Param state flags
 */
enum {
	GL_ROOT_INFO_F			= 0x00000001,
	// Old, currently obsolete
	GL_ABS_LOC_PARAM_F		= 0x00000002,	// Param gets the relative location state
	GL_REL_LOC_PARAM_F		= 0x00000004,	// Param gets the relative location state
	GL_REL_RAD_PARAM_F		= 0x00000008	// Param gets the relative radius state
};

#define GL_DEF_ROW			(-1)

/***************************************************************************
 * GL-PARAM-TYPE
 * I define a single type of controllable parameter.
 ****************************************************************************/
class GlParamType
{
public:
	virtual ~GlParamType();

	virtual int32			Type() const = 0;
	/* By default, answer true for my type.  If a subclass
	 * is a factory for a type other than what I am, then
	 * override this (for example, the menu param generates
	 * int32 params).
	 */
	virtual bool			AcceptType(int32 type) const;
	
	int32					Key() const;
	const BString16*		Label() const;
	void					SetLabel(const BString16* label);
	void					SetLabel(const char* label);
	uint32					StateFlags() const;
	/* The row is a bit of candy for the user, types can be defined in
	 * the same row, and they'll be stacked horizontally instead of
	 * vertically.  Rows below zero mean 'no row.'
	 */
	int32					Row() const;
	
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const = 0;
	/* Subclases that have an init value, write it to the wrap.
	 */
	virtual status_t		GetInit(GlParamWrap& wrap) const;
	/* For reading/writing
	 */
	bool					Matches(int32 typeCode, int32 key) const;

protected:
	GlParamType(int32 key, const BString16* label, int32 row,
				uint32 stateFlags = 0);
	GlParamType(int32 key, const char* label, int32 row,
				uint32 stateFlags = 0);

private:
	int32					mKey;
	BString16				mLabel;
	int32					mRow;
	uint32					mStateFlags;
		
public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-BOOL-PARAM-TYPE
 ****************************************************************************/
class GlBoolParamType : public GlParamType
{
public:
	GlBoolParamType(int32 key, const BString16* label, bool inInit, int32 row = -1);
	GlBoolParamType(const GlBoolParamType& o);

	virtual int32			Type() const		{ return GL_BOOL_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	bool					Init() const;

private:
	typedef GlParamType		inherited;
	bool					mInit;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-COLOR-PARAM-TYPE
 ****************************************************************************/
class GlColorParamType : public GlParamType
{
public:
	GlColorParamType(	int32 key, const BString16* label,
						const ArpVoxel* init = 0, int32 row = -1);
	GlColorParamType(const GlColorParamType& o);

	virtual int32			Type() const		{ return GL_COLOR_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	ArpVoxel				Init() const;

private:
	typedef GlParamType		inherited;
	ArpVoxel				mInit;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-FILE-NAME-PARAM-TYPE
 ****************************************************************************/
class GlFileNameParamType : public GlParamType
{
public:
	GlFileNameParamType(int32 key, const BString16* label, int32 row = -1);
	GlFileNameParamType(const GlFileNameParamType& o);

	virtual int32			Type() const		{ return GL_FILENAME_TYPE; }
	virtual bool			AcceptType(int32 type) const;
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;

private:
	typedef GlParamType		inherited;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-FLOAT-PARAM-TYPE
 ****************************************************************************/
class GlFloatParamType : public GlParamType
{
public:
	GlFloatParamType(	int32 key, const BString16* label,
						float inMin, float inMax, float inInit,
						float inSteps, int32 row = -1, uint32 stateFlags = 0);
	GlFloatParamType(const GlFloatParamType& o);

	virtual int32			Type() const		{ return GL_FLOAT_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	float					Min() const;
	float					Max() const;
	float					Init() const;
	float					Steps() const;

private:
	typedef GlParamType		inherited;
	mutable float			mMin, mMax;
	float					mInit, mSteps;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-FONT-PARAM-TYPE
 ****************************************************************************/
class GlFontParamType : public GlParamType
{
public:
	GlFontParamType(	int32 key, const BString16* label,
						ArpFont* init = 0, int32 row = -1);
	GlFontParamType(const GlFontParamType& o);

	virtual int32			Type() const		{ return GL_FONT_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	const ArpFont*			Init() const;

private:
	typedef GlParamType		inherited;
	ArpFont					mInit;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-INT32-PARAM-TYPE
 ****************************************************************************/
class GlInt32ParamType : public GlParamType
{
public:
	GlInt32ParamType(	int32 key, const BString16* label, int32 inMin,
						int32 inMax, int32 inInit, int32 row = GL_DEF_ROW,
						uint32 stateFlags = 0);
	GlInt32ParamType(const GlInt32ParamType& o);

	virtual int32			Type() const		{ return GL_INT32_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	int32					Min() const;
	int32					Max() const;
	int32					Init() const;

private:
	typedef GlParamType		inherited;
	int32					mMin, mMax, mInit;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-MENU-PARAM-TYPE
 ****************************************************************************/
class GlMenuParamType : public GlParamType
{
public:
	GlMenuParamType(	int32 key, const BString16* label,
						const BMessage& items, int32 inInit, int32 row = -1);
	GlMenuParamType(const GlMenuParamType& o);

	virtual int32			Type() const		{ return GL_MENU_TYPE; }
	virtual bool			AcceptType(int32 type) const;
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	int32					Init() const;
	const BMessage&			Items() const;
	int32					ItemSize() const;
	/* Given a value between 1 and ItemSize() - 1, answer the
	 * appropriate index into the item message.
	 */
	int32					Selection(int32 i) const;
	
private:
	typedef GlParamType		inherited;
	BMessage				mItems;
	int32					mInit;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-POINT-PARAM-TYPE
 ****************************************************************************/
class GlPointParamType : public GlParamType
{
public:
	GlPointParamType(	int32 key, const BString16* label,
						const BString16* xLabel, const BString16* yLabel,
						const BPoint& inMin, const BPoint& inMax,
						const BPoint& inInit, float inSteps,
						int32 row = -1, uint32 stateFlags = 0);
	GlPointParamType(const GlPointParamType& o);
	virtual ~GlPointParamType();
	
	virtual int32			Type() const		{ return GL_POINT_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	const BString16*		XLabel() const;
	const BString16*		YLabel() const;

	BPoint 					Min() const;
	BPoint 					Max() const;
	BPoint 					Init() const;
	float 					Steps() const;

private:
	typedef GlParamType		inherited;
	BString16				mXLabel;
	BString16				mYLabel;
	BPoint					mMin, mMax, mInit;
	float					mSteps;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-POINT-3D-PARAM-TYPE
 ****************************************************************************/
class GlPoint3dParamType : public GlParamType
{
public:
	GlPoint3dParamType(	int32 key, const BString16* label, const BString16* xLabel,
						const BString16* yLabel, const BString16* zLabel,
						const ArpPoint3d& inMin, const ArpPoint3d& inMax,
						const ArpPoint3d& inInit, float inSteps,
						int32 row = -1, uint32 stateFlags = 0);
	GlPoint3dParamType(const GlPoint3dParamType& o);
	virtual ~GlPoint3dParamType();
	
	virtual int32			Type() const		{ return GL_POINT_3D_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	const BString16*		XLabel() const;
	const BString16*		YLabel() const;
	const BString16*		ZLabel() const;

	ArpPoint3d				Min() const;
	ArpPoint3d				Max() const;
	ArpPoint3d				Init() const;
	float 					Steps() const;

private:
	typedef GlParamType		inherited;
	BString16				mXLabel;
	BString16				mYLabel;
	BString16				mZLabel;
	ArpPoint3d				mMin, mMax, mInit;
	float					mSteps;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-REL-ABS-PARAM-TYPE
 ****************************************************************************/
class GlRelAbsParamType : public GlParamType
{
public:
	GlRelAbsParamType(	int32 key, const BString16* label, const GlRelAbs& min,
						const GlRelAbs& max, const GlRelAbs& init,
						float steps, int32 row = GL_DEF_ROW, uint32 stateFlags = 0);
	GlRelAbsParamType(const GlRelAbsParamType& o);

	virtual int32			Type() const		{ return GL_REL_ABS_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	GlRelAbs				Min() const;
	GlRelAbs				Max() const;
	GlRelAbs				Init() const;
	float					Steps() const;
	
private:
	typedef GlParamType		inherited;
	GlRelAbs				mMin, mMax, mInit;
	float					mSteps;
	
public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-TEXT-PARAM-TYPE
 ****************************************************************************/
class GlTextParamType : public GlParamType
{
public:
	/* Default to an init of "AaBbCc"
	 */
	GlTextParamType(int32 key, const BString16* label, int32 row = GL_DEF_ROW,
					uint32 stateFlags = 0);
	GlTextParamType(int32 key, const BString16* label, const char* inInit,
					int32 row = GL_DEF_ROW, uint32 stateFlags = 0);
	GlTextParamType(const GlTextParamType& o);

	virtual int32			Type() const		{ return GL_TEXT_TYPE; }
	virtual GlParam*		NewParam(gl_id id, const BMessage* config = 0) const;
	virtual status_t		GetInit(GlParamWrap& wrap) const;

	const char*				Init() const;

private:
	typedef GlParamType		inherited;
	const char*				mInit;
	
public:
	virtual void			Print() const;
};

#endif
