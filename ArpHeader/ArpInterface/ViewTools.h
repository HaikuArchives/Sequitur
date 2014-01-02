/* BView.h
 * Copyright (c)2002 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.06.24			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPINTERFACE_VIEW_TOOLS_H
#define ARPINTERFACE_VIEW_TOOLS_H

#include <ArpCore/String16.h>
#include <interface/View.h>

/* Answer the current mouse buttons for the supplied view.
 */
int32		arp_get_mouse_buttons(BView& view);
/* Answer the current font height for the supplied view.
 */
float		arp_get_font_height(const BView* view);
/* Answer the width of the string for the supplied view.
 */
float		arp_get_string_width(const BView* view, const char* text);
float		arp_get_string_width(const BView* view, const BString16* text);

void		arp_set_frame(BView& view, BRect f);

#endif
