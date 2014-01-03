/*
	
	ArpTelnetSettings.cpp
	
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

#ifndef ARPNETWORK_ARPTELNET_H
#include <ArpNetwork/ArpTelnet.h>
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include <ArpLayout/ViewStubs.h>
#endif

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include <ArpLayout/ArpRunningBar.h>
#endif

#ifndef ARPTELNETSETTINGS_H
#include "ArpTelnetSettings.h"
#endif

#include <stdlib.h>

ArpMOD();

ArpTelnetSettings::ArpTelnetSettings(
					 const BMessenger& telnet,
					 const BMessage& initSettings,
					 const BFont* font,
					 uint32 resizingMode, 
					 uint32 flags)
	: ArpRootLayout(BRect(0,0,100,100), "Telnet", resizingMode, flags),
	  mTelnet(telnet), mSettings(initSettings),
	  mImpl(telnet, this, mSettings)
{
	ArpD(cdb << ADH << "ArpTelnetSettings init vals: " << mSettings
					<< endl);
					
	try {
		AddLayoutChild((new ArpRunningBar("TopHBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpTextControl(
									ArpTelnet::HostConfigName,"Host:","",
									mImpl.AttachTextControl(ArpTelnet::HostConfigName)))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "WWWWWWW")
					.SetString(ArpTextControl::PrefTextStringP, "WWWWWWWWWWWWW")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpTextControl(
									ArpTelnet::PortConfigName,"Port:","23",
									mImpl.AttachTextControl(ArpTelnet::PortConfigName)))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "88")
					.SetString(ArpTextControl::PrefTextStringP, "88888")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);
	} catch(...) {
		throw;
	}
	
	mImpl.RefreshControls(mSettings);
}

ArpTelnetSettings::~ArpTelnetSettings()
{
}

void ArpTelnetSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	BView* par = Parent();
	ArpColor col = ui_color(B_PANEL_BACKGROUND_COLOR);
	if( par ) col = par->ViewColor();
	ArpMessage updColor;
	updColor.AddRGBColor("StdBackColor", col);
	UpdateGlobals(&updColor);
	
	mImpl.AttachedToWindow();
}

void ArpTelnetSettings::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mImpl.DetachedFromWindow();
}

#if 0
void ArpTelnetSettings::ShowCurrentHost(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTelnetSettings::ShowCurrentHost()" << endl);
	const char* host = 0;
	if( settings.FindString(ArpTelnet::HostConfigName,
							&host) == B_OK ) {
		if( mHostText ) {
			mHostText->SetText(host);
		}
	}
}

void ArpTelnetSettings::ChangeCurrentHost(void)
{
	ArpD(cdb << ADH << "ArpTelnetSettings::ChangeCurrentHost()" << endl);
	const char* host;
	if( mSettings.FindString(ArpTelnet::HostConfigName,
							&host) == B_OK ) {
		if( mHostText ) {
			host = mHostText->Text();
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddString(ArpTelnet::HostConfigName, host);
		update.AddMessage("settings", &settings);
		mTelnet.SendMessage(&update);
		mSettings.Update(settings);
	}
}

void ArpTelnetSettings::ShowCurrentPort(const ArpMessage& settings)
{
	ArpD(cdb << ADH << "ArpTelnetSettings::ShowCurrentPort()" << endl);
	int32 port = 23;
	if( settings.FindInt32(ArpTelnet::PortConfigName,
							&port) == B_OK ) {
		if( mPortText ) {
			ArpString portStr(port);
			mPortText->SetText(portStr);
		}
	}
}

void ArpTelnetSettings::ChangeCurrentPort(void)
{
	ArpD(cdb << ADH << "ArpTelnetSettings::ChangeCurrentPort()" << endl);
	int32 port;
	if( mSettings.FindInt32(ArpTelnet::PortConfigName,
							&port) == B_OK ) {
		if( mPortText ) {
			ArpString portStr(mPortText->Text());
			bool valid = false;
			int32 tmp = portStr.AsInt(10,&valid);
			if( valid ) port = tmp;
		}
		ArpMessage update(ARP_PUT_CONFIGURATION_MSG);
		ArpMessage settings;
		settings.AddInt32(ArpTelnet::PortConfigName, port);
		update.AddMessage("settings", &settings);
		mTelnet.SendMessage(&update);
		mSettings.Update(settings);
	}
}
#endif

void ArpTelnetSettings::MessageReceived(BMessage *message)
{
	if( mImpl.MessageReceived(message) == B_OK ) return;
	inherited::MessageReceived(message);
}
