#include <ArpCore/ArpDebug.h>
#include <ArpMath/ArpPoint3d.h>

static const char*		X_STR			= "x";
static const char*		Y_STR			= "y";
static const char*		Z_STR			= "z";

/***************************************************************************
 * ARP-POINT-3D
 ****************************************************************************/
ArpPoint3d::ArpPoint3d()
		: x(0), y(0), z(0)
{
}

ArpPoint3d::ArpPoint3d(float inX, float inY, float inZ)
		: x(inX), y(inY), z(inZ)
{
}

ArpPoint3d::ArpPoint3d(const ArpPoint3d& o)
		: x(o.x), y(o.y), z(o.z)
{
}

bool ArpPoint3d::operator==(const ArpPoint3d& o) const
{
	return x == o.x && y == o.y && z == o.z;
}

ArpPoint3d& ArpPoint3d::operator=(const ArpPoint3d& o)
{
	x = o.x;
	y = o.y;
	z = o.z;
	return *this;
}

ArpPoint3d ArpPoint3d::operator+(const ArpPoint3d& pt) const
{
	return ArpPoint3d(x + pt.x, y + pt.y, z + pt.z);
}

ArpPoint3d ArpPoint3d::operator-(const ArpPoint3d& pt) const
{
	return ArpPoint3d(x - pt.x, y - pt.y, z - pt.z);
}

status_t ArpPoint3d::ReadFrom(const BMessage& msg, const char* name)
{
	ArpVALIDATE(name, return B_ERROR);
	BMessage		ptMsg;
	if (msg.FindMessage(name, &ptMsg) == B_OK) {
		float		f;
		if (ptMsg.FindFloat(X_STR, &f) == B_OK) x = f;
		if (ptMsg.FindFloat(Y_STR, &f) == B_OK) y = f;
		if (ptMsg.FindFloat(Z_STR, &f) == B_OK) z = f;
	}
	return B_OK;
}

status_t ArpPoint3d::WriteTo(BMessage& msg, const char* name) const
{
	ArpVALIDATE(name, return B_ERROR);
	BMessage		ptMsg;
	status_t		err;
	if ((err = ptMsg.AddFloat(X_STR, x)) != B_OK) return err;
	if ((err = ptMsg.AddFloat(Y_STR, y)) != B_OK) return err;
	if ((err = ptMsg.AddFloat(Z_STR, z)) != B_OK) return err;
	return msg.AddMessage(name, &ptMsg);
}
