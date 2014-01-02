/* SeqAboutWindow.h
 * Copyright (c)2000 by Eric Hackborn.
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 09.09.00		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQABOUTWINDOW_H
#define SEQUITUR_SEQABOUTWINDOW_H

#include <interface/Window.h>

class BMessageRunner;
class BStringView;

/*************************************************************************
 * SEQ-ABOUT-WINDOW
 *************************************************************************/
class SeqAboutWindow : public BWindow
{
public:
	SeqAboutWindow(bool startupMode = false);
	virtual ~SeqAboutWindow();

	virtual void	MessageReceived(BMessage* msg);

			void	SetStartupStatus(const char* msg);
	
private:
	typedef BWindow	inherited;

	void			AddViews();
	
	bool			mStartupMode;
	BMessageRunner*	mStartupRunner;
	BStringView*	mStatusText;
};

#endif
