/*
	
	ArpRemoteTerminal.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPTERMINAL_ARPREMOTETERMINAL_H
#include "ArpTerminal/ArpRemoteTerminal.h"
#endif

#ifndef ARPTERMINALSETTINGS_H
#include "ArpTerminalSettings.h"
#endif

#ifndef ARPTERMINAL_ARPEMULATOR_H
#include "ArpTerminal/ArpEmulator.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPCOLLECTIONS_ARPSTLVECTOR_H
#include <ArpCollections/ArpSTLVector.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
#endif

ArpMOD();

ArpRemoteTerminal::ArpRemoteTerminal(BRect frame, const char* name,
									uint32 resizeMode, uint32 flags)
	: ArpTerminal(frame,name,resizeMode,flags), manager(NULL),
	  mWatchers(new ArpSTLVector<BMessenger>)
{
}

ArpRemoteTerminal::~ArpRemoteTerminal()
{
	delete mWatchers;
}

void ArpRemoteTerminal::UseEmulatorManager(ArpEmulatorManager* mgr)
{
	manager = mgr;
	if( manager ) SendEmulatorsMsg();
}

ArpEmulatorManager* ArpRemoteTerminal::EmulatorManager(void)
{
	return manager;
}

void ArpRemoteTerminal::SetRemote(const BMessenger& dev)
{
	device = dev;
	if( device.IsValid() ) {
		termhand = BMessenger(this);
		ArpD(cdb << ADH << "ArpRemoteTerminal::SetRemote()" << endl);
		ArpD(cdb << ADH << "Window = " << Window()
					<< "valid = " << termhand.IsValid() << endl);
		if( termhand.IsValid() ) {
			SendAttachMsg();
			if( manager ) SendEmulatorsMsg();
		}
	}
}

const BMessenger& ArpRemoteTerminal::Remote(void) const
{
	return device;
}

ArpEmulatorInterface* ArpRemoteTerminal::SetEmulator(const char* name)
{
	if( !name || !manager ) {
		return SetEmulator((ArpEmulatorInterface*)NULL);
	}
	
	return SetEmulator(manager->AllocEmulator(name,this));
}

/*	------------------------------------------------------------
	PUBLIC CONFIGURATION INFORMATION.
	------------------------------------------------------------ */

const char* ArpRemoteTerminal::ModeConfigName = "Mode";
const char* ArpRemoteTerminal::LFCharsConfigName = "LFChars";
const char* ArpRemoteTerminal::CRCharsConfigName = "CRChars";
const char* ArpRemoteTerminal::EnterStreamConfigName = "EnterStream";
const char* ArpRemoteTerminal::PlainFontConfigName = "PlainFont";
const char* ArpRemoteTerminal::EncodingConfigName = "Encoding";
const char* ArpRemoteTerminal::StyleConfigName = "Style";
const char* ArpRemoteTerminal::NumRowsConfigName = "NumRows";
const char* ArpRemoteTerminal::NumColsConfigName = "NumCols";
const char* ArpRemoteTerminal::RegionTopConfigName = "RegionTop";
const char* ArpRemoteTerminal::RegionBottomConfigName = "RegionBottom";
const char* ArpRemoteTerminal::RegionLeftConfigName = "RegionLeft";
const char* ArpRemoteTerminal::RegionRightConfigName = "RegionRight";
const char* ArpRemoteTerminal::AutoScrollConfigName = "AutoScroll";
const char* ArpRemoteTerminal::HistoryUseConfigName = "HistoryUse";
const char* ArpRemoteTerminal::HistorySizeConfigName = "HistorySize";
const char* ArpRemoteTerminal::VerifyPasteConfigName = "VerifyPaste";
const char* ArpRemoteTerminal::RMBPasteConfigName = "RMBPaste";
const char* ArpRemoteTerminal::CurEmulatorConfigName = "CurEmulator";
const char* ArpRemoteTerminal::EmuSettingsConfigName = "EmuSettings";

enum {
	TERM_NUMCOLORS = (ArpTerminalInterface::TERM_COLOR_8+1
					  - ArpTerminalInterface::TERM_CURSORCOLOR) * 2
};

