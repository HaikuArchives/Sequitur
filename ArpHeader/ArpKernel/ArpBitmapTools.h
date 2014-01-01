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
 * ArpColor.h
 *
 * A class wrapper around rgb_color.
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
 * • Add methods for creating ArpColor objects that are scaled
 *   between two other colors.
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

#ifndef ARPKERNEL_ARPBITMAPTOOLS_H
#define ARPKERNEL_ARPBITMAPTOOLS_H

#include <SupportDefs.h>
#include <GraphicsDefs.h>

class BBitmap;
class BDataIO;

status_t overlay_bitmap(BBitmap* behind, const BBitmap* overlay,
						uint8 amount = 255);

status_t tint_bitmap(BBitmap* base, rgb_color tint);

#endif
