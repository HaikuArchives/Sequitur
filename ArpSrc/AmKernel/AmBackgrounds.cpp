/* AmBackgrounds.cpp
 */
#include <cstdio>
#include <cstdlib>
#include "AmPublic/AmBackgrounds.h"
#include "AmPublic/AmPrefsI.h"

/*************************************************************************
 * AM-GRID-BACKGROUND
 *************************************************************************/
void AmGridBackground::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor( Prefs().Color(AM_GRID_C) );

	AmTime					grid = GridTime();
	const AmTimeConverter&	mtc = TimeConverter();
	AmTime					leftTime = mtc.PixelToTick(clip.left);
	AmTime					rightTime = mtc.PixelToTick(clip.right);
	AmTime					time = leftTime - (leftTime % grid);
	while (time <= rightTime) {
		float				x = mtc.TickToPixel(time);
		view->StrokeLine(BPoint(x, clip.top), BPoint(x, clip.bottom));
		time += grid + 1;
	}
}