static const char* colorNames[TERM_NUMCOLORS] = {
	"Black Background",			// 0
	"Black Foreground",
	"Blue Background",			// 2
	"Blue Foreground",
	"Cursor Background",		// 4
	"Cursor Foreground",
	"Cyan Background",			// 6
	"Cyan Foreground",
	"Green Background",			// 8
	"Green Foreground",
	"Highlight Background",		// 10
	"Highlight Foreground",
	"Magenta Background",		// 12
	"Magenta Foreground",
	"Red Background",			// 14
	"Red Foreground",
	"Text Background",			// 16
	"Text Foreground",
	"White Background",			// 18
	"White Foreground",
	"Yellow Background",		// 20
	"Yellow Foreground"
};

static const char* colorVars[TERM_NUMCOLORS] = {
	"BlackBackColor",			// 0
	"BlackForeColor",
	"BlueBackColor",			// 2
	"BlueForeColor",
	"CursorBackColor",			// 4
	"CursorForeColor",
	"CyanBackColor",			// 6
	"CyanForeColor",
	"GreenBackColor",			// 8
	"GreenForeColor",
	"HighlightBackColor",		// 10
	"HighlightForeColor",
	"MagentaBackColor",			// 12
	"MagentaForeColor",
	"RedBackColor",				// 14
	"RedForeColor",
	"TextBackColor",			// 16
	"TextForeColor",
	"WhiteBackColor",			// 18
	"WhiteForeColor",
	"YellowBackColor",			// 20
	"YellowForeColor"
};

enum {
	TERM_ORDERCOLORS = TERM_NUMCOLORS+1
};

static const int32 colorOrder[TERM_ORDERCOLORS] = {
	17, 16,				// Text
	11, 10,				// Highlight
	5, 4,				// Cursor
	TERM_NUMCOLORS,		// Divider
	1, 0,				// Black
	15, 14,				// Red
	9, 8,				// Green
	21, 20,				// Yellow
	3, 2,				// Blue
	13, 12,				// Magenta
	7, 6,				// Cyan
	19, 18,				// White
};

static const ArpTerminalInterface::TermColorID colorValues[TERM_NUMCOLORS] = {
	ArpTerminalInterface::TERM_COLOR_1,			// Black
	ArpTerminalInterface::TERM_COLOR_1,
	ArpTerminalInterface::TERM_COLOR_5,			// Blue
	ArpTerminalInterface::TERM_COLOR_5,
	ArpTerminalInterface::TERM_CURSORCOLOR,
	ArpTerminalInterface::TERM_CURSORCOLOR,
	ArpTerminalInterface::TERM_COLOR_7,			// Cyan
	ArpTerminalInterface::TERM_COLOR_7,
	ArpTerminalInterface::TERM_COLOR_3,			// Green
	ArpTerminalInterface::TERM_COLOR_3,
	ArpTerminalInterface::TERM_HIGHLIGHTCOLOR,
	ArpTerminalInterface::TERM_HIGHLIGHTCOLOR,
	ArpTerminalInterface::TERM_COLOR_6,			// Magenta
	ArpTerminalInterface::TERM_COLOR_6,
	ArpTerminalInterface::TERM_COLOR_2,			// Red
	ArpTerminalInterface::TERM_COLOR_2,
	ArpTerminalInterface::TERM_NORMALCOLOR,
	ArpTerminalInterface::TERM_NORMALCOLOR,
	ArpTerminalInterface::TERM_COLOR_8,			// White
	ArpTerminalInterface::TERM_COLOR_8,
	ArpTerminalInterface::TERM_COLOR_4,			// Yellow
	ArpTerminalInterface::TERM_COLOR_4
};

const char* ArpRemoteTerminal::ColorIndex2Name(int32 idx)
{
	if( idx < 0 || idx >= TERM_NUMCOLORS ) return 0;
	return colorNames[idx];
}

const char* ArpRemoteTerminal::ColorIndex2Var(int32 idx)
{
	if( idx < 0 || idx >= TERM_NUMCOLORS ) return 0;
	return colorVars[idx];
}

int32 ArpRemoteTerminal::ColorOrder(int32 idx)
{
	if( idx < 0 || idx >= TERM_ORDERCOLORS ) return -1;
	return colorOrder[idx];
}

