/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpBitmapWrite.h
 *
 * Functions for saving bitmaps.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 0.1: Created this file.
 *
 */

#pragma once

#ifndef ARPKERNEL_ARPBITMAPWRITE_H
#define ARPKERNEL_ARPBITMAPWRITE_H

#include <SupportDefs.h>

class BBitmap;
class BDataIO;

// These functions delete 'bitmap' when they return.
status_t save_bitmap_as_png(BBitmap* bitmap, const char* filename);
status_t save_bitmap_as_png(BBitmap* bitmap, BDataIO* out_stream);

#endif
