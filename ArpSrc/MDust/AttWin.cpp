#include "MDust/AttWin.h"
#include <Menu.h>
#include <MenuItem.h>
#include <MenuBar.h>

AttWin::AttWin()
	: BWindow(BRect(100,100,510,510), "3D Life", B_TITLED_WINDOW, 0)
{
	mView = new AttView(BRect(0,20,410,410));
	AddChild(mView);

	BMenuBar *mb = new BMenuBar(BRect(0,0,410,20), "menubar");
	
	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
	menu->SetTargetForItems(be_app);
		
	mb->AddItem(menu);
	AddChild(mb);
}

AttWin::~AttWin()
{
}

bool AttWin::QuitRequested()
{
	mView->ExitThreads();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
