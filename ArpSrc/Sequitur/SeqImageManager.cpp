/* SeqImageManager.cpp
 */
#include <cstdio>
#include <cassert>
#include <cstring>
#include <malloc.h>
#include <interface/View.h>
#include <Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "Sequitur/SeqImageManager.h"
#include "Sequitur/SequiturDefs.h"

/*************************************************************************
 * SEQ-IMAGE-MANAGER
 *************************************************************************/
SeqImageManager::SeqImageManager()
{
	mShutdown = false;
	SetImageManager(*this);
}

SeqImageManager::~SeqImageManager()
{
	Shutdown();
}

void SeqImageManager::Shutdown()
{
	BAutolock l(&mAccess);
	mShutdown = true;
}

const BBitmap* SeqImageManager::FindBitmap(const char *name) const
{
	ArpASSERT( name );
	
	BAutolock l(const_cast<BLocker*>(&mAccess));
	if (mShutdown) return 0;

	return Resources().FindBitmap(name);
}
