/* AmKernelDefs.h
 * Global definitions for the ARP MIDI classes.
 */

#ifndef AMKERNEL_AMKERNELDEFS_H
#define AMKERNEL_AMKERNELDEFS_H

#include <vector.h>
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmToolBarRef.h"
#include "AmPublic/AmToolRef.h"

typedef vector<AmToolBarRef>	toolbarref_vec;
typedef vector<AmToolRef>		toolref_vec;

typedef void*			file_entry_id;

extern const char* NULL_INPUT_KEY;
extern const char* NULL_OUTPUT_KEY;

// This is the same as BMessage::Flatten(), but makes sure you are
// writing the B_MESSAGE_VERSION_1 flattened form on post-R5 machines.
status_t AmFlatten(const BMessage& who, BDataIO* stream, ssize_t* out_size=NULL);

/*****************************************************************************
 * Filter utilities
 * Since the multi filters are sort of tacked onto the existing system,
 * the experience is not very smooth for clients.  These messages can be
 * used instead of their AmFilterRoster and AmMultiFilterRoster counterparts
 * to make the operations transparent.
 *****************************************************************************/
ArpRef<AmFilterAddOn> am_find_filter_addon(const BMessage* archive);
ArpRef<AmFilterAddOn> am_find_filter_addon(const char* uniqueName);

#endif
