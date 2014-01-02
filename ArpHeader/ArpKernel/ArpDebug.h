/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpDebug.h
 *
 * Debugging Support
 *
 * This is a set of macros for defining a standard debugging architecture
 * for the ARP libraries and applications.  All of these macros and functions
 * are designed to only instrument the code if ArpDEBUG is defined;
 * otherwise, they introduce no overhead.
 *
 * ArpASSERT(condition) is the same as ASSERT(), except the assertion check is
 * made only when ArpDEBUG is defined.  Note that this means you should check
 * the condition twice -- first with ArpASSERT(), then to do some "sane
 * thing" when the condition fails in a production environment.
 *
 * Example:
 *
 *   void my_func(int posnum) {
 *     ArpASSERT(posnum >= 0);
 *     if( posnum < 0 ) return;
 *     // now do something useful
 *   }
 *
 * ArpVALIDATE(condition, recovery) is a standard implementation of the
 * above assertion strategy.  It says, "if 'condition' is false, either
 * raise an assertion or execute 'recovery' code."  An assertion raised when
 * ArpDEBUG is defined; otherwise, the 'recovery' code is simply executed,
 * which should keep the program in a sane state.  The above example can
 * be re-written with this macro as:
 *
 *   void my_func(int posnum) {
 *     ArpVALIDATE(posnum >= 0, return);
 *     // now do something useful
 *   }
 *
 * ArpD(x) and ArpDL(mod,level,x) are used to place debugging output in the
 * code.  The 'x' argument is the statement needed to display the
 * output -- typically, this is something like 'cdb << "My text" << endl'.
 * In the second form, 'mod' is the name of the module being
 * instrumented, and 'level' is the debugging level it needs to be set
 * at to have this particular statement executed; the first form sets
 * these to __FILE__ and 1, respectively.
 *
 * Example:
 *
 *   ...
 *   int myvar = get_value();
 *   ArpD(cdb << ADH << "My value set to " << myvar << endl);
 *   ...
 *   for( int i=0; i<10000; i++ ) {
 *     ArpDL("mymod",3,cdb << ADH << "At index " << i << endl);
 *     do_something(i);
 *   }
 *   ...
 *
 * ArpDB() and ArpDLB(mod,level) are used to create complete blocks of
 * instrumentation; these are essentially wrappers around a conditional,
 * and the code block should follow.  Note that you MUST still surround
 * the entire section by conditional compilation so that it will be removed
 * during a production build.
 *
 * Example:
 *
 *     int* array = create_data(100);
 *     ArpVALIDATE(array != NULL, return);
 *
 *   #if defined(ArpDEBUG)
 *     ArpDLB("example",3) {
 *       for( int i=0; i<100; i++ )
 *         cdb << ADH << "Value " << i << " = " << array[i] << endl;
 *     }
 *   #endif
 *
 *     ...
 *
 * WARNING:
 *
 * The above described debugging code is implemented using the ArpString class.
 * This means that you MUST be careful when instrumenting that class: when using
 * ArpD() et. al., always specify the module name of "core", which is treated as a
 * special case so that it does not use any library functionality.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 0.1: Created this file.
 *
 */

#ifndef ARPKERNEL_ARPDEBUG_H
#define ARPKERNEL_ARPDEBUG_H


//FORCE DEBUG
#define ArpDEBUG 1

#if defined(ArpDEBUG)
#ifndef DEBUG
#define DEBUG 1
#endif
#endif

#include <assert.h>

#include <be/support/Debug.h>

#define RPTERR(ARGS)	_debugPrintf ARGS

#if defined(ArpDEBUG)

#include <iostream.h>

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#define ArpASSERT(x) ASSERT(x)
#define ArpVALIDATE(x, ret) if( !(x) ) { ASSERT(x); ret; }

#ifndef DB_DELAY
#define DB_WAIT(x)
#else
#define DB_WAIT(x) snooze(x)
#endif

// Get and set the debug level for a particular module; valid levels
// are >= 0, with 0 turning off debug output for that module.
int ArpDebugLevel(const char* module);
void ArpDebugLevel(const char* module, int level);
void ArpDebugDeclare(const char* module, int level);

// Set the debug level for ALL modules.
void ArpAllDebugLevels(int level);

// Set debug levels from command line options
void ArpParseDBOpts(int32& argc, char** argv);

