/*
 * Copyright (c)1997 by Dianne Hackborn.
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
 * at <hackbod@lucent.com> or <hackbod@enteract.com>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpTelnet.cpp
 *
 */

#ifndef ARPNETWORK_ARPTELNET_H
#include "ArpNetwork/ArpTelnet.h"
#endif

#ifndef ARPTELNETSETTINGS_H
#include "ArpTelnetSettings.h"
#endif

#ifndef ARPTERMINAL_ARPEMULATOR_H
#include "ArpTerminal/ArpEmulator.h"
#endif

#ifndef ARPTERMINAL_ARPTERMINALMSG_H
#include "ArpTerminal/ArpTerminalMsg.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#include <signal.h>
#include <string.h>

ArpMOD();

// ---------------------------------------------------------------------------
// ConnectWin: Prompt user for host and port.
// ---------------------------------------------------------------------------

#ifndef _WINDOW_H
#include <be/interface/Window.h>
#endif

#ifndef _SCREEN_H
#include <be/interface/Screen.h>
#endif

#ifndef _SCROLL_VIEW_H
#include <be/interface/ScrollView.h>
#endif

#ifndef _TEXT_CONTROL_H
#include <be/interface/TextControl.h>
#endif

#ifndef _BUTTON_H
#include <be/interface/Button.h>
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
#endif

#include <stdlib.h>

enum {
	SEND_CONNECT_MSG = '.scn',
};

class ConnectWin : public BWindow {
private:
	typedef BWindow inherited;
	
public:
	ConnectWin(BMessenger inreply, BWindow* inwin,
				const char* defhost = "", int32 defport=23);
	~ConnectWin();
	
	virtual void MessageReceived(BMessage *message);
	virtual	bool QuitRequested();
	
	BMessenger reply;
	
	BTextControl* hosttext;
	BTextControl* porttext;
};

ConnectWin::ConnectWin(BMessenger inreply, BWindow* inwin,
						const char* defhost, int32 defport)
	: BWindow(inwin ? inwin->Frame() : BScreen().Frame(), "Telnet Connect",
				B_TITLED_WINDOW_LOOK,
				inwin ? B_MODAL_SUBSET_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL,
				B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS ),
	  reply(inreply), hosttext(NULL), porttext(NULL)
{
	ArpD(cdb << ADH << "ConnectWin::ConnectWin()" << endl);
	
	if( inwin ) AddToSubset(inwin);
	
	// set up a rectangle and instantiate a new view
	// view rect should be same size as window rect but with
	// left top at (0, 0)
	BRect aRect(Bounds());
	
	const BFont* font = be_plain_font;
	
	font_height fh;
	font->GetHeight(&fh);
	float spaceh = (fh.ascent+fh.descent+fh.leading)/2;
	float spacew = font->StringWidth("W");
	
	BView* top = new BView(Bounds(),"top",B_FOLLOW_ALL,B_WILL_DRAW);
	if( !top ) return;
	top->SetViewColor(0xe0,0xe0,0xe0);
	AddChild(top);
	
	aRect.top += spaceh;
	aRect.bottom += spaceh;
	aRect.left += spacew;
	aRect.right += spacew;
	
	hosttext = new BTextControl(aRect, "host", "Host: ", defhost,
								NULL, B_FOLLOW_NONE);
	float hw=0, hh=0;
	if( hosttext ) {
		top->AddChild(hosttext);
		hosttext->GetPreferredSize(&hw,&hh);
		hw += font->StringWidth("telnet.enteract.com");
		hosttext->ResizeTo(hw,hh);
		hosttext->SetDivider(font->StringWidth("Host: "));
		hosttext->MakeFocus(true);
	}
	hw += spacew;
	
	char buffer[16];
	sprintf(&buffer[0],"%ld",defport);
	porttext = new BTextControl(BRect(aRect.left+hw,aRect.top,
										aRect.right,aRect.bottom),
								"port", "Port: ", &buffer[0],
								NULL, B_FOLLOW_NONE);
	float pw=0, ph=0;
	if( porttext ) {
		top->AddChild(porttext);
		porttext->GetPreferredSize(&pw,&ph);
		pw += font->StringWidth("00");
		porttext->ResizeTo(pw,ph);
		porttext->SetDivider(font->StringWidth("Port: "));
	}
	
	hh += spaceh;
	
	BButton* connectbut =
		new BButton(aRect, "connect", "Connect",
					new BMessage(SEND_CONNECT_MSG), B_FOLLOW_NONE);
	float cw=0, ch=0;
	if( connectbut ) {
		top->AddChild(connectbut);
		SetDefaultButton(connectbut);
		connectbut->GetPreferredSize(&cw,&ch);
		connectbut->ResizeToPreferred();
		connectbut->MoveTo(aRect.left+hw+pw-cw, aRect.top+hh);
		connectbut->SetTarget(this);
	}
	
	BButton* cancelbut =
		new BButton(BRect(aRect.left,aRect.top+hh,
							0,1), "cancel", "Cancel",
					new BMessage(B_QUIT_REQUESTED), B_FOLLOW_NONE);
	if( cancelbut ) {
		float canw=0, canh=0;
		top->AddChild(cancelbut);
		cancelbut->GetPreferredSize(&canw,&canh);
		cancelbut->ResizeToPreferred();
		cancelbut->MoveTo(aRect.left,aRect.top+hh+(ch-canh)/2);
		cancelbut->SetTarget(this);
	}

	BRect frm = Frame();
	ArpD(cdb << ADH << "Parent window frame = " << frm << endl);
	ResizeTo(aRect.left+hw+pw+spacew,aRect.top+hh+ch+spaceh);
	if( inwin ) {
		BRect cfrm = Frame();
		ArpD(cdb << ADH << "Resized frame = " << cfrm << endl);
		MoveTo( frm.left
				+ (frm.Width()-cfrm.Width())/2,
			 	frm.top
			 	+ (frm.Height()-cfrm.Height())/2);
		ArpD(cdb << ADH << "Moved frame = " << Frame() << endl);
	}
}

ConnectWin::~ConnectWin()
{
}

