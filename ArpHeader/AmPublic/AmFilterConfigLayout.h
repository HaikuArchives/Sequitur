
#ifndef AMPUBLIC_AMFILTERCONFIGLAYOUT_H
#define AMPUBLIC_AMFILTERCONFIGLAYOUT_H

#ifndef ARPKERNEL_ARPCONFIGUREIMPL_H
#include <ArpKernel/ArpConfigureImpl.h>
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#include <ArpLayout/ArpRootLayout.h>
#endif

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include <ArpLayout/ViewStubs.h>
#endif

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include <ArpLayout/ArpRunningBar.h>
#endif

#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 *
 *	AM-FILTER-CONFIG-LAYOUT CLASS
 *
 *	Same as an AmFilterConfig, but more convenient when using ArpLayout
 *	to create your UI.
 *
 *****************************************************************************/

class AmFilterConfigLayout : public ArpRootLayout
{
public:
	AmFilterConfigLayout(AmFilterHolderI* target,
					const BMessage& initSettings,	// copied
					const BFont* font = be_plain_font,
					uint32 resizingMode = B_FOLLOW_ALL, 
					uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS)
		: ArpRootLayout(BRect(0,0,100,100),
						target->Filter()->AddOn()->Name().String(),
						resizingMode, flags),
		  mTarget(target), mSettings(initSettings),
		  mImpl(target, this, mSettings)
	{
		mTarget->IncRefs();
	}

	virtual ~AmFilterConfigLayout()
	{
		mTarget->DecRefs();
	}
	
	virtual void AttachedToWindow()
	{
		inherited::AttachedToWindow();
		
		BView* par = Parent();
		ArpColor col = ui_color(B_PANEL_BACKGROUND_COLOR);
		if( par ) col = par->ViewColor();
		ArpMessage updColor;
		updColor.AddRGBColor("StdBackColor", col);
		UpdateGlobals(&updColor);
		
		mImpl.AttachedToWindow();
	}

	virtual void DetachedFromWindow()
	{
		mImpl.DetachedFromWindow();
		inherited::DetachedFromWindow();
	}

	virtual void MessageReceived(BMessage *message)
	{
		if( mImpl.MessageReceived(message) == B_OK ) return;
		inherited::MessageReceived(message);
	}


	AmFilterHolderI* Target()
	{
		return mTarget;
	}
	
	ArpConfigureImpl& Implementation()
	{
		return mImpl;
	}

	ArpMessage& Settings()
	{
		return mSettings;
	}
	
protected:
	typedef ArpRootLayout inherited;
	
	AmFilterHolderI* mTarget;
	ArpMessage mSettings;
	
	ArpConfigureImpl mImpl;
};

#endif