ArpTerminalInterface::TermColorID
ArpRemoteTerminal::ColorIndex2ID(int32 idx, bool* isBackground)
{
	if( idx < 0 || idx >= TERM_NUMCOLORS ) {
		if( isBackground ) *isBackground=false;
		return ArpTerminalInterface::TERM_NUMTEXTCOLORS;
	}
	if( isBackground ) *isBackground = (idx&1) ? false : true;
	return colorValues[idx];
}

static int32 binsearch(const char** array, int32 size, const char* text)
{
	int32 curMin=0, curMax=size-1;
	ArpD(cdb << ADH << "Bin search for " << text << endl);
	while( curMin <= curMax ) {
		int32 mid = (curMin+curMax)/2;
		ArpD(cdb << ADH << "Min=" << curMin << " Mid=" << mid
					<< " Max=" << curMax << endl);
		int32 cmp = strcmp(array[mid], text);
		ArpD(cdb << ADH << "Compare with " << array[mid] << ": " << cmp << endl);
		if( cmp > 0 ) curMax=mid-1;
		else if ( cmp < 0 ) curMin=mid+1;
		else return mid;
	}
	
	ArpD(cdb << ADH << "*** Not found!" << endl);
	
	return -1;
}

int32 ArpRemoteTerminal::ColorName2Index(const char* text)
{
	return binsearch(colorNames, TERM_NUMCOLORS, text);
}

int32 ArpRemoteTerminal::ColorVar2Index(const char* text)
{
	return binsearch(colorVars, TERM_NUMCOLORS, text);
}

/*	------------------------------------------------------------
	CONFIGURATION AND THOSE WHO WATCH IT.
	------------------------------------------------------------ */

bool ArpRemoteTerminal::HaveWatchers()
{
	const int N = mWatchers->size();
	int j=0;
	
	for( int i=0; i<N; i++ ) {
		if( mWatchers->at(i).IsValid() ) j++;
		if( j < i ) mWatchers->at(j) = mWatchers->at(i);
	}
	
	return j > 0 ? true : false;
}

void ArpRemoteTerminal::AddWatcher(const BMessenger& w)
{
	const int N = mWatchers->size();
	
	for( int i=0; i<N; i++ ) {
		if( !(mWatchers->at(i).IsValid()) ) {
			mWatchers->at(i) = w;
			return;
		}
	}
	
	mWatchers->push_back(w);
}

void ArpRemoteTerminal::RemWatcher(const BMessenger& w)
{
	const int N = mWatchers->size();
	
	for( int i=0; i<N; i++ ) {
		if( mWatchers->at(i) == w ) mWatchers->at(i) = BMessenger();
	}
	
	// Compact watch vector
	HaveWatchers();
}

void ArpRemoteTerminal::ReportChange(const BMessage* changes,
								   BMessenger* to)
{
	if( !to && !HaveWatchers() ) return;
	
	BMessage report(ARP_PUT_CONFIGURATION_MSG);
	report.AddMessage("settings", changes);
	
	if( to ) {
		to->SendMessage(&report);
		return;
	}
	
	const int N = mWatchers->size();
	
	for( int i=0; i<N; i++ ) {
		if( mWatchers->at(i).IsValid() ) {
			mWatchers->at(i).SendMessage(&report);
		}
	}
}

