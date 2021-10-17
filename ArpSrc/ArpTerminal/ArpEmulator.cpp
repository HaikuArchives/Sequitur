/*
	
	ArpEmulator.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPTERMINAL_ARPEMULATOR_H
#include "ArpTerminal/ArpEmulator.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef _INTERFACE_DEFS_H
#include <interface/InterfaceDefs.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#include <cstring>

ArpMODL(__FILE__, 0, arp_declstdmod);

static const ArpEmulationType type_info[] = {
	{ sizeof(ArpEmulationType), "DUMB", "Dumb Terminal", NULL },
};

static int32 DoCountEmulations(void)
{
	return 1;
}

static const ArpEmulationType* DoEmulationType(int32 /*idx*/)
{
	return &type_info[0];
}

static ArpEmulatorInterface* DoAllocEmulator(const char* name,
											ArpTerminalInterface& terminal)
{
	if( strcmp(name,type_info[0].Name) ) {
		return new ArpEmulator(terminal);
	}
	return NULL;
}

ArpEmulatorAddon ArpEmulator::AddonInterface = {
	ArpEmulator_Current,
	
	"Basic Emulator",
	"Angry Red Planet",
	B_UTF8_COPYRIGHT "1997-98 Dianne Hackborn",
	__DATE__, __TIME__,
	2,
	
	"A default emulator add-on that implements the simplest"
	" type of emulation possible: a dumb terminal.",
	
	&DoCountEmulations,
	&DoEmulationType,
	&DoAllocEmulator,
};

ArpEmulator::ArpEmulator(ArpTerminalInterface& myterm)
	: terminal(&myterm)
{
}

ArpEmulator::~ArpEmulator()
{
}

void ArpEmulator::Reset(bool hard)
{
	terminal->TermReset(hard);
}

void ArpEmulator::AttachTerminal(ArpTerminalInterface& terminal)
{
	this->terminal = &terminal;
}

const ArpEmulationType* ArpEmulator::EmulationType(void) const
{
	return &type_info[0];
}

void ArpEmulator::EmulateToTTY(const ichar* d, size_t len)
{
	if( len == 1 ) {
		ArpD(cdb << ADH << "ArpEmulator::EmulateToTTY(" << *d << ")" << std::endl);
		if( EmuRemoteNextSpecial(d,1) != d ) {
			ArpD(cdb << ADH << "WebTerm: Text: '" << charToString(*d) << "'");
			terminal->TermSendTTY(d,1);
		} else {
			if( EmuRemoteChar(*d) ) {
				terminal->TermSendTTY(d,1);
			}
			terminal->TermSendTTY(NULL,0,0);
		}
		return;
	}
	while( len > 0 ) {
		const ichar* spec = EmuRemoteNextSpecial(d,len);
		if( spec > d ) {
			size_t cnt = (spec-d)/sizeof(ichar);
			ArpD(cdb << ADH << "ArpEmulator: Text: '"
						<< sequenceToString(ArpString(d,0,cnt))
						<< "'");
			terminal->TermSendTTY(d,cnt,terminal->TERM_OUTPARTIAL);
			d = spec;
			if( cnt < len ) len -= cnt;
			else len = 0;
		}
		if( len > 0 ) {
			if( EmuRemoteChar(*d) ) {
				terminal->TermSendTTY(d,1,terminal->TERM_OUTPARTIAL);
			}
			d++;
			len--;
		}
	}
	ArpD(cdb << ADH << "WebTerm: Text: '"
				<< sequenceToString(ArpString(d,0,len)) << "'"
				<< std::endl);
	terminal->TermSendTTY(d,len,0);
}

void ArpEmulator::EmulateToRemote(const ichar* d, size_t len)
{
	terminal->TermSendRemote(d,len);
}

bool ArpEmulator::EmulateToRemote(BMessage* msg)
{
	if( !msg ) return false;

	ArpD(cdb << ADH << "ArpEmulator::EmulateToRemote " << (*msg) << std::endl);

	return EmuTTYMessage(msg);
}

enum { NUMPAD_FLAG = 0x20000 };

