/* ArpPopUpMenu.h
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
 * 2002.08.05			hackborn@angryredplanet.com
 * Created this file.
 */
 
#ifndef ARPINTERFACE_ARPPOPUPMENU_H
#define ARPINTERFACE_ARPPOPUPMENU_H

#include <be/interface/PopUpMenu.h>
#include <ArpInterface/ViewTools.h>

/***************************************************************************
 * ARP-POP-UP-MENU
 * Since the windows side doesn't currently support asynchronous popups,
 * it's got a simplified way of doing things, so I mimic that.
 ***************************************************************************/
class ArpPopUpMenu : public BPopUpMenu
{
public:
	ArpPopUpMenu(const BString16* title);

	/* Start the menu, deliver the selected message,
	 * delete the menu.
	 */
	void		GoAndDeliver(	const BPoint& where,
								BView& owner);

private:
	typedef BPopUpMenu	inherited;
};

#endif
