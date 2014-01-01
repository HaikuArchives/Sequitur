/* AmToolBar.h
 * Copyright (c)2001 by Angry Red Planet and Eric Hackborn.
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
 *
 *	- None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.03.30			hackborn@angryredplanet.com
 * Initial version.
 */
#ifndef AMKERNEL_AMTOOLBAR_H
#define AMKERNEL_AMTOOLBAR_H

#include <vector.h>
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/MultiLocker.h"
#include "AmKernel/AmNotifier.h"

/*************************************************************************
 * AM-TOOL-BAR
 * This class stores a collection of tools.
 *************************************************************************/
class AmToolBar
{
public:
	AmToolBar(const char* name, bool showing = false);
	AmToolBar(const BMessage& config);
	AmToolBar(const AmToolBar& o);
	virtual ~AmToolBar();

	void				AddRef() const;
	void				RemoveRef() const;
	bool				ReadLock() const;
	bool				WriteLock(const char* name = NULL);
	bool				ReadUnlock() const;
	bool				WriteUnlock();

	enum {
		/* NOTE:  This is the same constant as one defined in
		 * AmGlobalsI, but a different value.  I might change
		 * the constant later, but right now they are SUPPOSED
		 * to be different values.
		 */
		TOOL_BAR_OBS	= 'Otb2'
	};
	status_t			AddObserver(BHandler* handler);
	status_t			RemoveObserver(BHandler* handler);

	/* These properties are immutable -- they can't be changed once
	 * the tool bar is constructed.
	 */
	tool_bar_id			Id() const;
	const char*			Name() const;

	bool				IsEmpty() const;	
	/* Get and set whether or not I am currently visible to the user.
	 */
	bool				IsShowing() const;
	void				SetShowing(bool showing);
	uint32				CountTools() const;
	/* I suspect it is a very bad idea to lock the tool while
	 * the tool bar is locked, so most clients probably want to
	 * make a copy of all the tool refs, then unlock the tool
	 * bar, then lock each tool ref.
	 */
	AmToolRef			ToolAt(uint32 index) const;
	/* -1 adds it to the end.
	 */
	status_t			InsertTool(const BString& toolKey, int32 index = -1);
	status_t			ReplaceTool(const BString& toolKey, int32 index);
	status_t			RemoveTool(const BString& toolKey);
	/* This method is necessary because of the way the tool roster
	 * is:  There's no such thing as a change operation, only remove
	 * and add.  However, to the user, there IS a change operation,
	 * who doesn't want making a change to a tool to remove it from
	 * the tool bar.  So when tools are deleted, the roster just
	 * tells the tool bar that the tool has changed.  Since the tool
	 * is invalid, the UI will display the tool bar without the tool.
	 * If a tool with the same key gets created shortly after it
	 * was deleted, the roster again tells the tool about the change,
	 * this time telling the tool bar the tool is valid, so the UI
	 * redisplays, again with the tool present.  If the roster doesn't
	 * get notified about a new tool with the same key soon enough,
	 * then it goes ahead and calls RemoveTool on the tool, taking it
	 * out of the tool bar.
	 */
	status_t			ToolChange(const BString& toolKey);
	/* This method runs through all my tool keys and removes any from
	 * me that aren't currently in the tool roster.
	 */
	status_t			Sync();

	status_t			WriteTo(BMessage& config) const;

private:
	int32				mRefCount;
	mutable MultiLocker	mLock;
	AmNotifier			mNotifier;

	BString				mName;
	vector<BString>		mToolKeys;
	bool				mShowing;

	bool				ContainsTool(const BString& toolKey) const;
	/* Answer -1 if I don't have to tool, otherwise its index.
	 */
	int32				ToolIndex(const BString& toolKey, uint32 startingAt = 0) const;
};

#endif 