const int32 ArpEmulator::keymap[KEYMAP_SIZE] = {
	/* 0x0n*/
	ArpEmulator::KEY_ESCAPE, 0,
	ArpEmulator::KEY_F1, ArpEmulator::KEY_F2, ArpEmulator::KEY_F3,
	ArpEmulator::KEY_F4, ArpEmulator::KEY_F5, ArpEmulator::KEY_F6,
	ArpEmulator::KEY_F7, ArpEmulator::KEY_F8, ArpEmulator::KEY_F9,
	ArpEmulator::KEY_F10, ArpEmulator::KEY_F11, ArpEmulator::KEY_F12,
	ArpEmulator::KEY_PRINT, ArpEmulator::KEY_SCROLL,
	/* 0x1n */
	ArpEmulator::KEY_PAUSE, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, ArpEmulator::KEY_BACKSPACE, ArpEmulator::KEY_INSERT,
	/* 0x2n */
	ArpEmulator::KEY_HOME, ArpEmulator::KEY_PAGE_UP,
	0, 0, 0, 0, ArpEmulator::KEY_TAB, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x3n */
	0, 0, 0, 0,
	ArpEmulator::KEY_DELETE, ArpEmulator::KEY_END, ArpEmulator::KEY_PAGE_DOWN,
	NUMPAD_FLAG|ArpEmulator::KEY_HOME,
	NUMPAD_FLAG|ArpEmulator::KEY_CURSOR_UP,
	NUMPAD_FLAG|ArpEmulator::KEY_CURSOR_UP,
	0, 0, 0, 0, 0, 0,
	/* 0x4n */
	0, 0, 0, 0, 0, 0, 0, ArpEmulator::KEY_RETURN,
	NUMPAD_FLAG|ArpEmulator::KEY_CURSOR_LEFT,
	0,
	NUMPAD_FLAG|ArpEmulator::KEY_CURSOR_RIGHT, 0, 0, 0, 0, 0,
	/* 0x5n */
	0, 0, 0, 0, 0, 0, 0, 
	ArpEmulator::KEY_CURSOR_UP,
	NUMPAD_FLAG|ArpEmulator::KEY_END,
	NUMPAD_FLAG|ArpEmulator::KEY_CURSOR_DOWN,
	NUMPAD_FLAG|ArpEmulator::KEY_PAGE_DOWN,
	ArpEmulator::KEY_ENTER, 0, 0, ArpEmulator::KEY_SPACE, 0,
	/* 0x6n */
	0, ArpEmulator::KEY_CURSOR_LEFT, ArpEmulator::KEY_CURSOR_DOWN,
	ArpEmulator::KEY_CURSOR_RIGHT,
	NUMPAD_FLAG|ArpEmulator::KEY_INSERT,
	NUMPAD_FLAG|ArpEmulator::KEY_DELETE,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x7n */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	ArpEmulator::KEY_SYSREQ, ArpEmulator::KEY_BREAK,
};

bool ArpEmulator::EmuTTYMessage(BMessage* msg)
{
	switch( msg->what ) {
	case B_VIEW_RESIZED: {
		int32 rows,cols;
		terminal->TermGetSize(&rows,&cols);
		return EmuTTYResize(cols,rows);
	} break;
	case B_KEY_DOWN: {
		int32 key = 0;
		const char* utfchar = 0;
		int32 mods = 0;
		msg->FindInt32("key",&key);
		msg->FindString("bytes",&utfchar);
		msg->FindInt32("modifiers",&mods);
		int32 unichar = 0;
		if( utfchar ) {
			if( ((*utfchar)&0x80) != 0 ) {
				const char* pos = utfchar;
				int count=0;
				while( ((*pos)&(((unsigned char)0xc0)>>count)) != 0xc0
							&& count<6 ) {
					count++;
				}
				unichar = (*pos)&(0x3F>>count);
				unichar++;
				while( *pos != 0 && count > 0 ) {
					unichar = (unichar<<6) | ((*pos)&0x3F);
					pos++;
					count--;
				}
			} else {
				unichar = *utfchar;
			}
		}

#if defined(ArpDEBUG)
		ArpDB() {
			cdb << ADH << std::hex << "key=" << key << ", mods=" << mods
				<< ", utfchar=" << utfchar << " (";
			while( *utfchar ) {
				const int32 val = *((const unsigned char*)utfchar);
				cdb << char( (val>>4) >= 0xA ? (val>>4)-0xA + 'A' : (val>>4) + '0' )
					<< char( (val&0xF) >= 0xA ? (val&0xF)-0xA + 'A' : (val&0xF) + '0' );
				utfchar++;
				if( *utfchar ) cdb << " ";
			}
			cdb << "), unichar=" << unichar << std::dec << std::endl;
		}
#endif
		
		int32 trans = key <= KEYMAP_SIZE ? keymap[key] : 0;
		
		ArpD(cdb << ADH << "ArpEmulator::EmuTTYMessage key="
					<< std::hex << unichar << ", trans=" << trans << std::dec << std::endl);
					
		if( !trans ) return EmuTTYKeyPress(msg, unichar, mods);
		
		if( trans&NUMPAD_FLAG ) {
			trans &= ~NUMPAD_FLAG;
			ArpD(cdb << ADH << "This is a numeric keypad." << std::endl);
			if( mods&B_CONTROL_KEY ) {
				ArpD(cdb << ADH << "Control pressed: sending as cursor." << std::endl);
				return EmuTTYKeyPress(msg, trans,
							mods&~(B_CONTROL_KEY|B_LEFT_CONTROL_KEY|B_RIGHT_CONTROL_KEY));
			}
			const int32 old_mods = mods;
			mods &= ~(B_SHIFT_KEY|B_RIGHT_SHIFT_KEY|B_LEFT_SHIFT_KEY);
			if( ((old_mods&B_SHIFT_KEY)!=0) ^ ((old_mods&B_NUM_LOCK)!=0) ) {
				ArpD(cdb << ADH << "Shift/NumLock pressed: sending as number." << std::endl);
				return EmuTTYKeyPress(msg, unichar, mods);
			}
		}
		ArpD(cdb << ADH << "Sending translated key code." << std::endl);
		return EmuTTYKeyPress(msg, trans, mods);
	}
	case B_MOUSE_DOWN: {
		BPoint point;
		int32 buttons = 0;
		msg->FindPoint("where",&point);
		msg->FindInt32("buttons",&buttons);
		ArpD(cdb << ADH << "ArpEmulator::EmuTTYMessage down="
					<< buttons << std::endl);
		return EmuTTYMouseDown(msg,point,buttons);
	}
	case B_MOUSE_MOVED: {
		BPoint point;
		int32 buttons = 0;
		msg->FindPoint("where",&point);
		msg->FindInt32("buttons",&buttons);
		return EmuTTYMouseMove(msg,point,buttons);
	}
	case B_MOUSE_UP: {
		BPoint point;
		int32 buttons = 0;
		msg->FindPoint("where",&point);
		msg->FindInt32("buttons",&buttons);
		return EmuTTYMouseUp(msg,point,buttons);
	}
	}
	return false;
}

