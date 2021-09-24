/* AmToolRef.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmToolRef.h"
#include "AmKernel/AmTool.h"

//#define _TRACE_TOOL_REF_LOCK	(1)

/*************************************************************************
 * AM-TOOL-REF
 *************************************************************************/
AmToolRef::AmToolRef(const AmTool* tool)
		: mTool(const_cast<AmTool*>(tool))
{
	if (mTool) mTool->AddRef();
}

AmToolRef::AmToolRef(const AmToolRef& ref)
		: mTool(ref.mTool)
{
	if (mTool) mTool->AddRef();
}

AmToolRef::~AmToolRef()
{
	if (mTool) mTool->RemoveRef();
}

AmToolRef& AmToolRef::operator=(const AmToolRef& ref)
{
	SetTo(ref.mTool);
	return *this;
}

AmToolRef& AmToolRef::operator=(const AmTool* tool)
{
	SetTo(tool);
	return *this;
}

bool AmToolRef::operator==(const AmToolRef& ref) const
{
	return mTool == ref.mTool;
}

bool AmToolRef::operator!=(const AmToolRef& ref) const
{
	return mTool != ref.mTool;
}

bool AmToolRef::SetTo(const AmTool* tool)
{
	if (mTool) mTool->RemoveRef();
	mTool = const_cast<AmTool*>(tool);
	if (mTool) mTool->AddRef();
	
	return IsValid();
}

bool AmToolRef::IsValid() const
{
	return mTool != 0;
}

tool_id AmToolRef::ToolId() const
{
	if ( !IsValid() ) return 0;
	return mTool->Id();
}

BString AmToolRef::ToolKey() const
{
	if (!mTool) return BString("");
	return mTool->Key();
}

const AmTool* AmToolRef::ReadLock() const
{
	if (!mTool) return NULL;
	if (!mTool->ReadLock() ) return NULL;
	#ifdef _TRACE_TOOL_REF_LOCK
	printf("AmToolRef::ReadLock()\n");
	#endif
	return mTool;
}

void AmToolRef::ReadUnlock(const AmTool* tool) const
{
	if (tool) {
		if (tool != mTool) debugger("Bad tool returned to ReadUnlock()");
		else {
			#ifdef _TRACE_TOOL_REF_LOCK
			printf("AmToolRef::ReadUnlock()\n");
			#endif
			tool->ReadUnlock();
		}
	}
}

AmTool* AmToolRef::WriteLock()
{
	if (!mTool) return NULL;
	if (!mTool->WriteLock() ) return NULL;
	#ifdef _TRACE_TOOL_REF_LOCK
	printf("AmToolRef::WriteLock()\n");
	#endif
	return mTool;
}

void AmToolRef::WriteUnlock(AmTool* tool)
{
	if (tool) {
		if (tool != mTool) debugger("Bad tool returned to WriteUnlock()");
		else {
			#ifdef _TRACE_TOOL_REF_LOCK
			printf("AmToolRef::WriteUnlock()\n");
			#endif
			tool->WriteUnlock();
		}
	}
}

status_t AmToolRef::AddMatrixPipelineObserver(pipeline_id id, BHandler* handler)
{
	if (!mTool) return B_NO_INIT;
	return mTool->AddMatrixPipelineObserver(id, handler);
}

status_t AmToolRef::AddMatrixFilterObserver(pipeline_id id, BHandler* handler)
{
	if (!mTool) return B_NO_INIT;
	return mTool->AddMatrixFilterObserver(id, handler);
}

status_t AmToolRef::RemoveMatrixObserver(pipeline_id id, BHandler* handler)
{
	if (!mTool) return B_NO_INIT;
	return mTool->RemoveMatrixObserver(id, handler);
}
