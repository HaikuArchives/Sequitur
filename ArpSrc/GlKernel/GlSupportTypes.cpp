#include <ArpKernel/ArpDebug.h>
#include <GlPublic/GlSupportTypes.h>

static const char*		REL_STR			= "rel";
static const char*		ABS_STR			= "abs";

/***************************************************************************
 * GL-REL-ABS
 ****************************************************************************/
GlRelAbs::GlRelAbs(float r, int32 a)
		: rel(r), abs(a)
{
}

GlRelAbs::GlRelAbs(const GlRelAbs& o)
		: rel(o.rel), abs(o.abs)
{
}

GlRelAbs& GlRelAbs::operator=(const GlRelAbs& o)
{
	rel = o.rel;
	abs = o.abs;
	return *this;
}

bool GlRelAbs::operator==(const GlRelAbs& o) const
{
	return (rel == o.rel && abs == o.abs);
}

bool GlRelAbs::operator!=(const GlRelAbs& o) const
{
	return (rel != o.rel || abs != o.abs);
}

void GlRelAbs::Set(float r, int32 a)
{
	rel = r;
	abs = a;
}

status_t GlRelAbs::ReadFrom(const BMessage& msg, const char* name)
{
	ArpVALIDATE(name, return B_ERROR);
	BMessage		raMsg;
	if (msg.FindMessage(name, &raMsg) == B_OK) {
		float		f;
		int32		i32;
		if (raMsg.FindFloat(REL_STR, &f) == B_OK) rel = f;
		if (raMsg.FindInt32(ABS_STR, &i32) == B_OK) abs = i32;
	}
	return B_OK;
}

status_t GlRelAbs::WriteTo(BMessage& msg, const char* name) const
{
	ArpVALIDATE(name, return B_ERROR);
	BMessage		raMsg;
	status_t		err;
	if ((err = raMsg.AddFloat(REL_STR, rel)) != B_OK) return err;
	if ((err = raMsg.AddInt32(ABS_STR, abs)) != B_OK) return err;
	return msg.AddMessage(name, &raMsg);
}
