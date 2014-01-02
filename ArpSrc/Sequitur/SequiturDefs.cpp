/* SequiturDefs.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <InterfaceKit.h>
#include "AmPublic/AmPrefsI.h"
#include "Sequitur/SequiturDefs.h"

const char*			SEQ_TIME_STR			= "time";

void low_memory_warning()
{
	BAlert*	alert = new BAlert(	"Warning", "Low memory",
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
}


float	PIPELINE_SHADE_1 = 0.90;
float	PIPELINE_SHADE_2 = 0.80;
float	PIPELINE_SHADE_3 = 0.65;

rgb_color seq_darken(rgb_color c, float f)
{
	rgb_color	a;
	a.red =		(uint8)(c.red * f);
	a.green =	(uint8)(c.green * f);
	a.blue =	(uint8)(c.blue *f);
	a.alpha =	c.alpha;
	return a;
}


void seq_draw_hrz_pipe(	BView* view, float left, float top,
						float right, float shade)
{
	rgb_color		c = Prefs().Color(AM_PIPELINE_C);
	view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * shade) );
	view->StrokeLine( BPoint(left, top + 1),	BPoint(right, top + 1) );
	view->SetHighColor(c);
	view->StrokeLine( BPoint(left, top + 2),	BPoint(right,top + 2) );
	view->SetHighColor( seq_darken(c, PIPELINE_SHADE_2 * shade * shade) );
	view->StrokeLine( BPoint(left, top + 3),	BPoint(right, top + 3) );
	view->SetHighColor( seq_darken(c, PIPELINE_SHADE_3 * shade * shade * shade) );
	view->StrokeLine( BPoint(left, top),		BPoint(right, top) );
	view->StrokeLine( BPoint(left, top + 4),	BPoint(right, top + 4) );
}
