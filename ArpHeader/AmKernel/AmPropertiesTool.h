/* AmPropertiesTool.h
 * Copyright (c)2000 by Angry Red Planet and Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2000.06.22		hackborn
 * Created this file.
 */
 
#ifndef AMKERNEL_AMPROPERTIESTOOL_H
#define AMKERNEL_AMPROPERTIESTOOL_H

#include <support/Locker.h>
#include "AmPublic/AmToolTarget.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmTrack.h"
class BPopUpMenu;
class AmTrackWinPropertiesI;

/**********************************************************************
 * AM-PROPERTIES-TOOL
 * This tool invokes a context sensitive menu.  It is currently where
 * clients can get access to copying and pasting data to and from the
 * clipboard.
 **********************************************************************/
class AmPropertiesTool : public AmTool
{
public:
	AmPropertiesTool(); 
	virtual ~AmPropertiesTool();

	virtual bool		IsImmutable() const;

	virtual BView*		NewPropertiesView() const;

	virtual void		MouseDown(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where);
	virtual void		MouseUp(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where);
	virtual void		MouseMoved(	AmSongRef songRef,
									AmToolTarget* target,
									BPoint where,
									uint32 code);

	/* Answer true if I can paste.
	 */
	bool				CanPaste();
	/* Remove all events in selections from the system.  The caller
	 * is responsible for setting the current selections object to 0.
	 */
	void				Cut(	AmSong* song,
								AmSelectionsI* selections,
								BMessenger sender);
	void				Copy(	AmSelectionsI* selections);
	/* The target is optional since it's an addition being made long
	 * after I finished this code, but if it's there, it allows pasting
	 * between different CC numbers.
	 */
	status_t			Paste(	AmSong* song,
								AmTrackWinPropertiesI& trackWinProps,
								AmTime atTime,
								AmSelectionsI* selections,
								BMessenger sender,
								AmToolTarget* target);

protected:
	/* This does the actual work of the MouseDown() function.  The public
	 * method is just a shell to guarantee the AmTrack gets deleted.
	 */
	void MouseDown(AmSong* song, AmToolTarget* target, BPoint where);
	/* Answer true if I can copy at the given point.  I can only copy if the
	 * user is clicking on a currently-selected event.
	 */
	bool CanCopy(const AmTrack* track, AmToolTarget* target, BPoint where);
	
private:
	typedef AmTool		inherited;
	BPopUpMenu*			mMenu;
	/* This is the duration of notes the user is currently placing,
	 * which I use as a quantize time.
	 */
	AmTime				mQuantizeTime;
	/* Answer a new instance of my menu.
	 */
	BPopUpMenu*			NewMenu() const;
};

#endif 


