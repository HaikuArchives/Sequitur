#include <ArpInterface/ArpBitmapCache.h>

/*************************************************************************
 * ARP-BITMAP-CACHE
 *************************************************************************/
ArpBitmapCache::ArpBitmapCache()
{
}

ArpBitmapCache::~ArpBitmapCache()
{
}

BView* ArpBitmapCache::StartDrawing(BView* owner, BRect updateRect)
{
	return owner;
}

void ArpBitmapCache::FinishDrawing(BView* offscreen)
{
}
