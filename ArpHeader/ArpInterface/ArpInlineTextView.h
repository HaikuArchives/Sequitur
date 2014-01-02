/* ArpInlineTextView.h
 * Copyright (c)2000 by Angry Red Planet.
 * All rights reserved.
 *
 * A text view for providing in-place text editing.
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
 * 05.08.00		hackborn
 * Created this file.
 */

#ifndef OSINTERFACE_OSINLINETEXTVIEW_H
#define OSINTERFACE_OSINLINETEXTVIEW_H

#include <ArpCore/String16.h>
#include <app/Messenger.h>
#include <interface/View.h>

enum {
	INLINE_INTERMEDIATEUPDATE_MSG = '.PIU',
	INLINE_FINALUPDATE_MSG = '.PFU',
	INLINE_GOTOPREVIOUS_MSG = '.PGP',
	INLINE_GOTONEXT_MSG = '.PGN',
	
	INLINE_STARTFOCUS_MSG = '.PSF',
	INLINE_ENDFOCUS_MSG = '.PSE'
};

namespace ARP {
class InlineEditText;
}

class ArpInlineTextView : public BView
{
private:
	typedef BView inherited;
	
public:
	ArpInlineTextView(const BMessenger& owner,
					const char *name,
					const BFont* font,
					float textleft, float rightmost, float baseline,
					uint32 resizeMask = B_FOLLOW_NONE,
					uint32 flags = B_WILL_DRAW);
	
	virtual ~ArpInlineTextView();
	
	void SetOwner(const BMessenger& owner);
	BMessenger Owner() const;
	
	bool StartWithFocus() const;
	void StartWithFocus(bool state);
	
//	virtual void		SetText(const char* text);
//	const char*			Text() const;
	virtual void		SetText(const BString16* text);
	const BString16*	Text() const;
	
	bool HasChanged() const;
	
	bool ContinueEdit();
	
	virtual void MoveOver(float textleft, float rightmost,
						  float baseline);
	
	virtual void AllAttached();
	virtual void DetachedFromWindow();
	
	virtual void MessageReceived(BMessage* message);

	void MakeFocus(bool focusState=true);
	virtual void SetViewColor(rgb_color col);
	virtual void SetHighColor(rgb_color col);
	virtual void SetFont(const BFont *font, uint32 mask = B_FONT_ALL);
	
	virtual	void Draw(BRect inRect);

private:
	enum { FRAMESIZE=2 };
	
	static BRect frame_from_font(float textleft, float rightmost,
								 float baseline, const font_height* fh);
	static BRect frame_from_font(float textleft, float rightmost,
								 float baseline, const BFont* font);
	BRect frame_from_font(float textleft, float rightmost, float baseline);
	
	BMessenger mOwner;
	ARP::InlineEditText* mEditText;
	bool mStartWithFocus;
};

#endif
