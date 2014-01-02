/*
 * Copyright (c)1999 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpTelnetSettings.h
 *
 * Window to configure the telnet protocol.
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
 * 2/28/1999:
 *	• Created from ArpTerminalSettings.
 *
 */

#ifndef ARPTELNETSETTINGS_H
#define ARPTELNETSETTINGS_H

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#ifndef _MESSENGER_H
#include <app/Messenger>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREIMPL_H
#include <ArpKernel/ArpConfigureImpl.h>
#endif

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#include <ArpLayout/ArpRootLayout.h>
#endif

class ArpTelnetSettings : public ArpRootLayout
{
public:
	ArpTelnetSettings(const BMessenger& telnet,
					  const BMessage& initSettings,	// copied
					  const BFont* font = be_plain_font,
					  uint32 resizingMode = B_FOLLOW_ALL, 
					  uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);
	
	~ArpTelnetSettings();
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void MessageReceived(BMessage *message);
	
protected:
	typedef ArpRootLayout inherited;
	
	BMessenger mTelnet;
	ArpMessage mSettings;
	
	ArpConfigureImpl mImpl;
};

#endif
