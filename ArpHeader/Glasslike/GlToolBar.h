/* GlToolBar.h
 * Copyright (c)2004 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2004.03.07		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLTOOLBAR_H
#define GLASSLIKE_GLTOOLBAR_H

#include <ArpCore/StlVector.h>
#include <support/Locker.h>
#include <ArpSupport/ArpSafeDelete.h>
#include <GlPublic/GlDefs.h>

class _GlToolBarEntry;

/* GlToolBar Flags()
 */
enum {
	GL_TB_HIDDEN_F		= 0x00000001
};

/***************************************************************************
 * GL-TOOL-BAR
 ***************************************************************************/
class GlToolBar : public ArpSafeDelete
{
public:
	GlToolBar(int32 type, const BString16* label);
	~GlToolBar();

	bool						ReadLock() const;
	bool						ReadUnlock() const;
	bool						WriteLock(const char* name = NULL);
	bool						WriteUnlock();

	gl_id						Id() const;
	/* The type is one of the GL_IMAGE_NODE or other flags - but ONLY
	 * one, combinations aren't valid.
	 */
	int32						Type() const;
	BString16					Label() const;
	uint32						Flags() const;
	
	status_t					GetTool(uint32 index, BString16& creator, int32* key) const;
	status_t					AddTool(const BString16& creator, int32 key);

	bool						Matches(int32 type, const BString16* str);
			
private:
	mutable	BLocker				mReadLock;
	mutable	BLocker				mWriteLock;
//	mutable ArpReadWriteLock	mLock;
	uint32						mFlags;
	int32						mType;
	BString16					mLabel;
	std::vector<_GlToolBarEntry*>	mTools;

public:
	void						Print(uint32 tabs = 0) const;
};

/***************************************************************************
 * GL-TOOL-BAR-REF
 ****************************************************************************/
class GlToolBarRef
{
public:
	GlToolBarRef();
	GlToolBarRef(const GlToolBar* bar);
	GlToolBarRef(const GlToolBarRef& ref);
	virtual ~GlToolBarRef();

	GlToolBarRef&		operator=(const GlToolBarRef& ref);
	bool				SetTo(const GlToolBar* bar);
	bool				SetTo(const GlToolBarRef& ref);
	/* Answer true if this ref can create AmTrack objects for reading and
	 * writing.
	 */
	bool				IsValid() const;
	/* Answer a unique value for the tool bar or 0 if invalid.
	 */
	gl_id				ToolBarId() const;
	/* The type is immutable so this is safe.
	 */
	int32				ToolBarType() const;
	
	/* LOCKING
	 */
	const GlToolBar*	ReadLock() const;
	void				ReadUnlock(const GlToolBar* tb) const;
	GlToolBar*			WriteLock(const char* name = NULL);
	void				WriteUnlock(GlToolBar* tb);

private:
	GlToolBar*			mBar;
};

/***************************************************************************
 * GL-TOOL-BAR-ROSTER
 ****************************************************************************/
class GlToolBarRoster
{
public:
	GlToolBarRoster();

	uint32					Size() const;

	GlToolBarRef			GetToolBar(int32 type, uint32 index) const;
	status_t				AddToolBar(GlToolBar* bar);

private:
	/* All public calls must go through the lock.
	 */
	mutable BLocker 		mLock; 
	std::vector<GlToolBarRef>	mRefs;

public:
	void					Print() const;
};

#endif
