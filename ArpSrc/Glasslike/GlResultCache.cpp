#include <be/StorageKit.h>
#include <be/support/Autolock.h>
#include <ArpCore/ArpDebug.h>
#include <ArpInterface/ArpBitmap.h>
#include "GlPublic/GlImage.h"
#include "Glasslike/GlDefs.h"
#include "Glasslike/GlResultCache.h"

/***************************************************************************
 * GL-RESULT-CACHE
 ***************************************************************************/
GlResultCache::GlResultCache()
		: mFrozenImage(0), mDisplayedImage(0), mPreviewW(0), mPreviewH(0),
		  mPreviewImage(0)
{
}

GlResultCache::~GlResultCache()
{
	delete mFrozenImage;
	delete mDisplayedImage;
	delete mPreviewImage;
}

void GlResultCache::SetTarget(const BMessenger& target)
{
	mTarget = target;
}

void GlResultCache::SetPreviewSize(int32 w, int32 h)
{
	mPreviewW = w;
	mPreviewH = h;
}

status_t GlResultCache::GetFileName(BString16& str) const
{
	if (mFileName.Length() < 1) return B_ERROR;
	str = mFileName;
	return B_OK;
}

GlImage* GlResultCache::NewFrozenImage() const
{
	BAutolock		l(mAccess);
	if (!mFrozenImage) return 0;
	return mFrozenImage->Clone();
}

ArpBitmap* GlResultCache::NewDisplayedBitmap() const
{
	BAutolock		l(mAccess);
	if (!mDisplayedImage) return 0;
	return mDisplayedImage->AsBitmap();
}

GlImage* GlResultCache::NewPreviewImage() const
{
	BAutolock		l(mAccess);
	if (!mPreviewImage) return 0;
	return mPreviewImage->Clone();
}

ArpBitmap* GlResultCache::NewPreviewBitmap() const
{
	BAutolock		l(mAccess);
	if (!mPreviewImage) return 0;
	return mPreviewImage->AsBitmap();
}

status_t GlResultCache::UpdateImage(GlImage* img)
{
	BAutolock		l(mAccess);
	delete mDisplayedImage;
	mDisplayedImage = img;
	if (img && !mFrozenImage) mFrozenImage = img->Clone();
	mTarget.SendMessage(GL_RESULT_CHANGED_MSG);
	return B_OK;
}

status_t GlResultCache::FreezeDisplay()
{
	BAutolock		l(mAccess);
	if (!mDisplayedImage) return B_ERROR;
	mFileName = "";
	delete mFrozenImage;
	mFrozenImage = mDisplayedImage;
	mDisplayedImage = 0;
	return B_OK;
}

status_t GlResultCache::FreezeImage(GlImage* img)
{
	BAutolock		l(mAccess);
	mFileName = "";
	delete mFrozenImage;
	mFrozenImage = img;
	delete mDisplayedImage;
	mDisplayedImage = 0;
	delete mPreviewImage;
	mPreviewImage = 0;
	CachePreview();
	return B_OK;
}

ArpBitmap* GlResultCache::FreezeImage(BMessage* msg)
{
	entry_ref fileRef;
	BString16				fn;
	if (msg->FindRef("refs", &fileRef) == B_OK) {
		BEntry				e(&fileRef);
		if (e.InitCheck() == B_OK && e.Exists()) {
			BPath			p(&e);
			if (p.InitCheck() == B_OK && p.Path()) {
				fn = p.Path();
			}
		}
	}
	return FreezeImage(fn);
}

ArpBitmap* GlResultCache::FreezeImage(const BString16& fn)
{
	mFileName = "";
	if (fn.Length() < 1) return 0;

	ArpBitmap*				newBm = new ArpBitmap(fn);
	if (!newBm) return 0;
	if (newBm->Width() < 1 || newBm->Height() < 1) {
		delete newBm;
		return 0;
	}
	FreezeImage(new GlImage(*newBm));
	mFileName = fn;
	return newBm;
}

status_t GlResultCache::Rewind()
{
	BAutolock		l(mAccess);
	if (!mFrozenImage) return B_ERROR;
	delete mDisplayedImage;
	mDisplayedImage = mFrozenImage->Clone();
	mTarget.SendMessage(GL_RESULT_CHANGED_MSG);
	return B_OK;
}

status_t GlResultCache::CachePreview()
{
	ArpVALIDATE(mPreviewW > 0 && mPreviewH > 0, return B_ERROR);
	delete mPreviewImage;
	mPreviewImage = 0;
	if (!mFrozenImage) return B_ERROR;
	int32			srcW = mFrozenImage->Width(), srcH = mFrozenImage->Height();
	if (srcW <= mPreviewW && srcH <= mPreviewH) {
		mPreviewImage = mFrozenImage->Clone();
		return B_OK;
	}
	int32			newW, newH;
	gl_scale_to_dimens(srcW, srcH, mPreviewW, mPreviewH, &newW, &newH);
	ArpVALIDATE(newW > 0 && newH > 0 && newW <= mPreviewW && newH <= mPreviewH, return 0);
//	mPreviewImage = mFrozenImage->AsScaledImage(newW, newH, 0);
	mPreviewImage = mFrozenImage->AsScaledImage(newW, newH, 1);
	return B_OK;
}
