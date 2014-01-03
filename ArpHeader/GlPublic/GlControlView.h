/* GlControlView.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2004.02.02			hackborn@angryredplanet.com
 * Created this file.  Happy birthday, me.
 */
#ifndef GLPUBLIC_GLCONTROLVIEW_H
#define GLPUBLIC_GLCONTROLVIEW_H

#include <InterfaceKit.h>
#include <ArpInterface/ArpFont.h>
#include <ArpInterface/ViewTools.h>
#include <ArpInterface/ArpBitmapView.h>
#include <ArpInterface/ArpColourControl.h>
#include <ArpInterface/ArpFileNameControl.h>
#include <ArpInterface/ArpFloatControl.h>
#include <ArpInterface/ArpFontControl.h>
#include <ArpInterface/ArpIntControl.h>
#include <ArpInterface/ArpMenuControl.h>
#include <ArpInterface/ArpPrefs.h>
class _ControlPanel;
class BInvoker;

// FIX:  Ooops, already defined in windows!  Move this!
/*
enum {
	WM_USER		= 1630350637	// 'a---'
};
*/

/***************************************************************************
 * GL-OS-NODE-VIEW
 * Abstract superclass for window node views. whatever form they might take.
 ***************************************************************************/
class GlControlView : public BView
{
public:
	GlControlView(const BRect& frame);
	virtual ~GlControlView();

	virtual void		AttachedToWindow();
	virtual	void		GetPreferredSize(float* width, float* height);
	virtual void		MessageReceived(BMessage* msg);

	ArpBitmapView*		AddBitmapView(		const BRect& frame, const char* name);
	BButton*			AddButton(			const BRect& frame, const char* name,
											const BString16* label, uint32 msg);
	BCheckBox*			AddCheckBox(		const BRect& frame, const char* name,
											const BString16* label, uint32 msg, bool on);
	ArpColourControl*	AddColourControl(	const BRect& frame, const char* name,
											const BString16* label, const ArpVoxel& c,
											uint32 changingMsg, uint32 changedMsg, float div);
//	GlOsNodePreviewView* AddNodePreviewView(const BRect& frame, const char* name, uint32 msg);
	ArpFileNameControl*	AddFileNameControl(	const BRect& frame, const char* name,
											const BString16& fileName, uint32 message);
	ArpFloatControl*	AddFloatControl(	const BRect& frame, const char* name,
											const BString16* label, uint32 msg, uint32 finishedMsg, float low,
											float high, float value, float steps = 0.1, float divider = 0);
	ArpFontControl*		AddFontControl(		const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											const ArpFont& font, float divider = 0);
	ArpIntControl*		AddIntControl(		const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											int32 low, int32 high, int32 value, float divider = 0);
	void				AddLabel(			const BRect& frame, const char* name, const BString16& text);
	ArpMenuControl*		AddMenuControl(		const BRect& frame, const char* name,
											const BString16* label, uint32 message,
											const BMessage& items, float divider = 0);
	status_t			AddControl(			BControl* ctrl);
	status_t			AddView(			BView* view);
	/* If modificationMsg is 0, it won't be set.
	 */
	BTextControl*		AddTextControl(	const BRect& frame, const char* name,
										const BString16* label, uint32 msg, uint32 modificationMsg,
										const BString16& text,	float divider = 0);

	BTextView*			AddTextView(	const BRect& frame, const char* name,
										const BRect& textRect, const BString16& text);

	/* Conveniences for laying out controls.
	 */
	float				ViewFontHeight() const;
	float				CheckBoxW(const BString16* label);
	float				MenuW(const BString16* label, const BMessage& items);
	float				MenuW(const BString16* label, const char* longItem);

	void				ShiftButtonDown(BRect& f);
	void				ShiftCheckBoxDown(BRect& f);
	void				ShiftIntDown(BRect& f);
	void				ShiftMenuDown(BRect& f);
	void				ShiftTextDown(BRect& f);

protected:
	/* Subclasses must answer with B_OK if they handled the
	 * message, B_ERROR otherwise.
	 */
	virtual	status_t	ControlMessage(uint32 what) = 0;

private:
	typedef BView			inherited;
	_ControlPanel*			mPanel;
};

/***************************************************************************
 * Inline
 ***************************************************************************/
inline void GlControlView::ShiftButtonDown(BRect& f)
{
	f.top = (f.bottom + Prefs().GetInt32(ARP_PADY));
	f.bottom = (f.top + Prefs().GetInt32(ARP_BUTTON_Y));
}

inline void GlControlView::ShiftCheckBoxDown(BRect& f)
{
	f.top = (f.bottom + Prefs().GetInt32(ARP_PADY));
	f.bottom = (f.top + Prefs().GetInt32(ARP_CHECKBOX_Y));
}

inline void GlControlView::ShiftIntDown(BRect& f)
{
	f.top = (f.bottom + Prefs().GetInt32(ARP_PADY));
	f.bottom = (f.top + Prefs().GetInt32(ARP_INTCTRL_Y));
}

inline void GlControlView::ShiftMenuDown(BRect& f)
{
	f.top = (f.bottom + Prefs().GetInt32(ARP_PADY));
	f.bottom = (f.top + Prefs().GetInt32(ARP_MENUCTRL_Y));
}

inline void GlControlView::ShiftTextDown(BRect& f)
{
	f.top = (f.bottom + Prefs().GetInt32(ARP_PADY));
	f.bottom = (f.top + Prefs().GetInt32(ARP_TEXTCTRL_Y));
}


#endif
