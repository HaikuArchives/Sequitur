#ifndef _DOC_APPLICATION_H
#define _DOC_APPLICATION_H


#include <Application.h>
#include <FilePanel.h>
#include <Menu.h>
#include <ObjectList.h>
#include <String.h>
#include <Window.h>

namespace BExperimental
{

class DocApplication;
class DocWindow;

typedef DocWindow *(*DocWindowFactory)(DocApplication* wr, entry_ref* entry,
		const char* title, window_look look, window_feel feel,
		uint32 flags, uint32 workspace);
		
class DocApplication : public BApplication
{
	typedef BApplication inherited;
	
public:
					DocApplication(const char* signature, DocWindowFactory factory);
	virtual			~DocApplication();

	virtual	void	MessageReceived(BMessage* msg);
	virtual void	ReadyToRun();
	virtual	void	RefsReceived(BMessage* msg);
	virtual	void	FileOpen();
	virtual	void	New();

private:

	// Mostly for source compatibility
	struct	window_info
	{
		entry_ref	*ref;
		node_ref	nref;
		DocWindow	*w;
		BMenu		*windowmenu;
		int32		number;
	};
	BList				window_list;
	DocWindowFactory	factory;
	BRect				lastframe;
	int32				untitledcount;
	BFilePanel			*openpanel;
	BString				fSignature;
	
	void	AddWindow(DocWindow* window, entry_ref* ref);
	void	ChangeWindow(DocWindow* window, entry_ref* ref);
	void	RemoveWindow(DocWindow* window);
	BMenu	*WindowMenu(DocWindow* window);
	void	UpdateWindowMenu(DocWindow* window);
	void	LoadRefs(BMessage* message);

	struct window_info* Get(DocWindow* window);
};

#define DOC_APP_NEW_WINDOW		'Dnew'
#define DOC_APP_OPEN			'Dopn'
#define DOC_APP_ACTIVATE_WINDOW	'Dact'

}

using namespace BExperimental;

#endif