void ConnectWin::MessageReceived(BMessage *message)
{
	switch( message->what ) {
		case SEND_CONNECT_MSG: {
			if( reply.IsValid() && hosttext && porttext ) {
				BMessage cmsg(ARPTELNET_CONNECT_MSG);
				ArpString url;
				url = ArpLIT("telnet://") + ArpString(hosttext->Text())
							+ ":" + ArpString(porttext->Text()) + "/";
				cmsg.AddString("url",(const char*)url);
				reply.SendMessage(&cmsg);
			}
			PostMessage(B_QUIT_REQUESTED);
		} break;
		default:
			inherited::MessageReceived(message);
	}
}

bool ConnectWin::QuitRequested()
{
	return(true);
}

// ---------------------------------------------------------------------------
// ArpTelnet implementation.
// ---------------------------------------------------------------------------

class PromptInfo {
  public:

	ArpString prompt;
	int pos;
	ArpString reply;
	bool terminate;

	PromptInfo(const char* inprompt,
  				const char* inreply, bool term=false)
		: prompt(inprompt), pos(0), reply(inreply), terminate(term)
	{
		terminate = term;
	}
};

const char* ArpTelnet::telcmds[TELCMD_NUM+1] = {
    "EOF", "SUSP", "ABORT", "EOR",
    "SE", "NOP", "DMARK", "BRK", "IP", "AO", "AYT", "EC",
    "EL", "GA", "SB", "WILL", "WONT", "DO", "DONT", "IAC",
    NULL
};
  
  // ASCII names of the telnet options.

const char* ArpTelnet::telopts[TELOPT_NUM+1] = {
    /*  1 */ "BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
    /*  2 */ "STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
    /*  3 */ "NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
    /*  4 */ "NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
    /*  5 */ "DATA ENTRY TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
    /*  6 */ "SEND LOCATION", "TERMINAL TYPE", "END OF RECORD",
    /*  7 */ "TACACS UID", "OUTPUT MARKING", "TTYLOC",
    /*  8 */ "3270 REGIME", "X.3 PAD", "NAWS", "TSPEED", "LFLOW",
    /*  9 */ "LINEMODE", "XDISPLOC", "OLD-ENVIRON", "AUTHENTICATION",
    /* 10 */ "ENCRYPT", "NEW-ENVIRON",
    NULL
};

const bool ArpTelnet::locopts_impl[TELOPT_NUM+1] = {
    /*  1 */ true,  false, false,  true, false,
    /*  2 */ false, false, false, false, false,
    /*  3 */ false, false,  true, false, false,
    /*  4 */ false, false, false, false, false,
    /*  5 */ false, false, false,
    /*  6 */ false,  true, false,
    /*  7 */ false, false, false,
    /*  8 */ false, false,  true,  true, false,
    /*  9 */ false, false, false, false,
    /* 10 */ false, false,
    false
};

const bool ArpTelnet::remopts_impl[TELOPT_NUM+1] = {
    /*  1 */ true,   true, false,  true, false,
    /*  2 */ false, false, false, false, false,
    /*  3 */ false, false, false, false, false,
    /*  4 */ false, false, false, false, false,
    /*  5 */ false, false, false,
    /*  6 */ false, false, false,
    /*  7 */ false, false, false,
    /*  8 */ false, false, false, false, false,
    /*  9 */ false, false, false, false,
    /* 10 */ false, false,
    false
};

/*	------------------------------------------------------------
	PUBLIC CONFIGURATION INFORMATION.
	------------------------------------------------------------ */

const char* ArpTelnet::HostConfigName = "HostName";
const char* ArpTelnet::PortConfigName = "HostPort";

/* ----------------------------------------------------------
	"ArpTelnet" class add-on interface.
   ---------------------------------------------------------- */

enum { NUM_TYPES = 1 };

static const ArpRemoteType type_info[NUM_TYPES] = {
	{ sizeof(ArpRemoteType), "telnet", "Telnet", NULL }
};

static int32 DoCountDevices(void)
{
	return NUM_TYPES;
}

static const ArpRemoteType* DoDeviceType(int32 idx)
{
	if( idx < 0 || idx >= NUM_TYPES ) return NULL;
	return &type_info[idx];
}

static ArpRemoteInterface* DoAllocRemote(const char* name)
{
	if( !name ) return NULL;
	if( !strcmp(name, type_info[0].URL) ) return new ArpTelnet;
	return NULL;
}

const ArpRemoteAddon ArpTelnet::AddonInterface = {
	ArpRemote_Current,
	
	"TELNET Remote Device",
	"Angry Red Planet",
	B_UTF8_COPYRIGHT "1998 Dianne Hackborn and Angry Red Planet, Inc.",
	__DATE__, __TIME__,
	1,
	
	"This is a fairly complete (or at least usable... for me) "
	"TELNET client implementation.",
	
	&DoCountDevices,
	&DoDeviceType,
	&DoAllocRemote,
};

ArpTelnet::ArpTelnet(const BMessenger& term, const char* url)
	: BLooper("ARP™ Telnet Looper"), terminal(term),
	  mWatch(this, this)
{
	initialize(url);
}

ArpTelnet::ArpTelnet(const char* url)
	: BLooper("ARP™ Telnet Looper"),
	  mWatch(this, this)
{
	initialize(url);
}

ArpTelnet::~ArpTelnet()
{
	CloseConnectWin();
	DoDisconnect();
	PromptInfo* pr;
	for ( long i=0; (pr=(PromptInfo*)(prompts.ItemAt(i))) != NULL; i++ ) {
		delete pr;
	}
}

void ArpTelnet::initialize(const char* url)
{
	// Global state
	isTelnet = false;
	doPrompts = true;
	socketError = B_NO_ERROR;
	
	hostName = "";
	hostPort = 23;
	
	curEmuType = -1;
	curEmuName = "";
	delayedTTYPE = false;
	curWidth = curHeight = 0;
	connected = false;
	mInUse = false;
	
	// Reader thread and its associated private data.
	// (Note that the socket input handle is also private to
	//  the reader thread.)
	readThread = -1;
	mDisconnectRequested = false;
	mInputState = InputData;
	
	sock.Close();

	int i;
	for( i=0; i<TELOPT_NUM; i++ ) {
		loc_options[i] = rem_options[i] = false;
	}
	
	Run();
	
	if( url ) Connect(url);
}

