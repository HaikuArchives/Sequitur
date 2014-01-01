/* FakePreferences.h
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
 * 02.24.00		hackborn
 * Created this file.
 */

#ifndef AMKERNELTEST_FAKEPREFERENCES_H
#define AMKERNELTEST_FAKEPREFERENCES_H

#include <be/interface/Bitmap.h>
#include <be/interface/Picture.h>
#include "ArpViewsPublic/ArpPrefsI.h"

/***************************************************************************
 * FAKE-PREFERENCES
 ***************************************************************************/
class FakePreferences : public ArpPreferencesI
{
public:
	FakePreferences();

	virtual float Size(uint32 constant) const;
	virtual rgb_color Color(uint32 constant) const;
};

/***************************************************************************
 * FAKE-IMAGE-MANAGER
 ****************************************************************************/
class FakeImageManager : public ArpImageManagerI
{
public:
	FakeImageManager();
	virtual ~FakeImageManager();
	
	virtual const BBitmap* BitmapAt(const char *name) const;
	virtual BPicture* PictureAt(const char* name,
								BPoint size,
								rgb_color viewColor,
								uint32 flags = 0);
	virtual BPicture* PictureAt(const BBitmap *bitmap,
								BPoint size,
								rgb_color viewColor,
								uint32 flags = 0);
};

#endif
