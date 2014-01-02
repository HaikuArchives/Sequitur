#include <be/experimental/DocWindow.h>
#include <stdio.h>
#include <Message.h>
DocWindow::DocWindow(WindowRoster* r, entry_ref* ref, BRect frame,
	const char *title, window_look look, window_feel feel,
	uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE)
	:
	BWindow(frame, title, look, feel, flags, workspace)
{
	windowroster = r;
	if(ref)
		fileref = *ref;
	dirty = false;
	printf("Nothign here yet");
}

DocWindow::~DocWindow()
{
	printf("Nothign here yet");
}

bool
DocWindow::QuitRequested()
{
	printf("Nothign here yet");
	return true;
}

void
DocWindow::MessageReceived(BMessage* msg)
{
	printf("Nothign here yet");
	BWindow::MessageReceived(msg);
}

void
DocWindow::MenusBeginning()
{
	printf("Nothign here yet");	
}

bool
DocWindow::IsDirty()
{
	return dirty;
}

void
DocWindow::EntryChanged(BMessage* msg)
{
	printf("Nothign here yet");
}

status_t
DocWindow::Load(BEntry* e)
{
	printf("Nothign here yet");
	return B_OK;
}

status_t
DocWindow::Save(BEntry* e, const BMessage* args = 0)
{
	printf("Nothign here yet");
	return B_ERROR;
}

void
DocWindow::SaveAs()
{
	printf("Nothign here yet");
}

BFilePanel*
DocWindow::CreateSavePanel() const
{
	printf("Nothign here yet");	
	return NULL;
}

void
DocWindow::WindowFrame(BRect* proposed)
{
	*proposed = Frame();
}

void
DocWindow::SetDirty(bool flag)
{
	dirty = flag;
}

entry_ref
DocWindow::FileRef() const
{
	return fileref;
}