#if 0
const char** ArpTelnet::getParameterInfo()
{
	String[][] info = {
		{ "prompt#", "string", "prompt to look for in text stream" },
		{ "reply#", "string", "reply to send when prompt# is encountered" },
		{ "endprompts#", "boolean",
		  "if true, turn of all prompts when this one is encountered" }
	};

	return info;
}

void ArpTelnet::ParseParameters(Parameters p)
{
	BAutolock llock(this);
	BAutolock glock(&globals);
	
    boolean have_prompts = false;

    int num_prompts;
    for( num_prompts=1;
	p.getParameter("prompt" + num_prompts) != null;
	num_prompts++ )
      ;

    if( DEBUG ) System.out.println("WebTerm: Found " + num_prompts + " prompts");

    prompts = new PromptInfo[num_prompts];

    for( int i=0; i<num_prompts; i++ ) {
      boolean term = false;
      String term_s = p.getParameter("endprompts" + i);
      if( term_s != null && term_s.compareTo("true") == 0 ) term = true;
      else term = false;

      prompts[i] = new PromptInfo(parseString(p.getParameter("prompt" + i)),
				  parseString(p.getParameter("reply" + i)),
				  term);
      if( prompts[i].prompt != null ) have_prompts = true;

      if( DEBUG ) {
	char[] prompt = { };
	String reply = "";
	if( prompts[i].prompt != null ) prompt = prompts[i].prompt;
	if( prompts[i].reply != null ) reply = prompts[i].reply;
	System.out.println("WebTerm: Prompt " + i + ": prompt=" + prompt +
			   ", reply=" + reply + ", term=" +
			   prompts[i].terminate);
      }
    }

    if( !have_prompts ) prompts = null;
  }
#endif

void ArpTelnet::clearPrompts()
{
	BAutolock glock(&globals);
	
	doPrompts = false;
	PromptInfo* pr;
	for ( long i=0; (pr=(PromptInfo*)(prompts.ItemAt(i))) != NULL; i++ ) {
		pr->pos = 0;
	}
}

status_t ArpTelnet::GetConfiguration(BMessage* values) const
{
	if( !values ) return B_BAD_VALUE;
	
	BAutolock l(const_cast<ArpTelnet*>(this));
	if( !l.IsLocked() ) return B_ERROR;
	
	status_t res = values->AddString(HostConfigName, (const char*)hostName);
	if( !res ) res = values->AddInt32(PortConfigName, hostPort);
	
	return res;
}

status_t ArpTelnet::PutConfiguration(const BMessage* values)
{
	if( !values ) return B_BAD_VALUE;
	
	BAutolock l(this);
	if( !l.IsLocked() ) return B_ERROR;
	
	const char* str;
	int32 val;
	
	if( !values->FindString(HostConfigName, &str) ) hostName = str;
	if( !values->FindInt32(PortConfigName, &val) ) hostPort = val;
	
	mWatch.ReportChange(values);
	
	return B_OK;
}

status_t ArpTelnet::Configure(ArpVectorI<BView*>& views)
{
	status_t err = B_NO_ERROR;
	
	BAutolock l(this);
	if( !l.IsLocked() ) return B_ERROR;
	
	BMessage settings;
	if( (err=GetConfiguration(&settings)) != B_NO_ERROR ) {
		ArpD(cdb << ADH << "Error getting settings: " << err << endl);
		return err;
	}
	
	views.push_back(new ArpTelnetSettings(BMessenger(this), settings));
	
	return err;
}

const ArpRemoteType* ArpTelnet::RemoteType(void) const
{
	return &type_info[0];
}
	
const char* ArpTelnet::Host()
{
	BAutolock llock(this);
	BAutolock glock(&globals);
	if( !host.Error() ) return host.IPName();
	else return hostName;
}

int ArpTelnet::Port()
{
	BAutolock llock(this);
	BAutolock glock(&globals);
	if( !host.Error() ) return host.IPPort();
	else return hostPort;
}

status_t ArpTelnet::Connect(BWindow* win)
{
	ArpD(cdb << ADH << "ArpTelnet::Connect(win=" << win << ")" << endl);
	BAutolock llock(this);
	BAutolock glock(&globals);
	CloseConnectWin();
	ConnectWin* cn = new ConnectWin(BMessenger(this), win,
									hostName, hostPort);
	ArpD(cdb << ADH << "Created connect window: " << cn << endl);
	if( !cn ) return B_ERROR;
	cn->Show();
	cnwin = BMessenger(cn);
	return B_NO_ERROR;
}

void ArpTelnet::Disconnect()
{
	BAutolock llock(this);
	BAutolock glock(&globals);
	BMessage connmsg(ARPTELNET_DISCONNECT_MSG);
	PostMessage(&connmsg, this);
}

status_t ArpTelnet::Connect(const char* url, BWindow* win)
{
	BAutolock llock(this);
	BAutolock glock(&globals);
	status_t res = B_ERROR;
	BMessage connmsg(ARPTELNET_CONNECT_MSG);
	if( url ) {
		ArpString protocol, address;
		ArpParseURL(ArpString(url), &protocol, &address, NULL, NULL, NULL);
		if( !protocol.IsEmpty() && !address.IsEmpty() ) {
			if( (res=connmsg.AddString("url", url)) != B_NO_ERROR ) return res;
			res = PostMessage(&connmsg, this);
		} else {
			if( !address.IsEmpty() ) {
				ArpStrTok tok(address, ArpLIT(":"));
				ArpString host = tok.Next();
				ArpString port = tok.Next();
				if( !host.IsEmpty() ) hostName = host;
				bool portValid = false;
				int portNum = port.AsInt(10,&portValid);
				if( portValid ) hostPort = portNum;
				mWatch.ReportState();
			}
			res = Connect(win);
		}
	} else if( !hostName.IsEmpty() && hostPort >= 0 ) {
		res = PostMessage(&connmsg, this);
	} else res = Connect(win);
	
	return res;
}

