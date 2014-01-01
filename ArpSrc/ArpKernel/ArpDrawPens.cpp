/*
	
	ArpDrawPens.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPKERNEL_ARPDRAWPENS_H
#include <ArpKernel/ArpDrawPens.h>
#endif

#include <stdarg.h>

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

ArpMOD();

ArpLogicalPen ArpDrawPens::highMap[_Num_ArpLogicalPen] = {
	/*    ArpBackgroundPen -> */ ArpHighlightFillPen,
	/*          ArpTextPen -> */ ArpHighlightTextPen,
	/*      ArpStandoutPen -> */ ArpHighlightTextPen,
	/*         ArpShinePen -> */ ArpShadowPen,
	/*        ArpShadowPen -> */ ArpShinePen,
	/*     ArpBasicFillPen -> */ ArpHighlightFillPen,
	/*     ArpBasicTextPen -> */ ArpHighlightTextPen,
	/*      ArpForeFillPen -> */ ArpHighlightFillPen,
	/*      ArpForeTextPen -> */ ArpHighlightTextPen,
	/* ArpHighlightFillPen -> */ ArpBackgroundPen,
	/* ArpHighlightTextPen -> */ ArpTextPen,
	/*     ArpErrorFillPen -> */ ArpBackgroundPen,
	/*     ArpErrorTextPen -> */ ArpTextPen
};

ArpLogicalPen ArpDrawPens::compMap[_Num_ArpLogicalPen] = {
	/*    ArpBackgroundPen -> */ ArpTextPen,
	/*          ArpTextPen -> */ ArpBackgroundPen,
	/*      ArpStandoutPen -> */ ArpBackgroundPen,
	/*         ArpShinePen -> */ ArpShadowPen,
	/*        ArpShadowPen -> */ ArpShinePen,
	/*     ArpBasicFillPen -> */ ArpBasicTextPen,
	/*     ArpBasicTextPen -> */ ArpBasicFillPen,
	/*      ArpForeFillPen -> */ ArpForeTextPen,
	/*      ArpForeTextPen -> */ ArpForeFillPen,
	/* ArpHighlightFillPen -> */ ArpHighlightTextPen,
	/* ArpHighlightTextPen -> */ ArpHighlightFillPen,
	/*     ArpErrorFillPen -> */ ArpErrorTextPen,
	/*     ArpErrorTextPen -> */ ArpErrorFillPen
};

ArpLogicalPen ArpDrawPens::disableMap[_Num_ArpLogicalPen] = {
#if 0
#if 1
	/*    ArpBackgroundPen -> */ ArpBackgroundPen,
	/*          ArpTextPen -> */ ArpTextPen,
	/*      ArpStandoutPen -> */ ArpTextPen,
	/*         ArpShinePen -> */ ArpTextPen,
	/*        ArpShadowPen -> */ ArpTextPen,
	/*     ArpBasicFillPen -> */ ArpBackgroundPen,
	/*     ArpBasicTextPen -> */ ArpTextPen,
	/*      ArpForeFillPen -> */ ArpBackgroundPen,
	/*      ArpForeTextPen -> */ ArpTextPen,
	/* ArpHighlightFillPen -> */ ArpBackgroundPen,
	/* ArpHighlightTextPen -> */ ArpTextPen,
	/*     ArpErrorFillPen -> */ ArpBackgroundPen,
	/*     ArpErrorTextPen -> */ ArpTextPen
#else
	/*    ArpBackgroundPen -> */ ArpBackgroundPen,
	/*          ArpTextPen -> */ ArpTextPen,
	/*      ArpStandoutPen -> */ ArpTextPen,
	/*         ArpShinePen -> */ ArpShadowPen,
	/*        ArpShadowPen -> */ ArpShadowPen,
	/*     ArpBasicFillPen -> */ ArpBasicFillPen,
	/*     ArpBasicTextPen -> */ ArpBasicTextPen,
	/*      ArpForeFillPen -> */ ArpBasicFillPen,
	/*      ArpForeTextPen -> */ ArpBasicTextPen,
	/* ArpHighlightFillPen -> */ ArpBackgroundPen,
	/* ArpHighlightTextPen -> */ ArpTextPen,
	/*     ArpErrorFillPen -> */ ArpBackgroundPen,
	/*     ArpErrorTextPen -> */ ArpTextPen
#endif
#else
#if 1
	/*    ArpBackgroundPen -> */ ArpBasicFillPen,
	/*          ArpTextPen -> */ ArpBasicTextPen,
	/*      ArpStandoutPen -> */ ArpBasicTextPen,
	/*         ArpShinePen -> */ ArpShinePen,
	/*        ArpShadowPen -> */ ArpShadowPen,
	/*     ArpBasicFillPen -> */ ArpBasicFillPen,
	/*     ArpBasicTextPen -> */ ArpBasicTextPen,
	/*      ArpForeFillPen -> */ ArpBasicFillPen,
	/*      ArpForeTextPen -> */ ArpBasicTextPen,
	/* ArpHighlightFillPen -> */ ArpBasicFillPen,
	/* ArpHighlightTextPen -> */ ArpBasicTextPen,
	/*     ArpErrorFillPen -> */ ArpBasicFillPen,
	/*     ArpErrorTextPen -> */ ArpBasicTextPen
#else
	/*    ArpBackgroundPen -> */ ArpBackgroundPen,
	/*          ArpTextPen -> */ ArpTextPen,
	/*      ArpStandoutPen -> */ ArpTextPen,
	/*         ArpShinePen -> */ ArpShinePen,
	/*        ArpShadowPen -> */ ArpShadowPen,
	/*     ArpBasicFillPen -> */ ArpBasicFillPen,
	/*     ArpBasicTextPen -> */ ArpBasicTextPen,
	/*      ArpForeFillPen -> */ ArpBasicFillPen,
	/*      ArpForeTextPen -> */ ArpBasicTextPen,
	/* ArpHighlightFillPen -> */ ArpBackgroundPen,
	/* ArpHighlightTextPen -> */ ArpTextPen,
	/*     ArpErrorFillPen -> */ ArpBackgroundPen,
	/*     ArpErrorTextPen -> */ ArpTextPen
#endif
#endif
};

