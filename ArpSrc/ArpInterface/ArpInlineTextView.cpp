#include <ArpInterface/ArpInlineTextView.h>

#include <InterfaceKit.h>
#include <String.h>

namespace ARP {

static bool has_view(BView* parent, BView* who)
{
	BView* c;
	for (int32 i=0; (c=parent->ChildAt(i)) != NULL; i++) {
		if (c == who)
			return true;
		bool has = has_view(c, who);
		if (has)
			return true;
	}
	return false;
}

static bool has_view(BWindow* win, BView* who)
{
	BView* c;
	for (int32 i=0; (c=win->ChildAt(i)) != NULL; i++) {
		if (c == who)
			return true;
		bool has = has_view(c, who);
		if (has)
			return true;
	}
	return false;
}

class InlineEditText : public BTextView
{
private:
	typedef BTextView inherited;
	
public:
	InlineEditText(	BMessenger	owner,
					BRect		frame,
					const char	*name,
					BRect		textRect,
					uint32		resizeMask = B_FOLLOW_NONE,
					uint32		flags = B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE)
		: BTextView(frame, name, textRect, resizeMask, flags),
		  mOwner(owner), mOrigText(""), mDefButton(NULL), mHasChanged(false)
	{ }
	
	virtual ~InlineEditText()
	{ }
	
	virtual void KeyDown(const char *bytes, int32 numBytes)
	{
		int32 mods = 0;
		BMessage* msg = Window() ? Window()->CurrentMessage() : 0;
		if( msg ) {
			msg->FindInt32("modifiers",&mods);
		}
		
		if( *bytes == B_ESCAPE ) {
			BString16   tmp = mOrigText;
			mOrigText = Text();
			SetText(tmp.String());
			mHasChanged = true;
			BMessage msg(INLINE_FINALUPDATE_MSG);
			mOwner.SendMessage(&msg);
			return;
		}
		
		if( *bytes == '\n' || *bytes == '\r' ) {
			BMessage msg(INLINE_FINALUPDATE_MSG);
			mOwner.SendMessage(&msg);
			return;
		}
		
		if( Window() && Parent() ) {
			if( *bytes == B_UP_ARROW && CurrentLine() <= 0 ) {
				BMessage msg(INLINE_GOTOPREVIOUS_MSG);
				mOwner.SendMessage(&msg);
				return;
			} else if( *bytes == B_DOWN_ARROW
							&& CurrentLine() >= (CountLines()-1) ) {
				BMessage msg(INLINE_GOTONEXT_MSG);
				mOwner.SendMessage(&msg);
				return;
			}
		}
		inherited::KeyDown(bytes, numBytes);
	}
	
	virtual void AttachedToWindow()
	{
		mDefButton = Window()->DefaultButton();
		Window()->SetDefaultButton(NULL);
	}
	
	virtual void DetachedFromWindow()
	{
		if( IsFocus() ) {
			BMessage msg(INLINE_ENDFOCUS_MSG);
			mOwner.SendMessage(&msg);
		}
		if( !Window()->DefaultButton() && mDefButton ) {
			if (has_view(Window(), mDefButton)) {
				Window()->SetDefaultButton(mDefButton);
			}
		}
		mDefButton = NULL;
	}
	
	void MakeFocus(bool focusState=true)
	{
		if( Window() && Parent() && IsFocus() != focusState ) {
			BMessage msg(focusState ? INLINE_STARTFOCUS_MSG:INLINE_ENDFOCUS_MSG);
			mOwner.SendMessage(&msg);
		}
		
		inherited::MakeFocus(focusState);
	}

	void SetInitText(const BString16 *inText, 
					 const text_run_array *inRuns = NULL)
	{
		SetText(inText, inRuns);
		SelectAll();
		mOrigText = inText;
	}
	
	bool HasChanged() const			{ return mHasChanged; }
	void HasChanged(bool state)		{ mHasChanged = state; }
	
protected:
	virtual	void			InsertText(const char				*inText, 
									   int32					inLength, 
									   int32					inOffset,
									   const text_run_array		*inRuns)
	{
		inherited::InsertText(inText, inLength, inOffset, inRuns);
		mHasChanged = true;
		BMessage msg(INLINE_INTERMEDIATEUPDATE_MSG);
		mOwner.SendMessage(&msg);
	}
	