status_t ArpTelnet::DoConnect(const char* host, int32 port)
{
	BAutolock llock(this);
	
	DoDisconnect();

	BAutolock glock(&globals);
	
	isTelnet = false;
	mDisconnectRequested = false;
	clearPrompts();
    
	// Just start up the reader thread, and let it do all else.
	if( host ) hostName = host;
	if( port >= 0 ) hostPort = port;
	mWatch.ReportState();
	ArpD(cdb << ADH << "ArpTelnet: Spawn reader thread." << endl);
	readThread = spawn_thread(readThreadEntry,
								"ARP™ Telnet Reader",
								B_NORMAL_PRIORITY,
								this);
	if( readThread < 0 ) {
		Disconnect();
		return (status_t)readThread;
	}
	ArpD(cdb << ADH << "ArpTelnet: Resume reader thread." << endl);
	socketError = resume_thread(readThread);
	
	// Kill the thread if unable to start it.
	if( socketError ) Disconnect();
	
	return socketError;
}

void ArpTelnet::DoDisconnect()
{
	BAutolock llock(this);
	
	ArpD(cdb << ADH << "ArpTelnet: Disconnecting..." << endl);

	globals.Lock();
	
	CloseConnectWin();
	
	thread_id mythread = readThread;
	readThread = -1;
	mDisconnectRequested = true;
	
	// If the connected flag has been set and the thread is
	// running, it's in its read loop and needs to be forced
	// to quit.
	while( connected && mythread >= 0 ) {
		ArpD(cdb << ADH << "ArpTelnet: signal " << SIGUSR1
							<< "to reader " << mythread << endl);
		send_signal(mythread, SIGUSR1);
		globals.Unlock();
		ArpD(cdb << ADH << "ArpTelnet: waiting for reader." << endl);
		snooze(20000);
		//wait_for_thread(mythread,&ret);
		ArpD(cdb << ADH << "ArpTelnet: checking reader again." << endl);
	}
	ArpD(cdb << ADH << "ArpTelnet: no reader running." << endl);
	
	globals.Unlock();
	mInUse = false;
	mDisconnectRequested = false;
}

void ArpTelnet::CloseConnectWin()
{
	BAutolock llock(this);
	BAutolock glock(&globals);
	if( cnwin.IsValid() ) {
		BMessage close(B_QUIT_REQUESTED);
		cnwin.SendMessage(&close);
		cnwin = BMessenger();
	}
}

/* ------------------------------------------------------------
   HIGH-LEVEL MESSAGE HANDLING
   ------------------------------------------------------------ */

void ArpTelnet::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	ArpD(cdb << ADH << "Telnet::MessageReceived: " <<
				*message << endl);

	if( mWatch.MessageReceived(message) == B_OK ) return;
	
	switch( message->what ) {
		case ARPTELNET_CONNECT_MSG: {
			const char* url = NULL;
			if( message->FindString("url", &url) == B_NO_ERROR ) {
				ArpString protocol, address;
				ArpParseURL(ArpString(url), &protocol, &address,
							NULL, NULL, NULL);
				if( protocol != ArpLIT("telnet") ) {
					receive(ArpLIT("\r\nMalformed url: unknown protocol \"")
							+ protocol + ArpLIT("\"\r\n"));
					return;
				}
				ArpStrTok tok(address, ArpLIT(":"));
				ArpString host = tok.Next();
				ArpString portstr = tok.Next(ArpLIT(""));
				if( host.IsEmpty() ) {
					receive(ArpLIT("\r\nNo host in url: ") + url + ArpLIT("\r\n"));
					return;
				}
				bool valid=false;
				int port=23;
				if( portstr.IsEmpty() ) valid = true;
				else port = portstr.AsInt(10,&valid);
				if( !valid ) {
					receive(ArpLIT("\r\nBad port in url: ") + url + ArpLIT("\r\n"));
					return;
				}
				DoConnect(host,port);
			} else {
				DoConnect();
			}
		} break;
		case ARPTELNET_DISCONNECT_MSG: {
			DoDisconnect();
		} break;
		case TERM_XFER_TEXT_MSG: {
			const char* str = NULL;
			ssize_t len = 0;
			int32 pos = 0;
			while( message->FindData("text", B_ASCII_TYPE, pos,
									(const void**)&str, &len) == B_NO_ERROR ) {
				if( str && len > 0 ) write(str,len);
				pos++;
			}
		} break;
		case TERM_ATTACH_MSG: {
			message->FindMessenger("terminal",&terminal);
			emuTypes = BMessage();
		} break;
		case TERM_WINDOW_SIZE_MSG: {
			int32 width=0, height=0;
			if( message->FindInt32("columns",&width) == B_OK
				&& message->FindInt32("rows",&height) == B_OK ) {
				if( curWidth != width || curHeight != height ) {
					setWindowSize(width,height);
				}
			}
		} break;
		case TERM_EMULATORS_MSG: {
			emuTypes = *message;
			if( delayedTTYPE ) {
				send_ttype();
				delayedTTYPE = false;
			}
		} break;
		case TERM_SELECT_EMULATION_MSG: {
			const char* name = NULL;
			if( message->FindString("name",&name) == B_OK ) {
				setEmulator(name);
			}
		} break;
		default:
			inherited::MessageReceived(message);
	}
}

void ArpTelnet::receive(const char* str, int32 len)
{
	if( !str ) return;
	if( len < 0 ) len = strlen(str);
	
	BAutolock glock(&globals);
	
	if( !terminal.IsValid() ) return;
	
	BMessage msg(TERM_XFER_TEXT_MSG);
	
	if( msg.AddData("text",B_ASCII_TYPE,str,len) != B_NO_ERROR ) {
		return;
	}
	
	// A quick and dirty way to buffer data going to terminal.
	terminal.SendMessage(&msg);
}

void ArpTelnet::changeEmulator(const char* name)
{
	BAutolock glock(&globals);
	
	if( !terminal.IsValid() ) return;
	
	BMessage msg(TERM_SELECT_EMULATION_MSG);
	
	if( msg.AddString("name",name) != B_NO_ERROR ) return;
	
	terminal.SendMessage(&msg);
}

/* ------------------------------------------------------------
   DATA SENDING METHODS
   ------------------------------------------------------------ */

void ArpTelnet::setWindowSize(int32 width, int32 height)
{
	BAutolock glock(&globals);
	
	if( curWidth != width || curHeight != height ) {
		curWidth = width;
		curHeight = height;
		send_naws(width,height);
	}
}

void ArpTelnet::setEmulator(const char* name)
{
	BAutolock glock(&globals);
	
	if( name && curEmuName != (const ichar*)name ) {
		curEmuName = name;
		// TELNET protocol doesn't let us dynamically change, yes?
	}
}

