/*
 * Copyright (c)1999 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * ----------------------------------------------------------------------
 *
 * ArpTerminalSettings.h
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
 * 2/1/1998:
 *	• First release, corresponding to ArpTelnet v2.0.
 *
 * 9/19/1998:
 *	• Created this file.
 *
 */

#ifndef ARPTERMINALSETTINGS_H
#define ARPTERMINALSETTINGS_H

#ifndef _VIEW_H
#include <be/interface/View.h>
#endif

#ifndef _MESSENGER_H
#include <be/app/Messenger>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#include <ArpLayout/ArpRootLayout.h>
#endif

// forward refs
class ArpButton;
class ArpMenuField;
class BPopUpMenu;
class BColorControl;
class ArpTextControl;
class BCheckBox;

class ArpTerminalSettings : public ArpRootLayout
{
public:
	ArpTerminalSettings(const BMessenger& terminal,
						const BMessage& initSettings,	// copied
						const BFont* font = be_plain_font,
						uint32 resizingMode = B_FOLLOW_ALL, 
						uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);
	
	~ArpTerminalSettings();
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void MessageReceived(BMessage *message);
	
protected:
	typedef ArpRootLayout inherited;
	
	void ShowCurrentColor(const ArpMessage& settings);
	void ChangeCurrentColor(void);
	void ChangeStandardColor(void);
	void ShowCurrentFont(const ArpMessage& settings);
	void ChangeCurrentFont(void);
	void ShowCurrentEncoding(const ArpMessage& settings);
	void ChangeCurrentEncoding(void);
	void ShowCurrentReceive(const ArpMessage& settings);
	void ChangeCurrentReceive(void);
	void ShowCurrentSend(const ArpMessage& settings);
	void ChangeCurrentSend(void);
	void ShowCurrentMode(const ArpMessage& settings);
	void ChangeCurrentMode(void);
	void ShowCurrentVerifyPaste(const ArpMessage& settings);
	void ChangeCurrentVerifyPaste(void);
	void ShowCurrentQuickPaste(const ArpMessage& settings);
	void ChangeCurrentQuickPaste(void);
	void ShowCurrentAutoScroll(const ArpMessage& settings);
	void ChangeCurrentAutoScroll(void);
	void ShowCurrentHistorySize(const ArpMessage& settings);
	void ChangeCurrentHistorySize(void);
	void ShowCurrentHistoryUse(const ArpMessage& settings);
	void ChangeCurrentHistoryUse(void);
	
	BMessenger mTerminal;
	ArpMessage mSettings;
	
	BPopUpMenu* mColorPopUpMenu;
	BPopUpMenu* mFontPopUpMenu;
	BPopUpMenu* mEncodingPopUpMenu;
	ArpMenuField* mColorMenu;
	ArpMenuField* mFontMenu;
	ArpMenuField* mEncodingMenu;
	ArpTextControl* mColorForeground;
	ArpTextControl* mColorBackground;
	BColorControl* mColorPalette;
	ArpTextControl* mFontSizeText;
	ArpTextControl* mHistoryText;
	BPopUpMenu* mHistoryPopUpMenu;
	ArpMenuField* mHistoryMenu;
	BPopUpMenu* mReceivePopUpMenu;
	ArpMenuField* mReceiveMenu;
	BPopUpMenu* mSendPopUpMenu;
	ArpMenuField* mSendMenu;
	BCheckBox* mSwapBSCheck;
	BCheckBox* mWrapCheck;
	BCheckBox* mInverseCheck;
	BCheckBox* mVerifyPasteCheck;
	BCheckBox* mQuickPasteCheck;
	BCheckBox* mScrollInCheck;
	BCheckBox* mScrollOutCheck;
	ArpButton* mSoftResetButton;
	ArpButton* mHardResetButton;
};

#endif
