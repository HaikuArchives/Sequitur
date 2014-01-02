/* GlDefs.h
 * Global definitions for Glasslike.
 */
#ifndef GLKERNEL_DEFS_H
#define GLKERNEL_DEFS_H

#include <stdio.h>
#include <ArpCore/String16.h>
#include <support/SupportDefs.h>

extern const int32		GL_ROOT_KEY;
extern const int32		GL_WRAPPER_KEY;

extern const int32		GL_PARAM_ROOT_CREATOR;
extern const int32		GL_PARAM_ROOT_KEY;
extern const int32		GL_PARAM_ROOT_CATEGORY;
extern const int32		GL_PARAM_ROOT_LABEL;

/* The unset values indicate that the user has never configured
 * the midi or control property of a param.
 */
extern const int32		GL_UNSET_MIDI;
enum {
//	GL_UNSET_CONTROL	= -1,
	GL_CONTROL_OFF		= 0,
	GL_CONTROL_ON		= 1
};

#if 0
/* INTERNAL STRINGS
 * Miscellaneous text that isn't in the UI and can't change
 */
const BString16*	SZP(uint32 i);
enum {
};
#endif

#if 0
/* The root node writes this info out -- other nodes
 * might need to read it.  FIX:  Think this root
 * stuff has been obsoleted.
 */
extern const char*		GL_ROOT_CREATOR_STR;
extern const char*		GL_ROOT_CATEGORY_STR;
extern const char*		GL_ROOT_LABEL_STR;
extern const char*		GL_ROOT_KEY_STR;
#endif

extern const char*		GL_NI_STR;
extern const char*		GL_NMSG_STR;
extern const char*		GL_RH_STR;

/* Print out the int in a friendly fashion (as four chars)
 */
void gl_print_key(int32 key);

#endif
