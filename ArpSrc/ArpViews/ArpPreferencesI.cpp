/* ArpPreferencesI.cpp
*/

#ifndef ARPVIEWSPUBLIC_ARPPREFSI_H
#include "ArpViewsPublic/ArpPrefsI.h"
#endif

static ArpPreferencesI* gPrefs = 0;

ArpPreferencesI& Prefs()
{
	if (!gPrefs) debugger("Preferences not yet set");
	return *gPrefs;
}

void SetPrefs(ArpPreferencesI& prefs)
{
	if (gPrefs) debugger("Preferences already set");
	gPrefs = &prefs;
}

static ArpImageManagerI* gImg = 0;

ArpImageManagerI& ImageManager()
{
	if (!gImg) debugger("Image manager not yet set");
	return *gImg;
}

bool HasImageManager()
{
	return gImg != 0;
}

void SetImageManager(ArpImageManagerI& img)
{
	if (gImg) debugger("Image manager already set");
	gImg = &img;
}
