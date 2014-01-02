#include <experimental/DocWindow.h>
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
	else
		fileref.name = "";
	dirty = false;
	printf("DocWindow::DocWindow(%s)\n", title);
}

DocWindow::~DocWindow()
{
	printf("DocWindow::~DocWindow()\n");
}

bool
DocWindow::QuitRequested()
{
	printf("DocWindow::QuitRequested()\n");
	return true;
}

void
DocWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'Dsav':
			if(strlen(fileref.name) > 0)
				Save(new BEntry(&fileref));
			else
				SaveAs();
			break;
		case 'Dsas':
			SaveAs();
			break;
		default:
			printf("DocWindow::MessageReceived()\n");
			BWindow::MessageReceived(msg);
	}
}

void
DocWindow::MenusBeginning()
{
	printf("DocWindow::MenusBeginning()\n");	
}

bool
DocWindow::IsDirty()
{
	return dirty;
}

void
DocWindow::EntryChanged(BMessage* msg)
{
	printf("DocWindow::EntryChanged()\n");
}

status_t
DocWindow::Load(BEntry* e)
{
	printf("DocWindow::Load()\n");
	return B_OK;
}

status_t
DocWindow::Save(BEntry* e, const BMessage* args = 0)
{
	printf("DocWindow::Save()\n");
	return B_ERROR;
}

void
DocWindow::SaveAs()
{
	printf("DocWindow::SaveAs()\n");
}

BFilePanel*
DocWindow::CreateSavePanel() const
{
	printf("DocWindow::CreateSavePanel()\n");	
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
