#include <experimental/DocApplication.h>
#include <experimental/DocWindow.h>
#include <FilePanel.h>

#include <stdio.h>

DocApplication::DocApplication(const char* signature, DocWindowFactory factory)
	:
	inherited(signature),
	factory(factory),
	untitledcount(0),
	openpanel(NULL)
{
	printf("DocApplication::DocApplication\n");
}


bool
DocApplication::QuitRequested()
{
	printf("DocApplication::QuitRequested\n");
	return true;
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
	New();
}


void
DocApplication::RefsReceived(BMessage* msg)
{
	printf("DocApplication::RefsReceived\n");
	entry_ref ref;
	
	msg->FindRef("refs", 0, &ref);
	
	DocWindow* window = factory(this, &ref, ref.name, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, B_ALL_WORKSPACES);
	
	AddWindow(window, &ref);
	
	msg->PrintToStream();
	inherited::RefsReceived(msg);
}


void
DocApplication::ArgvReceived(int32 argc, char** argv)
{
	printf("DocApplication::ArgvReceived\n");
	
	inherited::ArgvReceived(argc, argv);
}


void
DocApplication::FileOpen()
{
	printf("DocApplication::FileOpen\n");
	if(openpanel)
		delete openpanel;
	openpanel = new BFilePanel(B_OPEN_PANEL);
	openpanel->Show();
}


void
DocApplication::New()
{
	printf("DocApplication::New\n");
	BString title = "Untitled ";
	title << ++untitledcount;
	AddWindow(factory(this, NULL, title.String(), B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, B_ALL_WORKSPACES), NULL);
}


void
DocApplication::WindowFrame(BRect* proposed)
{
	printf("DocApplication::WindowFrame\n");
	proposed->Set(100, 100, 800, 800);
}


void
DocApplication::AddWindow(DocWindow* w, entry_ref* ref)
{
	printf("DocApplication::AddWindow\n");
	struct window_info* info = new struct window_info;
	info->ref = ref;
	if (ref) {
		BNode node(ref);
		node.GetNodeRef(&info->nref);
	}
	info->w = w;
	info->windowmenu = NULL;
	info->number = window_list.CountItems()+1;
	
	window_list.AddItem(info);
	w->Show();
}


void
DocApplication::ChangeWindow(DocWindow* w, entry_ref* ref)
{
	printf("DocApplication::ChangeWindow\n");
	struct window_info* info = Get(w);
	
	info->ref = ref;
	if (ref) {
		BNode node(ref);
		node.GetNodeRef(&info->nref);
	}
}

void
DocApplication::RemoveWindow(DocWindow* w)
{
	printf("DocApplication::RemoveWindow\n");
	struct window_info* info = Get(w);
	struct window_info inr = *info;
	
	window_list.RemoveItem(info);
}


BMenu*
DocApplication::WindowMenu(DocWindow* w)
{
	printf("DocApplication::WindowMenu\n");
	BMenu* magic = NULL;
	#define do return
	#define some

	do some magic;
	
	#undef do
	#undef some
}


void
DocApplication::UpdateWindowMenu(DocWindow* w)
{
	printf("DocApplication::UpdateWindowMenu\n");
}

void
DocApplication::LoadRefs(BMessage* message)
{
	printf("DocApplication::LoadRefs\n");
	message->PrintToStream();
}


struct DocApplication::window_info*
DocApplication::Get(DocWindow* w)
{
	for(int32 i = 0; i < window_list.CountItems(); i++)
	{
		struct window_info* info = reinterpret_cast<struct window_info*>(window_list.ItemAt(i));
		if(info->w == w)
			return info;
	}
	return NULL;
}
