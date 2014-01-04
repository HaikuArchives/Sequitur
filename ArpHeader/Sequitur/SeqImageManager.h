/* SeqImageManager.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 03.08.00		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQIMAGEMANAGER_H
#define SEQUITUR_SEQIMAGEMANAGER_H

#include <interface/Bitmap.h>
#include <interface/Picture.h>
#include <interface/Window.h>
#include <Locker.h>
#include "ArpViewsPublic/ArpPrefsI.h"

/***************************************************************************
 * SEQ-IMAGE-MANAGER
 * This class stores all the images available for the application.  When
 * the images are no longer needed -- i.e., when the application is done
 * running -- it should be deleted, so that the images get released.
 ****************************************************************************/
class SeqImageManager : public ArpImageManagerI
{
public:
	SeqImageManager();
	virtual ~SeqImageManager();

	void Shutdown();
	
	/* Answer the bitmap at the supplied name.  Name should be
	 * a resource name bound with the project.  Answer 0 if the
	 * bitmap doesn't exist.
	 */
	virtual const BBitmap* FindBitmap(const char *name) const;

private:
	BLocker					mAccess;
	bool					mShutdown;
};

#endif
