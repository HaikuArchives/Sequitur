/* AmPipelineMatrixRef.h
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
 * 02.19.01		hackborn@angryredplanet.com
 * Created this file
 */
#ifndef AMPUBLIC_AMPIPELINEMATRIXREF_H
#define AMPUBLIC_AMPIPELINEMATRIXREF_H

#include <vector.h>
#include <be/app/Messenger.h>
#include <be/support/SupportDefs.h>
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmToolRef.h"

/***************************************************************************
 * AM-PIPELINE-MATRIX-REF
 * This class represents a reference to a single pipeline matrix object.
 ****************************************************************************/
class AmPipelineMatrixRef
{
public:
	AmPipelineMatrixRef();
	AmPipelineMatrixRef(const AmPipelineMatrixI* matrix);
	AmPipelineMatrixRef(const AmPipelineMatrixRef& ref);
	AmPipelineMatrixRef(const AmSongRef& ref);
	AmPipelineMatrixRef(const AmToolRef& ref);
	virtual ~AmPipelineMatrixRef();

	AmPipelineMatrixRef& operator=(const AmPipelineMatrixRef& ref);
	AmPipelineMatrixRef& operator=(const AmSongRef& ref);
	AmPipelineMatrixRef& operator=(const AmToolRef& ref);
	/* Set this object to the supplied track impl and answer the
	 * result of IsValid().
	 */
	bool			SetTo(const AmPipelineMatrixI* matrix);
	/* Answer true if this ref can create AmTrack objects for reading and
	 * writing.
	 */
	bool			IsValid() const;
	/* Answer a unique value for the matrix.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	pipeline_matrix_id Id() const;
	
	/* LOCKING
	 */
	const AmPipelineMatrixI* ReadLock() const;
	void			ReadUnlock(const AmPipelineMatrixI* matrix) const;
	AmPipelineMatrixI* WriteLock(const char* name = NULL);
	void			WriteUnlock(AmPipelineMatrixI* matrix);

	/* CHANGE NOTIFICATION
	 */
	status_t		AddMatrixPipelineObserver(pipeline_id id, BHandler* handler);
	status_t		AddMatrixFilterObserver(pipeline_id id, BHandler* handler);
	status_t		RemoveMatrixObserver(pipeline_id id, BHandler* handler);

private:
	AmPipelineMatrixI*	mMatrix;
};

typedef vector<AmPipelineMatrixRef>		pipelinematrixref_vec;

#endif 
