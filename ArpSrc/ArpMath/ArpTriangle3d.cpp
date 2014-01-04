#include <ArpKernel/ArpDebug.h>
#include <ArpMath/ArpTriangle3d.h>

/***************************************************************************
 * ARP-TRIANGLE-3D
 ****************************************************************************/
ArpTriangle3d::ArpTriangle3d()
{
}

ArpTriangle3d::ArpTriangle3d(	float x0, float y0, float z0,
								float x1, float y1, float z1,
								float x2, float y2, float z2)
		: v0(x0, y0, z0), v1(x1, y1, z1), v2(x2, y2, z2)
{
}

ArpTriangle3d::ArpTriangle3d(	const ArpPoint3d& in0, const ArpPoint3d& in1,
								const ArpPoint3d& in2)
		: v0(in0), v1(in1), v2(in2)
{
}

enum {
	_LEFT		= 0x01,
	_TOP		= 0x02,
	_RIGHT		= 0x04,
	_BOTTOM		= 0x08
};

void ArpTriangle3d::MakeClockwise()
{
	uint8			f0 = 0, f1 = 0, f2 = 0;
	if (v0.x <= v1.x && v0.x <= v2.x) f0 |= _LEFT;
	if (v1.x <= v0.x && v1.x <= v2.x) f1 |= _LEFT;
	if (v2.x <= v0.x && v2.x <= v1.x) f2 |= _LEFT;

	if (v0.y <= v1.y && v0.y <= v2.y) f0 |= _TOP;
	if (v1.y <= v0.y && v1.y <= v2.y) f1 |= _TOP;
	if (v2.y <= v0.y && v2.y <= v1.y) f2 |= _TOP;

	if (v0.x >= v1.x && v0.x >= v2.x) f0 |= _RIGHT;
	if (v1.x >= v0.x && v1.x >= v2.x) f1 |= _RIGHT;
	if (v2.x >= v0.x && v2.x >= v1.x) f2 |= _RIGHT;

	if (v0.y >= v1.y && v0.y >= v2.y) f0 |= _BOTTOM;
	if (v1.y >= v0.y && v1.y >= v2.y) f1 |= _BOTTOM;
	if (v2.y >= v0.y && v2.y >= v1.y) f2 |= _BOTTOM;

	int16			pts[3];
	pts[0] = pts[1] = pts[2] = -1;
	int16			index = 0;
	uint8			f0Used = 0, f1Used = 0, f2Used = 0;
	
	if (f0 == (_LEFT | _TOP)) { pts[index++] = 0; f0Used = 1; }
	else if (f1 == (_LEFT | _TOP)) { pts[index++] = 1; f1Used = 1; }
	else if (f2 == (_LEFT | _TOP)) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("1: %d\n", pts[k]);

	if (f0Used == 0 && f0 == _TOP) { pts[index++] = 0; f0Used = 1; }
	else if (f1Used == 0 && f1 == _TOP) { pts[index++] = 1; f1Used = 1; }
	else if (f2Used == 0 && f2 == _TOP) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("2: %d\n", pts[k]);

	if (f0Used == 0 && f0 == (_TOP | _RIGHT)) { pts[index++] = 0; f0Used = 1; }
	else if (f1Used == 0 && f1 == (_TOP | _RIGHT)) { pts[index++] = 1; f1Used = 1; }
	else if (f2Used == 0 && f2 == (_TOP | _RIGHT)) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("3: %d\n", pts[k]);

	/* At this point I might have the order of my points, if Not
	 * I have to keep looking.
	 */
	if (index < 3) {
		if (f0Used == 0 && f0 == _RIGHT) { pts[index++] = 0; f0Used = 1; }
		else if (f1Used == 0 && f1 == _RIGHT) { pts[index++] = 1; f1Used = 1; }
		else if (f2Used == 0 && f2 == _RIGHT) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("4: %d\n", pts[k]);
		if (index < 3) {
			if (f0Used == 0 && f0 == (_BOTTOM | _RIGHT)) { pts[index++] = 0; f0Used = 1; }
			else if (f1Used == 0 && f1 == (_BOTTOM | _RIGHT)) { pts[index++] = 1; f1Used = 1; }
			else if (f2Used == 0 && f2 == (_BOTTOM | _RIGHT)) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("5: %d\n", pts[k]);
			if (index < 3) {
				if (f0Used == 0 && f0 == _BOTTOM) { pts[index++] = 0; f0Used = 1; }
				else if (f1Used == 0 && f1 == _BOTTOM) { pts[index++] = 1; f1Used = 1; }
				else if (f2Used == 0 && f2 == _BOTTOM) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("6: %d\n", pts[k]);
				if (index < 3) {
					if (f0Used == 0 && f0 == (_BOTTOM | _LEFT)) { pts[index++] = 0; f0Used = 1; }
					else if (f1Used == 0 && f1 == (_BOTTOM | _LEFT)) { pts[index++] = 1; f1Used = 1; }
					else if (f2Used == 0 && f2 == (_BOTTOM | _LEFT)) { pts[index++] = 2; f2Used = 1; }

//for (int16 k = 0; k < index; k++) printf("7: %d\n", pts[k]);
					if (index < 3) {
						if (f0Used == 0 && f0 == _LEFT) { pts[index++] = 0; f0Used = 1; }
						else if (f1Used == 0 && f1 == _LEFT) { pts[index++] = 1; f1Used = 1; }
						else if (f2Used == 0 && f2 == _LEFT) { pts[index++] = 2; f2Used = 1; }
					}
				}
			}
		}
	}
	if (index != 3 || f0Used != 1 || f1Used != 1 || f2Used != 1 || pts[0] == pts[1] || pts[1] == pts[2]) {
		printf("(%f, %f) (%f, %f) (%f, %f)\n\tindex %d f0 %d f1 %d f2 %d pts[0] %d pts[1] %d pts[2] %d\n",
			v0.x, v0.y, v1.x, v1.y, v2.x, v2.x, index, f0, f1, f2, pts[0], pts[1], pts[2]);
	}
	ArpASSERT(index == 3 && f0Used == 1 && f1Used == 1 && f2Used == 1);
	ArpASSERT(pts[0] != pts[1] && pts[1] != pts[2]);

	if (pts[0] == 1) {
		ArpPoint3d		pt(v0);
		v0 = v1;
		v1 = pt;
	} else if (pts[0] == 2) {
		ArpPoint3d		pt(v0);
		v0 = v2;
		v2 = pt;
	}

	if (pts[1] == 0) {
		ArpPoint3d		pt(v1);
		v1 = v0;
		v0 = pt;
	} else if (pts[1] == 2) {
		ArpPoint3d		pt(v1);
		v1 = v2;
		v2 = pt;
	}

	if (pts[2] == 0) {
		ArpPoint3d		pt(v2);
		v2 = v0;
		v0 = pt;
	} else if (pts[2] == 1) {
		ArpPoint3d		pt(v2);
		v2 = v1;
		v1 = pt;
	}
}

void ArpTriangle3d::MakeCounterClockwise()
{
	MakeClockwise();
	ArpPoint3d			pt(v2);
	v2 = v1;
	v1 = pt;
}

void ArpTriangle3d::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("Triangle (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)\n",
		v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
}