status_t ArpRemoteTerminal::GetConfiguration(BMessage* values) const
{
	if( !values ) return B_BAD_VALUE;
	
	// Use the ArpMessage interface for adding our values, since it
	// provides some convenience functions for working with colors
	// and fonts.
	ArpMessage msg(*values);
	int i;
	
	const bool locked = const_cast<ArpRemoteTerminal*>(this)->LockLooper();
	
	try {
		ArpD(cdb << ADH << "Getting settings; initial error = "
						<< (msg.GetError()) << endl);

		msg.SetInt32(ModeConfigName, TermMode());
		msg.SetString(LFCharsConfigName, TermLFChars());
		msg.SetString(CRCharsConfigName, TermCRChars());
		msg.SetString(EnterStreamConfigName, TermEnterStream());
		msg.SetFont(PlainFontConfigName, &GetStyledFont(TERM_STYLEPLAIN));
		msg.SetInt32(EncodingConfigName, TermEncoding());
		msg.SetInt32(StyleConfigName, TermStyle());
		const char* varName;
		for( i=0; (varName=ColorIndex2Var(i)) != 0; i++ ) {
			bool bg=false;
			TermColorID col = ColorIndex2ID(i, &bg);
			if( bg ) msg.SetRGBColor(varName, ArpColor(TermTextBackground(col)));
			else msg.SetRGBColor(varName, ArpColor(TermTextForeground(col)));
		}
		{
			int32 r=0,c=0;
			TermGetFixedSize(&r, &c);
			msg.SetInt32(NumRowsConfigName, r);
			msg.SetInt32(NumColsConfigName, c);
		}
		{
			int32 t=0, b=0, l=0, r=0;
			TermGetRegion(&t, &b, &l, &r);
			msg.SetInt32(RegionTopConfigName, t);
			msg.SetInt32(RegionBottomConfigName, b);
			msg.SetInt32(RegionLeftConfigName, l);
			msg.SetInt32(RegionRightConfigName, r);
		}
		msg.SetInt32(AutoScrollConfigName, AutoScrollMode());
		msg.SetInt32(HistoryUseConfigName, HistoryUse());
		msg.SetInt32(HistorySizeConfigName, HistorySize());
		msg.SetBool(VerifyPasteConfigName, PasteVerified());
		msg.SetBool(RMBPasteConfigName, RMBPaste());
		
		const ArpEmulationType* type = Emulator().EmulationType();
		if( type ) {
			msg.AddString(CurEmulatorConfigName,
						  type->Synonym ? type->Synonym : type->Name);
		}
		
		ArpD(cdb << ADH << "Got settings; final error = "
						<< (msg.GetError()) << endl);
					
		if( msg.GetError() == B_NO_ERROR ) {
			*values = msg;
		}
	} catch(...) {
		if( locked ) const_cast<ArpRemoteTerminal*>(this)->UnlockLooper();
		throw;
	}
	
	if( locked ) const_cast<ArpRemoteTerminal*>(this)->UnlockLooper();
	
	return msg.GetError();
}

void ArpRemoteTerminal::backcolor_from_msg(ArpMessage& msg, const char* name,
										 int32 midx, TermColorID cidx)
{
	if( msg.HasRGBColor(name, midx) )
		TermSetTextBackground(msg.GetRGBColor(name,
							  	ArpColor(TermTextBackground(cidx)), midx),
							  cidx);
}

void ArpRemoteTerminal::forecolor_from_msg(ArpMessage& msg, const char* name,
										 int32 midx, TermColorID cidx)
{
	if( msg.HasRGBColor(name, midx) )
		TermSetTextForeground(msg.GetRGBColor(name,
							  	ArpColor(TermTextForeground(cidx)), midx),
							  cidx);
}

