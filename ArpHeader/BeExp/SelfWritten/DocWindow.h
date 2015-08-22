#ifndef _DOC_WINDOW_H
#define _DOC_WINDOW_H

#include <Window.h>
#include <Entry.h>

namespace BExperimental {

class DocApplication;

class DocWindow : public BWindow
{
public:
					DocWindow(DocApplication* app, entry_ref* entry, BRect frame,
							const char* title, window_look look, window_feel feel,
							uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE);
	virtual			~DocWindow();
	virtual	bool	QuitRequested();
	virtual	void	MessageReceived(BMessage* msg);

	virtual	bool		IsDirty();
	virtual	status_t	Load(BEntry* e);
	virtual status_t	Save(BEntry* e, const BMessage* args = 0);
	virtual	void		SaveAs();

	void				SetDirty(bool dirty = true);
	entry_ref			FileRef() const;

protected:
	virtual	BFilePanel*	CreateSavePanel() const;
	DocApplication	*windowroster;
	void			AddMe(entry_ref *ref);
	void			RemoveMe();
	entry_ref		fileref;
	bool			dirty;
	bool			untitled;
	BFilePanel		*savepanel;
	bool			waitforsave;
};

#define DOC_WIN_SAVE	'Dsav'
#define DOC_WIN_SAVE_AS	'Dsas'

};

using namespace BExperimental;

#endif