ArpLogicalPen ArpDrawPens::bwMap[_Num_ArpLogicalPen] = {
	/*    ArpBackgroundPen -> */ ArpBackgroundPen,
	/*          ArpTextPen -> */ ArpTextPen,
	/*      ArpStandoutPen -> */ ArpTextPen,
	/*         ArpShinePen -> */ ArpTextPen,
	/*        ArpShadowPen -> */ ArpTextPen,
	/*     ArpBasicFillPen -> */ ArpBackgroundPen,
	/*     ArpBasicTextPen -> */ ArpTextPen,
	/*      ArpForeFillPen -> */ ArpBackgroundPen,
	/*      ArpForeTextPen -> */ ArpTextPen,
	/* ArpHighlightFillPen -> */ ArpBackgroundPen,
	/* ArpHighlightTextPen -> */ ArpTextPen,
	/*     ArpErrorFillPen -> */ ArpBackgroundPen,
	/*     ArpErrorTextPen -> */ ArpTextPen
};

#if 0
ArpLogicalPen ArpDrawPens::Map[_Num_ArpLogicalPen] = {
	/*    ArpBackgroundPen -> */ ArpPen,
	/*          ArpTextPen -> */ ArpPen,
	/*      ArpStandoutPen -> */ ArpPen,
	/*         ArpShinePen -> */ ArpPen,
	/*        ArpShadowPen -> */ ArpPen,
	/*     ArpBasicFillPen -> */ ArpPen,
	/*     ArpBasicTextPen -> */ ArpPen,
	/*      ArpForeFillPen -> */ ArpPen,
	/*      ArpForeTextPen -> */ ArpPen,
	/* ArpHighlightFillPen -> */ ArpPen,
	/* ArpHighlightTextPen -> */ ArpPen
	/*     ArpErrorFillPen -> */ ArpPen,
	/*     ArpErrorTextPen -> */ ArpPen
};
#endif

static ArpColor initpen_colors[_Num_ArpLogicalPen] = {
	ArpColor::White, ArpColor::Black,	// text
	ArpColor::Black,					// stand
	ArpColor::Black, ArpColor::Black,	// 3d
	ArpColor::Black, ArpColor::White,	// fill
	ArpColor::White, ArpColor::Black,	// fore
	ArpColor::Black, ArpColor::White,	// high
	ArpColor::Black, ArpColor::White,	// error
};

ArpDrawPens::ArpDrawPens(ArpDrawPens const& from) : penMode(0)
{
	for( int i=0; i<_Num_ArpLogicalPen; i++ ) pens[i] = from.pens[i];
}

ArpDrawPens& ArpDrawPens::operator=(ArpDrawPens const& from)
{
	for( int i=0; i<_Num_ArpLogicalPen; i++ ) pens[i] = from.pens[i];
	return *this;
}

ArpColor const& ArpDrawPens::GetPen(ArpLogicalPen pen) const
{
	if( pen >= _Num_ArpLogicalPen ) pen = ArpTextPen;
	else if( pen <= _End_ArpLogicalPen ) pen = ArpBackgroundPen;
	if( penMode&PMF_Complement ) pen = compMap[pen];
	if( penMode&PMF_Highlight ) pen = highMap[pen];
	if( penMode&PMF_Disable ) pen = disableMap[pen];
	if( penMode&PMF_BlackWhite ) pen = bwMap[pen];
	return pens[pen];
}

ArpColor& ArpDrawPens::GetPen(ArpLogicalPen pen)
{
	if( pen >= _Num_ArpLogicalPen ) pen = ArpTextPen;
	else if( pen <= _End_ArpLogicalPen ) pen = ArpBackgroundPen;
	if( penMode&PMF_Complement ) pen = compMap[pen];
	if( penMode&PMF_Highlight ) pen = highMap[pen];
	if( penMode&PMF_Disable ) pen = disableMap[pen];
	if( penMode&PMF_BlackWhite ) pen = bwMap[pen];
	return pens[pen];
}