status_t ArpRemoteTerminal::PutConfiguration(const BMessage* values)
{
	if( !values ) return B_BAD_VALUE;
	
	// Use the ArpMessage interface for extracting our values, since it
	// provides some convenience functions for working with colors
	// and fonts.
	ArpMessage msg(*values);
	int i;
	
	const bool locked = LockLooper();
	
	try {
		int32 mode = 0;
		bool hasMode = msg.HasInt32(ModeConfigName);
		if( hasMode )
			TermSetMode((mode=msg.GetInt32(ModeConfigName, TermMode())));
			
		if( msg.HasString(LFCharsConfigName) )
			TermSetLFChars(msg.GetString(LFCharsConfigName, TermLFChars()));
		else if( hasMode && (mode&1<<1) )
			TermSetLFChars("\n");
		if( msg.HasString(CRCharsConfigName) )
			TermSetCRChars(msg.GetString(CRCharsConfigName, TermCRChars()));
		else if( hasMode && (mode&1<<1) )
			TermSetCRChars("\n\r");
			
		if( msg.HasString(EnterStreamConfigName) )
			TermSetEnterStream(msg.GetString(EnterStreamConfigName, TermEnterStream()));
		else if( hasMode && (mode&1<<6) )
			TermSetEnterStream("\n");
			
		if( msg.HasFont(PlainFontConfigName) ) {
			BFont font(&GetStyledFont(TERM_STYLEPLAIN));
			if( msg.FindFont(PlainFontConfigName, &font) == B_OK ) {
				SetFont(&font);
			}
		}
		if( msg.HasInt32(EncodingConfigName) )
			TermSetEncoding(msg.GetInt32(EncodingConfigName, TermEncoding()));
		if( msg.HasInt32(StyleConfigName) )
			TermSetStyle(msg.GetInt32(StyleConfigName, TermStyle()));
		if( msg.HasFloat("Tint Foreground") ) {
			SetTintForeground(msg.GetFloat("Tint Foreground", 1.75));
		}
		if( msg.HasFloat("Tint Background") ) {
			SetTintBackground(msg.GetFloat("Tint Background", .25));
		}
		{
			char* name;
			type_code type;
			int32 count;
			for( i=0;
				 msg.GetInfo(B_RGB_COLOR_TYPE,i,&name,&type,&count)==B_OK;
				 i++ ) {
				int32 idx = ColorVar2Index(name);
				if( idx >= 0 ) {
					bool bg=false;
					TermColorID col = ColorIndex2ID(idx, &bg);
					if( bg ) backcolor_from_msg(msg, name, 0, col);
					else forecolor_from_msg(msg, name, 0, col);
				}
			}
		}
		if( msg.HasInt32(NumRowsConfigName) || msg.HasInt32(NumColsConfigName) ) {
			int32 r=0,c=0;
			TermGetFixedSize(&r, &c);
			TermSetFixedSize(msg.GetInt32(NumRowsConfigName, r),
							 msg.GetInt32(NumColsConfigName, c));
		}
		if( msg.HasInt32(RegionTopConfigName) ||
			msg.HasInt32(RegionBottomConfigName) ||
			msg.HasInt32(RegionLeftConfigName) ||
			msg.HasInt32(RegionRightConfigName) ) {
			int32 t=0, b=0, l=0, r=0;
			TermGetRegion(&t, &b, &l, &r);
			TermSetRegion(msg.GetInt32(RegionTopConfigName, t),
						  msg.GetInt32(RegionBottomConfigName, b),
						  msg.GetInt32(RegionLeftConfigName, l),
						  msg.GetInt32(RegionRightConfigName, r));
		}
		if( msg.HasInt32(AutoScrollConfigName) )
			SetAutoScrollMode((AutoScrollType)msg.GetInt32(AutoScrollConfigName,
														   AutoScrollMode()));
		if( msg.HasInt32(HistoryUseConfigName) )
			SetHistoryUse((HistoryUseType)msg.GetInt32(HistoryUseConfigName,
													   HistoryUse()));
		if( msg.HasInt32(HistorySizeConfigName) )
			SetHistorySize(msg.GetInt32(HistorySizeConfigName, HistorySize()));
		if( msg.HasBool(VerifyPasteConfigName) )
			SetPasteVerified(msg.GetBool(VerifyPasteConfigName, PasteVerified()));
		if( msg.HasBool(RMBPasteConfigName) )
			SetRMBPaste(msg.GetBool(RMBPasteConfigName, RMBPaste()));
		if( msg.HasString(CurEmulatorConfigName) ) {
			const char* name = msg.GetString(CurEmulatorConfigName, 0);
			if( name ) {
				const ArpEmulationType* type = Emulator().EmulationType();
				if( !type || strcmp(name, type->Name) ) {
					delete SetEmulator(name);
				}
			}
		}
		
		TermClean();
	} catch(...) {
		if( locked ) UnlockLooper();
		throw;
	}
	
	ReportChange(&msg);
	
	if( locked ) UnlockLooper();
		
	return msg.GetError();
}

status_t ArpRemoteTerminal::Configure(ArpVectorI<BView*>& views)
{
	status_t err = B_NO_ERROR;
	
	BMessage settings;
	if( (err=GetConfiguration(&settings)) != B_NO_ERROR ) {
		ArpD(cdb << ADH << "Error getting settings: " << err << endl);
		return err;
	}
	
	views.push_back(new ArpTerminalSettings(BMessenger(this), settings));
	
	return err;
}

/* ------------------------------------------------------------
	 EVENT HANDLING METHODS.
	 ------------------------------------------------------------ */

void ArpRemoteTerminal::TermSendRemote(const ichar * d, size_t len)
{
	if( device.IsValid() ) {
		BMessage msg(TERM_XFER_TEXT_MSG);
		msg.AddData("text",B_ASCII_TYPE,d,len);
		ArpD(cdb << ADH << "ArpTerminal::TermSendRemote: " << msg << endl);
		device.SendMessage(&msg);
	}
}