	virtual	void			DeleteText(int32 fromOffset, int32 toOffset)
	{
		inherited::DeleteText(fromOffset, toOffset);
		mHasChanged = true;
		BMessage msg(INLINE_INTERMEDIATEUPDATE_MSG);
		mOwner.SendMessage(&msg);
	}
	
private:
	BMessenger  mOwner;
	BString16   mOrigText;
	BButton*	mDefButton;
	bool		mHasChanged;
};

}	// namespace ARP

using namespace ARP;

ArpInlineTextView::ArpInlineTextView(const BMessenger& owner,
				const char *name,
				const BFont* font,
				float textleft, float rightmost, float baseline,
				uint32 resizeMask, uint32 flags)
//				uint32 resizeMask = B_FOLLOW_NONE,
//				uint32 flags = B_WILL_DRAW)
	: BView(frame_from_font(textleft, rightmost, baseline, font),
			name, resizeMask, flags),
	  mOwner(owner), mEditText(0), mStartWithFocus(true)
{
}

ArpInlineTextView::~ArpInlineTextView()
{
}

void ArpInlineTextView::SetOwner(const BMessenger& owner)
{
	mOwner = owner;
}

BMessenger ArpInlineTextView::Owner() const
{
	return mOwner;
}

bool ArpInlineTextView::StartWithFocus() const
{
	return mStartWithFocus;
}

void ArpInlineTextView::StartWithFocus(bool state)
{
	mStartWithFocus = state;
}

void ArpInlineTextView::SetText(const BString16* text)
{
	if (!mEditText) {
		BRect tbnd(Bounds());
		tbnd.InsetBy(FRAMESIZE, FRAMESIZE);
		BRect tfrm(tbnd);
		tfrm.OffsetTo(0,0);
		
		mEditText = new InlineEditText(mOwner, tbnd, "InlineTextEdit",
									 tfrm, B_FOLLOW_ALL);
		mEditText->SetStylable(false);
		mEditText->MakeSelectable();
		mEditText->MakeEditable();
		mEditText->MakeResizable(true);
		mEditText->SetWordWrap(false);
		mEditText->DisallowChar('\n');
		mEditText->DisallowChar('\r');
		mEditText->SetViewColor(ViewColor());
		BFont font;
		GetFont(&font);
		rgb_color col = HighColor();
		mEditText->SetFontAndColor(&font, B_FONT_ALL, &col);
		AddChild(mEditText);
	}
	mEditText->SetInitText(text);
}

const BString16* ArpInlineTextView::Text() const
{
	if (mEditText) {
		mEditText->HasChanged(false);
		return mEditText->Text();
	}
	return NULL;
}

/*
void ArpInlineTextView::SetText(const char* text)
{
	if( !mEditText ) {
		BRect tbnd(Bounds());
		tbnd.InsetBy(FRAMESIZE, FRAMESIZE);
		BRect tfrm(tbnd);
		tfrm.OffsetTo(0,0);
		
		mEditText = new InlineEditText(mOwner, tbnd, "InlineTextEdit",
									 tfrm, B_FOLLOW_ALL);
		mEditText->SetStylable(false);
		mEditText->MakeSelectable();
		mEditText->MakeEditable();
		mEditText->MakeResizable(true);
		mEditText->SetWordWrap(false);
		mEditText->DisallowChar('\n');
		mEditText->DisallowChar('\r');
		mEditText->SetViewColor(ViewColor());
		BFont font;
		GetFont(&font);
		rgb_color col = HighColor();
		mEditText->SetFontAndColor(&font, B_FONT_ALL, &col);
		AddChild(mEditText);
	}
	mEditText->SetInitText(text);
}

const char* ArpInlineTextView::Text() const
{
	if( mEditText ) {
		mEditText->HasChanged(false);
		return mEditText->Text();
	}
	return "";
}
*/

bool ArpInlineTextView::HasChanged() const
{
	if( mEditText ) return mEditText->HasChanged();
	return false;
}

