#include <app/Message.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpSupport/ArpVoxel.h>

/*******************************************************
 * ARP-VOXEL
 *******************************************************/
ArpVoxel::ArpVoxel()
		: r(255), g(255), b(255), a(255), z(0)
{
}

ArpVoxel::ArpVoxel(const ArpVoxel& o)
		: r(o.r), g(o.g), b(o.b), a(o.a), z(o.z)
{
}

ArpVoxel::ArpVoxel(	uint8 inR, uint8 inG, uint8 inB,
					uint8 inA, uint8 inZ)
		: r(inR), g(inG), b(inB), a(inA), z(inZ)
{
}

ArpVoxel& ArpVoxel::operator=(const ArpVoxel& o)
{
	r = o.r;
	g = o.g;
	b = o.b;
	a = o.a;
	z = o.z;
	return *this;
}

/* Read floats because I'm replacing the ArpColour,
 * which was in floats.
 */
status_t ArpVoxel::ReadFrom(const BMessage& msg)
{
	status_t		err;
	float			fr, fg, fb, fa;
	if ((err = msg.FindFloat("r", &fr)) != B_OK) return err;
	if ((err = msg.FindFloat("g", &fg)) != B_OK) return err;
	if ((err = msg.FindFloat("b", &fb)) != B_OK) return err;
	if ((err = msg.FindFloat("a", &fa)) != B_OK) return err;
	r = arp_clip_255(fr * 255);
	g = arp_clip_255(fg * 255);
	b = arp_clip_255(fb * 255);
	a = arp_clip_255(fa * 255);
	if (msg.FindFloat("z", &fa) == B_OK) z = arp_clip_255(fa * 255);
	return B_OK;
}

/* Write floats because I'm replacing the ArpColour,
 * which was in floats.
 */
status_t ArpVoxel::WriteTo(BMessage& msg) const
{
	status_t		err;
	if ((err = msg.AddFloat("r", r / 255.0f)) != B_OK) return err;
	if ((err = msg.AddFloat("g", g / 255.0f)) != B_OK) return err;
	if ((err = msg.AddFloat("b", b / 255.0f)) != B_OK) return err;
	if ((err = msg.AddFloat("a", a / 255.0f)) != B_OK) return err;
	if ((err = msg.AddFloat("z", z / 255.0f)) != B_OK) return err;
	return B_OK;
}

void ArpVoxel::Print(uint32 tabs) const
{
	for (uint32 tab = 0; tab < tabs; tab++) printf("\t");
	printf("ArpVoxel r %d g %d b %d a %d z %d\n", r, g, b, a, z);
}