ArpString ArpDebugHeader(const char* module, int line);

// Show and hide a user interface for changing module debug levels.
//void ArpShowDebugGUI(void);
//void ArpHideDebugGUI(void);

// Define debug output stream.  Under Unix, this is sent out the stderr
// stream; under Windows, it is streamed into the debugger using
// OutputDebugText().
ostream& ArpDBStream(void);
#define cdb ArpDBStream()

// Sequencing of debug output between multiple threads.
void ArpLockDebugOut();
void ArpUnlockDebugOut();

// Tag that should be placed at the front of debug output.
#define ADH ArpDebugHeader(__FILE__,__LINE__)

// Declare the existence of a debugging module.
class ArpDeclareModule {
public:
	ArpDeclareModule(const char* mod, int level=0)
		{ ArpDebugDeclare(mod,level); }
};

#define ArpMODL(mod,level,var) static ArpDeclareModule var(mod,level)
#define ArpMOD() ArpMODL(__FILE__,0,arp_decldefmod)

// Instrumenting full code blocks
#define ArpDLB(mod,level) if( ArpDebugLevel(mod) >= level )
#define ArpDB() ArpDLB(__FILE__,1)

// Instrumenting single statements
#define ArpDL(mod,level,x) ArpDLB(mod,level) { ArpLockDebugOut(); x; ArpUnlockDebugOut(); }
#define ArpD(x) ArpDL(__FILE__,1,x)

// Marking place in a file
#define ArpMARKL(mod,level) ArpDL(mod,level,						\
					 cdb << "ArpMARK: at line " << __LINE__		\
						   << " of file " << __FILE__ << endl);
#define ArpMARK() ArpMARKL(__FILE__,1)

#define ArpPOLISH(__A__) cdb << "ArpPOLISH: At line " << __LINE__ << \
								"of file " << __FILE__ <<": " __A__
#define ArpFINISH() cdb << "ArpFINISH: At line " << __LINE__ << \
								"of file " << __FILE__ <<", finish this!"
// OLD DEBUGGING INFRASTRUCTURE -- DO NOT USE IN NEW CODE
#define D(x) x
#define DB(level,x) { if(level) { x; } cdb.flush(); DB_WAIT(DB_DELAY) }
#define DBALL 0xffff
#define stddb stdout
#define bug fprintf

#else

// If debugging is not enabled, clear all of the debugging macros so that
// no instrumentation is added into the program.

#define ArpASSERT(x)
#define ArpVALIDATE(x, ret) if( !(x) ) { ret; }
#define ArpParseDBOpts(argc, argv)
#define ADH
#define ArpMOD()
#define ArpMODL(mod,level,var)
#define ArpD(x)
#define ArpDB()
#define ArpDL(mod,level,x)
#define ArpDLB(mod,level)
#define ArpMARK()
#define ArpMARKL(mod,level)
//#define ArpHideDebugGUI()
//#define ArpShowDebugGUI()


// OLD DEBUGGING INFRASTRUCTURE -- DO NOT USE IN NEW CODE
#define DB_WAIT(x)
#define D(x)
#define DB(level,x)
#define DBALL 0
#define bug

#endif

#if defined(DEBUG) || defined(USE_STREAMS)
#include <iostream.h>

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

// We only define these stream operators when debugging is turned
// on, since using streams bloats code so much...

inline ostream& operator << (ostream& os, const ArpString & str)
{ os << ((const char*)str ? (const char*)str : "<null>"); return os; }

class BPoint;
class BRect;
class BFont;
class BMessenger;
class BMessage;
class BPath;
struct entry_ref;
struct rgb_color;
struct pattern;

ostream& operator << (ostream& os, const BPoint & bp);
ostream& operator << (ostream& os, const BRect & br);
ostream& operator << (ostream& os, const BFont & fn);
ostream& operator << (ostream& os, const BMessenger & o);
ostream& operator << (ostream& os, const BPath & o);
ostream& operator << (ostream& os, const entry_ref & o);
ostream& operator << (ostream& os, const rgb_color & o);
ostream& operator << (ostream& os, const pattern & o);
ostream& operator << (ostream& os, const BMessage & msg);

ostream& ArpToStream(ostream& os, const BMessage& msg, const char* prefix="");

#endif
	
#endif
