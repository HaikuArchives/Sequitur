/* GlParamList.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.02.04			hackborn&angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLPARAMLIST_H
#define GLPUBLIC_GLPARAMLIST_H

#include <be/app/Message.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlParamTypeList.h>
#include <GlPublic/GlParamWrap.h>
class GlParam;
class GlStrainedParamList;
class _GlParamListData;

/***************************************************************************
 * GL-PARAM-LIST
 * Store all the params in a node.  Perform operations on them.
 ***************************************************************************/
class GlParamList
{
public:
	GlParamList(gl_id owner, const GlParamTypeList* paramTypes,
				const char* name = 0);
	GlParamList(const GlParamList& o, gl_id owner);
	~GlParamList();
	
	uint32				Size() const;

	const GlParam*		At(uint32 index) const;
	GlParam*			At(uint32 index);
	const GlParam*		Find(int32 ptKey, int32 i = 0) const;
	GlParam*			Find(int32 ptKey, int32 i = 0);

	status_t			ReadFrom(const BMessage& config);
	status_t			WriteTo(BMessage& config) const;
	/* In very special circumstances, a node might allow duplicate params
	 * with the same key.  In these cases, that node will have to be
	 * responsible for nearly all handling of the params, the list will
	 * merely be a repository to save and load them.
	 */	
	status_t			AddParam(GlParam* param, bool allowDups = false);
	status_t			MakeEmpty();

	bool				Bool(		int32 key, bool init = true, int32 i = 0) const;
	ArpVoxel			Color(		int32 key, int32 i = 0) const;
	BString16			FileName(	int32 key, int32 i = 0) const;
	float				Float(		int32 key, int32 i = 0) const;
	ArpFont				Font(		int32 key, int32 i = 0) const;
	int32				Int32(		int32 key, int32 i = 0) const;
	int32				Menu(		int32 key, int32 i = 0) const;
	BPoint				Point(		int32 key, int32 i = 0) const;
	ArpPoint3d			Point3d(	int32 key, int32 i = 0) const;
	GlRelAbs			RelAbs(		int32 key, const GlRelAbs* init = 0, int32 i = 0) const;
	BString16			Text(		int32 key, int32 i = 0) const;

	/* Setting a value will create a new param if it isn't there.
	 */
	status_t			SetValue(	int32 key, const GlParamWrap& wrap, int32 i = 0);
	/* An alternative to the individual data accessing methods.  If
	 * the param doesn't exist, find the param type and answer its init.
	 */
	status_t			GetValue(	const gl_process_args* args, int32 ptKey,
									GlParamWrap& outWrap, int32 i = 0) const;
	/* Just like GetValue(), but answer B_ERROR if there's no param --
	 * don't find the param type Init() value.
	 */
	status_t			GetValueNoInit(	const gl_process_args* args, int32 ptKey,
										GlParamWrap& outWrap, int32 i = 0) const;
	status_t			EraseValue(	int32 key, int32 i = 0);

protected:
	friend class GlParameterView;
	/* This is an ugly hack -- if no changes have been made, then the node
	 * shouldn't even write an empty message for the params in WriteTo().,
	 * so that it knows to initialize itself with all param types when it's
	 * read back in.  If changes have been made, then those changes might
	 * have left me with an empty param list, in which case I SHOULD store
	 * any empty params msg, so I know not to load any params in.
	 */
	bool				mParamsChanged;

private:
	_GlParamListData*	mData;
	const GlParamTypeList* mParamTypes;	// All param lists are based on a sole param type list.
	gl_id				mOwner;			// The ID of my owner -- generally a node but not always.
	const char*			mName;			// A name to use for file IO.
	
	status_t			ExtractParamValue(	const gl_process_args* args,
											int32 ptKey, GlParamWrap& wrap,
											int32 i) const;

	status_t			GetParamValue(int32 ptKey, GlParamWrap& outWrap, int32 i) const;

public:
	void				Print() const;
};

#endif
