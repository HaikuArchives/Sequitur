/*
	
	ArpRemoteDevice.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/


#if 0

#ifndef ARPCOMMON_ARPREMOTEDEVICE_H
#include <ArpCommon/ArpRemoteDevice.h>
#endif

#ifndef ARPCOMMON_ARPTERMINALMSG_H
#include <ArpCommon/ArpTerminalMsg.h>
#endif

#ifndef ARPCOMMON_ARPDEBUG_H
#include <ArpCommon/ArpDebug.h>
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#include <signal.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>

#include <OS.h>
#include <image.h>
#include <errno.h>
#include <cstdlib>
#include <termios.h>

ArpMOD();

#define ctrl(c) ((c) - 0100)  
              
const static struct termios tdefault = {
	ICRNL,                          /* c_iflag */
	OPOST|ONLCR,                    /* c_oflag */
	B19200|CS8|CREAD|HUPCL,         /* c_cflag */
	ISIG|ICANON|ECHO|ECHOE|ECHONL,  /* c_lflag */
	0,                              /* c_line */
	0,                              /* c_ixxxxx */
	0,                              /* c_oxxxxx */
	ctrl( 'C'),                     /* c_cc[VINTR] */
	ctrl( '\\'),                    /* c_cc[VQUIT] */
	ctrl( 'H'),                     /* c_cc[VERASE] */
	ctrl( 'U'),                     /* c_cc[VKILL] */
	ctrl( 'D'),                     /* c_cc[VEOF] */
	0,                              /* c_cc[VEOL] */
	0,                              /* c_cc[VEOL2] */
	0,                              /* c_cc[VSWTCH] */
	ctrl( 'S'),                     /* c_cc[VSTART] */
	ctrl( 'Q'),                     /* c_cc[VSTOP] */
	ctrl( 'Z')                      /* c_cc[VSUSP] */
};

          
/* find a tty/pty pair, launch argv[0], return ptyfd on success, -1 on failure */
static int32 ExecControlled(char *argv[], int argc, thread_id *thr)
{
	uint32 count;
	char *suf1Pt = "pqrstuvwxyzPQRST";
	char ptyDev[20];
	char ttyDev[20];
	int32 ttyfd = -1;
	int32 ptyfd = -1;
	int32 pgid;
	thread_id shellThread;
	int stdinfd, stdoutfd, stderrfd;
		
	/* open a pseudoterminal */
	while(*suf1Pt && ttyfd < 0) {
		for(count = 0; count < 16; count++) {
			/* find an unused pty/tty name */
			char ptyDev[20];
			sprintf(ptyDev, "/dev/pt/%c%x", *suf1Pt, count);
			sprintf(ttyDev, "/dev/tt/%c%x", *suf1Pt, count);
			ptyfd = open(ptyDev, O_RDWR);
			if (ptyfd >= 0) {
				/* pty master opened, try opening matching slave */
				ttyfd = open(ttyDev, O_RDWR);
				if (ttyfd >= 0) {
					/* we are done */
					break;
				}				
				/* failed to open slave, try with another master */
				close(ptyfd);
				ptyfd = -1;
			} else if (errno == ENOENT) {	
				break;
			}				
		}
		suf1Pt++;		 
	}
	
	if (ptyfd < 0) {
		fprintf(stderr,"failed to oen a pty\n");
		return -1;
	}
	
	/* initialize the tty */
	ioctl(ttyfd, TCSETA, &tdefault);
	
	/* save copies of our existing STDIO */
	stdinfd = dup(STDIN_FILENO);
	stdoutfd = dup(STDOUT_FILENO);
	stderrfd = dup(STDERR_FILENO);
	
	/* close the originals */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	/* hook up the tty */
	dup(ttyfd);
	dup(ttyfd);
	dup(ttyfd);
	
	/* ensure the spawned process doesn't keep our fd's */
	fcntl(stdinfd, F_SETFD, FD_CLOEXEC);
	fcntl(stdoutfd, F_SETFD, FD_CLOEXEC);
	fcntl(stderrfd, F_SETFD, FD_CLOEXEC);
	fcntl(ptyfd, F_SETFD, FD_CLOEXEC);
	fcntl(ttyfd, F_SETFD, FD_CLOEXEC);
        
		/* prepare an environment */
	{
		char buffer[40];
		sprintf(buffer, "TTY=%s", ttyDev);
		putenv("TERM=beterm");
		putenv(buffer);
	}


	/* load the new process */
	shellThread = load_image(argc, argv, environ);

	/* restore STDIO */
	dup2(stdinfd, STDIN_FILENO);
	dup2(stdoutfd, STDOUT_FILENO);
	dup2(stderrfd, STDERR_FILENO);
	close(stdinfd);
	close(stdoutfd);
	close(stderrfd);
	close(ttyfd);
	
	if (shellThread < 0) {             
		close(ptyfd);
		fprintf(stderr,"can't find the shell\n");
		return -1;
	}

	*thr = shellThread;
	
	/* start the child process goinf */
	setpgid(shellThread, shellThread);
	resume_thread(shellThread);
	pgid = shellThread;
	ioctl(ptyfd, 'pgid', pgid);
	
	return ptyfd;
}


