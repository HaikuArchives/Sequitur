#include <stdio.h>
#include <stdlib.h>
#include <vector.h>
#include <interface/Screen.h>
#include <interface/View.h>
#include <interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmDefs.h"
#include "AmKernel/MultiLocker.h"
#include "AmStdFactory/AmPianoRollGenerator.h"

static void fill_key_dimens(_AmKeyDimens* blackKeys, _AmKeyDimens* whiteKeys, float noteHeight);
static void render_black_key(BView* view, _AmKeyDimens* dimens, rgb_color c, BRect r);
static void render_white_key(BView* view, _AmKeyDimens* dimens, rgb_color c, BRect r);

/*************************************************************************
 * _AM-PIANO-ROLL-CACHE
 *************************************************************************/
class _AmPianoRollCache
{
public:
	_AmPianoRollCache(uint32 limit = 5);
	virtual ~_AmPianoRollCache();
	
	bool ReadLock() const;
	bool WriteLock();
	bool ReadUnlock() const;
	bool WriteUnlock();
	void close();

	const BBitmap*	PianoRollView(		float width, float noteHeight) const;
	const BBitmap*	NewPianoRollView(	float width, float noteHeight,
										_AmKeyDimens* blackKeys,
										_AmKeyDimens* whiteKeys);

private:
	mutable MultiLocker	mLock;
	vector<BBitmap*>	mBitmaps;
	uint32				mLimit;
};

static _AmPianoRollCache		gCache;

void closePianoRollCache() {	// Hack for Haiku
	gCache.close();
}

/*************************************************************************
 * AM-PIANO-ROLL-GENERATOR
 *************************************************************************/
AmPianoRollGenerator::AmPianoRollGenerator(float width)
		: mWidth(width), mLockState(HAS_NO_LOCK)
{
}

AmPianoRollGenerator::~AmPianoRollGenerator()
{
	if (mLockState == HAS_READ_LOCK) gCache.ReadUnlock();
	else if (mLockState == HAS_WRITE_LOCK) gCache.WriteUnlock();
}

const BBitmap* AmPianoRollGenerator::PianoRollView(float noteHeight)
{
	fill_key_dimens(mBlackKeys, mWhiteKeys, noteHeight);

	mLockState = HAS_NO_LOCK;
	gCache.ReadLock();
	mLockState = HAS_READ_LOCK;
	const BBitmap*	bm = gCache.PianoRollView(mWidth, noteHeight);
	if (bm) return bm;

	gCache.ReadUnlock();
	gCache.WriteLock();
	mLockState = HAS_WRITE_LOCK;
	bm = gCache.NewPianoRollView(mWidth, noteHeight, mBlackKeys, mWhiteKeys);
	if (bm) return bm;
	
	gCache.WriteUnlock();
	mLockState = HAS_NO_LOCK;
	return NULL;
}

void AmPianoRollGenerator::RenderKey(BView* view, uint8 key, rgb_color c, BRect r)
{
	ArpVALIDATE(view, return);
	ArpVALIDATE(mWhiteKeys[0].bottom > 0, return);
	
	_AmKeyDimens* black = NULL; 
	_AmKeyDimens* white = NULL; 
	if (key == 0) white = &mWhiteKeys[6];
	else if (key == 1) black = &mBlackKeys[4];
	else if (key == 2) white = &mWhiteKeys[5];
	else if (key == 3) black = &mBlackKeys[3];
	else if (key == 4) white = &mWhiteKeys[4];
	else if (key == 5) white = &mWhiteKeys[3];
	else if (key == 6) black = &mBlackKeys[2];
	else if (key == 7) white = &mWhiteKeys[2];
	else if (key == 8) black = &mBlackKeys[1];
	else if (key == 9) white = &mWhiteKeys[1];
	else if (key == 10) black = &mBlackKeys[0];
	else if (key == 11) white = &mWhiteKeys[0];

	if (black) render_black_key(view, black, c, r);
	else if (white) render_white_key(view, white, c, r);
}

// #pragma mark -

/*************************************************************************
 * _AM-PIANO-ROLL-CACHE
 *************************************************************************/
