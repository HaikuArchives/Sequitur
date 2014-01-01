#include <GlPublic/GlGlobalsI.h>

/***************************************************************************
 * GL-GLOBALS-I
 ****************************************************************************/
const GlNodeAddOn* GlGlobalsI::GetAddOn(int32 key) const
{
	return GetAddOn(SZI[SZI_arp], key);
}

GlNode* GlGlobalsI::NewNode(int32 key, const BMessage* config) const
{
	return NewNode(SZI[SZI_arp], key, config);
}

