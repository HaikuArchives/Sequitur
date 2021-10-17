/* _GlStrainList.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2004.04.12			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLSTRAINLIST_H
#define GLKERNEL_GLSTRAINLIST_H

#include <ArpCore/String16.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlParamListI.h>
class _GlStrainListNode;
class _GlStrainListParam;

/***************************************************************************
 * _GL-STRAIN-LIST
 ***************************************************************************/
class _GlStrainList
{
public:
	_GlStrainList();
	~_GlStrainList();

	status_t		Load(	const gl_param_key& key, const GlParam* param,
							const BString16* label, int32 control, int32 midi);
	status_t		Load(	const gl_param_key& key, const GlParamType* paramType,
							const BString16* label, int32 control, int32 midi);

	status_t		Strain(	const gl_param_key& key, const BString16* label,
							int32 control, int32 midi);

	status_t		Install(GlParamListI& target);

private:
	std::vector<_GlStrainListNode*>	mNodes;

	_GlStrainListParam*		Param(const gl_param_key& key, bool create = false);
	_GlStrainListNode*		Node(gl_node_id nid, bool create = false);

public:
	void			Print() const;
};


#endif
