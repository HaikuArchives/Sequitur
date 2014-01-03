#include "ArpKernel/ArpBitmapWrite.h"
#include "ArpKernel/ArpDebug.h"

#include <Bitmap.h>
#include <DataIO.h>
#include <File.h>
#include <NodeInfo.h>

#include <stdio.h>

// The R5 PNG translator sometimes has problem writing alpha correctly,
// so talk to libpng.so directly.

//TODO:
// This is found in png.h
/* The complete definition of png_struct has, as of libpng-1.5.0,
 * been moved into a separate header file that is not accessible to
 * applications.  Read libpng-manual.txt or libpng.3 for more info.
 */
#define USE_LIB_PNG 0

#if USE_LIB_PNG
#include "png.h"

static void PNGCallbackFlush(png_structp const /*a_png*/)
{
	// To my knowledge, there is no concept of "flushing" associated
	// with BPositionIOs
	;
}

static void PNGCallbackWrite(png_structp const a_png, png_bytep const a_buf, const png_uint_32 a_buf_len)
{
	BDataIO *io(static_cast<BDataIO *>(png_get_io_ptr(a_png)));
	ssize_t buf_len(a_buf_len);
	
	if (io->Write(a_buf, buf_len) != buf_len)
	{
		// We're imitating the library's own error handler here
		longjmp(a_png->jmpbuf, B_IO_ERROR);
	}
}


status_t save_bitmap_as_png(BBitmap* bitmap, BDataIO* out_stream)
{
	ArpASSERT(bitmap && out_stream);
	if (!bitmap || !out_stream) {
		delete bitmap;
		return B_ERROR;
	}
	
	const size_t		width(static_cast<size_t>(bitmap->Bounds().Width() + 1.0));
	const size_t		height(static_cast<size_t>(bitmap->Bounds().Height() + 1.0));
	const size_t		row_bytes(bitmap->BytesPerRow());
	status_t			status(B_NO_ERROR);
	
	png_structp			dest_png;
	png_infop			dest_info;
	
	// Try to allocate the PNG image data structures
	if ((dest_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL
			|| (dest_info = png_create_info_struct(dest_png)) == NULL) {
		if (dest_png != NULL) {
			png_destroy_write_struct(&dest_png, (dest_info != NULL) ? &dest_info : NULL);
		}
		
		dest_png = NULL;
		dest_info = NULL;
		status = B_NO_MEMORY;
	} else if ((status = setjmp(dest_png->jmpbuf)) != B_NO_ERROR) {
		// Catch error.
	} else {
		png_set_write_fn(	dest_png, out_stream,
							static_cast<png_rw_ptr>(PNGCallbackWrite),
							static_cast<png_flush_ptr>(PNGCallbackFlush));
		png_set_IHDR(dest_png, dest_info, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_set_pHYs(dest_png, dest_info, 4000, 4000, PNG_RESOLUTION_METER);
		png_set_oFFs(dest_png, dest_info, 0, 0, PNG_OFFSET_PIXEL);
					
		// Write the info struct
		png_write_info(dest_png, dest_info);
	
		// Write out the image
		png_set_bgr(dest_png);
		size_t num_passes(png_set_interlace_handling(dest_png));
		
		for (size_t i(0); i < num_passes; i++) {
			png_byte* row = (png_byte*)bitmap->Bits();
			for (size_t j(0); j < height; j++, row += row_bytes) {
				// Write out the row
				png_write_rows(dest_png, &row, 1);
			}
		}
		png_write_end(dest_png, NULL);
	}
	
	if (dest_png != NULL) {
		png_destroy_write_struct(&dest_png, (dest_info != NULL) ? &dest_info : NULL);
	}
	
	delete bitmap;
	
	return status;
}

//TODO:
// filename isn't even given as a parameter
// the function is probably an awful merge
// of the one in the #if and the one below this one

/*#else

status_t save_bitmap_as_png(BBitmap* bitmap, BDataIO* out_stream)
{
	printf("Saving bitmap %s, colorspace = 0x%08x\n", filename, bitmap->ColorSpace());
	ArpASSERT(bitmap && filename);
	if (!bitmap || !filename) return B_ERROR;
	// The bitmap stream will delete the bitmap.
	BBitmapStream		stream(bitmap);
	BTranslatorRoster*	roster = BTranslatorRoster::Default(); 
	if (!roster) return B_ERROR;
	BFile				file(filename,  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t			err = file.InitCheck();
	if (err != B_OK) return err;
	BMessage ioExt;
	ioExt.AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, B_RGBA32);
	printf("ioExt: "); ioExt.PrintToStream();
	if ((err = roster->Translate(&stream, NULL, &ioExt, &file, B_PNG_FORMAT)) != B_OK) {
//printf("Error translating %s\n", strerror(err) );
		return err;
	}
	printf("Translated!\n");
	BNodeInfo	nodeInfo(&file);
	nodeInfo.SetType("image/png");
	return err;
}*/

#endif

status_t save_bitmap_as_png(BBitmap* bitmap, const char* filename)
{
	ArpASSERT(bitmap && filename);
	if (!bitmap || !filename) {
		delete bitmap;
		return B_ERROR;
	}
	
	BFile		file(filename,  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t	status = file.InitCheck();
	
	if (status != B_OK) {
		delete bitmap;
		return status;
	}
	
	status = save_bitmap_as_png(bitmap, &file);
	
	if (status == B_OK) {
		BNodeInfo	nodeInfo(&file);
		nodeInfo.SetType("image/png");
	}
	
	return status;
}
