/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpConfigurePanel.h
 *
 * Manage a BTabView of a set of ArpConfigurableI interfaces.
 *
 * ArpConfigurePanel is the base BTabView of configurable views.
 *
 * ArpConfigureView includes an ArpConfigurePanel as well as
 * buttons for performing operations on it -- this is the one
 * you will probably want to use.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 10/5/1998:
 *	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPCONFIGUREPANEL_H
#define ARPKERNEL_ARPCONGIGUREPANEL_H

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREFILE_H
#include <ArpKernel/ArpConfigureFile.h>
#endif

#ifndef _TABVIEW_H
#include <interface/TabView.h>
#endif

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _INVOKER_H
#include <app/Invoker.h>
#endif

#ifndef _PATH_H
#include <storage/Path.h>
#endif

// forward references
class BButton;

class ArpConfigurePanel : public BTabView
{
public:
	ArpConfigurePanel(BRect frame, const char* name,
					  const ArpConfigureFile& config,
					  button_width width=B_WIDTH_AS_USUAL,
					  uint32 resizingMode = B_FOLLOW_ALL,
					  uint32 flags = 
								B_WILL_DRAW | B_NAVIGABLE_JUMP |
								B_FRAME_EVENTS | B_NAVIGABLE);
	virtual ~ArpConfigurePanel();

	virtual	void GetPreferredSize(float *width, float *height);

	virtual	void AttachedToWindow();
	virtual	void AllAttached();
	virtual void MessageReceived(BMessage *message);
	
	virtual void Select(int32 tab);
	
	const ArpConfigureFile& Config() const	{ return mConfig; }
	
	// Revert all settings to initial values.
	void Revert();
	
	// Regenerate all configuration views.
	void RebuildPanes();
	
private:
	typedef BTabView inherited;
	
	void FreeMemory(void);
	void ClearPanes();
	void SendSetPanel(int32 tab);
	
	ArpConfigureFile mConfig;
	ArpVectorI<BView*>* mConfigViews;
	ArpVectorI<BMessage*>* mInitSettings;
	float mMaxWidth, mMaxHeight;
	float mTabWidth, mTabHeight;
};

class ArpConfigureView : public BView, public BInvoker
{
public:
	ArpConfigureView(BRect frame, const char* name,
					 const ArpConfigureFile& config,
					 BMessage* message,
					 uint32 resizingMode = B_FOLLOW_ALL,
					 uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
					 				| B_FULL_UPDATE_ON_RESIZE );
	virtual ~ArpConfigureView();

	virtual	void GetPreferredSize(float *width, float *height);

	virtual	void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	virtual void Draw(BRect rect);
	
	const ArpConfigureFile& Config() const	{ return mPanel->Config(); }
	
private:
	typedef BView inherited;
	
	ArpConfigurePanel* mPanel;
	BButton* mRevertButton;
	BButton* mMakeDefaultButton;
	BButton* mDoneButton;
	
	float mSpaceWidth, mSpaceHeight;
	float mPanelFrameWidth, mPanelFrameHeight;
	float mButtonsWidth, mButtonsHeight;
};

#endif
