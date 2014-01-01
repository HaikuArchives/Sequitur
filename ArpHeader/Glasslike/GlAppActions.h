/* GlAppActions.h
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
 * 04.18.99		hackborn
 * Created this file.
 */
 
#ifndef GLASSLIKE_GLAPPACTIONS_H
#define GLASSLIKE_GLAPPACTIONS_H

#include "GlPublic/GlActions.h"
#include "GlPublic/GlNode.h"

/***************************************************************************
 * GL-GET-IO-ACTION
 * An action that counts the inputs and outputs for each port type.
 ***************************************************************************/
class GlGetIoAction : public GlNodeAction
{
public:
	uint32		count[_NUM_IO];

	GlGetIoAction() { for (uint32 k = 0; k < _NUM_IO; k++) count[k] = 0; }

	virtual int32		Perform(const GlNode* node)
	{
		if (!node || !(node->AddOn())) return GL_CONTINUE;
		int32			key = node->AddOn()->Key();
		if (key == GL_1D_IN_KEY) count[GL_1D_IO]++;
		else if (key == GL_2D_IN_KEY) count[GL_2D_IO]++;
		else if (key == GL_NUMBER_IN_KEY) count[GL_NUMBER_IO]++;
		else if (key == GL_TEXT_IN_KEY) count[GL_TEXT_IO]++;
		else if (key == GL_IMAGE_IN_KEY) count[GL_IMAGE_IO]++;
		return GL_CONTINUE;
	}
};

#endif
