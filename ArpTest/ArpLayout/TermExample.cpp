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
 */

//#define ArpDEBUG 1
//#define USE_STREAMS 1

#ifndef _BUTTON_H
#include <be/interface/Button.h>
#endif

#ifndef _TABVIEW_H
#include <be/interface/TabView.h>
#endif

#ifndef _MENUFIELD_H
#include <be/interface/MenuField.h>
#endif

#ifndef _POPUPMENU_H
#include <be/interface/PopUpMenu.h>
#endif

#ifndef _MENUITEM_H
#include <be/interface/MenuItem.h>
#endif

#ifndef _COLORCONTROL_H
#include <be/interface/ColorControl.h>
#endif

#ifndef _STRINGVIEW_H
#include <be/interface/StringView.h>
#endif

#ifndef _CHECKBOX_H
#include <be/interface/CheckBox.h>
#endif

#ifndef _SCREEN_H
#include <be/interface/Screen.h>
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
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

#ifndef ARPLAYOUT_ARPVIEWWRAPPER_H
#include <ArpLayout/ArpViewWrapper.h>
#endif

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include <ArpLayout/ViewStubs.h>
#endif

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include <ArpLayout/ArpRunningBar.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef TERMEXAMPLE_H
#include "TermExample.h"
#endif

#include <stdlib.h>

// Note: Even indicies are foreground colors, negative are background.
static char* colorNames[] = {
	"Text Foreground",
	"Text Background",
	"Highlight Foreground",
	"Highlight Background",
	"Cursor Foreground",
	"Cursor Background",
	"Black Foreground",
	"Black Background",
	"Red Foreground",
	"Red Background",
	"Green Foreground",
	"Green Background",
	"Yellow Foreground",
	"Yellow Background",
	"Blue Foreground",
	"Blue Background",
	"Magenta Foreground",
	"Magenta Background",
	"Cyan Foreground",
	"Cyan Background",
	"White Foreground",
	"White Background",
	NULL
};

static int32 colorValues[] = {
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21
};

static const char* encNames[] = {
	"ISO 8859 Latin 1",
	"ISO 8859 Latin 2",
	"ISO 8859 Latin 3",
	"ISO 8859 Latin 4",
	"ISO 8859 Latin/Cyrillic",
	"ISO 8859 Latin/Arabic",
	"ISO 8859 Latin/Hebrew",
	"ISO 8859 Latin 5",
	"ISO 8859 Latin 6",
	"Macintosh Roman",
	NULL
};
static int32 encCodes[] = {
	B_ISO_8859_1,
	B_ISO_8859_2,
	B_ISO_8859_3,
	B_ISO_8859_4,
	B_ISO_8859_5,
	B_ISO_8859_6,
	B_ISO_8859_7,
	B_ISO_8859_8,
	B_ISO_8859_9,
	B_ISO_8859_10,
	B_MACINTOSH_ROMAN 
};

static const char* histNames[] = {
	"Off",
	"Minimum",
	"Normal",
	"Aggressive",
	NULL
};
static int32 histCodes[] = {
	0,
	1,
	2,
	3
};

enum {
	CHOOSE_COLOR_MSG = '.ccl',
	COLOR_PALETTE_MSG = '.col',
	CHOOSE_FONT_MSG = '.cfn',
	CHOOSE_FONTSIZE_MSG = '.cfs',
	CHOOSE_ENCODING_MSG = '.cen',
	CHOOSE_CRASCRLF_MSG = '.clf',
	CHOOSE_ENTERASCRLF_MSG = '.ecr',
	CHOOSE_SWAPBS_MSG = '.sbs',
	CHOOSE_WRAP_MSG = '.wrp',
	CHOOSE_INVERSE_MSG = '.inv',
	CHOOSE_VERIFYPASTE_MSG = '.vps',
	CHOOSE_QUICKPASTE_MSG = '.qps',
	CHOOSE_SCROLLIN_MSG = '.sin',
	CHOOSE_SCROLLOUT_MSG = '.sot',
	CHOOSE_HISTORYSIZE_MSG = '.chs',
	CHOOSE_HISTORYMODE_MSG = '.chm',
	SOFT_RESET_MSG = 'srst',
	HARD_RESET_MSG = 'hrst'
};

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
	
protected:
	typedef ArpRootLayout inherited;
	
	BMessenger mTerminal;
	ArpMessage mSettings;
	float mMinWidth, mMinHeight;
	BPopUpMenu* mColorPopUpMenu;
	BPopUpMenu* mFontPopUpMenu;
	BPopUpMenu* mEncodingPopUpMenu;
	ArpMenuField* mColorMenu;
	ArpMenuField* mFontMenu;
	ArpMenuField* mEncodingMenu;
	BColorControl* mColorPalette;
	ArpTextControl* mFontSizeText;
	ArpTextControl* mHistoryText;
	BPopUpMenu* mHistoryPopUpMenu;
	ArpMenuField* mHistoryMenu;
	BCheckBox* mCRAsCRLFCheck;
	BCheckBox* mEnterAsCRLFCheck;
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

