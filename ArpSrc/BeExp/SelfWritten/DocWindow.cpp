#include <Entry.h>
#include <Message.h>

#include <stdio.h>

#include <experimental/DocWindow.h>

DocWindow::DocWindow(DocApplication* r, entry_ref* ref, BRect frame,
	const char *title, window_look look, window_feel feel,
	uint32 flags, uint32 workspace)
	:
	BWindow(frame, title, look, feel, flags, workspace), untitled(true)
{
	windowroster = r;
	if (ref) {
		fileref = *ref;
		untitled = false;
	}
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
//	return !dirty;
	return !IsDirty();	// Virtual!
}

void
DocWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case DOC_WIN_SAVE:
			if(untitled)
				SaveAs();
			else {
				BEntry ent(&fileref);
				Save(&ent);
			}
			break;
		case DOC_WIN_SAVE_AS:
			SaveAs();
			break;
		default:
			printf("DocWindow::MessageReceived()\n");
			BWindow::MessageReceived(msg);
	}
}


bool
DocWindow::IsDirty()
{
	return dirty;
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
	untitled = false;
}


BFilePanel*
DocWindow::CreateSavePanel() const
{
	printf("DocWindow::CreateSavePanel()\n");	
	return NULL;
}


void
DocWindow::SetDirty(bool setDirty)
{
	dirty = setDirty;
}


entry_ref
DocWindow::FileRef() const
{
	return fileref;
}