ArpColor& ArpDrawPens::GetRawPen(ArpLogicalPen pen)
{
	if( pen >= _Num_ArpLogicalPen ) {
		return pens[ArpTextPen];
	} else if( pen <= _End_ArpLogicalPen ) {
		return pens[ArpBackgroundPen];
	}
	return pens[pen];
}

ArpColor const& ArpDrawPens::GetRawPen(ArpLogicalPen pen) const
{
	if( pen >= _Num_ArpLogicalPen ) {
		return pens[ArpTextPen];
	} else if( pen <= _End_ArpLogicalPen ) {
		return pens[ArpBackgroundPen];
	}
	return pens[pen];
}

void ArpDrawPens::InitPens()
{
	for( int i=0; i<_Num_ArpLogicalPen; i++ ) {
		pens[i] = initpen_colors[i];
		ArpD(cdb << ADH << "Initialized pen #" << i
					 << " to color " << (rgb_color)pens[i] << endl);
	}
}

void ArpDrawPens::SetPens(ArpLogicalPen pen1, ...)
{
	va_list ap;
	va_start(ap, pen1);
	ArpColor col;
	while( pen1 != _End_ArpLogicalPen ) {
		col = va_arg(ap,ArpColor);
		SetPen(pen1,col);
		ArpD(cdb << ADH << "Set pen #" << (int)pen1
					 << " to color " << (rgb_color)col << endl);
		(void)GetPen(pen1);
		pen1 = va_arg(ap,ArpLogicalPen);
	}
	va_end(ap);
}

static const ArpColor Black = ArpColor(0x00,0x00,0x00);
static const ArpColor Gray90 = ArpColor(0xc0,0xc0,0xc0);

static const ArpColor stdpen_colors[_Num_ArpLogicalPen] = {
	ArpColor(0x00,0x00,0x00), ArpColor(0xc0,0xc0,0xc0),	// text
	ArpColor(0xff,0xff,0x00),								// stand
	ArpColor(0xff,0xff,0xff), ArpColor(0x30,0x30,0x30),	// 3d
	ArpColor(0x40,0x40,0x40), ArpColor(0xc0,0xc0,0xc0),	// fill
	ArpColor(0x80,0x80,0x80), ArpColor(0x00,0x00,0x00),	// fore
	ArpColor(0x00,0x80,0xff), ArpColor(0x00,0x00,0x00),	// high
	ArpColor(0x80,0xff,0x80), ArpColor(0x00,0x00,0x00),	// error
};

ArpStdDrawPens::ArpStdDrawPens()
{
	for( int i=0; i<_Num_ArpLogicalPen; i++ ) {
		pens[i] = stdpen_colors[i];
		ArpD(cdb << ADH << "Initialized pen #" << i
					 << " to color " << (rgb_color)pens[i] << endl);
	}
}

static const ArpColor tintpen_colors[_Num_ArpLogicalPen] = {
	ArpColor(0x00,0x00,0x00), ArpColor(0xe0,0xd0,0xc0),	// text
	ArpColor(0xff,0xff,0x00),								// stand
	ArpColor(0xff,0xff,0xff), ArpColor(0x80,0x80,0x80),	// 3d
	ArpColor(0x60,0x20,0x20), ArpColor(0xe0,0xd0,0xc0),	// fill
	ArpColor(0xe0,0xd0,0xc0), ArpColor(0x00,0x00,0x00),	// fore
	ArpColor(0x00,0x80,0xff), ArpColor(0x00,0x00,0x00),	// high
	ArpColor(0x80,0xff,0x80), ArpColor(0x00,0x00,0x00),	// error
};

ArpTintDrawPens::ArpTintDrawPens()
{
	for( int i=0; i<_Num_ArpLogicalPen; i++ ) {
		pens[i] = tintpen_colors[i];
		ArpD(cdb << ADH << "Initialized pen #" << i
					 << " to color " << (rgb_color)pens[i] << endl);
	}
}

static const ArpColor lightpen_colors[_Num_ArpLogicalPen] = {
	ArpColor(0xe0,0xd0,0xc0), ArpColor(0x00,0x00,0x00),	// text
	ArpColor(0x00,0x00,0x40),								// stand
	ArpColor(0xff,0xff,0xff), ArpColor(0x00,0x00,0x00),	// 3d
	ArpColor(0x50,0x40,0x30), ArpColor(0xe0,0xd0,0xc0),	// fill
	ArpColor(0xe0,0xd0,0xc0), ArpColor(0x00,0x00,0x00),	// fore
	ArpColor(0x00,0x00,0x50), ArpColor(0xe0,0xd0,0xc0),	// high
	ArpColor(0x60,0x00,0x00), ArpColor(0xe0,0xd0,0xc0),	// error
};

ArpLightDrawPens::ArpLightDrawPens()
{
	for( int i=0; i<_Num_ArpLogicalPen; i++ ) {
		pens[i] = lightpen_colors[i];
		ArpD(cdb << ADH << "Initialized pen #" << i
					 << " to color " << (rgb_color)pens[i] << endl);
	}
}
