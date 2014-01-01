/* GlDefs.h
 * Global definitions for Glasslike.
 */
#ifndef GLASSLIKE_GLDEFS_H
#define GLASSLIKE_GLDEFS_H

#include <GlPublic/GlDefs.h>
class GlRootNode;
class GlRootRef;
class GlSkinCache;

enum {
	SHOW_NODES_WIN					= 'gsnw',
	/* Post this message to the app to show the Preferences window
	 */
	SHOW_PREFERENCES_MSG			= 'gspr',

	GL_MIDI_EVENT_MSG				= 'gMdi',

	GL_DRAG_NODE_MSG				= 'gdrn',
	GL_INSPECT_NODE_MSG				= 'gInN',	// Display the supplied node in the inspector
	GL_INSPECT_ROOT_MSG				= 'gInR',	// Display the grid root in the inspector
	GL_INSPECT_ROOT_INFO_MSG		= 'gIRI',

	GL_RESULT_CHANGED_MSG			= 'gRsC',	// Sent when the result image changes
	
	GL_RECACHE_MSG					= 'gche',	// Rebuild and redraw the view

	GL_NODE_ADDON_DRAG_MSG			= 'gNAD',

	GL_ROOT_CHANGED_MSG				= 'gRtC',	//
	GL_PATH_CHANGED_MSG				= 'gPtC',	// A command to navigate up one level

	GL_PREVIEW_IMAGE_DROPPED		= 'gPID',

	GL_SET_STATUS_MSG				= 'gStV',
		// string GL_TEXT_STR	The message to display in the status view
	GL_ENABLE_PLAY_CONTROLS_MSG		= 'gEPC',
	GL_DISABLE_PLAY_CONTROLS_MSG	= 'gDPC',

	GL_PLAY_MSG						= 'gTPl',	// Begin playing
	GL_STOP_MSG						= 'gTSt',	// Stop playing
	GL_RECORD_MSG					= 'gTRc',	// Toggle recording

	GL_INVALIDATE_MSG				= 'ginv'
};

/* The different editor types.
 */
enum {
	GL_PAGE_EDITOR				= 'Page',
	GL_PROJECT_EDITOR			= 'Proj',
	GL_REMIX_EDITOR				= 'Remx',
	GL_STRUCTURE_EDITOR			= 'Strc'
};

/* These are the global annotations a node might have.
 */
enum {
//	GL_SUBTYPE_ANN			= GL_SUBTYPE_ANN_HACK,		// int32, a UI-defined subtype for any given node type.
	GL_ORIGIN_ANN			= 'A1',		// point, an upper left corner, assuming it's visual data
	GL_EXTENT_ANN			= 'A2'		// point, an extent, assuming it's visual data
};

extern const char*		GL_NODE_ADDON_ID_STR;
extern const char*		GL_INSPECTOR_ON_STR;
extern const char*		GL_INSPECTOR_TOP_STR;
extern const char*		GL_INSPECTOR_WIDTH_STR;
extern const char*		GL_NODE_VIEW_STR;
extern const char*		GL_I_STR;
extern const char*		GL_ROOT_STR;
extern const char*		GL_TEXT_STR;
extern const char*		GL_UNTITLED_STR;

extern const uint32		GL_ROOT_MSG;

extern gl_image_id		GL_MIDI_A_IMAGE_ID;
extern gl_image_id		GL_MIDI_B_IMAGE_ID;
extern gl_image_id		GL_MIDI_C_IMAGE_ID;
extern gl_image_id		GL_MIDI_Q_IMAGE_ID;
extern gl_image_id		GL_MIDI_RECORD_IMAGE_ID;

GlRootNode*		gl_new_root_node(int32 type);

void			gl_get_root_label(const GlRootRef& ref, BString16& outLabel);
void			gl_get_root_label(const GlRootNode* root, BString16& outLabel);

#endif