bool ArpInlineTextView::ContinueEdit()
{
	if( mEditText ) {
		mEditText->MakeFocus();
		return true;
	}
	return false;
}

void ArpInlineTextView::MoveOver(float textleft, float rightmost,
							  float baseline)
{
	BRect frame = frame_from_font(textleft, rightmost, baseline);
	this->MoveTo(frame.left, frame.top);
	ResizeTo(frame.right-frame.left+1, frame.bottom-frame.top+1);
}

void ArpInlineTextView::AllAttached()
{
	if( mEditText ) {
		if( mStartWithFocus ) {
			mEditText->MakeFocus();
			mStartWithFocus = false;
		}
		mEditText->SelectAll();
	}
}

void ArpInlineTextView::DetachedFromWindow()
{
	if( IsFocus() ) {
		BMessage msg(INLINE_ENDFOCUS_MSG);
		mOwner.SendMessage(&msg);
	}
}

void ArpInlineTextView::MessageReceived(BMessage* message)
{
	inherited::MessageReceived(message);
}

//void ArpInlineTextView::MakeFocus(bool focusState=true)
void ArpInlineTextView::MakeFocus(bool focusState)
{
	if( Window() && Parent() ) {
		BMessage msg(focusState ? INLINE_STARTFOCUS_MSG:INLINE_ENDFOCUS_MSG);
		mOwner.SendMessage(&msg);
	}
	
	inherited::MakeFocus(focusState);
}

void ArpInlineTextView::SetViewColor(rgb_color col)
{
	inherited::SetViewColor(col);
	if( mEditText ) mEditText->SetViewColor(col);
}

void ArpInlineTextView::SetHighColor(rgb_color col)
{
	inherited::SetHighColor(col);
	if( mEditText ) {
		BFont font;
		GetFont(&font);
		mEditText->SetFontAndColor(&font, B_FONT_ALL, &col);
	}
}

void ArpInlineTextView::SetFont(const BFont *font, uint32 mask)
{
	inherited::SetFont(font,mask);
	if( mEditText ) {
		rgb_color col = HighColor();
		mEditText->SetFontAndColor(font, mask, &col);
	}
}

static rgb_color mix_color_z(rgb_color color1, rgb_color color2, float portion)
{
	rgb_color ret;
	ret.red = uint8(color1.red*portion + color2.red*(1-portion) + .5);
	ret.green = uint8(color1.green*portion + color2.green*(1-portion) + .5);
	ret.blue = uint8(color1.blue*portion + color2.blue*(1-portion) + .5);
	ret.alpha = uint8(color1.alpha*portion + color2.alpha*(1-portion) + .5);
	return ret;
}

void ArpInlineTextView::Draw(BRect inRect)
{
	rgb_color textCol = mix_color_z(HighColor(), ViewColor(), .5);
	
	BRect bnd(Bounds());
	BeginLineArray(4);
	AddLine(BPoint(bnd.left, bnd.top),
			BPoint(bnd.left, bnd.bottom), textCol);
	AddLine(BPoint(bnd.right, bnd.top),
			BPoint(bnd.right, bnd.bottom), textCol);
	AddLine(BPoint(bnd.left, bnd.top),
			BPoint(bnd.right, bnd.top), textCol);
	AddLine(BPoint(bnd.left, bnd.bottom),
			BPoint(bnd.right, bnd.bottom), textCol);
	EndLineArray();
}

BRect ArpInlineTextView::frame_from_font(float textleft, float rightmost,
										float baseline, const font_height* fh)
{
	BRect rect;
	rect.left = textleft-FRAMESIZE-1;
	rect.right = rightmost;
	
	rect.top = baseline-fh->ascent-FRAMESIZE;
	rect.bottom = baseline+fh->descent+FRAMESIZE;
	
	return rect;
}

BRect ArpInlineTextView::frame_from_font(float textleft, float rightmost,
										float baseline, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);
	return frame_from_font(textleft, rightmost, baseline, &fh);
}

BRect ArpInlineTextView::frame_from_font(float textleft, float rightmost, float baseline)
{
	font_height fh;
	GetFontHeight(&fh);
	return frame_from_font(textleft, rightmost, baseline, &fh);
}
