/* GlParamTypeList.h
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
#ifndef GLPUBLIC_GLPARAMTYPELIST_H
#define GLPUBLIC_GLPARAMTYPELIST_H

#include <be/support/SupportDefs.h>
class GlParamType;
class _ParamTypeListData;

/***************************************************************************
 * GL-PARAM-TYPE-LIST
 * A list of param types.
 ****************************************************************************/
class GlParamTypeList
{
public:
	GlParamTypeList();
	virtual ~GlParamTypeList();

	uint32					Size() const;
	const GlParamType*		At(uint32 index) const;
	const GlParamType*		Find(int32 key) const;	
	/* I own any param types I'm given.
	 */
	status_t				Add(GlParamType* param);

private:
	_ParamTypeListData*		mData;

	void					DeleteAll();
	
public:
	void					Print() const;
};



#endif
