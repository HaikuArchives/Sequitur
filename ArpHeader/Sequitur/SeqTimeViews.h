/* SeqTimeViews.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com> or <hackborn@genomica.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 07 December 2000		hackborn
 * Abstracted some of the SeqTimeMbtView behaviour into
 * SeqSmallDigitView, added the SeqTimeHmsmView.
 *
 * 08.16.00		hackborn
 * Created this file.
 */
 
#ifndef SEQUITUR_SEQTIMEVIEWS_H
#define SEQUITUR_SEQTIMEVIEWS_H

#include <be/interface/View.h>
#include "AmPublic/AmSongObserver.h"
class AmPhrase;
class AmSignature;

/*************************************************************************
 * SEQ-SMALL-DIGIT-VIEW
 * Abstract class for non-interactive views that display small digits.
 *************************************************************************/
class SeqSmallDigitView : public BView,
						  public AmSongObserver
{
public:
	SeqSmallDigitView(	BRect frame,
						const char* name,
						AmSongRef songRef);
	virtual	~SeqSmallDigitView();

protected:
	/* The pixel width of the digit images.
	 */
	float			mDigitWidth;

	float			DrawBuffer(BView* view, BPoint leftTop, char* buf, size_t limit);
	const BBitmap*	BitmapFor(char value) const;

private:
	typedef	BView	inherited;
};

/*************************************************************************
 * SEQ-TIME-MBT-VIEW
 * This non-interactive view displays time in the form of MMMM.BB.TTTT,
 * where M = measures, B = beats, and T = ticks.
 *************************************************************************/
class SeqTimeMbtView : public SeqSmallDigitView
{
public:
	SeqTimeMbtView(	BRect frame,
					const char* name,
					AmSongRef songRef);
	virtual	~SeqTimeMbtView();

	void			SetTime(AmTime time);
	virtual	void	Draw(BRect clip);

private:
	typedef	SeqSmallDigitView inherited;
	AmTime			mTime;
	/* This stores the signature for the current measure I'm displaying.
	 * It will be NULL if the last call to SetTime() produced an error.
	 * In this case, the display will be all zeroes.
	 */
	AmSignature*	mSignature;

	void			DrawOn(BView* view, BRect clip);
	void			ConstructSignatureFromTime();
};

/*************************************************************************
 * SEQ-TIME-HMSM-VIEW
 * This non-interactive view displays time in the form of HH.MM.SS.MMM
 * where H = hours, M = minutes, S = seconds, a M = milliseconds.
 *************************************************************************/
class SeqTimeHmsmView : public SeqSmallDigitView
{
public:
	SeqTimeHmsmView(BRect frame,
					const char* name,
					AmSongRef songRef);
	virtual	~SeqTimeHmsmView();

	void			SetTime(AmTime time);
	virtual	void	Draw(BRect clip);

private:
	typedef	SeqSmallDigitView inherited;
	AmTime			mTime;
	double			mMilliseconds;
	
	void			DrawOn(BView* view, BRect clip);
	void			ConstructMilliseconds(const AmPhrase* tempos, AmTime time);
};

#endif 
