/*
	
	ArpTerminalSettings.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef _BUTTON_H
#include <interface/Button.h>
#endif

#ifndef _MENUFIELD_H
#include <interface/MenuField.h>
#endif

#ifndef _POPUPMENU_H
#include <interface/PopUpMenu.h>
#endif

#ifndef _MENUITEM_H
#include <interface/MenuItem.h>
#endif

#ifndef _COLORCONTROL_H
#include <interface/ColorControl.h>
#endif

#ifndef _STRINGVIEW_H
#include <interface/StringView.h>
#endif

#ifndef _CHECKBOX_H
#include <interface/CheckBox.h>
#endif

#ifndef _SCREEN_H
#include <interface/Screen.h>
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef _UTF8_H
#include <support/UTF8.h>
#endif

#ifndef ARPTERMINAL_ARPREMOTETERMINAL_H
#include <ArpTerminal/ArpRemoteTerminal.h>
#endif

#ifndef ARPTERMINAL_ARPTERMINALMSG_H
#include <ArpTerminal/ArpTerminalMsg.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include <ArpLayout/ViewStubs.h>
#endif

#ifndef ARPLAYOUT_ARPVIEWWRAPPER_H
#include <ArpLayout/ArpViewWrapper.h>
#endif

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include <ArpLayout/ArpRunningBar.h>
#endif

#ifndef ARPTERMINALSETTINGS_H
#include "ArpTerminalSettings.h"
#endif

#include <cstdlib>

ArpMOD();

class RTColorControl : public BColorControl {
	public:
		RTColorControl(	BPoint start,
						color_control_layout layout,
						float cell_size,
						const char *name,
						BMessage *message = NULL,
						bool use_offscreen = false)
			: BColorControl(start, layout, cell_size, name, message, use_offscreen)
		{
		}
		
		void SetValue(int32 color)
		{
			if( Value() != color ) {
				BColorControl::SetValue(color);
				Invoke();
			}
		}
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
	B_ISO1_CONVERSION,
	B_ISO2_CONVERSION,
	B_ISO3_CONVERSION,
	B_ISO4_CONVERSION,
	B_ISO5_CONVERSION,
	B_ISO6_CONVERSION,
	B_ISO7_CONVERSION,
	B_ISO8_CONVERSION,
	B_ISO9_CONVERSION,
	B_ISO10_CONVERSION,
	B_MAC_ROMAN_CONVERSION 
};

static const char* receiveNames[] = {
	"Separate",
	"LF as CRLF",
	"CR as CRLF",
	"Either",
	NULL
};

static const char* receiveLFChars[] = {
	"\n",
	"\n",
	"\n\r",
	"\n\r",
	NULL
};

static const char* receiveCRChars[] = {
	"\r",
	"\n\r",
	"\r",
	"\n\r",
	NULL
};

static const char* sendNames[] = {
	"Enter as CRLF",
	"Enter as CR",
	"Enter as LF",
	NULL
};

static const char* sendStreams[] = {
	"\r\n",
	"\r",
	"\n",
	NULL
};

static const char* histNames[] = {
	"Off",
	"Minimum",
	"Normal",
	"Aggressive",
	NULL
};
static int32 histCodes[] = {
	ArpCoreTerminal::HISTORY_NONE,
	ArpCoreTerminal::HISTORY_MINIMUM,
	ArpCoreTerminal::HISTORY_MODERATE,
	ArpCoreTerminal::HISTORY_AGGRESSIVE
};

enum {
	CHOOSE_COLOR_MSG = '.ccl',
	CHOOSE_FOREGROUND_MSG = '.cfg',
	CHOOSE_BACKGROUND_MSG = '.cbg',
	COLOR_PALETTE_MSG = '.col',
	CHOOSE_FONT_MSG = '.cfn',
	CHOOSE_FONTSIZE_MSG = '.cfs',
	CHOOSE_ENCODING_MSG = '.cen',
	CHOOSE_RECEIVE_MSG = '.rcv',
	CHOOSE_SEND_MSG = '.snd',
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
	CHOOSE_HISTORYUSE_MSG = '.chu',
	SOFT_RESET_MSG = 'srst',
	HARD_RESET_MSG = 'hrst'
};

ArpTerminalSettings::ArpTerminalSettings(
					 const BMessenger& terminal,
					 const BMessage& initSettings,
					 const BFont* font,
					 uint32 resizingMode, 
					 uint32 flags)
	: ArpRootLayout(BRect(0,0,100,100), "Terminal", resizingMode, flags),
	  mTerminal(terminal), mSettings(initSettings),
	  mColorPopUpMenu(0), mFontPopUpMenu(0), mEncodingPopUpMenu(0),
	  mColorMenu(0), mFontMenu(0), mEncodingMenu(0),
	  mColorForeground(0), mColorBackground(0),
	  mColorPalette(0), mFontSizeText(0), mHistoryText(0),
	  mHistoryPopUpMenu(0), mHistoryMenu(0),
	  mReceivePopUpMenu(0), mReceiveMenu(0),
	  mSendPopUpMenu(0), mSendMenu(0),
	  mSwapBSCheck(0), mWrapCheck(0), mInverseCheck(0),
	  mVerifyPasteCheck(0), mQuickPasteCheck(0),
	  mScrollInCheck(0), mScrollOutCheck(0),
	  mSoftResetButton(0), mHardResetButton(0)
{
	BMessage* ctrlMsg = NULL;
	int i;
	
	ArpD(cdb << ADH << "ArpTerminalSettings init vals: " << mSettings
					<< std::endl);
					
	mColorPopUpMenu = new BPopUpMenu("color_menu");
	mFontPopUpMenu = new BPopUpMenu("font_menu");
	mEncodingPopUpMenu = new BPopUpMenu("encoding_menu");
	mHistoryPopUpMenu = new BPopUpMenu("history_menu");
	mReceivePopUpMenu = new BPopUpMenu("receive_menu");
	mSendPopUpMenu = new BPopUpMenu("send_menu");
	try {
		int32 ii;
		for( i=0; (ii=ArpRemoteTerminal::ColorOrder(i)) >= 0; i++ ) {
			const char* name = ArpRemoteTerminal::ColorIndex2Name(ii);
			if( name ) {
				const char* var = ArpRemoteTerminal::ColorIndex2Var(ii);
				bool bg;
				ArpTerminalInterface::TermColorID col
					= ArpRemoteTerminal::ColorIndex2ID(ii, &bg);
				ctrlMsg = new BMessage(CHOOSE_COLOR_MSG);
				ctrlMsg->AddString("color_name", name);
				ctrlMsg->AddString("color_var", var);
				ctrlMsg->AddInt32("color_id", col);
				ctrlMsg->AddBool("color_background", bg);
				BMenuItem* item = new BMenuItem(name, ctrlMsg);
				if( i == 0 ) item->SetMarked(true);
				mColorPopUpMenu->AddItem(item);
				ctrlMsg = NULL;
			} else {
				mColorPopUpMenu->AddSeparatorItem();
			}
		}
		
		int32 num = count_font_families();
		for( i=0; i<num; i++ ) {
			font_family name;
			uint32 flags = 0;
			bool first=true;
			if( get_font_family(i,&name,&flags) == B_NO_ERROR ) {
				if( flags&B_IS_FIXED ) {
					ctrlMsg = new BMessage(CHOOSE_FONT_MSG);
					ctrlMsg->AddString("name",&name[0]);
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
			ctrlMsg->AddString("name", encNames[i]);
			ctrlMsg->AddInt32("value", encCodes[i]);
			BMenuItem* item = new BMenuItem(encNames[i], ctrlMsg);
			if( i == 0 ) item->SetMarked(true);
			mEncodingPopUpMenu->AddItem(item);
			ctrlMsg = NULL;
		}
		
		for( i=0; receiveNames[i] != NULL; i++ ) {
			ctrlMsg = new BMessage(CHOOSE_RECEIVE_MSG);
			ctrlMsg->AddString("name", receiveNames[i]);
			ctrlMsg->AddString("lf", receiveLFChars[i]);
			ctrlMsg->AddString("cr", receiveCRChars[i]);
			BMenuItem* item = new BMenuItem(receiveNames[i], ctrlMsg);
			if( i == 0 ) item->SetMarked(true);
			mReceivePopUpMenu->AddItem(item);
			ctrlMsg = NULL;
		}
		
		for( i=0; sendNames[i] != NULL; i++ ) {
			ctrlMsg = new BMessage(CHOOSE_SEND_MSG);
			ctrlMsg->AddString("name", sendNames[i]);
			ctrlMsg->AddString("stream", sendStreams[i]);
			BMenuItem* item = new BMenuItem(sendNames[i], ctrlMsg);
			if( i == 0 ) item->SetMarked(true);
			mSendPopUpMenu->AddItem(item);
			ctrlMsg = NULL;
		}
		
		for( i=0; histNames[i] != NULL; i++ ) {
			ctrlMsg = new BMessage(CHOOSE_HISTORYUSE_MSG);
			ctrlMsg->AddString("name", histNames[i]);
			ctrlMsg->AddInt32("value", histCodes[i]);
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
							new RTColorControl(BPoint(0,0),B_CELLS_32x8,
												8,"color_palette",
												new BMessage(COLOR_PALETTE_MSG)) ))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpNorth)
							.SetBool(ArpRunningBar::AlignLabelsC,false)
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
						->AddLayoutChild((new ArpViewWrapper(
								new BStringView(BRect(0,0,100,100), "PtsText",
												" pts  ", B_FOLLOW_NONE,
												B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE) ))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
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
					.SetBool(ArpRunningBar::AlignLabelsC,false)
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
						->AddLayoutChild((new ArpRunningBar("TermsVBar"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetBool(ArpRunningBar::AlignLabelsC,false)
							)
							->AddLayoutChild((mReceiveMenu =
											  new ArpMenuField("receive_menu", "Receive: ",
																mReceivePopUpMenu))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
							->AddLayoutChild((mSendMenu =
											  new ArpMenuField("send_menu", "Send: ",
																mSendPopUpMenu))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
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
							.SetBool(ArpRunningBar::AlignLabelsC,false)
						)
					)
					->AddLayoutChild((mHistoryMenu =
									  new ArpMenuField("history_menu", "Use: ",
														mHistoryPopUpMenu))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							.SetBool(ArpRunningBar::AlignLabelsC,false)
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
							.SetFloat(ArpRunningBar::InsetTopP, .5)
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
	} catch(...) {
		throw;
	}
	
	ShowCurrentColor(mSettings);
	ShowCurrentFont(mSettings);
	ShowCurrentEncoding(mSettings);
	ShowCurrentMode(mSettings);
	ShowCurrentVerifyPaste(mSettings);
	ShowCurrentQuickPaste(mSettings);
	ShowCurrentAutoScroll(mSettings);
	ShowCurrentHistorySize(mSettings);
	ShowCurrentHistoryUse(mSettings);
}

ArpTerminalSettings::~ArpTerminalSettings()
{
}

void ArpTerminalSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	BView* par = Parent();
	ArpColor col = ui_color(B_PANEL_BACKGROUND_COLOR);
	if( par ) col = par->ViewColor();
	ArpMessage updColor;
	updColor.AddRGBColor("StdBackColor", col);
	UpdateGlobals(&updColor);
	
	if( mColorPopUpMenu ) mColorPopUpMenu->SetTargetForItems(this);
	if( mFontPopUpMenu ) mFontPopUpMenu->SetTargetForItems(this);
	if( mEncodingPopUpMenu ) mEncodingPopUpMenu->SetTargetForItems(this);
	if( mColorForeground ) mColorForeground->SetTarget(this);
	if( mColorBackground ) mColorBackground->SetTarget(this);
	if( mColorPalette ) mColorPalette->SetTarget(this);
	if( mFontSizeText ) mFontSizeText->SetTarget(this);
	if( mHistoryText ) mHistoryText->SetTarget(this);
	if( mHistoryPopUpMenu ) mHistoryPopUpMenu->SetTargetForItems(this);
	if( mReceivePopUpMenu ) mReceivePopUpMenu->SetTargetForItems(this);
	if( mSendPopUpMenu ) mSendPopUpMenu->SetTargetForItems(this);
	if( mSwapBSCheck ) mSwapBSCheck->SetTarget(this);
	if( mWrapCheck ) mWrapCheck->SetTarget(this);
	if( mInverseCheck ) mInverseCheck->SetTarget(this);
	if( mVerifyPasteCheck ) mVerifyPasteCheck->SetTarget(this);
	if( mQuickPasteCheck ) mQuickPasteCheck->SetTarget(this);
	if( mScrollInCheck ) mScrollInCheck->SetTarget(this);
	if( mScrollOutCheck ) mScrollOutCheck->SetTarget(this);
	if( mSoftResetButton ) mSoftResetButton->SetTarget(this);
	if( mHardResetButton ) mHardResetButton->SetTarget(this);
	
	BMessenger w(this);
	BMessage add_watch(TERM_ADD_WATCHER_MSG);
	add_watch.AddMessenger("watch", w);
	mTerminal.SendMessage(&add_watch);
}

void ArpTerminalSettings::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	
	BMessenger w(this);
	BMessage rem_watch(TERM_REM_WATCHER_MSG);
	rem_watch.AddMessenger("watch", w);
	mTerminal.SendMessage(&rem_watch);
}

void ArpTerminalSettings::ShowCurrentColor(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentColor()" << std::endl);
	if( mColorPopUpMenu ) {
		BMenuItem* item = mColorPopUpMenu->FindMarked();
		BMessage* msg = item ? item->Message() : 0;
		if( msg && mColorPalette ) {
			ArpD(cdb << ADH << "Color menu message: " << *msg << std::endl);
			const char* cvar=0;
			if( msg->FindString("color_var", &cvar) == B_OK ) {
				ArpD(cdb << ADH << "Looking for color var: " << cvar << std::endl);
				rgb_color col = { 0,0,0 };
				if( settings.FindRGBColor(cvar,&col) == B_OK ) {
					ArpD(cdb << ADH << "Color is: " << col << std::endl);
					mColorPalette->SetValue(col);
				}
				return;
			}
		}
	}
}

void ArpTerminalSettings::ChangeCurrentColor(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentColor()" << std::endl);
	if( mColorPopUpMenu ) {
		BMenuItem* item = mColorPopUpMenu->FindMarked();
		BMessage* msg = item ? item->Message() : 0;
		if( msg && mColorPalette ) {
			ArpD(cdb << ADH << "Color menu message: " << *msg << std::endl);
			const char* cvar=0;
			if( msg->FindString("color_var", &cvar) == B_OK ) {
				ArpD(cdb << ADH << "Adding color var: " << cvar << std::endl);
				rgb_color col = mColorPalette->ValueAsColor();
				ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
				ArpMessage settings;
				settings.AddRGBColor(cvar, &col);
				update.AddMessage("settings", &settings);
				mTerminal.SendMessage(&update);
				mSettings.Update(settings);
			}
		}
	}
}

void ArpTerminalSettings::ChangeStandardColor(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeStandardColor()" << std::endl);
	ArpMessage values;
	values.what = 0;
	
	if( mColorForeground ) {
		ArpString txt(mColorForeground->Text());
		bool valid = false;
		double val = txt.AsDouble(&valid);
		if( valid ) {
			values.what = ARP_PUT_CONFIGURATION_MSG;
			values.AddFloat("Tint Foreground", float(val));
		}
	}
	if( mColorBackground ) {
		ArpString txt(mColorBackground->Text());
		bool valid = false;
		double val = txt.AsDouble(&valid);
		if( valid ) {
			values.what = ARP_PUT_CONFIGURATION_MSG;
			values.AddFloat("Tint Background", float(val));
		}
	}

	if( values.what != 0 ) {
		ArpMessage update;
		update.what = values.what;
		update.AddMessage("settings", &values);
		mTerminal.SendMessage(&update);
		mSettings.Update(values);
	}
}

void ArpTerminalSettings::ShowCurrentFont(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentFont()" << std::endl);
	BFont font;
	if( settings.FindFont(ArpRemoteTerminal::PlainFontConfigName,
							&font) == B_OK ) {
		if( mFontPopUpMenu ) {
			font_family family;
			font_style style;
			font.GetFamilyAndStyle(&family, &style);
			ArpD(cdb << ADH << "Showing family: " << family << std::endl);
			for( int32 i=0; i<mFontPopUpMenu->CountItems(); i++ ) {
				BMenuItem* item = mFontPopUpMenu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				if( msg ) {
					const char* name;
					if( msg->FindString("name", &name) == B_OK ) {
						if( strcmp(family, name) == 0 ) {
							ArpD(cdb << ADH << "Marking: " << name << std::endl);
							item->SetMarked(true);
						} else {
							ArpD(cdb << ADH << "Unmarking: " << name << std::endl);
							item->SetMarked(false);
						}
					}
				}
			}
		}
		if( mFontSizeText ) {
			ArpString sizeStr(int32(font.Size()));
			mFontSizeText->SetText(sizeStr);
		}
	}
}

void ArpTerminalSettings::ChangeCurrentFont(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentFont()" << std::endl);
	BFont font;
	if( mSettings.FindFont(ArpRemoteTerminal::PlainFontConfigName,
							&font) == B_OK ) {
		if( mFontPopUpMenu ) {
			BMenuItem* item = mFontPopUpMenu->FindMarked();
			BMessage* msg = item ? item->Message() : 0;
			if( msg ) {
				ArpD(cdb << ADH << "Found marked item: " << item->Label() << std::endl);
				const char* name;
				if( msg->FindString("name", &name) == B_OK ) {
					font_family family;
					family[0] = 0;
					strncat(family, name, sizeof(family));
					ArpD(cdb << ADH << "Setting family to: " << family << std::endl);
					font.SetFamilyAndFace(family, B_REGULAR_FACE);
				}
			}
		}
		if( mFontSizeText ) {
			ArpString sizeStr(mFontSizeText->Text());
			bool valid = false;
			int32 size = sizeStr.AsInt(10,&valid);
			if( valid ) font.SetSize(size);
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddFont(ArpRemoteTerminal::PlainFontConfigName, &font);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentEncoding(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentEncoding()" << std::endl);
	int32 encoding;
	if( settings.FindInt32(ArpRemoteTerminal::EncodingConfigName,
							&encoding) == B_OK ) {
		if( mEncodingPopUpMenu ) {
			for( int32 i=0; i<mEncodingPopUpMenu->CountItems(); i++ ) {
				BMenuItem* item = mEncodingPopUpMenu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				if( msg ) {
					int32 value;
					if( msg->FindInt32("value", &value) == B_OK ) {
						if( value == encoding ) {
							ArpD(cdb << ADH << "Marking: " << value << std::endl);
							item->SetMarked(true);
						} else {
							ArpD(cdb << ADH << "Unmarking: " << value << std::endl);
							item->SetMarked(false);
						}
					}
				}
			}
		}
	}
}

void ArpTerminalSettings::ChangeCurrentEncoding(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentEncoding()" << std::endl);
	int32 encoding;
	if( mSettings.FindInt32(ArpRemoteTerminal::EncodingConfigName,
							&encoding) == B_OK ) {
		if( mEncodingPopUpMenu ) {
			BMenuItem* item = mEncodingPopUpMenu->FindMarked();
			BMessage* msg = item ? item->Message() : 0;
			if( msg ) {
				ArpD(cdb << ADH << "Found marked item: " << item->Label() << std::endl);
				int32 value;
				if( msg->FindInt32("value", &value) == B_OK ) {
					encoding = value;
				}
			}
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddInt32(ArpRemoteTerminal::EncodingConfigName, encoding);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentReceive(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentReceive()" << std::endl);
	const char* lf=0;
	const char* cr=0;
	if( settings.FindString(ArpRemoteTerminal::LFCharsConfigName,
							&lf) == B_OK &&
		settings.FindString(ArpRemoteTerminal::CRCharsConfigName,
							&cr) == B_OK ) {
		if( mReceivePopUpMenu ) {
			for( int32 i=0; i<mReceivePopUpMenu->CountItems(); i++ ) {
				BMenuItem* item = mReceivePopUpMenu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				if( msg ) {
					const char* lfit;
					const char* crit;
					if( msg->FindString("lf", &lfit) == B_OK &&
							msg->FindString("cr", &crit) == B_OK ) {
						if( strcmp(lf,lfit) == 0 && strcmp(cr,crit) == 0 ) {
							ArpD(cdb << ADH << "Marking: " << i << std::endl);
							item->SetMarked(true);
						} else {
							ArpD(cdb << ADH << "Unmarking: " << i << std::endl);
							item->SetMarked(false);
						}
					}
				}
			}
		}
	}
}

void ArpTerminalSettings::ChangeCurrentReceive(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentReceive()" << std::endl);
	const char* lf=0;
	const char* cr=0;
	if( mSettings.FindString(ArpRemoteTerminal::LFCharsConfigName,
							&lf) == B_OK &&
		mSettings.FindString(ArpRemoteTerminal::CRCharsConfigName,
							&cr) == B_OK ) {
		if( mReceivePopUpMenu ) {
			BMenuItem* item = mReceivePopUpMenu->FindMarked();
			BMessage* msg = item ? item->Message() : 0;
			if( msg ) {
				ArpD(cdb << ADH << "Found marked item: " << item->Label() << std::endl);
				const char* lfit;
				const char* crit;
				if( msg->FindString("lf", &lfit) == B_OK &&
						msg->FindString("cr", &crit) == B_OK ) {
					lf = lfit;
					cr = crit;
				}
			}
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddString(ArpRemoteTerminal::LFCharsConfigName, lf);
		settings.AddString(ArpRemoteTerminal::CRCharsConfigName, cr);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentSend(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentSend()" << std::endl);
	const char* enter=0;
	if( settings.FindString(ArpRemoteTerminal::EnterStreamConfigName,
							&enter) == B_OK ) {
		if( mSendPopUpMenu ) {
			for( int32 i=0; i<mSendPopUpMenu->CountItems(); i++ ) {
				BMenuItem* item = mSendPopUpMenu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				if( msg ) {
					const char* value;
					if( msg->FindString("stream", &value) == B_OK ) {
						if( strcmp(enter,value) == 0 ) {
							ArpD(cdb << ADH << "Marking: " << i << std::endl);
							item->SetMarked(true);
						} else {
							ArpD(cdb << ADH << "Unmarking: " << i << std::endl);
							item->SetMarked(false);
						}
					}
				}
			}
		}
	}
}

void ArpTerminalSettings::ChangeCurrentSend(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentSend()" << std::endl);
	const char* enter=0;
	if( mSettings.FindString(ArpRemoteTerminal::EnterStreamConfigName,
							&enter) == B_OK ) {
		if( mSendPopUpMenu ) {
			BMenuItem* item = mSendPopUpMenu->FindMarked();
			BMessage* msg = item ? item->Message() : 0;
			if( msg ) {
				ArpD(cdb << ADH << "Found marked item: " << item->Label() << std::endl);
				const char* value;
				if( msg->FindString("stream", &value) == B_OK ) {
					enter = value;
				}
			}
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddString(ArpRemoteTerminal::EnterStreamConfigName, enter);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentMode(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentMode()" << std::endl);
	int32 mode;
	if( settings.FindInt32(ArpRemoteTerminal::ModeConfigName,
							&mode) == B_OK ) {
		if( mSwapBSCheck ) {
			mSwapBSCheck->SetValue(
				(mode&ArpTerminalInterface::TERM_MODESWAPBSDEL)
				? B_CONTROL_ON : B_CONTROL_OFF );
		}
		if( mWrapCheck ) {
			mWrapCheck->SetValue(
				(mode&ArpTerminalInterface::TERM_MODENOWRAP)
				? B_CONTROL_OFF : B_CONTROL_ON );
		}
		if( mInverseCheck ) {
			mInverseCheck->SetValue(
				(mode&ArpTerminalInterface::TERM_MODEINVERSE)
				? B_CONTROL_ON : B_CONTROL_OFF );
		}
	}
}

void ArpTerminalSettings::ChangeCurrentMode(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentMode()" << std::endl);
	int32 mode;
	if( mSettings.FindInt32(ArpRemoteTerminal::ModeConfigName,
							&mode) == B_OK ) {
		if( mSwapBSCheck ) {
			mode = (mode&~ArpTerminalInterface::TERM_MODESWAPBSDEL)
				| (mSwapBSCheck->Value() ?
					ArpTerminalInterface::TERM_MODESWAPBSDEL : 0);
		}
		if( mWrapCheck ) {
			mode = (mode&~ArpTerminalInterface::TERM_MODENOWRAP)
				| (mWrapCheck->Value() ?
					0 : ArpTerminalInterface::TERM_MODENOWRAP);
		}
		if( mInverseCheck ) {
			mode = (mode&~ArpTerminalInterface::TERM_MODEINVERSE)
				| (mInverseCheck->Value() ?
					ArpTerminalInterface::TERM_MODEINVERSE : 0);
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddInt32(ArpRemoteTerminal::ModeConfigName, mode);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentVerifyPaste(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentVerifyPaste()" << std::endl);
	bool state;
	if( settings.FindBool(ArpRemoteTerminal::VerifyPasteConfigName,
							&state) == B_OK ) {
		if( mVerifyPasteCheck ) {
			mVerifyPasteCheck->SetValue(state ? B_CONTROL_ON : B_CONTROL_OFF);
		}
	}
}

void ArpTerminalSettings::ChangeCurrentVerifyPaste(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentVerifyPaste()" << std::endl);
	bool state;
	if( mSettings.FindBool(ArpRemoteTerminal::VerifyPasteConfigName,
							&state) == B_OK ) {
		if( mVerifyPasteCheck ) {
			state = mVerifyPasteCheck->Value() ? true : false;
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddBool(ArpRemoteTerminal::VerifyPasteConfigName, state);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentQuickPaste(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentQuickPaste()" << std::endl);
	bool state;
	if( settings.FindBool(ArpRemoteTerminal::RMBPasteConfigName,
							&state) == B_OK ) {
		if( mQuickPasteCheck ) {
			mQuickPasteCheck->SetValue(state ? B_CONTROL_ON : B_CONTROL_OFF);
		}
	}
}

void ArpTerminalSettings::ChangeCurrentQuickPaste(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentQuickPaste()" << std::endl);
	bool state;
	if( mSettings.FindBool(ArpRemoteTerminal::RMBPasteConfigName,
							&state) == B_OK ) {
		if( mQuickPasteCheck ) {
			state = mQuickPasteCheck->Value() ? true : false;
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddBool(ArpRemoteTerminal::RMBPasteConfigName, state);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentAutoScroll(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentAutoScroll()" << std::endl);
	int32 mode;
	if( settings.FindInt32(ArpRemoteTerminal::AutoScrollConfigName,
							&mode) == B_OK ) {
		if( mScrollInCheck ) {
			mScrollInCheck->SetValue(
				(mode&ArpCoreTerminal::AUTOSCROLL_INPUT)
				? B_CONTROL_ON : B_CONTROL_OFF );
		}
		if( mScrollOutCheck ) {
			mScrollOutCheck->SetValue(
				(mode&ArpCoreTerminal::AUTOSCROLL_OUTPUT)
				? B_CONTROL_ON : B_CONTROL_OFF );
		}
	}
}

void ArpTerminalSettings::ChangeCurrentAutoScroll(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentAutoScroll()" << std::endl);
	int32 mode;
	if( mSettings.FindInt32(ArpRemoteTerminal::AutoScrollConfigName,
							&mode) == B_OK ) {
		if( mScrollInCheck ) {
			mode = (mode&~ArpCoreTerminal::AUTOSCROLL_INPUT)
				| (mScrollInCheck->Value() ?
					ArpCoreTerminal::AUTOSCROLL_INPUT : 0);
		}
		if( mScrollOutCheck ) {
			mode = (mode&~ArpCoreTerminal::AUTOSCROLL_OUTPUT)
				| (mScrollOutCheck->Value() ?
					ArpCoreTerminal::AUTOSCROLL_OUTPUT : 0);
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddInt32(ArpRemoteTerminal::AutoScrollConfigName, mode);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentHistorySize(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentHistorySize()" << std::endl);
	int32 size;
	if( settings.FindInt32(ArpRemoteTerminal::HistorySizeConfigName,
							&size) == B_OK ) {
		if( mHistoryText ) {
			ArpString sizeStr(size);
			mHistoryText->SetText(sizeStr);
		}
	}
}

void ArpTerminalSettings::ChangeCurrentHistorySize(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentHistorySize()" << std::endl);
	int32 size;
	if( mSettings.FindInt32(ArpRemoteTerminal::HistorySizeConfigName,
							&size) == B_OK ) {
		if( mHistoryText ) {
			ArpString sizeStr(mHistoryText->Text());
			bool valid = false;
			size = sizeStr.AsInt(10,&valid);
			if( !valid ) return;
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddInt32(ArpRemoteTerminal::HistorySizeConfigName, size);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::ShowCurrentHistoryUse(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ShowCurrentHistoryUse()" << std::endl);
	int32 use;
	if( settings.FindInt32(ArpRemoteTerminal::HistoryUseConfigName,
							&use) == B_OK ) {
		if( mHistoryPopUpMenu ) {
			for( int32 i=0; i<mHistoryPopUpMenu->CountItems(); i++ ) {
				BMenuItem* item = mHistoryPopUpMenu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				if( msg ) {
					int32 value;
					if( msg->FindInt32("value", &value) == B_OK ) {
						if( value == use ) {
							ArpD(cdb << ADH << "Marking: " << value << std::endl);
							item->SetMarked(true);
						} else {
							ArpD(cdb << ADH << "Unmarking: " << value << std::endl);
							item->SetMarked(false);
						}
					}
				}
			}
		}
	}
}

void ArpTerminalSettings::ChangeCurrentHistoryUse(void)
{
	ArpD(cdb << ADH << "ArpTerminalSettings::ChangeCurrentHistoryUse()" << std::endl);
	int32 use;
	if( mSettings.FindInt32(ArpRemoteTerminal::HistoryUseConfigName,
							&use) == B_OK ) {
		if( mHistoryPopUpMenu ) {
			BMenuItem* item = mHistoryPopUpMenu->FindMarked();
			BMessage* msg = item ? item->Message() : 0;
			if( msg ) {
				ArpD(cdb << ADH << "Found marked item: " << item->Label() << std::endl);
				int32 value;
				if( msg->FindInt32("value", &value) == B_OK ) {
					use = value;
				}
			}
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddInt32(ArpRemoteTerminal::HistoryUseConfigName, use);
		update.AddMessage("settings", &settings);
		mTerminal.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTerminalSettings::MessageReceived(BMessage *message)
{
	if( message ) {
		ArpD(cdb << ADH << "ArpTerminalSettings message: " << *message << std::endl);
		switch(message->what) {
			case CHOOSE_COLOR_MSG:
				ShowCurrentColor(mSettings);
				break;
			case COLOR_PALETTE_MSG:
				ChangeCurrentColor();
				break;
			case CHOOSE_FOREGROUND_MSG:
			case CHOOSE_BACKGROUND_MSG:
				ChangeStandardColor();
				break;
			case CHOOSE_FONT_MSG:
			case CHOOSE_FONTSIZE_MSG:
				ChangeCurrentFont();
				break;
			case CHOOSE_ENCODING_MSG:
				ChangeCurrentEncoding();
				break;
			case CHOOSE_RECEIVE_MSG:
				ChangeCurrentReceive();
				break;
			case CHOOSE_SEND_MSG:
				ChangeCurrentSend();
				break;
			case CHOOSE_SWAPBS_MSG:
			case CHOOSE_WRAP_MSG:
			case CHOOSE_INVERSE_MSG:
				ChangeCurrentMode();
				break;
			case CHOOSE_VERIFYPASTE_MSG:
				ChangeCurrentVerifyPaste();
				break;
			case CHOOSE_QUICKPASTE_MSG:
				ChangeCurrentQuickPaste();
				break;
			case CHOOSE_SCROLLIN_MSG:
			case CHOOSE_SCROLLOUT_MSG:
				ChangeCurrentAutoScroll();
				break;
			case CHOOSE_HISTORYSIZE_MSG:
				ChangeCurrentHistorySize();
				break;
			case CHOOSE_HISTORYUSE_MSG:
				ChangeCurrentHistoryUse();
				break;
			case SOFT_RESET_MSG: {
				ArpMessage reset(TERM_RESET_MSG);
				reset.AddBool("hard", false);
				mTerminal.SendMessage(&reset);
			} break;
			case HARD_RESET_MSG: {
				ArpMessage reset(TERM_RESET_MSG);
				reset.AddBool("hard", true);
				mTerminal.SendMessage(&reset);
			} break;
			case ARP_PUT_CONFIGURATION_MSG: {
				ArpD(cdb << ADH << "*** Terminal configuration changed." << std::endl);
				ArpMessage settings;
				if( message->FindMessage("settings", &settings) == B_OK ) {
					mSettings.Update(settings);
					ArpD(cdb << ADH << "New settings: " << mSettings << std::endl);
					ShowCurrentColor(settings);
					ShowCurrentFont(settings);
					ShowCurrentEncoding(settings);
					ShowCurrentReceive(settings);
					ShowCurrentSend(settings);
					ShowCurrentMode(settings);
					ShowCurrentVerifyPaste(settings);
					ShowCurrentQuickPaste(settings);
					ShowCurrentAutoScroll(settings);
					ShowCurrentHistorySize(settings);
					ShowCurrentHistoryUse(settings);
				}
			} break;
			default:
				inherited::MessageReceived(message);
		}
	}
}
