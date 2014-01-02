/* ArpFontControl.h
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
 * 2002.07.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPINTERFACE_ARPFONTCONTROL_H
#define ARPINTERFACE_ARPFONTCONTROL_H

#include <interface/Control.h>
#include <interface/MenuField.h>
#include <ArpInterface/ArpIntControl.h>
#include <ArpInterface/ArpFont.h>

/*******************************************************
 * ARP-FONT-CONTROL
 * Display a list of all installed fonts.
 *******************************************************/
class ArpFontControl : public BControl
{
public:
	ArpFontControl(	BRect frame, const char* name,
					const BString16* label, uint32 message,
					float divider);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* msg);
	
	ArpFont			Font() const;
	void			SetFont(const ArpFont& font);
	
private:
	typedef BControl	inherited;
	BMenuField*			mFontCtrl;
	ArpIntControl*		mSizeCtrl;
	uint32				mMsgWhat;
//	float			mSize;
};

#endif