_AmPianoRollCache::_AmPianoRollCache(uint32 limit)
		: mLimit(limit)
{
}

_AmPianoRollCache::~_AmPianoRollCache()
{
//	for (uint32 k = 0; k < mBitmaps.size(); k++) delete mBitmaps[k];
//	mBitmaps.resize(0);
}

void _AmPianoRollCache::close() {
	for (uint32 k = 0; k < mBitmaps.size(); k++) delete mBitmaps[k];
	mBitmaps.resize(0);
}

bool _AmPianoRollCache::ReadLock() const
{
	return mLock.ReadLock();
}

bool _AmPianoRollCache::WriteLock()
{
	return mLock.WriteLock();
}

bool _AmPianoRollCache::ReadUnlock() const
{
	return mLock.ReadUnlock();
}

bool _AmPianoRollCache::WriteUnlock()
{
	return mLock.WriteUnlock();
}

const BBitmap* _AmPianoRollCache::PianoRollView(float width, float noteHeight) const
{
	for (uint32 k = 0; k < mBitmaps.size(); k++) {
		BRect		b = mBitmaps[k]->Bounds();
		if (b.Width() == width && b.Height() == (noteHeight * 12) ) return mBitmaps[k];
	}
	return NULL;
}

const BBitmap* _AmPianoRollCache::NewPianoRollView(	float width, float noteHeight,
													_AmKeyDimens* blackKeys,
													_AmKeyDimens* whiteKeys)
{
	BRect		r(0, 0, width, noteHeight * 12);
	BWindow*	win = new BWindow(r, "PicWin", B_BORDERED_WINDOW, B_NOT_MOVABLE);
	if (!win) return NULL;
	BScreen screen(win);

	BBitmap*	bm = new BBitmap(r, screen.ColorSpace(), true);
	if (!bm) {
		delete win;
		return NULL;
	}
	
	BView	*view = new BView(bm->Bounds(), "View", B_FOLLOW_ALL, B_WILL_DRAW);
	if ( !view || (!bm->Lock()) ) {
		delete view;
		delete bm;
		delete win;
		return NULL;
	}
	bm->AddChild(view);

	rgb_color	c = Prefs().Color(AM_INFO_BG_C);
	for (uint32 k = 0; k < _AmKeyDimens::WHITE_KEY_NUM; k++)
		render_white_key(view, &whiteKeys[k], c, r);
	c.red = c.green = c.blue = 85;
	c.alpha = 255;
	for (uint32 k = 0; k < _AmKeyDimens::BLACK_KEY_NUM; k++)
		render_black_key(view, &blackKeys[k], c, r);

	bm->RemoveChild(view);
	bm->Unlock();
	delete view;
	delete win;

	if (mBitmaps.size() >= mLimit) {
		vector<BBitmap*>::iterator		i;
		i = mBitmaps.begin();
		delete (*i);
		mBitmaps.erase(i);
	}
	mBitmaps.push_back(bm);
	return bm;
}

// #pragma mark -

/*************************************************************************
 * Piano rendering
 *************************************************************************/
static void fill_key_dimens(_AmKeyDimens* blackKeys, _AmKeyDimens* whiteKeys,
							float noteHeight)
{
	float	top = 0;
	float	height = noteHeight * 12;
	float	whiteHeight = height / _AmKeyDimens::WHITE_KEY_NUM;
	for (uint32 k = 0; k < _AmKeyDimens::WHITE_KEY_NUM; k++) {
		whiteKeys[k].top = top;
		whiteKeys[k].bottom = (top + whiteHeight - 1);
		top += whiteHeight;
	}

	float	blackHeight = whiteHeight / 2;

	blackKeys[0].top = whiteKeys[0].bottom - (blackHeight * 0.75);
	blackKeys[1].top = whiteKeys[1].bottom - (blackHeight * 0.50);
	blackKeys[2].top = whiteKeys[2].bottom - (blackHeight * 0.25);
	blackKeys[3].top = whiteKeys[4].bottom - (blackHeight * 0.75);
	blackKeys[4].top = whiteKeys[5].bottom - (blackHeight * 0.25);

	for (uint32 k = 0; k < _AmKeyDimens::BLACK_KEY_NUM; k++)
		blackKeys[k].bottom = blackKeys[k].top + blackHeight;
}