ArpPTYRemote::ArpPTYRemote(const BMessenger& term)
	: BLooper("ARP™ PTY Remote"), terminal(term)
{
	initialize();
}

ArpPTYRemote::ArpPTYRemote()
	: BLooper("ARP™ PTY Remote")
{
	initialize();
}

ArpPTYRemote::~ArpPTYRemote()
{
	Disconnect();
}

void ArpPTYRemote::initialize()
{
	// Global state
	socketError = B_NO_ERROR;
	doEcho = true;
	curEmuType = -1;
	curEmuName = "";
	curWidth = curHeight = 0;
	
	// No files currently open
	ptyfd = -1;
	
	// No shell is running
	shellThread = -1;
	
	// Reader thread and its associated private data.
	// (Note that the socket input handle is also private to
	//  the reader thread.)
	readThread = -1;
	
	// Start looper to receive input from terminal.
	
	Run();
}

status_t ArpPTYRemote::Connect()
{
	static char *argv[] = { "/bin/sh", "-login", NULL };
	
	BAutolock llock(this);
	
	Disconnect();
	
	BAutolock glock(&globals);
	
	socketError = B_ERROR;
	
	if((ptyfd = ExecControlled(argv, 2, &shellThread)) >= 0) {
	
		// Now start reader thread to receive input from remote.
	
		BAutolock llock(this);
	
		BAutolock glock(&globals);
	
		// Just start up the reader thread, and let it do all else.
		ArpD(cdb << ADH << "ArpPTYRemote: Spawn reader thread." << std::endl);
		readThread = spawn_thread(readThreadEntry,
									"ARP™ PTY Reader",
									B_NORMAL_PRIORITY,
									this);
		if( readThread < 0 ) {
			Disconnect();
			return (status_t)readThread;
		}
		ArpD(cdb << ADH << "ArpPTYRemote: Resume reader thread." << std::endl);
		socketError = resume_thread(readThread);
		
		// Kill the thread if unable to start it.
		if( socketError ) Disconnect();
	}
	
	return socketError;
}

void ArpPTYRemote::Disconnect()
{
	BAutolock llock(this);
	
	ArpD(cdb << ADH << "ArpPTYRemote: Disconnecting..." << std::endl);

	globals.Lock();
	
	if( shellThread >= 0 ) {
		send_signal(shellThread, SIGHUP);
		shellThread = -1;
	}
	
	thread_id mythread = readThread;
	readThread = -1;
	// If the connected flag has been set and the thread is
	// running, it's in its read loop and needs to be forced
	// to quit.
	while( connected && mythread >= 0 ) {
		ArpD(cdb << ADH << "ArpPTYRemote: signal " << SIGUSR1
							<< "to reader " << mythread << std::endl);
		send_signal(mythread, SIGUSR1);
		globals.Unlock();
		ArpD(cdb << ADH << "ArpPTYRemote: waiting for reader." << std::endl);
		snooze(20000);
		//wait_for_thread(mythread,&ret);
		ArpD(cdb << ADH << "ArpPTYRemote: checking reader again." << std::endl);
	}
	ArpD(cdb << ADH << "ArpPTYRemote: no reader running." << std::endl);
	
	globals.Unlock();
	
	if( ptyfd >= 0 ) {
		close(ptyfd);
		ptyfd = -1;
	}
}

/* ------------------------------------------------------------
   HIGH-LEVEL MESSAGE HANDLING
   ------------------------------------------------------------ */