void ArpTelnet::write(char b)
{
	BAutolock glock(&globals);
	
	if( connected ) {
		ArpD(cdb << ADH << "ArpTelnet: Put: " << ArpString::CharToString(b)
							<< " (" << (int)b << ")" << endl);
		sock.Send(&b,1);
		if( !rem_options[TELOPT_ECHO] ) {
			receive(&b,1);
		}
		mInUse = true;
	} else receive(&b,1);
}
  
void ArpTelnet::write(const char* b, int32 len)
{
	BAutolock glock(&globals);
	
	if( connected && len > 0 ) {
		ArpD(cdb << ADH << "ArpTelnet: Put: " << ArpString(b,len).AsSequence()
						<< endl);
		sock.Send(b,len);
		if( !rem_options[TELOPT_ECHO] ) {
			receive(b,len);
		}
		mInUse = true;
	} else if( len > 0 ) receive(b,len);
}
  
void ArpTelnet::write(const ArpString& str)
{
	write(str,str.Length());
}
  
/* ------------------------------------------------------------
   INPUT THREAD METHODS
   ------------------------------------------------------------ */

static void ignore_signal(int)
{
	ArpD(cdb << ADH << "ArpTelnet: entered ignore_signal(), thread="
						<< find_thread(NULL) << endl);
}

/* Entry point and main loop. */

int32 ArpTelnet::readThreadEntry(void* arg)
{
	ArpD(cdb << ADH << "ArpTelnet: Enter the reader." << endl);
	signal(SIGUSR1,ignore_signal);
	ArpTelnet *obj = (ArpTelnet *)arg; 
	return (obj->startSession()); 
	ArpD(cdb << ADH << "ArpTelnet: Exit the reader." << endl);
}

int32 ArpTelnet::startSession(void)
{
	{
		BAutolock glock(globals);
		BMessage sessionMsg(TERM_START_SESSION_MSG);
		terminal.SendMessage(&sessionMsg);
	}
	
	status_t ret = startReader();
	
	{
		BAutolock glock(globals);
		BMessage sessionMsg(TERM_END_SESSION_MSG);
		sessionMsg.AddBool("remoteclosed",
						   mDisconnectRequested ? false : true);
		terminal.SendMessage(&sessionMsg);
	}
	
	return ret;
}

int32 ArpTelnet::startReader(void)
{
	{
		uint8 buffer[1024];
		
		ArpString tName;
		int32 tPort;
		{
			BAutolock glock(globals);
		
			// First connect to host.
			receive("Looking up host ");
			receive(hostName);
			receive("...\r\n");
			ArpD(cdb << ADH << "ArpTelnet: Looking up " << hostName
							<< " at " << hostPort << endl);
			tName = hostName;
			tPort = hostPort;
		}
		
		ArpHostName lookup = ArpHostName(tName,tPort);
		
		{
			BAutolock glock(globals);
			
			if( lookup.Error() != ENOERR || readThread < 0 ) {
				receive("Lookup error: ");
				if( readThread < 0 ) receive(strerror(EINTR));
				else receive(lookup.ErrorString());
				receive("\r\n");
				readThread = -1;
				return 0;
			}
			host = lookup;
		    doPrompts = true;
	    
		    uint32 addr = host.IPAddress();
			sprintf((char*)buffer,"Opening connection to "
							"%d.%d.%d.%d on port #%d...\r\n",
							(int)(addr>>24)&0xFF, (int)(addr>>16)&0xFF,
							(int)(addr>>8)&0xFF,  (int)addr&0xFF,
							(int)host.IPPort());
			receive((const char*)buffer);
			ArpD(cdb << ADH << "ArpTelnet: Opening socket at " << hostName
							   << " / " << hostPort << endl);
		}
		
		status_t ret = sock.Open();
		
		{
			BAutolock glock(globals);
		
			ArpD(cdb << ADH << "ArpTelnet: returned with " << ret
							<< ", readthread=" << readThread << endl);
			if( ret >= 0 && readThread < 0 ) ret = EINTR;
		}
		
		if( ret >= 0 ) ret = sock.ConnectToAddress(lookup);
		
		{
			BAutolock glock(globals);
			if( ret != B_OK ) {
				socketError = ret;
				receive("Connect error: ");
				receive(strerror(socketError));
				receive("\r\n");
				readThread = -1;
				return ret;
			}
		
			ArpD(cdb << ADH << "ArpTelnet: Connected to remote." << endl);
			connected = true;
		}
		
	}
	
	int32 retval = runReader();

	{
		BAutolock glock(&globals);		// Protect termination
		
		ArpD(cdb << ADH << "ArpTelnet: Cleaning up..." << endl);
		
		clearPrompts();
		if( socketError < 0 ) {
			ArpD(cdb << ADH << "ArpTelnet: Showing close error..." << endl);
			receive("\r\n\r\nConnection closed: ");
			receive(strerror(socketError));
			receive("\r\n");
		}
		
		ArpD(cdb << ADH << "ArpTelnet: Closing socket..." << endl);	
		sock.Close();
		
		ArpD(cdb << ADH << "ArpTelnet: Clearing hostname..." << endl);	
		host = ArpHostName();
		
		readThread = -1;
		socketError = B_NO_ERROR;
		isTelnet = false;
		for( int i=0; i<TELOPT_NUM; i++ ) {
			loc_options[i] = rem_options[i] = false;
		}
		
		ArpD(cdb << ADH << "ArpTelnet: Finished disconnect." << endl);
		connected = false;
		mInUse = false;
		readThread = -1;
	}
	
	return retval;
}

