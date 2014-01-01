/* AmDeleteService.h
 * Copyright (c)1997 - 2001 by Angry Red Planet and Eric Hackborn.
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
 * 2001.02.27		hackborn@angryredplanet.com
 * Mutated this from AmDeleteTool.  The tool went away, but I'd still
 * like the service.
 */
#ifndef AMKERNEL_AMDELETESERVICE_H
#define AMKERNEL_AMDELETESERVICE_H

#include <be/support/Locker.h>
#include "AmPublic/AmEvents.h"
class AmChangeEventUndo;
class AmPhraseEvent;
class AmSong;
class AmTrack;

/**********************************************************************
 * AM-DELETE-SERVICE
 * This service deletes any events it's given.
 **********************************************************************/
class AmDeleteService
{
public:
	AmDeleteService(); 
	virtual ~AmDeleteService();

	/* Always call this before any delete is initiated.  ALWAYS
	 * CALL Finished() AFTER THE DELETE IS FINISHED.
	 */
	void Prepare(AmSong* track);
	/* The song can be NULL, but if it is, the undo state won't
	 * get added.
	 */
	void Finished(AmSong* song, const char* deleteName = "Delete ");

	void Delete(AmTrack* track,
				AmEvent* event,
				AmPhraseEvent* container);
};

#endif 