void ArpPTYRemote::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	ArpD(cdb << ADH << "Telnet::MessageReceived: " <<
				*message << std::endl);

	switch( message->what ) {
		case TERM_XFER_TEXT_MSG: {
			const char* str = NULL;
			ssize_t len = 0;
			int32 pos = 0;
			while( message->FindData("text", B_ASCII_TYPE, pos,
									&str, &len) == B_NO_ERROR ) {
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
			#if 0
			if( delayedTTYPE ) {
				send_ttype();
				delayedTTYPE = false;
			}
			#endif
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

void ArpPTYRemote::receive(const char* str, int32 len)
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

void ArpPTYRemote::changeEmulator(const char* name)
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

void ArpPTYRemote::setWindowSize(int32 width, int32 height)
{
	BAutolock glock(&globals);
	
	if( curWidth != width || curHeight != height ) {
		curWidth = width;
		curHeight = height;
		struct winsize ws;
		ws.ws_row = height;
		ws.ws_col = width;
		ws.ws_xpixel = width*8;
		ws.ws_ypixel = height*8;
		ioctl(ptyfd,TIOCSWINSZ,(char*)&ws);
		//send_signal(shellThread, SIGWINCH);
		//send_naws(width,height);
	}
}

void ArpPTYRemote::setEmulator(const char* name)
{
	BAutolock glock(&globals);
	
	if( name && curEmuName != name ) {
		curEmuName = name;
		// TELNET protocol doesn't let us dynamically change, yes?
	}
}

void ArpPTYRemote::write(char b)
{
	BAutolock glock(&globals);
	
	if( connected ) {
		ArpD(cdb << ADH << "ArpPTYRemote: Put: " << (char)b
							<< " (" << (int)b << ")" << std::endl);
		int size = ::write(ptyfd, &b, 1);
		ArpD(cdb << ADH << "Bytes written: " << size << std::endl);
		ArpD(cdb << ADH << "Current error: " << strerror(errno) << std::endl);
		if( doEcho ) {
			receive(&b,1);
		}
	} else receive(&b,1);
}
  
void ArpPTYRemote::write(const char* b, int32 len)
{
	BAutolock glock(&globals);
	
	if( connected && len > 0 ) {
		ArpD(cdb << ADH << "WebTerm: Put: " << b
							<< " (" << len << " bytes)" << std::endl);
		int size = ::write(ptyfd, b, len);
		ArpD(cdb << ADH << "Bytes written: " << size << std::endl);
		ArpD(cdb << ADH << "Current error: " << strerror(errno) << std::endl);
		if( doEcho ) {
			receive(b,len);
		}
	} else if( len > 0 ) receive(b,len);
}
  
void ArpPTYRemote::write(const ArpString& str)
{
	write(str,str.Length());
}
  
/* ------------------------------------------------------------
   INPUT THREAD METHODS
   ------------------------------------------------------------ */

static void ignore_signal(int)
{
	ArpD(cdb << ADH << "ArpPTYRemote: entered ignore_signal(), thread="
						<< find_thread(NULL) << std::endl);
}

/* Entry point and main loop.
   Note well: we must be careful that any methods that read
   from the stream (either directly with in.read(), or indirectly
   with next_char()) are not synchronized.  Otherwise, they may
   block and leave other threads unable to access the class and
   write more data to the socket. */

int32 ArpPTYRemote::readThreadEntry(void* arg)
{
	ArpD(cdb << ADH << "ArpPTYRemote: Enter the reader." << std::endl);
	signal(SIGUSR1,ignore_signal);
	ArpPTYRemote *obj = (ArpPTYRemote *)arg; 
	return (obj->startReader()); 
}

int32 ArpPTYRemote::startReader(void)
{
	connected = true;
	doEcho = false;
	
	int32 retval = runReader();

	doEcho = true;
	connected = false;
	
	return retval;
}

int32 ArpPTYRemote::runReader(void)
{
	uint8 buffer[1024];
	
	bool client_kill = false;
    ArpD(cdb << ADH << "WebTerm: Telnet input thread has started."
    				<< std::endl);
	ArpD(cdb << ADH << "WebTerm: Telnet thread: "
					<< find_thread(NULL) << std::endl);
	receive("Connected.\r\n");
	for(;;) {
		long len = 0;
		status_t ret = EINTR;
		if( readThread >= 0 ) {
			ArpD(cdb << ADH << "ArpPTYRemote: Ready to receive." << std::endl);
			len = ::read(ptyfd, buffer, sizeof(buffer)-4);
		}
		//if( ret == 0 ) ret = ECONNABORTED;
		if( (ret < B_OK && ret != EINTR) || readThread < 0 ) {
			ArpD(cdb << ADH << "ArpPTYRemote: Input thread is terminating."
							<< std::endl);
			socketError = ret;
			return ret;
		}
		if( len > 0 ) {
			ArpD(cdb << ADH << "ArpPTYRemote: Now read " << len << " bytes:"
							<< ArpEmulator::sequenceToString(ArpString(&buffer[0],0,len))
							<< std::endl);
			receive((const char*)&buffer[0],len);
		} else {
			ArpD(cdb << ADH << "ArpPTYRemote: Nothing read." << std::endl);
		}
	}
}

/* ------------------------------------------------------------
   MISC SUPPORT METHODS
   ------------------------------------------------------------ */

/* Parse escapes in a parameter string. */

ArpString ArpPTYRemote::parseString(const ArpString& in)
{
	if( ((const char*)in) == NULL || *((const char*)in) == 0 ) {
		return ArpString("");
	}
	ArpString out;
	for( int i=0; i<in.Length(); i++ ) {
		if( in[i] != '\\' ) out += in[i];
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

#endif
