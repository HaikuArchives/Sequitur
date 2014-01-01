
#ifndef AMPUBLIC_AMFILTERCONFIG_H
#define AMPUBLIC_AMFILTERCONFIG_H

#ifndef ARPKERNEL_ARPCONFIGUREIMPL_H
#include <ArpKernel/ArpConfigureImpl.h>
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#include "AmPublic/AmFilterI.h"

/*****************************************************************************
 *
 *	AM-FILTER-CONFIG CLASS
 *
 *	This is a convenience class for implementing the root view of a
 *	filter's configuration panel.
 *
 *****************************************************************************/

class AmFilterConfig : public BView
{
public:
	AmFilterConfig(AmFilterHolderI* target,
					const BMessage& initSettings,	// copied
					const BFont* font = be_plain_font,
					uint32 resizingMode = B_FOLLOW_ALL, 
					uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS)
		: BView(BRect(0,0,100,100),
				target->Filter()->AddOn()->Name().String(),
				resizingMode, flags),
		  mTarget(target), mSettings(initSettings),
		  mImpl(target, this, mSettings)
	{
		mTarget->IncRefs();
	}
	
	virtual ~AmFilterConfig()
	{
		mTarget->DecRefs();
	}
	
	virtual void AttachedToWindow()
	{
		inherited::AttachedToWindow();
		
		BView* par = Parent();
		ArpColor col = ui_color(B_PANEL_BACKGROUND_COLOR);
		if( par ) col = par->ViewColor();
		SetViewColor(col);
		
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

	
	ArpConfigureImpl& Implementation()
	{
		return mImpl;
	}
	
protected:
	typedef BView inherited;
	
	AmFilterHolderI* mTarget;
	ArpMessage mSettings;
	
	ArpConfigureImpl mImpl;
};

#endif
