/* AmToolBarRef.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmToolBarRef.h"
#include "AmKernel/AmToolBar.h"

//#define _TRACE_TOOL_BAR_REF_LOCK	(1)

/*************************************************************************
 * AM-TOOL-BAR-REF
 *************************************************************************/
AmToolBarRef::AmToolBarRef(const AmToolBar* toolBar)
		: mToolBar(const_cast<AmToolBar*>(toolBar))
{
	if (mToolBar) mToolBar->AddRef();
}

AmToolBarRef::AmToolBarRef(const AmToolBarRef& ref)
		: mToolBar(ref.mToolBar)
{
	if (mToolBar) mToolBar->AddRef();
}

AmToolBarRef::~AmToolBarRef()
{
	if (mToolBar) mToolBar->RemoveRef();
}

AmToolBarRef& AmToolBarRef::operator=(const AmToolBarRef& ref)
{
	SetTo(ref.mToolBar);
	return *this;
}

bool AmToolBarRef::operator==(const AmToolBarRef& ref) const
{
	return mToolBar == ref.mToolBar;
}

bool AmToolBarRef::operator!=(const AmToolBarRef& ref) const
{
	return mToolBar != ref.mToolBar;
}

bool AmToolBarRef::SetTo(const AmToolBar* toolBar)
{
	if (mToolBar) mToolBar->RemoveRef();
	mToolBar = const_cast<AmToolBar*>(toolBar);
	if (mToolBar) mToolBar->AddRef();
	
	return IsValid();
}

bool AmToolBarRef::IsValid() const
{
	return mToolBar != 0;
}

tool_bar_id AmToolBarRef::ToolBarId() const
{
	if ( !IsValid() ) return 0;
	return mToolBar->Id();
}

const char* AmToolBarRef::ToolBarName() const
{
	if ( !IsValid() ) return NULL;
	return mToolBar->Name();
}

const AmToolBar* AmToolBarRef::ReadLock() const
{
	if (!mToolBar) return NULL;
	if (!mToolBar->ReadLock() ) return NULL;
	#ifdef _TRACE_TOOL_BAR_REF_LOCK
	printf("AmToolBarRef::ReadLock()\n");
	#endif
	return mToolBar;
}

void AmToolBarRef::ReadUnlock(const AmToolBar* toolBar) const
{
	if (toolBar) {
		if (toolBar != mToolBar) debugger("Bad tool bar returned to ReadUnlock()");
		else {
			#ifdef _TRACE_TOOL_BAR_REF_LOCK
			printf("AmToolBarRef::ReadUnlock()\n");
			#endif
			toolBar->ReadUnlock();
		}
	}
}

AmToolBar* AmToolBarRef::WriteLock()
{
	if (!mToolBar) return NULL;
	if (!mToolBar->WriteLock() ) return NULL;
	#ifdef _TRACE_TOOL_BAR_REF_LOCK
	printf("AmToolBarRef::WriteLock()\n");
	#endif
	return mToolBar;
}

void AmToolBarRef::WriteUnlock(AmToolBar* toolBar)
{
	if (toolBar) {
		if (toolBar != mToolBar) debugger("Bad tool bar returned to WriteUnlock()");
		else {
			#ifdef _TRACE_TOOL_BAR_REF_LOCK
			printf("AmToolBarRef::WriteUnlock()\n");
			#endif
			toolBar->WriteUnlock();
		}
	}
}

status_t AmToolBarRef::AddObserver(BHandler* handler)
{
	if (!mToolBar) return B_NO_INIT;
	return mToolBar->AddObserver(handler);
}

status_t AmToolBarRef::RemoveObserver(BHandler* handler)
{
	if (!mToolBar) return B_NO_INIT;
	return mToolBar->RemoveObserver(handler);
}
