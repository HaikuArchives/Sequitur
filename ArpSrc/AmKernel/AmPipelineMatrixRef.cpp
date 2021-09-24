/* AmPipelineMatrixRef.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmPublic/AmPipelineMatrixRef.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTool.h"

/*************************************************************************
 * AM-PIPELINE-MATRIX-REF
 *************************************************************************/
AmPipelineMatrixRef::AmPipelineMatrixRef()
		: mMatrix(NULL)
{
}

AmPipelineMatrixRef::AmPipelineMatrixRef(const AmPipelineMatrixI* matrix)
		: mMatrix(const_cast<AmPipelineMatrixI*>(matrix))
{
	if (mMatrix) mMatrix->AddRef();
}

AmPipelineMatrixRef::AmPipelineMatrixRef(const AmPipelineMatrixRef& ref)
		: mMatrix(NULL)
{
	mMatrix = ref.mMatrix;
	if (mMatrix) mMatrix->AddRef();
}

AmPipelineMatrixRef::AmPipelineMatrixRef(const AmSongRef& ref)
		: mMatrix(NULL)
{
	mMatrix = dynamic_cast<AmPipelineMatrixI*>(ref.mSong);
	if (mMatrix) mMatrix->AddRef();
}

AmPipelineMatrixRef::AmPipelineMatrixRef(const AmToolRef& ref)
		: mMatrix(NULL)
{
	mMatrix = dynamic_cast<AmPipelineMatrixI*>(ref.mTool);
	if (mMatrix) mMatrix->AddRef();
}

AmPipelineMatrixRef::~AmPipelineMatrixRef()
{
	if (mMatrix) mMatrix->RemoveRef();
}

AmPipelineMatrixRef& AmPipelineMatrixRef::operator=(const AmPipelineMatrixRef &ref)
{
	SetTo(ref.mMatrix);
	return *this;
}

AmPipelineMatrixRef& AmPipelineMatrixRef::operator=(const AmSongRef &ref)
{
	SetTo( dynamic_cast<AmPipelineMatrixI*>(ref.mSong) );
	return *this;
}

AmPipelineMatrixRef& AmPipelineMatrixRef::operator=(const AmToolRef &ref)
{
	SetTo( dynamic_cast<AmPipelineMatrixI*>(ref.mTool) );
	return *this;
}

bool AmPipelineMatrixRef::SetTo(const AmPipelineMatrixI* matrix)
{
	if (mMatrix) mMatrix->RemoveRef();
	mMatrix = const_cast<AmPipelineMatrixI*>(matrix);
	if (mMatrix) mMatrix->AddRef();
	
	return IsValid();
}

bool AmPipelineMatrixRef::IsValid() const
{
	return mMatrix != NULL;
}

pipeline_matrix_id AmPipelineMatrixRef::Id() const
{
	if ( !IsValid() ) return 0;
	return mMatrix->Id();
}

const AmPipelineMatrixI* AmPipelineMatrixRef::ReadLock() const
{
	if (!mMatrix) return NULL;
	if (!mMatrix->ReadLock() ) return NULL;
	return mMatrix;
}

void AmPipelineMatrixRef::ReadUnlock(const AmPipelineMatrixI* matrix) const
{
	if (matrix) {
		if (matrix != mMatrix) debugger("Bad matrix returned to ReadUnlock()");
		else matrix->ReadUnlock();
	}
}

AmPipelineMatrixI* AmPipelineMatrixRef::WriteLock(const char* name)
{
	if (!mMatrix) return NULL;
	if (!mMatrix->WriteLock(name) ) return NULL;
	return mMatrix;
}

void AmPipelineMatrixRef::WriteUnlock(AmPipelineMatrixI* matrix)
{
	if (matrix) {
		if (matrix != mMatrix) debugger("Bad matrix returned to WriteUnlock()");
		else matrix->WriteUnlock();
	}
}

status_t AmPipelineMatrixRef::AddMatrixPipelineObserver(pipeline_id id, BHandler* handler)
{
	if (!mMatrix) return B_NO_INIT;
	return mMatrix->AddMatrixPipelineObserver(id, handler);
}

status_t AmPipelineMatrixRef::AddMatrixFilterObserver(pipeline_id id, BHandler* handler)
{
	if (!mMatrix) return B_NO_INIT;
	return mMatrix->AddMatrixFilterObserver(id, handler);
}

status_t AmPipelineMatrixRef::RemoveMatrixObserver(pipeline_id id, BHandler* handler)
{
	if (!mMatrix) return B_NO_INIT;
	return mMatrix->RemoveMatrixObserver(id, handler);
}