static void render_black_key(BView* view, _AmKeyDimens* dimens, rgb_color c, BRect r)
{
	ArpASSERT(view && dimens);
	BRect		b(r.left, r.top + dimens->top, r.left + r.Width() * 0.55, r.top + dimens->bottom);
	
	/* Draw the black key.
	 */
	float		keyH = dimens->bottom - dimens->top;
	float		lightH = floor(keyH * 0.80);
	if (lightH >= keyH) lightH = keyH - 1;

	view->SetHighColor(c);
	view->FillRect( BRect(b.left, b.top, b.right, b.top + lightH) );

	view->SetHighColor( tint_color(c, B_DARKEN_3_TINT) );
	view->FillRect( BRect(b.left, b.top + lightH + 1, b.right, b.bottom) );
	view->FillRect( BRect(b.right - 3, b.top + 1, b.right, b.top + lightH) );

	view->SetHighColor( tint_color(c, 0.790F) );
	view->FillRect( BRect(b.left, b.top + 1, b.right - 2, b.top + 1) );

	/* Shade the white key(s) below this black key.
	 */
	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	view->SetHighColor(0, 0, 0, 100);
	view->StrokeLine( BPoint(b.right + 1, b.top + 1), BPoint(b.right + 1, b.bottom + 1) );
	view->StrokeLine( BPoint(b.left, b.bottom + 1), BPoint(b.right, b.bottom + 1) );

	view->SetHighColor(0, 0, 0, 75);
	view->StrokeLine( BPoint(b.right + 2, b.top + 1), BPoint(b.right + 2, b.bottom + 1) );

	view->SetHighColor(0, 0, 0, 50);
	view->StrokeLine( BPoint(b.right + 3, b.top + 2), BPoint(b.right + 3, b.bottom + 2) );
	view->StrokeLine( BPoint(b.left, b.bottom + 2), BPoint(b.right + 2, b.bottom + 2) );

	view->SetHighColor(0, 0, 0, 25);
	view->StrokeLine( BPoint(b.right + 4, b.top + 2), BPoint(b.right + 4, b.bottom + 2) );

	view->SetDrawingMode(mode);
}

static void render_white_key(BView* view, _AmKeyDimens* dimens, rgb_color c, BRect r)
{
	ArpASSERT(view && dimens);
	BRect		b(r.left, r.top + dimens->top, r.right, r.top + dimens->bottom);
	
	view->SetHighColor(c);
	view->FillRect(b);

	view->SetHighColor( tint_color(c, B_DARKEN_1_TINT) );
	view->StrokeLine( BPoint(b.left, b.bottom - 3), BPoint(b.right - 3, b.bottom - 3) );
	view->StrokeLine( BPoint(b.right - 2, b.top), BPoint(b.right - 2, b.bottom) );

	view->SetHighColor( tint_color(c, B_DARKEN_2_TINT) );
	view->StrokeLine( BPoint(b.left, b.bottom - 2), BPoint(b.right - 2, b.bottom - 2) );
	view->StrokeLine( BPoint(b.right - 1, b.top), BPoint(b.right - 1, b.bottom) );

	view->SetHighColor( tint_color(c, B_DARKEN_3_TINT) );
	view->StrokeLine( BPoint(b.left, b.bottom - 1), BPoint(b.right - 1, b.bottom - 1) );

	view->SetHighColor( tint_color(tint_color(c, B_LIGHTEN_2_TINT), B_LIGHTEN_1_TINT) );
	view->StrokeLine( BPoint(b.left, b.top), BPoint(b.right - 1, b.top) );

	view->SetHighColor( tint_color(c, B_LIGHTEN_1_TINT) );
	view->StrokeLine( BPoint(b.left, b.top + 1), BPoint(b.right - 2, b.top + 1) );

	view->SetHighColor(0, 0, 0);
	view->StrokeLine( BPoint(b.left, b.bottom), BPoint(b.right - 1, b.bottom) );
	view->StrokeLine( BPoint(b.right, b.top), BPoint(b.right, b.bottom) );
}
