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
 * ArpBuild.h
 *
 * Importing and exporting ARP library definitions.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * We aren't doing shared libraries yet, so this has rather
 * limited use.
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
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef _ARP_BUILD_H
#define _ARP_BUILD_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#if _STATIC_LINKING

#define _IMPEXP_ARPMIDI
#define _IMPEXP_ARPLAYOUT

#else /* _STATIC_LINKING */

#if __INTEL__

#if _BUILDING_AmKernel
#define	_IMPEXP_ARPMIDI	__declspec(dllexport)
#else
#define	_IMPEXP_ARPMIDI	__declspec(dllimport)
#endif

#if _BUILDING_ArpLayout
#define	_IMPEXP_ARPLAYOUT	__declspec(dllexport)
#else
#define	_IMPEXP_ARPLAYOUT	__declspec(dllimport)
#endif

#endif /* __INTEL__ */

#if __POWERPC__

#define _IMPEXP_ARPMIDI
#define _IMPEXP_ARPLAYOUT

#endif /* __POWERPC__ */

#if __SH__

#define _IMPEXP_ARPMIDI
#define _IMPEXP_ARPLAYOUT

#endif /* __SH__ */

#endif /* _STATIC_LINKING */

#define _IMPEXP_ARPMIDI
#define _IMPEXP_ARPLAYOUT

#ifdef __cplusplus

/* ArpMidi library */
class _IMPEXP_ARPMIDI ArpMidiEventI;
class _IMPEXP_ARPMIDI ArpMidiChannelPressureI;
class _IMPEXP_ARPMIDI ArpMidiControlChangeI;
class _IMPEXP_ARPMIDI ArpMidiKeyPressureI;
class _IMPEXP_ARPMIDI ArpMidiListHolderI;
class _IMPEXP_ARPMIDI ArpMidiNoteOnI;
class _IMPEXP_ARPMIDI ArpMidiPitchBendI;
class _IMPEXP_ARPMIDI ArpMidiProgramChangeI;
class _IMPEXP_ARPMIDI ArpMidiSignatureI;
class _IMPEXP_ARPMIDI ArpMidiSystemCommonI;
class _IMPEXP_ARPMIDI ArpMidiSongPositionI;
class _IMPEXP_ARPMIDI ArpMidiTempoChangeI;
class _IMPEXP_ARPMIDI ArpMidiGhostEventI;
class _IMPEXP_ARPMIDI ArpMidiFilterAddon;
class _IMPEXP_ARPMIDI ArpMidiFilterHolderI;
class _IMPEXP_ARPMIDI ArpMidiFilterI;

/* ArpLayout library */
//class _IMPEXP_ARPLAYOUT Arp;

#endif		/* __cplusplus */

#endif