bool ArpEmulator::EmuTTYKeyPress(BMessage* /*msg*/,
									int32 key, int32 /*mods*/)
{
	ArpD(cdb << ADH << "ArpEmulator::EmuTTYKeyPress(key="
				<< std::hex << key << std::dec << ")" << std::endl);
	return false;
}

bool ArpEmulator::EmuTTYMouseDown(BMessage* /*msg*/,
								BPoint /*pos*/, int32 /*buttons*/)
{
	return false;
}

bool ArpEmulator::EmuTTYMouseMove(BMessage* /*msg*/,
								BPoint /*pos*/, int32 /*buttons*/)
{
	return false;
}

bool ArpEmulator::EmuTTYMouseUp(BMessage* /*msg*/,
								BPoint /*pos*/, int32 /*buttons*/)
{
	return false;
}

bool ArpEmulator::EmuTTYResize(int32 /*cols*/, int32 /*rows*/)
{
	return false;
}

bool ArpEmulator::EmuRemoteChar(ichar /*c*/)
{
	return true;
}

const ichar*
ArpEmulator::EmuRemoteNextSpecial(const ichar* str, size_t len)
{
	return str+len;
}

const char* ArpEmulator::codes_00_20[] = {
	"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
	"BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
	"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
	"CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
	"[]"
};

const char* ArpEmulator::codes_7f_9f[] = {
	"DEL",
	"$80", "$81", "$82", "$83", "IND", "NEL", "SSA", "ESA",
	"HTS", "HTJ", "VTS", "PLD", "PLU", "RI",  "SS2", "SS3",
	"DCS", "PU1", "PU2", "STS", "CCH", "MW",  "SPA", "EPA",
	"$98", "$99", "$9a", "CSI", "ST",  "OSC", "PM",  "APC"
};

ArpString ArpEmulator::charToString(ichar c)
{
	if( c <= (ichar)0x20 ) {
		return ArpString(codes_00_20[(int)c]);
	} else if( c >= (ichar)0x7f && c <= (ichar)0x9f ) {
		return ArpString(codes_7f_9f[(int)c-0x7f]);
	} else if( c == (ichar)0xff ) {
		return ArpString("$ff");
#if 0	// ichar was going to become an int16, but instead it's going away.
	} else if( sizeof(ichar) > 1 && c >= (ichar)0x100 ) {
		return ArpString("$") + ArpString((int)c,16);
#endif
	}
	return ArpString((char)c);
}

ArpString ArpEmulator::sequenceToString(const ArpString& seq)
{
	ArpString res;
	for( int i=0; i<seq.Length(); i++ ) {
		if( i > 0 ) res += " ";
		res += charToString(seq[i]);
	}
	return res;
}