void ArpRemoteTerminal::TermSizeChanged(int32 rows, int32 cols)
{
	inherited::TermSizeChanged(rows,cols);
	SendSizeMsg(rows,cols);
}

void ArpRemoteTerminal::TermEmulationChanged(const char* name)
{
	inherited::TermEmulationChanged(name);
	if( device.IsValid() ) {
		BMessage msg(TERM_SELECT_EMULATION_MSG);
		if( msg.AddString("name",name) ) return;
		device.SendMessage(&msg);
	}
}

void ArpRemoteTerminal::TermReset(bool hard)
{
	ArpD(cdb << ADH << "ArpRemoteTerminal: Reset hard=" << hard << endl);
	inherited::TermReset(hard);
	if( HaveWatchers() ) {
		BMessage config;
		if( GetConfiguration(&config) == B_NO_ERROR ) {
			ArpD(cdb << ADH << "Reporting change: " << config << endl);
			ReportChange(&config);
		}
	}
}

void ArpRemoteTerminal::MessageReceived(BMessage *message)
{
	ArpD(cdb << ADH << "ArpRemoteTerminal::MessageReceived: " << *message << endl);
	switch( message->what ) {
	
		case TERM_XFER_TEXT_MSG: {
#if 1
			const ichar* txt = NULL;
			long len=0;
			int32 idx=0;
			while( message->FindData("text", B_ASCII_TYPE, idx,
									 (const void**)&txt, &len) == B_NO_ERROR ) {
				if( txt && len > 0 ) {
					Emulator().EmulateToTTY(txt,len);
				}
				idx++;
			}
			message->SendReply(B_NO_REPLY);
#else
			int32 recpos = 0;
			BMessage* curmsg = Window()->DetachCurrentMessage();
			BMessageQueue* queue = Window()->MessageQueue();
			while( curmsg ) {
				ichar* txt = NULL;
				long len=0;
				int32 idx=0;
				while( curmsg->FindData("text", B_ASCII_TYPE, idx,
									  &txt, &len) == B_NO_ERROR ) {
					if( txt ) {
						if( recpos + len > sizeof(receiveBuffer) ) {
							if( recpos > 0 ) {
								Emulator().EmulateToTTY(&receiveBuffer[0],recpos);
								recpos = 0;
							}
						}
						if( len > sizeof(receiveBuffer) ) {
							Emulator().EmulateToTTY(txt,len);
						} else {
							memcpy(&receiveBuffer[recpos],txt,len);
							recpos += len;
						}
					}
					idx++;
				}
				delete curmsg;
				curmsg = NULL;
				if( queue ) {
					curmsg = queue->FindMessage((int32)0);
					if( curmsg &&
						curmsg->what == TERM_XFER_TEXT_MSG ) {
						curmsg = queue->NextMessage();
					} else curmsg = NULL;
				}
			}
			if( recpos > 0 ) {
				Emulator().EmulateToTTY(&receiveBuffer[0],recpos);
			}
			return;
#endif
		} break;
		
		case TERM_RESET_MSG: {
			bool hard = false;
			if( message->FindBool("hard", &hard) != B_OK ) hard = false;
			Emulator().Reset(hard);
			TermClean();
			// HACK: The emulator often changes the terminal's mode
			// flags after calling its reset, leaving any active
			// settings view with incorrect settings.  Until we can
			// do better at tracking state changes, for this situation
			// send another update to the settings view.
			if( HaveWatchers() ) {
				BMessage config;
				if( GetConfiguration(&config) == B_NO_ERROR ) {
					ArpD(cdb << ADH << "Reporting change: " << config << endl);
					ReportChange(&config);
				}
			}
		} break;
		
		case TERM_START_SESSION_MSG: {
			StartSession();
			TermClean();
			Owner().SendMessage(message);
		} break;
		
		case TERM_END_SESSION_MSG: {
			EndSession();
			TermClean();
			Owner().SendMessage(message);
		} break;
		
		case TERM_REQUEST_STATUS_MSG: {
			int32 rows,cols;
			TermGetSize(&rows,&cols);
			SendSizeMsg(rows,cols);
			SendEmulatorsMsg();
		} break;
		
		case TERM_WINDOW_SIZE_MSG: {
			int32 rows,cols;
			TermGetFixedSize(&rows,&cols);
			message->FindInt32("rows",&rows);
			message->FindInt32("columns",&cols);
			TermSetFixedSize(rows,cols);
		} break;
		
		case TERM_SELECT_EMULATION_MSG: {
			BMenuItem* item = NULL;
			if( message->FindPointer("source",(void**)&item) ) item = NULL;
			BMenu* menu = NULL;
			if( item ) menu = item->Menu();
			int32 num=0;
			type_code type;
			if( !message->GetInfo("name",&type,&num) ) {
				for( int i=0; i<num; i++ ) {
					const char* str;
					if( !message->FindString("name",i,&str) ) {
						delete SetEmulator(str);
						if( !IsDefaultEmulator() ) {
							if( menu ) CheckEmulationMenu(menu);
							return;
						}
					}
				}
			}
			if( menu ) CheckEmulationMenu(menu);
		} break;
		
		case ARP_PUT_CONFIGURATION_MSG: {
			BMessage config;
			if( message->FindMessage("settings",&config) == B_NO_ERROR ) {
				PutConfiguration(&config);
			} else {
				inherited::MessageReceived(message);
			}
		} break;
		
		case TERM_ADD_WATCHER_MSG: {
			BMessenger w;
			if( message->FindMessenger("watch",&w) == B_NO_ERROR ) {
				AddWatcher(w);
				BMessage config;
				if( GetConfiguration(&config) == B_NO_ERROR ) {
					ReportChange(&config, &w);
				}
			} else {
				inherited::MessageReceived(message);
			}
		} break;
		
		case TERM_REM_WATCHER_MSG: {
			BMessenger w;
			if( message->FindMessenger("watch",&w) == B_NO_ERROR ) {
				RemWatcher(w);
			} else {
				inherited::MessageReceived(message);
			}
		} break;
		
		default:
			inherited::MessageReceived(message);
	}
}

