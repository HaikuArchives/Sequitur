/* AmToolBarRef.h
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
 * 2001.03.30		hackborn@angryredplanet.com
 * Created this file
 */
#ifndef AMPUBLIC_AMTOOLBARREF_H
#define AMPUBLIC_AMTOOLBARREF_H

#include <be/support/SupportDefs.h>
#include "AmPublic/AmDefs.h"
class BHandler;
class AmToolBar;

/***************************************************************************
 * AM-TOOL-BAR-REF
 * This class represents a reference to a single tool bar object.
 ****************************************************************************/
class AmToolBarRef
{
public:
	AmToolBarRef(const AmToolBar* toolBar = NULL);
	AmToolBarRef(const AmToolBarRef& ref);
	virtual ~AmToolBarRef();

	AmToolBarRef&		operator=(const AmToolBarRef& ref);
	bool				operator==(const AmToolBarRef& ref) const;
	bool				operator!=(const AmToolBarRef& ref) const;
	
	/* Set this object to the supplied tool bar and answer the
	 * result of IsValid().
	 */
	bool				SetTo(const AmToolBar* toolBar);
	/* Answer true if this ref can answer an AmToolBar object for
	 * reading and writing.
	 */
	bool				IsValid() const;
	/* Answer a unique value for the tool bat.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	tool_bar_id			ToolBarId() const;
	/* The name is immutable -- can't be changed once the tool bar
	 * has been constructed -- so it's safe to access without locking.
	 */
	const char*			ToolBarName() const;

	/* LOCKING
	 */
	const AmToolBar*	ReadLock() const;
	void				ReadUnlock(const AmToolBar* tool) const;
	AmToolBar*			WriteLock();
	void				WriteUnlock(AmToolBar* tool);
	/* NOTIFICATION
	 */
	status_t			AddObserver(BHandler* handler);
	status_t			RemoveObserver(BHandler* handler);

private:
	friend class	AmToolBarRoster;
	
	AmToolBar*		mToolBar;
};

#endif 
