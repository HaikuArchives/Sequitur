/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * ----------------------------------------------------------------------
 *
 * ArpLayout.h
 *
 * A mix-in of BHandler and ArpBaseLayout.  This is the class
 * you should derive from for implementing any layout manager that
 * does not need its own BView.
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
 * July 31, 1999:
 *	• Created from ArpViewWrapper.
 *
 */

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_ARPLAYOUT_H
#define ARPLAYOUT_ARPLAYOUT_H

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTHOOKS_H
#include <ArpLayout/ArpLayoutHooks.h>
#endif

/** -----------------------------------------------------------------

This is the class you should derive from when implementing a
layout manager that doesn't need a full BView context.  It
is a mix-in of ArpBaseLayout and BHandler, creating a full class
that can be used in the layout hierarchy.  You can also instantiate
this raw class if you just want to place spacers in a layout.

    ----------------------------------------------------------------- */
    
class _EXPORT ArpLayout : public BHandler, public ArpBaseLayout
{
private:
  	typedef	ArpBaseLayout inherited;
  	
public:
	/** Create a new layout object with the given name.
	 */
  	ArpLayout(const char* name = 0);
  	
	static ArpLayout*	Instantiate(BMessage* archive);

	/** Implement standard BHandler methods.
	 */
	ARPLAYOUT_HANDLERHOOKS(BHandler);
	/** Implement BHandler::GetSupportedSuites() method.
	 */
	ARPLAYOUT_SUITEHOOKS(BHandler);
	/** Implement BHandler archiving and re-hydrating
		interfaces.
	 */
	ARPLAYOUT_ARCHIVEHOOKS(ArpLayout, BHandler, false);

private:
	void initialize()										{ }
};

#endif
