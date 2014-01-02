/* SeqTimeViews.cpp
 */
#include <assert.h>
#include <stdio.h>
#include <app/Cursor.h>
#include <app/Message.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmPhrase.h"
#include "AmKernel/AmSong.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqTimeViews.h"

/* Number of pixels between each digit.
 */
static const float			DIGIT_SPACE	= 3;

static const BBitmap*		digit_0 = 0;
static const BBitmap*		digit_1 = 0;
static const BBitmap*		digit_2 = 0;
static const BBitmap*		digit_3 = 0;
static const BBitmap*		digit_4 = 0;
static const BBitmap*		digit_5 = 0;
static const BBitmap*		digit_6 = 0;
static const BBitmap*		digit_7 = 0;
static const BBitmap*		digit_8 = 0;
static const BBitmap*		digit_9 = 0;
static const BBitmap*		digit_c = 0;

/*************************************************************************
 * SEQ-SMALL-DIGIT-VIEW
 * Abstract class for non-interactive views that display small digits.
 *************************************************************************/
SeqSmallDigitView::SeqSmallDigitView(	BRect frame,
										const char* name,
										AmSongRef songRef )
		: inherited(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  AmSongObserver(songRef), mDigitWidth(0)
{
	if (!digit_0) digit_0 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 0");
	if (!digit_1) digit_1 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 1");
	if (!digit_2) digit_2 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 2");
	if (!digit_3) digit_3 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 3");
	if (!digit_4) digit_4 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 4");
	if (!digit_5) digit_5 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 5");
	if (!digit_6) digit_6 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 6");
	if (!digit_7) digit_7 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 7");
	if (!digit_8) digit_8 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 8");
	if (!digit_9) digit_9 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 9");
	if (!digit_c) digit_c = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit :");

	if (mDigitWidth == 0 && digit_0) mDigitWidth = digit_0->Bounds().Width();
}

SeqSmallDigitView::~SeqSmallDigitView()
{
}

float SeqSmallDigitView::DrawBuffer(BView* view, BPoint leftTop, char* buf, size_t limit)
{
	float		left = leftTop.x;
	size_t		len = strlen(buf);
	if (len > limit) len = limit;
	size_t		blen = limit - len;
	/* Draw any blanks
	 */
	for (size_t k = 0; k < blen; k++) {
		if (digit_0) view->DrawBitmapAsync( digit_0, BPoint(left, leftTop.y) );
		left += mDigitWidth + DIGIT_SPACE;
	}
	/* Draw the measure digits.
	 */
	for (size_t k = blen; k < limit; k++) {
		const BBitmap*		bm = BitmapFor( buf[k - blen] );
		if (bm) view->DrawBitmapAsync( bm, BPoint(left, leftTop.y) );
		left += mDigitWidth + DIGIT_SPACE;
	}
	return left;
}

const BBitmap* SeqSmallDigitView::BitmapFor(char value) const
{
	int32	val = value - 48;
	if (val == 0) return digit_0;
	else if (val == 1) return digit_1;
	else if (val == 2) return digit_2;
	else if (val == 3) return digit_3;
	else if (val == 4) return digit_4;
	else if (val == 5) return digit_5;
	else if (val == 6) return digit_6;
	else if (val == 7) return digit_7;
	else if (val == 8) return digit_8;
	else if (val == 9) return digit_9;
	return 0;
}

/*************************************************************************
 * SEQ-TIME-MBT-VIEW
 *************************************************************************/
SeqTimeMbtView::SeqTimeMbtView(	BRect frame,
								const char* name,
								AmSongRef songRef )
		: inherited(frame, name, songRef),
		  mTime(-1), mSignature( new AmSignature() )
{
	SetTime(0);
	SetViewColor(B_TRANSPARENT_COLOR);
}

SeqTimeMbtView::~SeqTimeMbtView()
{
	if (mSignature) mSignature->Delete();
}

void SeqTimeMbtView::SetTime(AmTime time)
{
	if (mTime != time) {
		mTime = time;
		ConstructSignatureFromTime();
		Invalidate();
	}
}

void SeqTimeMbtView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>(Window());
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqTimeMbtView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor( Prefs().Color(AM_LCD_C) );
	view->FillRect(clip);
	if (!mSignature) return;
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	float		left = 0, top = 0;
	char		buf[32];
	/* Display the measure value.
	 */
	sprintf( buf, "%ld", mSignature->Measure() );
	left = DrawBuffer(view, BPoint(left, top), buf, 4);
	/* -- display a ":" -- */
	if (digit_c) view->DrawBitmapAsync( digit_c, BPoint(left, top) );
	left += mDigitWidth + DIGIT_SPACE;
	/* Calculate and display the beat value.
	 */
	AmTime		time = mTime - mSignature->StartTime();
	AmTime		start = 0;
	AmTime		ticks = mSignature->TicksPerBeat();
	int32		beat = 1;
	while ( (start + ticks) <= time) {
		start += ticks;
		beat++;
	}
	sprintf(buf, "%ld", beat);
	left = DrawBuffer(view, BPoint(left, top), buf, 2);
	/* -- display a ":" -- */
	if (digit_c) view->DrawBitmapAsync( digit_c, BPoint(left, top) );
	left += mDigitWidth + DIGIT_SPACE;
	/* Calculate and display the tick value.
	 */
	sprintf(buf, "%lld", time - start);
	left = DrawBuffer( view, BPoint(left, top), buf, 4 );

	view->SetDrawingMode(B_OP_COPY);
}