int32 ArpTelnet::runReader(void)
{
	uint8 buffer[1024];
	
	//bool client_kill = false;
    ArpD(cdb << ADH << "WebTerm: Telnet input thread has started."
    				<< endl);
	ArpD(cdb << ADH << "WebTerm: Telnet thread: "
					<< find_thread(NULL) << endl);
	receive("Connected.\r\n");
	mInputState = InputData;
	for(;;) {
		long len = 0;
		status_t ret = EINTR;
		if( readThread >= 0 ) {
			ArpD(cdb << ADH << "ArpTelnet: Ready to receive." << endl);
			ret = sock.Recv((char*)buffer,sizeof(buffer)-4,&len);
		}
		//if( ret == 0 ) ret = ECONNABORTED;
		if( (ret < B_OK && ret != EINTR) || readThread < 0 ) {
			ArpD(cdb << ADH << "ArpTelnet: Input thread is terminating."
							<< endl);
			socketError = ret;
			return ret;
		}
		if( len > 0 ) {
			ArpD(cdb << ADH << "ArpTelnet: Now read " << len << " bytes:"
							<< (ArpString(&buffer[0],len).AsSequence())
							<< endl);
			const uint8* start = &buffer[0];
			const uint8* end = &buffer[len];
			while( start != 0 && start < end ) {
				start = process_buffer(start, (size_t)(end-start));
				ArpD(cdb << ADH << "ArpTelnet: New reader state is "
								<< int(mInputState) << endl);
			}
		} else {
			ArpD(cdb << ADH << "ArpTelnet: Nothing read." << endl);
		}
	}
}

const uint8* ArpTelnet::process_buffer(const uint8* buffer, size_t len)
{
	if( len <= 0 ) return 0;
	
	switch( mInputState ) {
		case InputData: {
			const uint8* start = buffer;
			while( *buffer != IAC && len > 0 ) {
				buffer++;
				len--;
			}
			if( buffer > start ) process_data(start, (size_t)(buffer-start));
			if( len > 0 ) {
				mInputState = InputIAC;
				buffer++;
			}
			return buffer;
		}
		
		case InputIAC: {
			
			print_cmd("Server",*buffer);
			switch( *buffer ) {
				case IAC:
					mInputState = InputData;
					process_data(*buffer);
					break;
				case DONT:
				case DO:
				case WONT:
				case WILL:
				case SB:
					mInputState = (InputState)*buffer;
					break;
				default:
					mInputState = InputData;
					return buffer+1;
			}
			
			return buffer+1;
		}
		
		case InputDONT:
		case InputDO:
		case InputWILL:
		case InputWONT: {
			print_opt("Server Opt",*buffer);
			process_opt(mInputState,*buffer);
			mInputState = InputData;
			return buffer+1;
		}
		
		case InputSB: {
			mInputState = InputSBData;
			mInSBCode = *buffer;
			mInSBData = "";
			print_opt("Server Sub",*buffer);
			return buffer+1;
		}
		
		case InputSBData: {
			const uint8* start = buffer;
			while( *buffer != IAC && len > 0 ) {
				buffer++;
				len--;
			}
			if( buffer > start ) {
				mInSBData += ArpString(start, (size_t)(buffer-start));
			}
			if( len > 0 ) {
				mInputState = InputSBIAC;
				buffer++;
			}
			return buffer;
		}
		
		case InputSBIAC: {
			if( *buffer == SE ) {
				process_sb(mInSBCode, mInSBData);
				mInputState = InputData;
				mInSBCode = 0;
				mInSBData = "";
			} else {
				mInSBData += *buffer;
			}
			return buffer+1;
		}
		
		default: {
			ArpD(cdb << ADH << "**** BAD STATE ****" << endl
							<< "State code: " << int(mInputState) << endl);
			mInputState = InputData;
		}
	}
	
	return buffer;
}

/* ------------------------------------------------------------
   CHARACTER DATA INPUT METHODS
   ------------------------------------------------------------ */

/* Methods for handling standard data.  These are synchronized
   because they call handlePrompts(), which accesses the common
   doPrompts and prompts variables. */

void ArpTelnet::process_data(uint8 b)
{
	BAutolock glock(&globals);
	
	ArpD(cdb << ADH << "ArpTelnet: Got: "
				<< ArpEmulator::charToString((ichar)b) << endl);
	handlePrompts(b);
	receive((char *)&b,1);
}

void ArpTelnet::process_data(const uint8* c, size_t len)
{
	BAutolock glock(&globals);
	
	ArpD(cdb << ADH << "ArpTelnet: Got: "
				<< ArpEmulator::sequenceToString(ArpString(c,len))
				<< endl);
	if( doPrompts && doPrompts) {
		const uint8* curPos = c;
		size_t curLen = len;
		while( curLen > 0 ) {
			handlePrompts(*curPos);
			curPos++;
			curLen--;
		}
	}
	receive((const char*)c,len);
}

/* Change state of all prompts with the next character.  Methods that
   call this must have locked looper, because this method manipulates
   shared variables. */

void ArpTelnet::handlePrompts(uint8 c)
{
	if( doPrompts && doPrompts ) {
		for( int i=0; i<prompts.CountItems(); i++ ) {
			PromptInfo* pi = (PromptInfo*)prompts.ItemAt(i);
			if( pi != NULL && pi->prompt.Length() > 0 ) {
				if( pi->prompt[pi->pos] == (char)c ) {
					pi->pos++;
					ArpD(cdb << ADH << i << ": prompt="
								<< pi->prompt << ", pos="
								<< pi->pos << endl);
					if( pi->pos >= pi->prompt.Length() ) {
						if( pi->reply.Length() > 0 ) {
							write(pi->reply);
						}
						if( pi->terminate ) {
							clearPrompts();
							return;
						}
						pi->pos = 0;
					}
				} else {
					pi->pos = 0;
				}
			}
		}
	}
}

/* ------------------------------------------------------------
   TELNET OPTION CONTROL METHODS
   ------------------------------------------------------------ */

/* Send an option command to the remote server. */

