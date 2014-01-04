/* GlNodeReaderWriter.h
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
 * 2003.02.17			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLNODEREADERWRITER_H
#define GLPUBLIC_GLNODEREADERWRITER_H

#include <app/Message.h>
#include <GlPublic/GlDefs.h>
class GlNode;

/***************************************************************************
 * GL-NODE-WRITER
 * A simple interface for node IO.
 ***************************************************************************/
class GlNodeReaderWriter
{
public:
	/* Given a node and a message with config for that node (which was
	 * probably generated by the write method), read in the config as appropriate.
	 * Answering B_ERROR will cause the message to not be added to the
	 * final result.
	 */
	virtual status_t	ReadNode(const GlNode* node, const BMessage& config) = 0;
	/* Given a node, write my data to the config for that node.
	 * Answering B_ERROR will cause the message to not be added to the
	 * final result.
	 */
	virtual status_t	WriteNode(const GlNode* node, BMessage& config) const = 0;
};

#endif
