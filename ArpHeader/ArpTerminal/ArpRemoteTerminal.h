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
 * ArpRemoteTerminal.h
 *
 * The ArpRemoteTerminal class is a subclass of ArpTerminal
 * that interacts with a BMessenger object as its data
 * source/sink.  Also implements the ArpConfigurableI interface
 * of the terminal, including a full settings view.
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
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 7/25/97:
 *	• Removed the receive buffer code, as that was old, no
 *	  longer needed, and probably doing more harm than good.
 *
 * 0.1: Created this file from the WebTerm source.
 *
 */

#ifndef ARPTERMINAL_ARPREMOTETERMINAL_H
#define ARPTERMINAL_ARPREMOTETERMINAL_H

#ifndef _MESSENGER_H
#include <app/Messenger.h>
#endif

#ifndef _MENU_ITEM_H
#include <interface/MenuItem.h>
#endif

#ifndef ARPTERMINAL_ARPTERMINAL_H
#include <ArpTerminal/ArpTerminal.h>
#endif

#ifndef ARPTERMINAL_ARPEMULATORMANAGER_H
#include <ArpTerminal/ArpEmulatorManager.h>
#endif

#ifndef ARPTERMINAL_ARPTERMINALMSG_H
#include <ArpTerminal/ArpTerminalMsg.h>
#endif

class ArpRemoteTerminal : public ArpTerminal {
private:
	typedef ArpTerminal inherited;

public:

	ArpRemoteTerminal(BRect frame, const char* name,
			 			uint32 resizeMode = B_FOLLOW_ALL_SIDES,
			 			uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE);
	virtual ~ArpRemoteTerminal();
	
	void UseEmulatorManager(ArpEmulatorManager* mgr);
	ArpEmulatorManager* EmulatorManager(void);
	
	void SetRemote(const BMessenger& dev);
	const BMessenger& Remote(void) const;
	
	ArpEmulatorInterface* SetEmulator(const char* name);
	ArpEmulatorInterface* SetEmulator(ArpEmulatorInterface* emu)
		{ return inherited::SetEmulator(emu); }
	
	/* ------------------------------------------------------------
	   DATA IN CONFIGURATION MESSAGES.
	   ------------------------------------------------------------ */

	static const char* ModeConfigName;
	static const char* LFCharsConfigName;
	static const char* CRCharsConfigName;
	static const char* EnterStreamConfigName;
	static const char* PlainFontConfigName;
	static const char* EncodingConfigName;
	static const char* StyleConfigName;
	static const char* NumRowsConfigName;
	static const char* NumColsConfigName;
	static const char* RegionTopConfigName;
	static const char* RegionBottomConfigName;
	static const char* RegionLeftConfigName;
	static const char* RegionRightConfigName;
	static const char* AutoScrollConfigName;
	static const char* HistoryUseConfigName;
	static const char* HistorySizeConfigName;
	static const char* VerifyPasteConfigName;
	static const char* RMBPasteConfigName;
	static const char* CurEmulatorConfigName;
	static const char* EmuSettingsConfigName;
	
	static const char* ColorIndex2Name(int32 idx);
	static const char* ColorIndex2Var(int32 idx);
	static TermColorID ColorIndex2ID(int32 idx, bool* isBackground=0);
	static int32 ColorOrder(int32 idx);
	static int32 ColorName2Index(const char* name);
	static int32 ColorVar2Index(const char* var);
	
	/* ------------------------------------------------------------
	   HOOKS INTO TERMINAL SUPERCLASS.
	   ------------------------------------------------------------ */

	virtual void TermSizeChanged(int32 rows, int32 cols);
	virtual void TermEmulationChanged(const char* name);

	// Send text from emulator to remote device.
	virtual void TermSendRemote(const ichar * d, size_t len);

	// Report changes to watchers.
	virtual void TermReset(bool hard);
	
	// Send text from remote device to emulator.
	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow(void);

	/* ------------------------------------------------------------
	   ARP-CONFIGURABLE-I IMPLEMENTATION.
	   ------------------------------------------------------------ */

	virtual status_t GetConfiguration(BMessage* values) const;
	virtual status_t PutConfiguration(const BMessage* values);
	virtual status_t Configure(ArpVectorI<BView*>& views);
	
protected:

	void SendAttachMsg(void);
	void SendSizeMsg(int32 rows, int32 cols);
	void SendEmulatorsMsg(void);
	
	/* ------------------------------------------------------------
	   IMPLEMENTATION SECTION
	   ------------------------------------------------------------ */

private:

	// Used for setting colors from a configuration message.
	void backcolor_from_msg(ArpMessage& msg, const char* name,
							int32 midx, TermColorID cidx);
	void forecolor_from_msg(ArpMessage& msg, const char* name,
							int32 midx, TermColorID cidx);
										 
	ArpEmulatorManager* manager;
	
	BMessenger device;
	BMessenger termhand;
	
	/* This is the list of objects (as BMessengers) that we send reports
	   of setting changes to. */
	bool HaveWatchers();		// Return true if there are any active watchers
	void AddWatcher(const BMessenger& w);
	void RemWatcher(const BMessenger& w);
	void ReportChange(const BMessage* changes, BMessenger* to = 0);
	ArpVectorI<BMessenger>* mWatchers;
	
#if 0
	ichar receiveBuffer[8192];
#endif
};

#endif
