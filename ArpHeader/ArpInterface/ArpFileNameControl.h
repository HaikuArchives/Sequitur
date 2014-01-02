/* ArpFileNameControl.h
 * Copyright (c)2002 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.07.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPINTERFACE_ARPFILENAMECONTROL_H
#define ARPINTERFACE_ARPFILENAMECONTROL_H

#include <ArpCore/String16.h>
#include <interface/Control.h>

/***************************************************************************
 * ARP-FILE-NAME-CONTROL
 * I display a file name.  One can be dropped on me to set it.
 ***************************************************************************/
class ArpFileNameControl : public BControl
{
public:
	ArpFileNameControl(BRect frame, const char* name, uint32 resizeMask);
	virtual ~ArpFileNameControl();
	
	virtual	void		AttachedToWindow();
	virtual void		Draw(BRect clip);
	virtual void		MessageReceived(BMessage *msg);

	BString16			FileName() const;
	void				SetFileName(const BString16* fileName);
	/* Little hack 'cause I didn't think this control out
	 * very well.
	 */
	void				SetFileNameNoInvoke(const BString16* fileName);
	
protected:
	void				DrawOn(BView* view, BRect clip);
	
private:
	typedef BControl	inherited;
	BString16			mFileName;
//	BMessage*			mInvokedMsg;
//	BMessenger			mTarget;
};

#endif
