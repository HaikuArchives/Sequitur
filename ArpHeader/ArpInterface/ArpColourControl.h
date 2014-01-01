/* ArpColourControl.h
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
#ifndef ARPINTERFACE_ARPCOLOURCONTROL_H
#define ARPINTERFACE_ARPCOLOURCONTROL_H

#include <be/interface/Control.h>
#include <ArpSupport/ArpVoxel.h>
#include <ArpInterface/ArpIntControl.h>

/*******************************************************
 * ARP-COLOUR-CONTROL
 * Let user set RGBA numeric values.
 *******************************************************/
class ArpColourControl : public BControl
{
public:
	ArpColourControl(BRect frame, const char* name,
					const BString16* label, float div);
	virtual ~ArpColourControl();

	virtual	void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* msg);

	void			SetChangingMessage(BMessage* msg);
	void			SetChangedMessage(BMessage* msg);
	
	ArpVoxel		Color() const;
	status_t		SetColor(const ArpVoxel& c);
	
private:
	typedef BControl inherited;
	ArpIntControl*	mR;
	ArpIntControl*	mG;
	ArpIntControl*	mB;
	ArpIntControl*	mA;
	BMessage*		mChangingMsg;
	BMessage*		mChangedMsg;
};

#endif
