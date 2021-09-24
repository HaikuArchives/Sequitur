#include <FilePanel.h>
#include <String.h>

#include <cstdio>

#include <experimental/DocApplication.h>
#include <experimental/DocWindow.h>

DocApplication::DocApplication(const char* signature, DocWindowFactory factory)
	:
	BApplication(signature),
	factory(factory),
	untitledcount(0),
	openpanel(NULL),
	fSignature(signature)
{
	printf("DocApplication::DocApplication\n");
}


DocApplication::~DocApplication()
{
	
}


void
DocApplication::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case DOC_APP_NEW_WINDOW:
			New();
			break;
		case DOC_APP_OPEN:
			FileOpen();
			break;
		default:
			printf("DocApplication::MessageReceived\n");
			msg->PrintToStream();
			inherited::MessageReceived(msg);		
	}
}


void
DocApplication::ReadyToRun()
{
	printf("DocApplication::ReadyToRun\n");
	if (window_list.IsEmpty()) New();
}


void
DocApplication::RefsReceived(BMessage* msg)
{
	printf("DocApplication::RefsReceived\n");
	LoadRefs(msg);
	inherited::RefsReceived(msg);
}


void
DocApplication::FileOpen()
{
	printf("DocApplication::FileOpen\n");
	if(!openpanel)
		openpanel = new BFilePanel(B_OPEN_PANEL);
	openpanel->Show();
}


void
DocApplication::New()
{
	printf("DocApplication::New\n");
	BString title = "Untitled ";
	title << ++untitledcount;
//	AddWindow(factory(this, NULL, title.String(), B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, B_ALL_WORKSPACES), NULL);
	AddWindow(factory(this, NULL, title.String(), B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, B_CURRENT_WORKSPACE), NULL);
}


void
DocApplication::AddWindow(DocWindow* window, entry_ref* ref)
{
	struct window_info* info = new struct window_info;
	BEntry* refEntry = new BEntry(ref);

	info->ref = ref;
	refEntry->GetNodeRef(&info->nref);
	info->w = window;
	info->windowmenu = WindowMenu(window);
	info->number = window_list.CountItems()+1;

	window_list.AddItem(info);
	
	window->Show();
}


void
DocApplication::ChangeWindow(DocWindow* window, entry_ref* ref)
{
	struct window_info* info = Get(window);
	BEntry* refEntry = new BEntry(ref);

	info->ref = ref;
	refEntry->GetNodeRef(&info->nref);
	
	for (int32 i = 0; i < window_list.CountItems(); i++) {
		UpdateWindowMenu(((struct window_info*)window_list.ItemAt(i))->w);
	}
}


void
DocApplication::RemoveWindow(DocWindow* window)
{
	struct window_info* info = Get(window);

	window_list.RemoveItem(info);
	delete info;
}


BMenu*
DocApplication::WindowMenu(DocWindow* window)
{
	return NULL;
}


void
DocApplication::UpdateWindowMenu(DocWindow* window)
{
}


void
DocApplication::LoadRef(entry_ref* ref)
{
	DocWindow* window = factory(this, ref, ref->name, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, B_CURRENT_WORKSPACE);
	AddWindow(window, ref);
}


void
DocApplication::LoadRefs(BMessage* message)
{
	entry_ref* ref = new entry_ref();
	message->FindRef("refs", 0, ref);
	LoadRef(ref);
}


struct DocApplication::window_info*
DocApplication::Get(DocWindow* window)
{
	for (int32 i = 0; i < window_list.CountItems(); i++) {
		struct window_info* info = (struct window_info*)window_list.ItemAt(i);

		if (info->w == window)
			return info;
	}

	return NULL;
}
