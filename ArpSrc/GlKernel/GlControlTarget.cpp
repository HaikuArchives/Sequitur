#include <GlPublic/GlControlTarget.h>

/***************************************************************************
 * GL-CONTROL-TARGET
 ****************************************************************************/
GlControlTarget::GlControlTarget(uint32 flags)
		: mFlags(flags)
{
}

GlControlTarget::~GlControlTarget()
{
}

uint32 GlControlTarget::Flags() const
{
	return mFlags;
}

status_t GlControlTarget::StartRecording()
{
	return B_OK;
}

void GlControlTarget::StopRecording()
{
}

void GlControlTarget::Populate(GlControlTargetPopulator& p)
{
}

void GlControlTarget::SetState(GlControlState& s) const
{
}
