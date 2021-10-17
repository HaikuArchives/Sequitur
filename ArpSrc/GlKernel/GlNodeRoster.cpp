#include <cstdio>
#include <app/Message.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlNode.h>
#include "GlKernel/GlNodeRoster.h"

// GL-NODE-ROSTER-DATA
class GlNodeRosterData {
public:
	std::vector<GlNodeAddOn*>	addons;
	
	GlNodeRosterData();
	~GlNodeRosterData();

	status_t				GetAddOnInfo(	uint32 index, gl_node_add_on_id* outId,
											uint32* outIo,
											BString16* outCreator, int32* outKey,
											BString16* outCategory, BString16* outLabel,
											const ArpBitmap** outImage) const;
	const GlNodeAddOn*		GetAddOn(gl_node_add_on_id id) const;
	const GlNodeAddOn*		GetAddOn(const BString16& creator, int32 key) const;

	status_t				Install(GlNodeAddOn* addon);

private:
	bool					_no_dup(GlNodeAddOn* add1, const GlNodeAddOn* add2);
};

/*************************************************************************
 * GL-NODE-ROSTER
 *************************************************************************/
GlNodeRoster::GlNodeRoster()
		: mData(0)
{
	mData = new GlNodeRosterData();
}

GlNodeRoster::~GlNodeRoster()
{
	delete mData;
}

status_t GlNodeRoster::GetAddOnInfo(uint32 index, gl_node_add_on_id* outId,
									uint32* outIo,
									BString16* outCreator, int32* outKey,
									BString16* outCategory, BString16* outLabel,
									const ArpBitmap** outImage) const
{
	ArpVALIDATE(mData, return B_NO_MEMORY);
	return mData->GetAddOnInfo(	index, outId, outIo, outCreator, outKey,
								outCategory, outLabel, outImage);
}

const GlNodeAddOn* GlNodeRoster::GetAddOn(gl_node_add_on_id id) const
{
	ArpVALIDATE(mData, return 0);
	return mData->GetAddOn(id);	
}

const GlNodeAddOn* GlNodeRoster::GetAddOn(	const BString16& creator,
											int32 key) const
{
	ArpVALIDATE(mData, return 0);
	return mData->GetAddOn(creator, key);	
}

status_t GlNodeRoster::Install(GlNodeAddOn* addon)
{
	ArpVALIDATE(mData, return B_NO_MEMORY);
	return mData->Install(addon);
}

// #pragma mark -

/*************************************************************************
 * GL-NODE-ROSTER-DATA
 *************************************************************************/
GlNodeRosterData::GlNodeRosterData()
{
}	

GlNodeRosterData::~GlNodeRosterData()
{
	for (uint32 k = 0; k < addons.size(); k++) delete addons[k];
	addons.resize(0);
}
	
status_t GlNodeRosterData::GetAddOnInfo(	uint32 index, gl_node_add_on_id* outId,
											uint32* outIo,
											BString16* outCreator, int32* outKey,
											BString16* outCategory, BString16* outLabel,
											const ArpBitmap** outImage) const
{
	if (index >= addons.size()) return B_ERROR;
	if (addons[index] == 0) return B_ERROR;
	if (outId) *outId = addons[index]->Id();
	if (outIo) *outIo = addons[index]->Io();
	if (outCreator) *outCreator = addons[index]->Creator();
	if (outKey) *outKey = addons[index]->Key();
	if (outCategory) *outCategory = addons[index]->Category();
	if (outLabel) *outLabel = addons[index]->Label();
	if (outImage) *outImage = addons[index]->Image();
	return B_OK;
}

const GlNodeAddOn* GlNodeRosterData::GetAddOn(gl_node_add_on_id id) const
{
	for (uint32 k = 0; k < addons.size(); k++) {
		if (id == addons[k]->Id()) return addons[k];
	}
	return 0;	
}

const GlNodeAddOn* GlNodeRosterData::GetAddOn(const BString16& creator, int32 key) const
{
	for (uint32 k = 0; k < addons.size(); k++) {
		float		match = addons[k]->GetMatch(creator, key);
		if (match > 0) return addons[k];
	}
	return 0;	
}

status_t GlNodeRosterData::Install(GlNodeAddOn* addon)
{
	ArpVALIDATE(addon, return B_ERROR);
	ArpVALIDATE(_no_dup(addon, GetAddOn(addon->Creator(), addon->Key())), return B_ERROR);
	addons.push_back(addon);
	return B_OK;
}

bool GlNodeRosterData::_no_dup(GlNodeAddOn* add1, const GlNodeAddOn* add2)
{
	if (!add2) return true;
	printf("Conflict between %s (%s:%ld) and %s (%s:%ld)\n",
			add1->Label().String(), add1->Creator().String(), add1->Key(),
			add2->Label().String(), add2->Creator().String(), add2->Key());
	return false;
}
