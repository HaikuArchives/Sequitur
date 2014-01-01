/* GlFluidAutomata.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.10.29			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLFLUIDAUTOMATA_H
#define GLPUBLIC_GLFLUIDAUTOMATA_H

#include "GlPublic/GlDefs.h"
class GlPlanes;

/***************************************************************************
 * GL-CA-REPORT
 * A structure for reporting on changes that occurred in one frame of
 * an automata.  Each report stores how much up to MAX_SIZE cells have given
 * to each surrounding cell.  The pix is the ARP_PIXEL of the cell in
 * question, then the compass directions equate to screen coords (obviously,
 * the report is intended for clients who are looking at an image).
 ****************************************************************************/
class GlCaReport
{
public:
	static const int32	MAX_SIZE = 100;
	
	GlCaReport();
	~GlCaReport();

	GlCaReport*			next;
	int32				pix[MAX_SIZE];
	float				north[MAX_SIZE], south[MAX_SIZE],
						east[MAX_SIZE], west[MAX_SIZE];
	int32				size;

	void				MakeEmpty();
};

/***************************************************************************
 * GL-CA-CELL
 ****************************************************************************/
class GlCaCell
{
public:
	float				mass, newMass;
	uint32				age;
	
	GlCaCell();

	GlCaCell&			operator=(const GlCaCell& o);
};

/***************************************************************************
 * GL-CA-SLICE
 ****************************************************************************/
class GlCaSlice
{
public:
	GlCaCell*			cells;
	uint16				size;		// Number of cells
	uint8				z;			// The z value of the original image at this (x, y).
									// If there are any cells, they will correspond
									// to depths starting at my z value.  For example,
									// if my z is 200 and I have 20 cells, cells[0]
									// will correspond to a depth of 200, cells[1] to
									// 201, etc.

	enum {
		NORTH_F			= 0x01,
		SOUTH_F			= 0x02,
		EAST_F			= 0x04,
		WEST_F			= 0x08
	};
	uint8				flow;

	GlCaSlice();
	~GlCaSlice();

	status_t			VerifyRange(uint8 end);
	status_t			FillRange(uint8 low, uint8 high, float mass);
	status_t			GetVisibleRange(uint8 z, uint32* outStart,
										uint32* outEnd) const;
};

/***************************************************************************
 * GL-FLUID-AUTOMATA
 ****************************************************************************/
class GlFluidAutomata
{
public:
	GlFluidAutomata(float rate, float evaporation, float viscosity,
					float initAmount);
	~GlFluidAutomata();

	status_t			Init(GlPlanes& p);
	status_t			Init(GlPlanes& p, float radius);
	status_t			Init(GlPlanes& p, uint8* seed);
	status_t			Run(uint32 age, const GlCaReport** report = 0);
	/* Clients supply an area of values, which all get added to
	 * the appropriate cells.
	 */
	status_t			Add(uint8* z, float* v, int32 w, int32 h, int32 x, int32 y);

	GlCaSlice*			slices;
	int32				mW, mH;

private:
	float				mRate, mEvaporation, mViscosity;
	float				mInitAmount;
	GlCaReport			mReportHead;
	GlCaReport*			mReport;
	int32				mL, mT, mR, mB;
	uint32				mCurrentAge;
	enum {
		GRAVITY_BACK_F		= 0x00000001,
		GRAVITY_BOTTOM_F	= 0x00000002
	};
	uint32				mFlags;
	
	status_t			Seed(int32 seedX, int32 seedY, float r);
	status_t			AgeBack();
	status_t			AgeBottom();
	status_t			Balance(GlCaSlice& srcSlice, uint32 srcCell,
								int32 x, int32 y, int32 pix);
	void				Free();

#if 0
	/* Cached for Balance()
	 */
	GlCaCell*			mN[4];
	float				mTake[4], mDiff[4];
	float*				mReportChange[4];
	bool				mIncReport;

// Temp while I look at Balance();

	uint32		src, dest;
	float		give, curGive;
//	GlCaSlice*	n;
//	GlCaSlice*	s;
//	GlCaSlice*	e;
//	GlCaSlice*	w;

	float		diffSrc;
	uint8		mK, count;
	float*		rc;

	status_t	Balance1(	GlCaSlice& srcSlice, uint32 srcCell,
							int32 x, int32 y, int32 pix);
	status_t	Balance2(	GlCaSlice& srcSlice, uint32 srcCell,
							int32 x, int32 y, int32 pix);
	void		Balance3(	GlCaSlice& srcSlice, uint32 srcCell,
							int32 x, int32 y, int32 pix);
	void		Balance4(	GlCaSlice& srcSlice, uint32 srcCell,
							int32 x, int32 y, int32 pix);
#endif
};

#endif
