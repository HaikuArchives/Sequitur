/* AmTimeConverter.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "AmPublic/AmTimeConverter.h"

static const float		UNDEFINED_CONVERSION	= 0.0000001;

/*************************************************************************
 * AM-TIME-CONVERTER
 *************************************************************************/
AmTimeConverter::AmTimeConverter()
{
	mBeatConversion = UNDEFINED_CONVERSION;
	mBeatLength = 0;
}

AmTimeConverter::AmTimeConverter(float beatLength)
{
	SetBeatLength(beatLength);
}

AmTimeConverter::~AmTimeConverter()
{
}

float AmTimeConverter::BeatLength() const
{
	return mBeatLength;
}

void AmTimeConverter::SetBeatLength(float beatLength)
{
	mBeatLength = beatLength;
	mBeatConversion = (mBeatLength / (float)PPQN);
	if (mBeatConversion == 0) mBeatConversion = UNDEFINED_CONVERSION;
}

AmTime AmTimeConverter::PixelToTick(float pixel) const
{
	if( pixel < 0 ) return 0;
	return (AmTime) (pixel / mBeatConversion);
}

float AmTimeConverter::TickToPixel(AmTime tick) const
{
	return (float) (tick * mBeatConversion);
}
