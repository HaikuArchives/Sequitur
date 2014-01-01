#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlUtils.h>

GlImage* gl_new_node_image(GlNode* n, bool border)
{
	GlImage*				img = GlGlobals().CloneImage(GL_NODE_IMAGE_ID);
	if (!img) return 0;
	if (!n) return img;

	gl_generate_args		gargs;
	gargs.flags = GL_NODE_ICON_F;
	GlAlgo*					a = n->Generate(gargs);
	if (a) {
		GlNodeDataList		list;
		list.AddImage(img);
		gl_process_args		pargs;
		pargs.flags = GL_NODE_ICON_F;
		a->PerformAll(list, &pargs);
		img = list.DetachImage();
	}
	
	/* FIX: If the img being returned doesn't match the original
	 * image dimensions, scale it.  Maybe.
	 */
return img;
//	if (!border) return img;
//	return gl_node_image_border(img, n->Io());
}

GlImage* gl_node_image_border(GlImage* img, uint32 io)
{
	ArpVALIDATE(img, return img);
	if (io != GL_IMAGE_IO) return img;
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);

	int32			l = 2, t = 2, r = 2, b = 2, x, y, srcPix;
	int32			innerW = w - l - r, innerH = h - t - b;
	/* Fix: if img is less than the requested size, don't
	 * scale it. -- sheesh, and maintain ratios, too.  This
	 * is really dicey.
	 */	
	GlImage*		innerImg = 0;
	if (img->Width() <= innerW && img->Height() <= innerH)
		innerImg = img->Clone(); 
	else innerImg = img->AsScaledImage(innerW, innerH, 1);
	if (!innerImg) return img;

	if (img->Width() != w || img->Height() != h) {
		delete img;
		ArpVoxel	bg(0, 0, 0, 0, 0);
		img = new GlImage(w, h, &bg);
		if (!img) {
			delete innerImg;
			return 0;
		}
	}

	GlPlanes*		destP = img->LockPixels(GL_PIXEL_RGBA, true);
	GlPlanes*		srcP = innerImg->LockPixels(GL_PIXEL_RGBA, true);
	if (destP && srcP) {
		srcPix = 0;
		for (int32 srcY = 0; srcY < srcP->h; srcY++) {
			if (srcY >= destP->h) break;
			for (int32 srcX = 0; srcX < srcP->w; srcX++) {
				if (srcX >= destP->w) break;
				int32	destPix = ARP_PIXEL(l + srcX, t + srcY, destP->w);
				ArpASSERT(destPix < destP->w * destP->h);
				destP->r[destPix] = srcP->r[srcPix];
				destP->g[destPix] = srcP->g[srcPix];
				destP->b[destPix] = srcP->b[srcPix];
				destP->a[destPix] = srcP->a[srcPix];
				srcPix++;
			}
		}
	}
	if (destP) {
		for (y = 0; y < t; y++) {
			for (x = 0; x < w; x++) {
				int32	pix = ARP_PIXEL(x, y, destP->w);
				destP->r[pix] = destP->g[pix] = destP->b[pix] = 0;
				destP->a[pix] = 255;
			}
		}
	}
	img->UnlockPixels(destP);
	innerImg->UnlockPixels(srcP);
	
	return img;
}

status_t gl_get_notifier_msg(	const BMessage& changeMsg,
								uint32 code, BMessage& codeMsg)
{
	codeMsg.MakeEmpty();
	for (int32 i = 0; changeMsg.FindMessage("m", i, &codeMsg) == B_OK; i++) {
		if (code == codeMsg.what) return B_OK;
	}
	return B_ERROR;
}
