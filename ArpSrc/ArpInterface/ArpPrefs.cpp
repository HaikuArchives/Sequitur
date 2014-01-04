#include <ArpKernel/ArpDebug.h>
#include <ArpInterface/ArpPrefs.h>

static ArpPrefs*		gPrefs = 0;

const ArpPrefs& Prefs()
{
	if (!gPrefs) ArpASSERT(false);
	return *gPrefs;
}

void SetPrefs(ArpPrefs& prefs)
{
	if (gPrefs) ArpASSERT(false);
	gPrefs = &prefs;
}
