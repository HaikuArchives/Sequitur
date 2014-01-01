/* ArpPreferencesI.cpp
*/

#ifndef AMPUBLIC_AMPREFSI_H
#include "AmPublic/AmPrefsI.h"
#endif

static AmPreferencesI* gPrefs = 0;

AmPreferencesI& AmPrefs()
{
	if (!gPrefs) debugger("AmPreferences not yet set");
	return *gPrefs;
}

void SetAmPrefs(AmPreferencesI& prefs)
{
	if (gPrefs) debugger("AmPreferences already set");
	gPrefs = &prefs;
}