void SeqTimeMbtView::ConstructSignatureFromTime()
{
	if (!mSignature) mSignature = new AmSignature();
	if (!mSignature) return;
	status_t		err = B_ERROR;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqTimeMbtView::ConstructSignatureFromTime() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if (song) err = song->GetSignature(mTime, *mSignature);
	ReadUnlock(song);
	// END READ TRACK BLOCK
	if (err != B_OK) {
		mSignature->Delete();
		mSignature = 0;
	}
	return;
}

/*************************************************************************
 * SEQ-TIME-HMSM-VIEW
 *************************************************************************/
static int32		MS_IN_HRS = 3600000;
static int32		MS_IN_MIN = 60000;
static int32		MS_IN_SEC = 1000;	

SeqTimeHmsmView::SeqTimeHmsmView(	BRect frame,
									const char* name,
									AmSongRef songRef )
		: inherited(frame, name, songRef),
		  mTime(-1), mMilliseconds(0)
{
	SetTime(0);
	SetViewColor(B_TRANSPARENT_COLOR);
}

SeqTimeHmsmView::~SeqTimeHmsmView()
{
}

void SeqTimeHmsmView::SetTime(AmTime time)
{
	if (mTime != time) {
		mTime = time;
		// READ SONG BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqTimeHmsmView::SetTime() read lock\n"); fflush(stdout);
		#endif
		const AmSong*	song = ReadLock();
		if (song) ConstructMilliseconds(song->TempoPhrase(), mTime);
		ReadUnlock(song);
		// END READ TRACK BLOCK
		Invalidate();
	}
}

void SeqTimeHmsmView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqTimeHmsmView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor( Prefs().Color(AM_LCD_C) );
	view->FillRect(clip);

	int32		hr = 0;
	int32		min = 0;
	int32		sec = 0;
	int32		ms = (int32)mMilliseconds;
	/* Is this faster than doing a modulo on each part?
	 */
	while (ms >= MS_IN_HRS) {
		hr++;
		ms -= MS_IN_HRS;
	}
	while (ms >= MS_IN_MIN) {
		min++;
		ms -= MS_IN_MIN;
	}
	while (ms >= MS_IN_SEC) {
		sec++;
		ms -= MS_IN_SEC;
	}

	float		left = 0, top = 0;
	char		buf[32];

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	/* Display the hours.
	 */
	sprintf(buf, "%ld", hr);
	left = DrawBuffer(view, BPoint(left, top), buf, 2);
	/* -- display a ":" -- */
	if (digit_c) view->DrawBitmapAsync( digit_c, BPoint(left, top) );
	left += mDigitWidth + DIGIT_SPACE;
	/* Display the minutes.
	 */
	sprintf(buf, "%ld", min);
	left = DrawBuffer(view, BPoint(left, top), buf, 2);
	/* -- display a ":" -- */
	if (digit_c) view->DrawBitmapAsync( digit_c, BPoint(left, top) );
	left += mDigitWidth + DIGIT_SPACE;
	/* Display the seconds.
	 */
	sprintf(buf, "%ld", sec);
	left = DrawBuffer(view, BPoint(left, top), buf, 2);
	/* -- display a ":" -- */
	if (digit_c) view->DrawBitmapAsync( digit_c, BPoint(left, top) );
	left += mDigitWidth + DIGIT_SPACE;
	/* Display the milliseconds.
	 */
	sprintf(buf, "%ld", ms);
	left = DrawBuffer(view, BPoint(left, top), buf, 3);

	view->SetDrawingMode(mode);
}

void SeqTimeHmsmView::ConstructMilliseconds(const AmPhrase* tempos, AmTime time)
{
	mMilliseconds = 0;
	if (!tempos) return;
	AmNode*			node = tempos->HeadNode();
	AmTempoChange*	leftTempo = NULL;
	AmTempoChange*	rightTempo = NULL;
	if (node) rightTempo = dynamic_cast<AmTempoChange*>( node->Event() );

	while (node) {
		/* Generate a tempo range -- from left to right.
		 */
		leftTempo = rightTempo;
		rightTempo = NULL;
		if (node->next) rightTempo = dynamic_cast<AmTempoChange*>( node->next->Event() );
		/* Figure out if I fall in this range, and do something based on
		 * whether or not I do.
		 */
		if (!leftTempo || time <= leftTempo->StartTime() ) return;
		if (!rightTempo || time <= rightTempo->StartTime() ) {
			double		beats = (double)(time - leftTempo->StartTime()) / (double)PPQN;
			mMilliseconds += (beats * (60000/ leftTempo->Tempo() ));
			return;
		}
		double		beats = (double)(rightTempo->StartTime() - leftTempo->StartTime()) / (double)PPQN;
		mMilliseconds += (beats * (60000/ leftTempo->Tempo() ));

		node = node->next;
	}
}