BView* GetTermSettings(	const BMessenger& terminal,
						const BMessage& initSettings,
						const BFont* font,
						uint32 resizingMode, 
						uint32 flags )
{
	return new ArpTerminalSettings(terminal,initSettings,font,resizingMode,flags);
}

ArpTerminalSettings::ArpTerminalSettings(
					 const BMessenger& terminal,
					 const BMessage& initSettings,
					 const BFont* font,
					 uint32 resizingMode, 
					 uint32 flags)
	: ArpRootLayout(BRect(0,0,100,100), "Terminal", resizingMode, flags),
	  mTerminal(terminal), mSettings(initSettings),
	  mMinWidth(0), mMinHeight(0),
	  mColorMenu(NULL)
{
	BMessage* ctrlMsg = NULL;
	int i;
	
	mColorPopUpMenu = new BPopUpMenu("color_menu");
	mFontPopUpMenu = new BPopUpMenu("font_menu");
	mEncodingPopUpMenu = new BPopUpMenu("encoding_menu");
	mHistoryPopUpMenu = new BPopUpMenu("history_menu");
	try {
		for( i=0; colorNames[i] != NULL; i++ ) {
			ctrlMsg = new BMessage(CHOOSE_COLOR_MSG);
			ctrlMsg->AddString("color_name", colorNames[i]);
			ctrlMsg->AddInt32("color_item", colorValues[i]);
			ctrlMsg->AddBool("color_background", (i&1) ? true : false);
			BMenuItem* item = new BMenuItem(colorNames[i], ctrlMsg);
			if( i == 0 ) item->SetMarked(true);
			mColorPopUpMenu->AddItem(item);
			ctrlMsg = NULL;
		}
		
		int32 num = count_font_families();
		for( i=0; i<num; i++ ) {
			font_family name;
			uint32 flags = 0;
			bool first=true;
			if( get_font_family(i,&name,&flags) == B_NO_ERROR ) {
				if( flags&B_IS_FIXED ) {
					ctrlMsg = new BMessage(CHOOSE_FONT_MSG);
					ctrlMsg->AddString("font_family",&name[0]);
					BMenuItem* item = new BMenuItem(&name[0], ctrlMsg);
					if( first ) item->SetMarked(true);
					first = false;
					mFontPopUpMenu->AddItem(item);
					ctrlMsg = NULL;
				}
			}
		}
		
		for( i=0; encNames[i] != NULL; i++ ) {
			ctrlMsg = new BMessage(CHOOSE_ENCODING_MSG);
			ctrlMsg->AddString("encoding_name", encNames[i]);
			ctrlMsg->AddInt32("encoding_item", encCodes[i]);
			BMenuItem* item = new BMenuItem(encNames[i], ctrlMsg);
			if( i == 0 ) item->SetMarked(true);
			mEncodingPopUpMenu->AddItem(item);
			ctrlMsg = NULL;
		}
		
		for( i=0; histNames[i] != NULL; i++ ) {
			ctrlMsg = new BMessage(CHOOSE_HISTORYMODE_MSG);
			ctrlMsg->AddString("history_name", histNames[i]);
			ctrlMsg->AddInt32("history_item", histCodes[i]);
			BMenuItem* item = new BMenuItem(histNames[i], ctrlMsg);
			if( i == 0 ) item->SetMarked(true);
			mHistoryPopUpMenu->AddItem(item);
			ctrlMsg = NULL;
		}
		
		AddLayoutChild((new ArpRunningBar("TopVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpBox("StyleBox","Style"))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpFillAll)
				)
				->AddLayoutChild((new ArpRunningBar("StyleVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
					)
					->AddLayoutChild((new ArpRunningBar("ColorHBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
						->AddLayoutChild((mColorMenu =
										  new ArpMenuField("color_menu", "Color: ",
															mColorPopUpMenu))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
		#if 0
						->AddLayoutChild((mColorForeground =
										  new ArpTextControl(
												"ColorForeground","Foreground:","1.75",
												new BMessage(CHOOSE_FOREGROUND_MSG)))
							->SetParams(ArpMessage()
								.SetString(ArpTextControl::MinTextStringP, "8.88")
								.SetString(ArpTextControl::PrefTextStringP, "8.88")
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						->AddLayoutChild((mColorBackground =
										  new ArpTextControl(
												"ColorBackground","Background:",".25",
												new BMessage(CHOOSE_BACKGROUND_MSG)))
							->SetParams(ArpMessage()
								.SetString(ArpTextControl::MinTextStringP, "8.88")
								.SetString(ArpTextControl::PrefTextStringP, "8.88")
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
		#endif
					)
					->AddLayoutChild((new ArpViewWrapper( mColorPalette = 
							new BColorControl(BPoint(0,0),B_CELLS_32x8,
												8,"color_palette",
												new BMessage(COLOR_PALETTE_MSG)) ))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpNorth)
						)
					)
					->AddLayoutChild((new ArpRunningBar("FontHBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((mFontMenu =
										  new ArpMenuField("font_menu", "Font: ",
															mFontPopUpMenu))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						->AddLayoutChild((mFontSizeText =
										  new ArpTextControl(
												"FontSize","Size:","10",
												new BMessage(CHOOSE_FONTSIZE_MSG)))
							->SetParams(ArpMessage()
								.SetString(ArpTextControl::MinTextStringP, "888")
								.SetString(ArpTextControl::PrefTextStringP, "888")
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						#if 0
						// Well, it looks like BStringView is yet another
						// control that doesn't return useful preferred
						// size information.  We can never have too many
						// wrapper classes, I suppose. :p
						->AddLayoutChild((new ArpViewWrapper(
								new BStringView(BRect(0,0,100,100), "PtsText",
												"pts", B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						#endif
						->AddLayoutChild((mEncodingMenu =
										  new ArpMenuField("encoding_menu", "",
															mEncodingPopUpMenu))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
					)
				)
			)
			->AddLayoutChild((new ArpRunningBar("OptsHBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
				)
				->AddLayoutChild((new ArpBox("ModeBox","Mode"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpRunningBar("OptsVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							//.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((new ArpViewWrapper(mCRAsCRLFCheck =
								new BCheckBox(BRect(0,0,10,10), "LFAsCRLF",
												"Receive LF as CRLF",
												new BMessage(CHOOSE_CRASCRLF_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mEnterAsCRLFCheck =
								new BCheckBox(BRect(0,0,10,10), "EnterAsCRLF",
												"Send Enter as CRLF",
												new BMessage(CHOOSE_ENTERASCRLF_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mSwapBSCheck =
								new BCheckBox(BRect(0,0,10,10), "SwapBS",
												"Swap Backspace/Del",
												new BMessage(CHOOSE_SWAPBS_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mWrapCheck =
								new BCheckBox(BRect(0,0,10,10), "Wrap",
												"Auto Wrap Cursor",
												new BMessage(CHOOSE_WRAP_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mInverseCheck =
								new BCheckBox(BRect(0,0,10,10), "Inverse",
												"Inverse Video",
												new BMessage(CHOOSE_INVERSE_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
				)
				->AddLayoutChild((new ArpBox("InteractionBox","Interaction"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpRunningBar("OptsVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							//.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((new ArpViewWrapper(mVerifyPasteCheck =
								new BCheckBox(BRect(0,0,10,10), "VerifyPaste",
												"Verify Paste",
												new BMessage(CHOOSE_VERIFYPASTE_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mQuickPasteCheck =
								new BCheckBox(BRect(0,0,10,10), "QuickPaste",
												"Quick Paste",
												new BMessage(CHOOSE_QUICKPASTE_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mScrollInCheck =
								new BCheckBox(BRect(0,0,10,10), "ScrollIn",
												"Scroll on Input",
												new BMessage(CHOOSE_SCROLLIN_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mScrollOutCheck =
								new BCheckBox(BRect(0,0,10,10), "ScrollOut",
												"Scroll on Output",
												new BMessage(CHOOSE_SCROLLOUT_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("MiscVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
					)
					->AddLayoutChild((mHistoryText =
									  new ArpTextControl(
											"History","History Lines:","1000",
											new BMessage(CHOOSE_HISTORYSIZE_MSG)))
						->SetParams(ArpMessage()
							.SetString(ArpTextControl::MinTextStringP, "88")
							.SetString(ArpTextControl::PrefTextStringP, "8888")
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((mHistoryMenu =
									  new ArpMenuField("history_menu", "Use: ",
														mHistoryPopUpMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpLayout("Spacer"))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
					)
					->AddLayoutChild((new ArpRunningBar("ResetVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpCenter)
						)
						->AddLayoutChild((mSoftResetButton =
										new ArpButton("SoftReset","Soft Reset",
											new BMessage(SOFT_RESET_MSG)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						->AddLayoutChild((mHardResetButton =
										new ArpButton("HardReset","Hard Reset",
											new BMessage(HARD_RESET_MSG)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
					)
				)
			)
		);
#if 0
		AddLayoutChild((new ArpRunningBar("TopVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpBox("StyleBox","Style"))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
				)
				->AddLayoutChild((new ArpRunningBar("StyleVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
					)
					->AddLayoutChild((mColorMenu =
									  new ArpMenuField("color_menu", "Color: ",
														mColorPopUpMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper( mColorPalette = 
							new BColorControl(BPoint(0,0),B_CELLS_32x8,
												8,"color_palette",
												new BMessage(COLOR_PALETTE_MSG)) ))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpNorth)
						)
					)
					->AddLayoutChild((new ArpRunningBar("FontHBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((mFontMenu =
										  new ArpMenuField("font_menu", "Font: ",
															mFontPopUpMenu))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						->AddLayoutChild((mFontSizeText =
										  new ArpTextControl(
												"FontSize","Size:","10",
												new BMessage(CHOOSE_FONTSIZE_MSG)))
							->SetParams(ArpMessage()
								.SetString(ArpTextControl::MinTextStringP, "888")
								.SetString(ArpTextControl::PrefTextStringP, "888")
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						#if 0
						// Well, it looks like BStringView is yet another
						// control that doesn't return useful preferred
						// size information.  We can never have too many
						// wrapper classes, I suppose. :p
						->AddLayoutChild((new ArpViewWrapper(
								new BStringView(BRect(0,0,100,100), "PtsText",
												"pts", B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						#endif
						->AddLayoutChild((mEncodingMenu =
										  new ArpMenuField("encoding_menu", "",
															mEncodingPopUpMenu))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
					)
				)
			)
			->AddLayoutChild((new ArpRunningBar("OptsHBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
				)
				->AddLayoutChild((new ArpBox("ModeBox","Mode"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
					)
					->AddLayoutChild((new ArpRunningBar("OptsVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							//.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((new ArpViewWrapper(mCRAsCRLFCheck =
								new BCheckBox(BRect(0,0,10,10), "LFAsCRLF",
												"Receive LF as CRLF",
												new BMessage(CHOOSE_CRASCRLF_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mEnterAsCRLFCheck =
								new BCheckBox(BRect(0,0,10,10), "EnterAsCRLF",
												"Send Enter as CRLF",
												new BMessage(CHOOSE_ENTERASCRLF_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mSwapBSCheck =
								new BCheckBox(BRect(0,0,10,10), "SwapBS",
												"Swap Backspace/Del",
												new BMessage(CHOOSE_SWAPBS_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mWrapCheck =
								new BCheckBox(BRect(0,0,10,10), "Wrap",
												"Wrap Cursor",
												new BMessage(CHOOSE_WRAP_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mInverseCheck =
								new BCheckBox(BRect(0,0,10,10), "Inverse",
												"Inverse Video",
												new BMessage(CHOOSE_INVERSE_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
				)
				->AddLayoutChild((new ArpBox("InteractionBox","Interaction"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
					)
					->AddLayoutChild((new ArpRunningBar("OptsVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							//.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((new ArpViewWrapper(mVerifyPasteCheck =
								new BCheckBox(BRect(0,0,10,10), "VerifyPaste",
												"Verify Paste",
												new BMessage(CHOOSE_VERIFYPASTE_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mQuickPasteCheck =
								new BCheckBox(BRect(0,0,10,10), "QuickPaste",
												"Quick Paste",
												new BMessage(CHOOSE_QUICKPASTE_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mScrollInCheck =
								new BCheckBox(BRect(0,0,10,10), "ScrollIn",
												"Scroll on Input",
												new BMessage(CHOOSE_SCROLLIN_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(mScrollOutCheck =
								new BCheckBox(BRect(0,0,10,10), "ScrollOut",
												"Scroll on Output",
												new BMessage(CHOOSE_SCROLLOUT_MSG),
												B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("MiscVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
					)
					->AddLayoutChild((mHistoryText =
									  new ArpTextControl(
											"History","History Lines:","1000",
											new BMessage(CHOOSE_HISTORYSIZE_MSG)))
						->SetParams(ArpMessage()
							.SetString(ArpTextControl::MinTextStringP, "88")
							.SetString(ArpTextControl::PrefTextStringP, "8888")
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((mHistoryMenu =
									  new ArpMenuField("history_menu", "Use: ",
														mHistoryPopUpMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpLayoutable("Spacer"))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
					)
					->AddLayoutChild((new ArpRunningBar("ResetVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpCenter)
						)
						->AddLayoutChild((new ArpButton("SoftReset","Soft Reset",
											new BMessage(SOFT_RESET_MSG)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						->AddLayoutChild((new ArpButton("HardReset","Hard Reset",
											new BMessage(HARD_RESET_MSG)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
					)
				)
			)
		);
#endif
	} catch(...) {
		throw;
	}
}

ArpTerminalSettings::~ArpTerminalSettings()
{
}

void ArpTerminalSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	BView* par = Parent();
	rgb_color col = ui_color(B_PANEL_BACKGROUND_COLOR);
	if( par ) col = par->ViewColor();
	ArpMessage updColor;
	updColor.AddRGBColor("StdBackColor", &col);
	UpdateGlobals(&updColor);
}
