#ifndef GLKERNEL_GLNODEROSTER_H
#define GLKERNEL_GLNODEROSTER_H

#include <ArpCore/String16.h>
#include <app/Message.h>
#include <GlPublic/GlDefs.h>
class GlNodeAddOn;
class GlNodeRosterData;

/*************************************************************************
 * GL-NODE-ROSTER
 * I store all installed node add ons in the system.
 *************************************************************************/
class GlNodeRoster
{
public:
	GlNodeRoster();
	virtual ~GlNodeRoster();

	status_t				GetAddOnInfo(	uint32 index, gl_node_add_on_id* outId, uint32* outIo = 0,
											BString16* outCreator = 0, int32* outKey = 0,
											BString16* outCategory = 0, BString16* outLabel = 0,
											const ArpBitmap** outImage = 0) const;
	const GlNodeAddOn*		GetAddOn(	gl_node_add_on_id id) const;
	const GlNodeAddOn*		GetAddOn(	const BString16& creator,
										int32 key) const;

protected:
	// Temp until addons are loading themselves
	friend class GlGlobalsImpl;
	status_t			Install(GlNodeAddOn* addon);

private:
	GlNodeRosterData*		mData;
};

#endif
