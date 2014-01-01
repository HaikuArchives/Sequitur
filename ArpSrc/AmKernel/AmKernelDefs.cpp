/* AmKernelDefs.cpp
 */
#define _BUILDING_AmKernel 1

#include <kernel/image.h>
#include <app/Application.h>

#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmKernelDefs.h"

const char* NULL_INPUT_KEY = "arp:NullInput";
const char* NULL_OUTPUT_KEY = "arp:NullOutput";

// These are the types of flattened BMessage formats.
enum my_message_version {
	MESSAGE_VERSION_ANY		= 0,					// Don't care.
	MESSAGE_VERSION_1		= 1,					// R5 and before.
	MESSAGE_VERSION_2		= 2,					// Post-R5.
	MESSAGE_VERSION_CURRENT	= MESSAGE_VERSION_2
};

typedef status_t (*flatten_func)(const BMessage*, my_message_version, BDataIO *, ssize_t *);

static bool gLookedForFlatten = false;
static flatten_func gFlattenFunc = NULL;

status_t AmFlatten(const BMessage& who, BDataIO* stream, ssize_t* out_size)
{
	// This magic code is to be able to flatten as old-style messages
	// when running on post-R5 machines.
	if (!gLookedForFlatten) {
		image_info info;
		int32 cookie = 0;
		int32 i = 0;
		while (!gLookedForFlatten && get_next_image_info(be_app->Team(), &cookie, &info) == B_OK) {
			if (info.type == B_LIBRARY_IMAGE) {
				if (get_image_symbol(info.id, "Flatten__C8BMessage15message_versionP7BDataIOPl",
						B_SYMBOL_TYPE_TEXT, (void**)&gFlattenFunc) == B_OK) {
					gLookedForFlatten = true;
				}
			}
			if (i++ > 5) gLookedForFlatten = true;
		}
		printf("Result of looking for flatten func: %p\n", gFlattenFunc);
	}
	if (gFlattenFunc) return gFlattenFunc(&who, MESSAGE_VERSION_1, stream, out_size);
	return who.Flatten(stream, out_size);
}

/*****************************************************************************
 * Filter utilities
 *****************************************************************************/
ArpRef<AmFilterAddOn> am_find_filter_addon(const BMessage* archive)
{
	ArpRef<AmFilterAddOn> addon = NULL;
	if (AmFilterRoster::Default() ) {
		addon = AmFilterRoster::Default()->FindFilterAddOn(archive);
		if (addon) return addon;
	}
	if (AmMultiFilterRoster::Default() )
		return ArpRef<AmFilterAddOn>(AmMultiFilterRoster::Default()->FindFilterAddOn(archive));
	return NULL;
}

ArpRef<AmFilterAddOn> am_find_filter_addon(const char* uniqueName)
{
	ArpRef<AmFilterAddOn> addon = NULL;
	if (AmFilterRoster::Default() ) {
		addon = AmFilterRoster::Default()->FindFilterAddOn(uniqueName);
		if (addon) return addon;
	}
	if (AmMultiFilterRoster::Default() )
		return ArpRef<AmFilterAddOn>(AmMultiFilterRoster::Default()->FindFilterAddOn(uniqueName));
	return NULL;
}
