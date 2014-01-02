#include "ArpNote.h"

#include <stdio.h>
#include <stdlib.h>
#include <interface/ScrollView.h>
#include <interface/TextView.h>
#include <interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmPrefsI.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		NOTE_STR			= "Note";
static const float		PREFERRED_WIDTH		= 150;
static const float		PREFERRED_HEIGHT	= 200;

/*****************************************************************************
 * _NOTE-VIEW
 *****************************************************************************/
class _DumbTextView;

class _NoteView : public BView
{
public:
	_NoteView(AmFilterHolderI* target, const BMessage& initSettings);
	virtual ~_NoteView();
	
	virtual	void		AttachedToWindow();
	virtual	void		GetPreferredSize(float* width, float* height);
	virtual	void		MessageReceived(BMessage* msg);

private:
	AmFilterHolderI*	mTarget;
	_DumbTextView*		mTextView;
};

/******************************************************************
 * _DUMB-TEST-VIEW
 * The text view doesn't resize its text rect when resizing.  This
 * class fixes that.  Dumb, dumb text view.
 ******************************************************************/
class _DumbTextView : public BTextView
{
public:
	_DumbTextView(BRect frame, const char* name, BRect textRect,
					uint32 resizeMask, uint32 flags = B_WILL_DRAW | B_PULSE_NEEDED);
	virtual ~_DumbTextView();
	
	virtual	void	DeleteText(int32 fromOffset, int32 toOffset);
	virtual	void	FrameResized(float width, float height);
	virtual	void	InsertText(	const char *inText, int32 inLength, 
								int32 inOffset, const text_run_array *inRuns);

	void			SetModificationMessage(BMessage* msg);

private:
	typedef BTextView	inherited;
	BMessage*			mModificationMessage;
};

/*****************************************************************************
 * ARP-NOTE-FILTER
 *****************************************************************************/
ArpNoteFilter::ArpNoteFilter(	ArpNoteAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder)
{
	if (config) PutConfiguration(config);
}

AmEvent* ArpNoteFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (event && mHolder) event->SetNextFilter(mHolder->FirstConnection() );
	return event;
}

status_t ArpNoteFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	if (mNote.Length() > 0) {
		if ((err = values->AddString(NOTE_STR, mNote)) != B_OK) return err;
	}
	return B_OK;
}

status_t ArpNoteFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	const char*		s;
	if (values->FindString(NOTE_STR, &s) == B_OK) mNote = s;
	return B_OK;
}

status_t ArpNoteFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	BView*		v = new _NoteView(mHolder, config);
	if (v) panels.push_back(v);
	return B_OK;
}

/*****************************************************************************
 * ARP-NOTE-ADD-ON
 *****************************************************************************/
void ArpNoteAddOn::LongDescription(BString& name, BString& str) const
{
	inherited::LongDescription(name, str);
	str << "<p>I perform no processing on events.  I am merely a place to store
	some text.</p>";
}

void ArpNoteAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpNoteAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpNoteAddOn(cookie);
	return NULL;
}

// #pragma mark -

/*****************************************************************************
 * _NOTE-VIEW
 *****************************************************************************/
static const uint32		MOD_MSG		= 'imod';

_NoteView::_NoteView(AmFilterHolderI* target, const BMessage& initSettings)
		: BView(BRect(0, 0, PREFERRED_WIDTH, PREFERRED_HEIGHT), "Note", B_FOLLOW_ALL, 0),
		  mTarget(target), mTextView(NULL)
{
	mTarget->IncRefs();
	BRect			b(Bounds() );
	BRect			r(5, 2, b.Width() - 5 - B_V_SCROLL_BAR_WIDTH, b.Height() - B_H_SCROLL_BAR_HEIGHT - 2);
	mTextView = new _DumbTextView(	r, "text",
									BRect(5, 5, r.Width() - 5, 0),
									B_FOLLOW_ALL);
	if (mTextView) {
		BScrollView*		sv = new BScrollView("scroll", mTextView, B_FOLLOW_ALL, 0, true, true);
		if (sv) AddChild(sv);
		else AddChild(mTextView);
		
		mTextView->SetModificationMessage(new BMessage(MOD_MSG) );
		const char*		s;
		if (initSettings.FindString(NOTE_STR, &s) == B_OK) mTextView->SetText(s);
	}
}

_NoteView::~_NoteView()
{
	mTarget->DecRefs();
}

void _NoteView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(Prefs().Color(AM_AUX_WINDOW_BG_C) );
}

void _NoteView::GetPreferredSize(float* width, float* height)
{
	*width = PREFERRED_WIDTH;
	*height = PREFERRED_HEIGHT;
}

void _NoteView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MOD_MSG:
			if (mTextView && mTarget) {
				BMessage		config;
				if (config.AddString(NOTE_STR, mTextView->Text() ) == B_OK)
					mTarget->PutConfiguration(&config);
			}
			break;
		default:
			BView::MessageReceived(msg);
	}
}

// #pragma mark -

/******************************************************************
 * _DUMB-TEST-VIEW
 ******************************************************************/
_DumbTextView::_DumbTextView(	BRect frame, const char* name,
								BRect textRect, uint32 resizeMask,
								uint32 flags)
		: inherited(frame, name, textRect, resizeMask, flags),
		  mModificationMessage(NULL)
{
}

_DumbTextView::~_DumbTextView()
{
	delete mModificationMessage;
}
	
void _DumbTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	inherited::DeleteText(fromOffset, toOffset);
	if (mModificationMessage && Window() ) {
		BMessage		m(*mModificationMessage);
		if (!Parent() ) Window()->PostMessage(&m);
		else Window()->PostMessage(&m, Parent() );
	}
}

void _DumbTextView::FrameResized(float width, float height)
{
	inherited::FrameResized(width, height);
	SetTextRect(BRect(5, 5, Bounds().Width() - 10, 0) );
}

void _DumbTextView::InsertText(	const char *inText, int32 inLength, 
								int32 inOffset, const text_run_array *inRuns)
{
	inherited::InsertText(inText, inLength, inOffset, inRuns);
	if (mModificationMessage && Window() ) {
		BMessage		m(*mModificationMessage);
		if (!Parent() ) Window()->PostMessage(&m);
		else Window()->PostMessage(&m, Parent() );
	}
}

void _DumbTextView::SetModificationMessage(BMessage* msg)
{
	delete mModificationMessage;
	mModificationMessage = msg;
}
