/* GlParenNodes.h
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
 * 2004.03.09				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLPARENNODES_H
#define GLNODES_GLPARENNODES_H

#include <GlPublic/GlAlgoNodes.h>

/***************************************************************************
 * GL-PAREN-SRF-ADD-ON
 ***************************************************************************/
class GlParenSrfAddOn : public GlNode2dAddOn
{
public:
	GlParenSrfAddOn(int32 key, const BString16* label, int32 token);

	virtual GlNode*			NewInstance(const BMessage* config) const;

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNode2dAddOn	inherited;
	int32					mToken;
};

/***************************************************************************
 * GL-PAREN-NBR-ADD-ON
 ***************************************************************************/
class GlParenNbrAddOn : public GlNodeAddOn
{
public:
	GlParenNbrAddOn(int32 key, const BString16* label, int32 token);

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_NUMBER_IO; }

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
	int32					mToken;
};


#endif
