/* AmToolRef.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 02.16.01		hackborn@angryredplanet.com
 * Created this file
 */
#ifndef AMPUBLIC_AMTOOLREF_H
#define AMPUBLIC_AMTOOLREF_H

#include <vector.h>
#include <be/app/Messenger.h>
#include <be/support/SupportDefs.h>
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmRange.h"
class AmGlobalsImpl;
class AmPipelineMatrixI;
class AmPipelineMatrixRef;
class AmTool;

/***************************************************************************
 * AM-TOOL-REF
 * This class represents a reference to a single tool object.
 ****************************************************************************/
class AmToolRef
{
public:
	AmToolRef(const AmTool* tool = NULL);
	AmToolRef(const AmToolRef& ref);
	virtual ~AmToolRef();

	AmToolRef&		operator=(const AmToolRef& ref);
	AmToolRef&		operator=(const AmTool* tool);
	bool			operator==(const AmToolRef& ref) const;
	bool			operator!=(const AmToolRef& ref) const;
	
	/* Set this object to the supplied tool and answer the
	 * result of IsValid().
	 */
	bool			SetTo(const AmTool* tool);
	/* Answer true if this ref can create AmTool objects for reading and
	 * writing.
	 */
	bool			IsValid() const;
	/* Answer a unique value for the tool.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	tool_id			ToolId() const;
	BString			ToolKey() const;
	
	/* LOCKING
	 */
	const AmTool*	ReadLock() const;
	void			ReadUnlock(const AmTool* tool) const;
	AmTool*			WriteLock();
	void			WriteUnlock(AmTool* tool);

	/* CHANGE NOTIFICATION
	 */
	status_t		AddMatrixPipelineObserver(pipeline_id id, BHandler* handler);
	status_t		AddMatrixFilterObserver(pipeline_id id, BHandler* handler);
	status_t		RemoveMatrixObserver(pipeline_id id, BHandler* handler);

private:
	friend class	AmGlobalsImpl;
	friend class	AmPipelineMatrixRef;
	friend class	AmToolRoster;
	
	AmTool*			mTool;
};

#endif 