void ArpRemoteTerminal::AttachedToWindow(void)
{
	inherited::AttachedToWindow();
	termhand = BMessenger(this);
	ArpD(cdb << ADH << "ArpRemoteTerminal::AttachedToWindow()" << endl);
	ArpD(cdb << ADH << "Window = " << Window()
				<< "valid = " << termhand.IsValid() << endl);
	SendAttachMsg();
}

void ArpRemoteTerminal::SendAttachMsg(void)
{
	if( device.IsValid() ) {
		BMessage msg(TERM_ATTACH_MSG);
		if( msg.AddMessenger("terminal",termhand) ) return;
		device.SendMessage(&msg);
		int32 rows,cols;
		TermGetSize(&rows,&cols);
		SendSizeMsg(rows,cols);
		SendEmulatorsMsg();
	}
}

void ArpRemoteTerminal::SendSizeMsg(int32 rows, int32 cols)
{
	if( device.IsValid() ) {
		BMessage msg(TERM_WINDOW_SIZE_MSG);
		if( msg.AddInt32("columns",cols) ) return;
		if( msg.AddInt32("rows",rows) ) return;
		device.SendMessage(&msg);
	}
}

static bool add_emu_info(BMessage& msg, const ArpEmulationType* type)
{
	if( type && !type->Synonym ) {
		if( msg.AddString("name",type->Name) != B_OK ) {
			return false;
		}
		if( msg.AddString("longname",type->LongName) != B_OK ) {
			return false;
		}
	}
	return true;
}

void ArpRemoteTerminal::SendEmulatorsMsg(void)
{
	if( !device.IsValid() || !manager ) return;
	
	BAutolock(manager->Lock());
	
	BMessage msg(TERM_EMULATORS_MSG);
	const ArpEmulationType* type = Emulator().EmulationType();
	if( !add_emu_info(msg,type) ) return;
	for( int i=0; i<manager->CountAddons(); i++ ) {
		ArpEmulatorManager::EmulatorAddon* adimage
			= (ArpEmulatorManager::EmulatorAddon*)
					manager->AddonAt(i);
		const ArpEmulatorAddon* addon = adimage->GetEmulator();
		if( addon ) {
			for( int j=0; j<addon->CountEmulations(); j++ ) {
				type = addon->EmulationType(j);
				if( !add_emu_info(msg,type) ) return;
			}
		}
	}
	device.SendMessage(&msg);
}
