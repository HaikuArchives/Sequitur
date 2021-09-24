/* FakePreferences.cpp
 */
#include <cstdio>
#include <cassert>
#include <cstring>
#include <malloc.h>
#include "FakeSupport.h"

/*************************************************************************
 * FAKE-PREFERENCES
 *************************************************************************/
FakePreferences::FakePreferences() 
{
}

float FakePreferences::Size(uint32 constant) const
{
	return 0;
}

rgb_color FakePreferences::Color(uint32 constant) const
{
	rgb_color	c;
	return c;
}

/*************************************************************************
 * FAKE-IMAGE-MANAGER
 *************************************************************************/
FakeImageManager::FakeImageManager()
{
}

FakeImageManager::~FakeImageManager()
{
}

const BBitmap* FakeImageManager::BitmapAt(const char *name) const
{
	return 0;
}

BPicture* FakeImageManager::PictureAt(	const char* name,
										BPoint size,
										rgb_color viewColor,
										uint32 flags)
{
	return 0;
}


BPicture* FakeImageManager::PictureAt(	const BBitmap *bitmap,
										BPoint size,
										rgb_color viewColor,
										uint32 flags)
{
	return 0;
}