void ArpTelnet::send_opt(int32 cmd, int32 opt, bool force)
{
	BAutolock glock(&globals);
	
	ArpD(cdb << ADH << "ArpTelnet send_opt: " << telcmd(cmd)
					<< " " << telopt(opt)
					<< ", rem ok=" << remopt_ok(opt) << " state="
					<< (remopt_ok(opt) ? rem_options[opt-TELOPT_FIRST]:false)
					<< ", loc ok=" << locopt_ok(opt) << " state="
					<< (locopt_ok(opt) ? loc_options[opt-TELOPT_FIRST]:false)
					<< endl);
					
	/* Send this command if we are being forced to,
	   OR if it is a change of state of the server options,
	   OR if it is a change in the state of our local options. */
	if( force ||
		( remopt_ok(opt) &&
		  ( cmd == DONT && rem_options[opt-TELOPT_FIRST] )
			|| ( cmd == DO && !rem_options[opt-TELOPT_FIRST] ) ) ||
       ( locopt_ok(opt) &&
		  ( cmd == WONT && loc_options[opt-TELOPT_FIRST] )
			|| ( cmd == WILL && !loc_options[opt-TELOPT_FIRST] ) ) ) {
		uint8 reply[3];
		reply[0] = IAC;
		reply[1] = cmd;
		reply[2] = opt;
		print_cmd("Client",IAC);
		print_cmd("Client",cmd);
		print_opt("Client",opt);
		sock.Send((char*)reply,3);
    }
    
	/* Change our options state.  We really shouldn't be turning
	   options on until we get a reply, but this isn't
	   a problem yet for the options that are currently implemented... */
	if( cmd == WILL ) {
		if( locopt_ok(opt) ) loc_options[opt-TELOPT_FIRST] = true;
	} else if( cmd == WONT ) {
		if( locopt_ok(opt) ) loc_options[opt-TELOPT_FIRST] = false;
	} else if( cmd == DO ) {
		if( remopt_ok(opt) ) rem_options[opt-TELOPT_FIRST] = true;
	} else if( cmd == DONT ) {
		if( remopt_ok(opt) ) rem_options[opt-TELOPT_FIRST] = false;
	}
}

/* Take action on and process command received from the server. */

void ArpTelnet::process_opt(int32 cmd, int32 opt)
{
	BAutolock glock(&globals);
	
	/* If this is a local option we don't understand or have not implemented,
	   refuse any 'DO' request. */
	if( cmd == DO && !locopt_ok(opt) ) {
		send_opt(WONT,opt,true);
	
	/* If this is a server option we don't understand or have not implemented,
	   refuse any 'WILL' request. */
	} else if( cmd == WILL && !remopt_ok(opt) ) {
		send_opt(DONT,opt,true);
	
	/* If this is a DONT request, (possibly) send a reply and turn off
	   the option. */
	} else if( cmd == DONT ) {
		send_opt(WONT,opt,false);
	
	/* If this is a WONT request, (possibly) send a reply and turn off
	   the option. */
	} else if( cmd == WONT ) {
		send_opt(DONT,opt,false);
	
	/* If this is a DO request, (possibly) send a reply and turn on
	   the option. */
	} else if( cmd == DO ) {
		send_opt(WILL,opt,false);
		if( opt == TELOPT_NAWS ) send_naws(curWidth,curHeight);
		else if( opt == TELOPT_TTYPE ) curEmuType = -1;
		else if( opt == TELOPT_NAOHTD ) send_naohtd();
	
	/* If this is a WILL request, (possibly) send a reply and turn on
	   the option. */
	} else if( cmd == WILL ) {
		send_opt(DO,opt,false);
	}
	
	// If this is the first command found, start up a true
	// TELNET session.
	
	if( !isTelnet ) {
		isTelnet = true;
		//send_opt(WILL,TELOPT_BINARY,false);
		send_opt(WILL,TELOPT_TSPEED,false);
		send_opt(WILL,TELOPT_TTYPE,false);
		send_opt(WILL,TELOPT_NAWS,false);
		send_opt(WILL,TELOPT_NAOHTD,false);
		//send_opt(DO,TELOPT_BINARY,false);
		send_opt(DO,TELOPT_ECHO,false);
		send_opt(DO,TELOPT_SGA,false);
	}
}

/* Process a subnegotiation sequence.  */

void ArpTelnet::process_sb(int32 opt, ArpString data)
{
	BAutolock glock(&globals);
	
	ArpD(cdb << ADH << "ArpTelnet: Processing SB #" << opt << ": "
					<< data.AsSequence() << endl);
					
	switch( opt ) {
		case TELOPT_TTYPE: {
			ArpD(cdb << ADH << "ArpTelnet: Terminal Type." << endl);
			if( data.Length() < 1 || data[0] != 1 ) break;
			curEmuType++;
			send_ttype();
		} break;
		case TELOPT_TSPEED: {
			ArpD(cdb << ADH << "ArpTelnet: Terminal Speed." << endl);
			if( data.Length() < 1 || data[0] != 1 ) break;
			send_tspeed();
		} break;
		default: {
			ArpD(cdb << ADH << "ArpTelnet: Unknown SB." << endl);
		} break;
	}
}

/* Used for constructing messages. */

int32 ArpTelnet::put_byte(uint8* b, int32 pos, uint8 val)
{
	  b[pos++] = val;
	  if( val == (uint8)IAC ) b[pos++] = val;
	  return pos;
}

/* Send a window size negotiation. */

void ArpTelnet::send_naws(int width, int height)
{
	BAutolock glock(&globals);
	
	if( loc_options[TELOPT_NAWS-TELOPT_FIRST] ) {
		uint8 reply[14];
		int i = 0;
		reply[i++] = IAC;
		reply[i++] = SB;
		reply[i++] = TELOPT_NAWS;
		i = put_byte(reply,i,((width>>8)&0xFF));
		i = put_byte(reply,i,(width&0xFF));
		i = put_byte(reply,i,((height>>8)&0xFF));
		i = put_byte(reply,i,(height&0xFF));
		reply[i++] = IAC;
		reply[i++] = SE;
		reply[i] = 0;
		print_cmd("Client",IAC);
		print_cmd("Client",SB);
		char buffer[128];
		sprintf(buffer,"Client %d x %d",width,height);
		print_opt(buffer,TELOPT_NAWS);
		print_cmd("Client",IAC);
		print_cmd("Client",SE);
		ArpD(cdb << ADH << "WebTerm: Client: len=" << i << ", dat="
						<< ArpEmulator::sequenceToString(ArpString(&reply[0],0,i))
						<< endl);
		sock.Send((char *)reply,i);
	}
}

/* Send a terminal type negotiation. */

