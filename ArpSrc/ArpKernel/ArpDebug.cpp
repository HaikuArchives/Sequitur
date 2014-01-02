/*
	
	ArpDebug.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

//#if defined(ArpDEBUG)
#define USE_STREAMS 1
//#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <Autolock.h>

// ----------------------- DEBUGGING IMPLEMENTATION -----------------------

ArpString ArpDebugHeader(const char* mod, int line)
{
	ArpString out("<");
	if( mod ) {
		const char* s1 = strrchr(mod,'/');
		if( s1 != 0 ) mod = s1+1;
		out += mod;
	}
	out += "#";
	out += int32(line);
	out += "> ";
	return out;
}

#if defined(ArpDEBUG)

#include <map>

ostream& ArpDBStream(void)
{
	return cerr;
}

static BLocker gDebugLock;

void ArpLockDebugOut()
{
	gDebugLock.Lock();
}

void ArpUnlockDebugOut()
{
	gDebugLock.Unlock();
}

// This is the object that maps module names to their current
// debugging level.  We don't make a static instance of the object,
// because that would introduce dependencies in the link order of
// files.  By doing it this way, the object is created as soon as
// any instrumented code is executed.
static map<ArpString,int>* modlevel = NULL;
static int corelevel = 0;
static bool terminated = false;

class ArpDebugCleanup {
public:
	~ArpDebugCleanup() {
		ArpDL("core", 1, cdb << ADH << "Cleaning up debug statics." << endl);
		delete modlevel;
		modlevel = 0;
		terminated = true;
	}
};

static ArpDebugCleanup debugcleanup;

static inline map<ArpString,int>* level_map(void)
{
	if( terminated ) return 0;
	if( !modlevel ) {
		modlevel = new map<ArpString,int>();
		#if 0
		ArpString one("One"), two("Two");
		cdb << "strcmp(one,two) == " << strcmp(one, two) << endl;
		cdb << "one.Compare(two) == " << one.Compare(two) << endl;
		cdb << "one < two == " << (one < two ? "true" : "false") << endl;
		#endif
	}
	return modlevel;
}

const map<ArpString,int>& ArpDebugLevel(void)
{
	map<ArpString,int>* level = level_map();
	ArpASSERT(level != NULL);
	(*level)["core"] = corelevel;
	return *level;
}

static ArpString ArpDebugName(const char* mod)
{
	ArpASSERT(mod != NULL);
	if( mod == NULL ) return ArpString("");
	
	ArpString retName(mod);
	int length = retName.Length()+1;
	char* name = retName.LockBuffer(length);
	if( name ) {
		char* start = strrchr(name,'/');
		if( !start ) start = name;
		else start++;
		
		char* end = strrchr(name,'.');
		
		length = 0;
		while( *start && (end == 0 || start < end) ) {
			*name = *start;
			name++;
			start++;
			length++;
		}
		
		*name = 0;
	}
	
	retName.UnlockBuffer(length);
	return retName;
}

int ArpDebugLevel(const char* module)
{
	ArpASSERT(module != NULL);
	if( module == 0 ) return 0;

	if( strcmp(module,"core") == 0 ) return corelevel;
	map<ArpString,int>* level = level_map();
	if( level == 0 ) return 0;
	
	const ArpString modName(ArpDebugName(module));
	if( corelevel > 4 ) {
		cdb << ADH << "Name to get module '" << module
			<< "' is '" << modName << "'"
			<< endl;
	}
	
	map<ArpString, int>::iterator item = level->find(modName);
	if( item == level->end() ) {
		if( corelevel >= 1 ) {
			cdb << ADH << "First access of module " << modName
				<< "; setting to level 0" << endl;
		}
		(*level)[modName] = 0;
	}
	
	return (*level)[modName];
}

void ArpDebugLevel(const char* module, int level)
{
	ArpVALIDATE(module != NULL, return);
	ArpVALIDATE(level >= 0, level = 0);

	if( strcmp(module,"core") == 0 ) {
		if( corelevel >= 1 ) {
			cdb << ADH << "Setting module " << module
				<< " to level " << level << endl;
		}
		corelevel = level;
		return;
	}
	map<ArpString,int>* levelmap = level_map();
	if( levelmap == 0 ) {
		cdb << ADH << "ArpDebugLevel: Unable to retrieve level map!" << endl;
		return;
	}
	
	const ArpString modName(ArpDebugName(module));
	if( corelevel > 4 ) {
		cdb << ADH << "Name to set module '" << module
			  << "' is '" << modName << "'"
			  << endl;
	}
	
	if( corelevel >= 1 ) {
		cdb << ADH << "Setting module " << modName
			<< " to level " << level << endl;
	}
	
	(*levelmap)[modName] = level;
}

void ArpDebugDeclare(const char* module, int level)
{
	//ArpDebugLevel(module, level);
#if 1
	ArpVALIDATE(module != NULL, return);
	ArpVALIDATE(level >= 0, level = 0);

	if( strcmp(module,"core") == 0 ) {
		if( corelevel >= 1 ) {
			cdb << ADH << "Initializing module " << module
				<< " to level " << level << endl;
		}
		return;
	}
	map<ArpString,int>* levelmap = level_map();
	if( levelmap == 0 ) {
		cdb << ADH << "ArpDebugDeclare: Unable to retrieve level map!" << endl;
		return;
	}
	
	const ArpString modName(ArpDebugName(module));
	if( corelevel > 4 ) {
		cdb << ADH << "Name to declare module '" << module
			  << "' is '" << modName << "'"
			  << endl;
	}
	
	map<ArpString, int>::iterator item = levelmap->find(modName);
	if( item == levelmap->end() ) {
		if( corelevel >= 1 ) {
			cdb << ADH << "Initializing module " << modName
				<< " to level " << level << endl;
		}
		(*levelmap)[modName] = level;
	} else {
		if( corelevel >= 1 ) {
			cdb << ADH << "Can't initialize module " << modName
				<< " to level " << level << ": already at level "
				<< (*levelmap)[modName] << endl;
		}
	}
#endif
}

void ArpAllDebugLevels(int level)
{
	map<ArpString,int>* levelmap = level_map();
	if( levelmap == 0 ) return;
	
	map<ArpString,int>::iterator i;
	for( i=levelmap->begin(); i!=levelmap->end(); i++ ) {
		ArpDebugLevel(i->first, level);
	}
	ArpDebugLevel("core", level);
}

void ArpParseDBOpts(int32& argc, char** argv)
{
	ArpDL("core", 1, cdb << "Parsing " << &argc << " debug arguments..." << endl);
	
	if( argv == 0 ) return;
	
	int i=1, j=1;
	while( i < argc ) {
		ArpDL("core", 1, cdb << "Parse: i=" << i << ", j=" << j
							<< ", argc=" << &argc << ", argv[i]="
							<< (argv[i] ? argv[i] : "<NULL>") << endl);
		if( argv[i] && strcmp(argv[i],"--debug") == 0 ) {
			ArpDL("core", 1, cdb << "Found option: " << argv[i] << endl);
			ArpAllDebugLevels(1);
			i++;
		} else if( argv[i] && strncmp(argv[i],"--debug=",8) == 0 ) {
			ArpDL("core", 1, cdb << "Parsing option: " << argv[i] << endl);
			char* str = argv[i] + 8;
			const char* module = 0;
			while( *str ) {
				ArpDL("core", 1, cdb << "Next parse: " << str << endl);
				const char* base = str;
				while( *str >= '0' && *str <= '9' ) str++;
				if( *str == 0 || *str == ':' || *str == ',' ) {
					if( *str != 0 ) {
						*str = 0;
						str++;
					}
					if( module ) {
						ArpDL("core", 1, cdb << "Setting module " << module
											<< " to level " << base << endl);
						ArpDebugLevel(module, atoi(base));
					} else {
						ArpDL("core", 1, cdb << "Setting everything"
											<< " to level " << base << endl);
						ArpAllDebugLevels(atoi(base));
					}
					module = 0;
				} else {
					module = base;
					while( *str != 0 && *str != ':' && *str != ',' ) str++;
					char term = *str;
					if( term ) {
						*str = 0;
						str++;
					}
					ArpDL("core", 1, cdb << "Found module: " << module << endl);
					if( term != ':' ) {
						ArpDL("core", 1, cdb << "Defaulting module to level 1"
											<< endl);
						ArpDebugLevel(module, 1);
						module = 0;
					}
				}
			}
			i++;
		} else if( argv[i] && strcmp(argv[i],"--list-modules") == 0 ) {
			ArpDL("core", 1, cdb << "Found option: " << argv[i] << endl);
			map<ArpString,int>* levelmap = level_map();
			if( levelmap != 0 ) {
				map<ArpString,int>::iterator i;
				for( i=levelmap->begin(); i!=levelmap->end(); i++ ) {
					cdb << "Module '" << i->first
						<< "' is at debug level " << i->second << endl;
				}
				cdb << "Module 'core' is at debug level " << corelevel << endl;
			}
			i++;
		} else if( argv[i] &&
					( strcmp(argv[i],"--help") == 0 ||
					  strcmp(argv[i],"-h") == 0 ||
					  strcmp(argv[i],"-?") == 0 ) ) {
			cdb << "Debug Options: --debug=[module][:level][...] --list-modules"
				<< endl;
			i++;
			j++;
		} else {
			ArpDL("core", 1, cdb << "Skipping argument: "
								<< (argv[i] ? argv[i]:"<NULL>") << endl);
			if( j < i ) argv[j] = argv[i];
			i++;
			j++;
		}
	}
	
	argc = j;
}

#else

// stubs

#include <SupportDefs.h>

void ArpLockDebugOut()
{
}

void ArpUnlockDebugOut()
{
}

int ArpDebugLevel(const char* )
{
	return 0;
}

void ArpDebugLevel(const char* , int )
{
}

void ArpDebugDeclare(const char* , int )
{
}

void ArpAllDebugLevels(int)
{
}

#undef ArpParseDBOpts
void ArpParseDBOpts(int32& , char** )
{
}

#endif

// ----------------------- IOSTREAM FUNCTIONS -----------------------

#ifndef _APPLICATION_H
#include <app/Application.h>
#endif

#ifndef _FONT_H
#include <Font.h>
#endif

#ifndef _ENTRY_H
#include <Entry.h>
#endif

#ifndef _PATH_H
#include <Path.h>
#endif

#ifndef _TEXTVIEW_H
#include <TextView.h>
#endif

#ifndef __BSTRING__
#include <support/String.h>
#endif

#ifndef _FFONT_H
#include <FFont/FFont.h>
#endif

enum {
	// This is our own custom type.  It indicates that the value for this field
	// is stored in some other "global" message, under the given name.  The value is
	// actually stored as a \0-terminated string.
	// the meaning of what a "global" message is and where to find it is undefined.
	ARP_INDIRECT_TYPE = 'INDr'
};

#if 0
// This is to get at the ARP_INDIRECT_TYPE definition.
#ifndef ARPLAYOUT_ARPPARAM_H
#include <ArpLayout/ArpParam.h>
#endif
#endif

#include <iostream.h>

ostream& operator << (ostream& os, const BPoint & bp)
{
	os << "BPoint(" << bp.x << "," << bp.y << ")"; return os;
}

ostream& operator << (ostream& os, const BRect & br)
{
	os << "BRect(" << br.left << "," << br.top
     << ")-(" << br.right << "," << br.bottom << ")";
	return os;
}

ostream& operator << (ostream& os, const BFont & fn)
{
	ios::fmtflags fl = os.flags();
	font_family family;
	font_style style;
	fn.GetFamilyAndStyle(&family,&style);
	os << "BFont(id=" << hex << fn.FamilyAndStyle() << dec
		<< ",fam=" << &family[0] << ",sty=" << &style[0]
		<< ",sz=" << fn.Size() << ",shr=" << fn.Shear()
		<< ",rot=" << fn.Rotation() << ",enc=" << fn.Encoding()
		<< ",spc=" << fn.Spacing() << ",fc=" << fn.Face()
		<< ",flg=" << fn.Flags() << ",dir=" << (int)fn.Direction()
		<< ")";
	os.flags(fl);
	return os;
}

ostream& operator << (ostream& os, const BMessenger & o)
{
	ios::fmtflags fl = os.flags();
	BHandler* hnd = NULL;
	BLooper* loop = NULL;
	hnd = o.Target(&loop);
	os << "BMessenger(valid=" << o.IsValid()
		<< hex //<< ",team=" << o.Team()
		<< ",hnd=" << hnd << ",lp=" << loop
		<< dec << ",local=" << o.IsTargetLocal() << ")";
	os.flags(fl);
	return os;
}
		
ostream& operator << (ostream& os, const BPath & o)
{ os << "BPath(" << (o.Path() ? o.Path() : "<null>") << ")"; return os; }

ostream& operator << (ostream& os, const entry_ref & o)
{
	ios::fmtflags fl = os.flags();
	os // << "entry_ref(dev=" << o.device
		<< ",dir=" << hex << (int)(o.directory>>32)
		<< (int)(o.directory) << (int) dec
		<< ",name=" << o.name << ")";
	os.flags(fl);
	return os;
}

ostream& operator << (ostream& os, const rgb_color & o)
{
	os << "rgb_color(r=" << (int)o.red << ",g=" << (int)o.green
		<< ",b=" << (int)o.blue << ",a=" << (int)o.alpha << ")";
	return os;
}

ostream& operator << (ostream& os, const pattern & o)
{ os << "pattern(0x" << (void*)&o << ")"; return os; }

ostream& operator << (ostream& os, const BMessage & msg)
{
	return ArpToStream(os, msg);
}

static ostream& print_code(ostream& os, uint32 code)
{
	const char c1 = (char)((code>>24)&0xFF);
	const char c2 = (char)((code>>16)&0xFF);
	const char c3 = (char)((code>>8)&0xFF);
	const char c4 = (char)(code&0xFF);
	if( c1 >= ' ' && c1 < 127 && c2 >= ' ' && c2 < 127
		&& c3 >= ' ' && c3 < 127 && c4 >= ' ' && c4 < 127 ) {
		os << "'" << c1 << c2 << c3 << c4 << "'";
	} else {
		os << "0x" << hex << code << dec;
	}
	
	return os;
}

ostream& ArpToStream(ostream& os, const BMessage & msg, const char* basePrefix)
{
	ios::fmtflags fl = os.flags();
	
	BString newPrefix(basePrefix);
	newPrefix += "  ";
	
	const char* prefix = newPrefix.String();

	os << "BMessage(";
	print_code(os, msg.what);
	
#if 0
	const char c1 = (char)((msg.what>>24)&0xFF);
	const char c2 = (char)((msg.what>>16)&0xFF);
	const char c3 = (char)((msg.what>>8)&0xFF);
	const char c4 = (char)(msg.what&0xFF);
	if( c1 >= ' ' && c1 < 127 && c2 >= ' ' && c2 < 127
		&& c3 >= ' ' && c3 < 127 && c4 >= ' ' && c4 < 127 ) {
		os << "BMessage('" << c1 << c2 << c3 << c4 << "'";
	} else {
		os << "BMessage(0x" << hex << msg.what << dec;
	}
#endif
	
	os << ") {" << endl;
	
	char* name;
	type_code type;
	long count;
	for( int32 i=0; !msg.GetInfo(B_ANY_TYPE,i,&name,&type,&count);
			i++ ) {
		for( int32 j=0; j<count; j++ ) {
			//os << prefix << i << "." << j << ": " << name << "=";
			os << prefix << name;
			if( count > 1 ) os << "[" << (int) j << "]";
			os << ": ";
			switch( type ) {
			case B_CHAR_TYPE: {
				const char* getit=NULL;
				ssize_t rsize=0;
				os << "char(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_INT8_TYPE: {
				const int8* getit=NULL;
				ssize_t rsize=0;
				os << "int8(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_INT16_TYPE: {
				const int16* getit=NULL;
				ssize_t rsize=0;
				os << "int16(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_INT32_TYPE: {
				const int32* getit=NULL;
				ssize_t rsize=0;
				os << "int32(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << getit;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_INT64_TYPE: {
				const int64* getit=NULL;
				ssize_t rsize=0;
				os << "int64(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (double)(*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_UINT8_TYPE: {
				const uint8* getit=NULL;
				ssize_t rsize=0;
				os << "uint8(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << hex << "0x" << (*getit) << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_UINT16_TYPE: {
				const uint16* getit=NULL;
				ssize_t rsize=0;
				os << "uint16(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << hex << "0x" << (*getit) << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_UINT32_TYPE: {
				const uint32* getit=NULL;
				ssize_t rsize=0;
				os << "uint32(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << hex << "0x" << (*getit) << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_UINT64_TYPE: {
				const uint64* getit=NULL;
				ssize_t rsize=0;
				os << "uint64(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << hex << "0x" << (*getit) << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_FLOAT_TYPE: {
				const float* getit=NULL;
				ssize_t rsize=0;
				os << "float(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_DOUBLE_TYPE: {
				const double* getit=NULL;
				ssize_t rsize=0;
				os << "double(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_BOOL_TYPE: {
				const bool* getit=NULL;
				ssize_t rsize=0;
				os << "bool(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit ? "true" : "false");
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_OFF_T_TYPE: {
				const off_t* getit=NULL;
				ssize_t rsize=0;
				os << "off_t(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (double)(*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_SIZE_T_TYPE: {
				const size_t* getit=NULL;
				ssize_t rsize=0;
				os << "size_t(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit);
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_SSIZE_T_TYPE: {
				const ssize_t* getit=NULL;
				ssize_t rsize=0;
				os << "ssize_t(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << hex << "0x" << getit << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_POINTER_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "pointer(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << hex << "0x" << (void*)getit << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_OBJECT_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "object(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << hex << "0x" << (void*)getit << dec;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_MESSAGE_TYPE: {
				BMessage getit;
				if( !msg.FindMessage(name,j,&getit) ) {
					ArpToStream(os, getit, prefix);
					os << endl;
				} else {
					os << "BMessage(<not found>)" << endl;
				}
			} break;
			case B_MESSENGER_TYPE: {
				const BMessenger* getit=NULL;
				ssize_t rsize=0;
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit) << endl;
				} else {
					os << "BMessenger(<not found>)" << endl;
				}
			} break;
			case B_POINT_TYPE: {
				BPoint getit;
				if( !msg.FindPoint(name,j,&getit) ) {
					os << getit << endl;
				} else {
					os << "BPoint(<not found>)" << endl;
				}
			} break;
			case B_RECT_TYPE: {
				BRect getit;
				if( !msg.FindRect(name,j,&getit) ) {
					os << getit << endl;
				} else {
					os << "BPoint(<not found>)" << endl;
				}
			} break;
			case B_REF_TYPE: {
				entry_ref getit;
				if( !msg.FindRef(name,j,&getit) ) {
					os << (getit) << endl;
				} else {
					os << "entry_ref(<not found>)" << endl;
				}
			} break;
			case B_RGB_COLOR_TYPE: {
				const rgb_color* getit=NULL;
				ssize_t rsize=0;
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit) << endl;
				} else {
					os << "rgb_color(<not found>)" << endl;
				}
			} break;
			case FFont::FONT_TYPE: {
				if( be_app ) {
					FFont getit;
					if( !msg.FindFlat(name,j,&getit) ) {
						os << getit << endl;
					} else {
						os << "BFont(<not found>)" << endl;
					}
				} else {
					os << "BFont(<no application>)" << endl;
				}
			} break;
			case B_PATTERN_TYPE: {
				const pattern* getit=NULL;
				ssize_t rsize=0;
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << (*getit) << endl;
				} else {
					os << "pattern(<not found>)" << endl;
				}
			} break;
			case B_ASCII_TYPE: {
				const char* getit=NULL;
				ssize_t rsize=0;
				os << "ASCII(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << getit;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_STRING_TYPE: {
				const char* getit=NULL;
				ssize_t rsize=0;
				os << "string(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << getit;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case ARP_INDIRECT_TYPE: {
				const char* getit=NULL;
				ssize_t rsize=0;
				os << "indirect(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					os << getit;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_MONOCHROME_1_BIT_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "monochrome_1_bit(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit);// << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_GRAYSCALE_8_BIT_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "grayscale_8_bit(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit); // << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_COLOR_8_BIT_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "color_8_bit(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit); // << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_RGB_32_BIT_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "rgb_32_bit(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit); // << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_TIME_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "TIME(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit); // << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_RAW_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "RAW(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit); // << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			case B_MIME_TYPE: {
				const char* getit=NULL;
				ssize_t rsize=0;
				os << "MIME(";
				if( !msg.FindData(name,type,j,(const void**)&getit,&rsize) ) {
					if( strncmp(name,"text/",5) == 0 ) {
						os << getit << ")";
					} else if (strcmp(name,"application/x-vnd.Be-text_run_array") == 0 ) {
						int32 mysize = rsize;
						text_run_array* array
							= BTextView::UnflattenRunArray((const void*)getit,&mysize);
						if( array ) {
							os << "count=" << (int) array->count << ") {"
								<< endl;
							for( i=0; i<array->count; i++ ) {
								os << prefix << "  offset="
									<< (int) array->runs[i].offset
									<< " color="
									<< array->runs[i].color
									<< " font="
									<< array->runs[i].font
									<< endl;
							}
							free(array);
							os << prefix << "}" << endl;
						} else {
							os << "can't unflatten / ptr=" << (void*)getit << ")";
						}
					} else {
						os << "<can't display>)";
					}
				} else {
					os << "<not found>)";
				}
				os << endl;
			} break;
			case B_ANY_TYPE: {
				const void* getit=NULL;
				ssize_t rsize=0;
				os << "any(";
				if( !msg.FindData(name,type,j,&getit,&rsize) ) {
					os << (void*)(getit); // << ", size=" << rsize;
				} else {
					os << "<not found>";
				}
				os << ")" << endl;
			} break;
			default: {
				BPath getit;
				if( !msg.FindFlat(name,j,&getit) ) {
					os << (getit) << endl;
				} else {
					os << "unknown(type=";
					print_code(os, type);
					os << ")" << endl;
				}
			}
			}
		}
	}
	os << basePrefix << "}";
	os.flags(fl);
	return os;
}
