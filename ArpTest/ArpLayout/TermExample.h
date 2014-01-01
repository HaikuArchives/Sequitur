/*
 * ArpTelnet Copyright (c)1997-98 by Dianne Hackborn.
 * All rights reserved.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * SettingsWin.h
 *
 * Window to configure the terminal.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 9/19/1998:
 *	• Created this file.
 *
 */

#ifndef TERMEXAMPLE_H
#define TERMEXAMPLE_H

#ifndef _VIEW_H
#include <be/interface/View.h>
#endif

#ifndef _MESSENGER_H
#include <be/app/Messenger>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

BView* GetTermSettings(	const BMessenger& terminal,
						const BMessage& initSettings,	// copied
						const BFont* font = be_plain_font,
						uint32 resizingMode = B_FOLLOW_ALL, 
						uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
						);
	
#endif