void ArpTelnet::send_ttype()
{
	BAutolock glock(&globals);
	
	if( emuTypes.what != TERM_EMULATORS_MSG ) {
		delayedTTYPE = true;
		return;
	}
	
	const char* name;
	status_t ret = B_ERROR;
	ret = emuTypes.FindString("name",curEmuType,&name);
	if( ret == B_OK && curEmuType == 0
			&& strcmp(name,"DUMB") == 0 ) {
		curEmuType = 1;
		ret = emuTypes.FindString("name",curEmuType,&name);
	}
	if( ret != B_OK && curEmuType > 0 ) {
		ret = emuTypes.FindString("name",curEmuType-1,&name);
	}
	if( ret != B_OK ) name = "UNKNOWN";
	int len = strlen(name);
	uint8* reply = new uint8[len + 6];
	if( reply ) {
		int i = 0;
		reply[i++] = IAC;
		reply[i++] = SB;
		reply[i++] = TELOPT_TTYPE;
		reply[i++] = 0;
		for( int j=0; j<len; j++ ) reply[i++] = name[j];
		reply[i++] = IAC;
		reply[i++] = SE;
		print_cmd("Client",IAC);
		print_cmd("Client",SB);
		char buffer[128];
		sprintf(buffer,"Client %s",name);
		print_opt(buffer,TELOPT_TTYPE);
		print_cmd("Client",IAC);
		print_cmd("Client",SE);
		sock.Send((char*)reply,i);
		delete[] reply;
		changeEmulator(name);
	}
}

/* Negotiate output horizontal tab disposition: let us do it all. */

void ArpTelnet::send_naohtd()
{
	BAutolock glock(&globals);
	
	if( loc_options[TELOPT_NAOHTD-TELOPT_FIRST] ) {
		uint8 reply[14];
		int i = 0;
		reply[i++] = IAC;
		reply[i++] = SB;
		reply[i++] = TELOPT_NAOHTD;
		reply[i++] = 0;
		reply[i++] = 0;
		reply[i++] = IAC;
		reply[i++] = SE;
		print_cmd("Client",IAC);
		print_cmd("Client",SB);
		print_opt("Client 0 0",TELOPT_NAOHTD);
		print_cmd("Client",IAC);
		print_cmd("Client",SE);
		ArpD(cdb << ADH << "WebTerm: Client: len=" << i
						<< ", dat=" << (char*)reply << endl);
		sock.Send((char*)reply,i);
	}
}

/* Negotiate about terminal speed: fast as possible! */

void ArpTelnet::send_tspeed()
{
	BAutolock glock(&globals);
	
	if( loc_options[TELOPT_TSPEED-TELOPT_FIRST] ) {
		uint8 reply[20];
		int i = 0;
		reply[i++] = IAC;
		reply[i++] = SB;
		reply[i++] = TELOPT_TSPEED;
		reply[i++] = 0;
		reply[i++] = '5';
		reply[i++] = '2';
		reply[i++] = '0';
		reply[i++] = '0';
		reply[i++] = '0';
		reply[i++] = ',';
		reply[i++] = '5';
		reply[i++] = '2';
		reply[i++] = '0';
		reply[i++] = '0';
		reply[i++] = '0';
		reply[i++] = IAC;
		reply[i++] = SE;
		print_cmd("Client",IAC);
		print_cmd("Client",SB);
		print_opt("Client 0 52000,52000",TELOPT_TSPEED);
		print_cmd("Client",IAC);
		print_cmd("Client",SE);
		ArpD(cdb << ADH << "WebTerm: Client: len=" << i
						<< ", dat=" << (char*)reply << endl);
		sock.Send((char*)reply,i);
	}
}

/* ------------------------------------------------------------
   MISC SUPPORT METHODS
   ------------------------------------------------------------ */

/* Parse escapes in a parameter string. */

ArpString ArpTelnet::parseString(const ArpString& in)
{
	if( ((const char*)in) == NULL || *((const char*)in) == 0 ) {
		return ArpString("");
	}
	ArpString out;
	for( int i=0; i<in.Length(); i++ ) {
		if( in[i] != '\\' ) out += (ichar)in[i];
		else if( ++i < in.Length() ) {
			switch( in[i] ) {
				case '\\':
					out += (ichar)'\\';
					break;
				case 'n':
					out += (ichar)'\n';
					break;
				case 'r':
					out += (ichar)'\r';
					break;
				case 't':
					out += (ichar)'\t';
					break;
				case 'f':
					out += (ichar)'\f';
					break;
				case 'b':
					out += (ichar)'\b';
					break;
				case 'x': {
					int c = 0;
					for( int j=0; j<2 && i<in.Length(); j++,i++ ) {
						c<<=4;
						c = c+(int)(in[i]-'0');
					}
					out += (ichar)c;
				} break;
				default: {
					if( in[i] >= '0' && in[i] <= '9' ) {
						int c = 0;
						for( int j=0; j<3 && i<in.Length(); j++,i++ ) {
							c<<=3;
							c = c+(int)(in[i]-'0');
						}
						out += (ichar)c;
					}
				} break;
			}
		}
	}
	return out;
}

void ArpTelnet::print_cmd(const ArpString& label, int32 cmd)
{
	ArpD(cdb << ADH << label << ": Cmd " << telcmd(cmd)
					<< " (" << cmd << ")" << endl);
}

void ArpTelnet::print_opt(const ArpString& label, int32 opt)
{
	ArpD(
		bool flag = false;
		if( opt >= TELOPT_FIRST && opt <= TELOPT_LAST ) {
			flag = locopts_impl[opt-TELOPT_FIRST];
		}
		cdb << ADH << label << ": Opt " << telopt(opt)
				<< " (" << opt << ") impl=" << flag << endl;
	)
}

bool ArpTelnet::telcmd_ok(int32 cmd)
{
	return (cmd <= TELCMD_LAST && cmd >= TELCMD_FIRST);
}

bool ArpTelnet::locopt_ok(int32 opt)
{
	return (opt <= TELOPT_LAST && opt >= TELOPT_FIRST
			&& locopts_impl[opt-TELOPT_FIRST]);
}

bool ArpTelnet::remopt_ok(int32 opt)
{
	return (opt <= TELOPT_LAST && opt >= TELOPT_FIRST
				&& remopts_impl[opt-TELOPT_FIRST]);
}

ArpString ArpTelnet::telcmd(int32 cmd)
{
	if( telcmd_ok(cmd) ) {
		return ArpString(telcmds[cmd-TELCMD_FIRST]);
	}
	return ArpString("<?CMD?>");
}

ArpString ArpTelnet::telopt(int32 opt)
{
	if( opt >= TELOPT_FIRST && opt <= TELOPT_LAST ) {
		return ArpString(telopts[opt-TELOPT_FIRST]);
	}
	return ArpString("<?OPT?>");
}

