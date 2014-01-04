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
 * ArpConfigureImpl.h
 *
 * This is a helper class for implementing configuration views.
 * It contains generic functions for implementing your view's
 * AttachedToWindow() function, message handler, applying and updating
 * the view's controls, etc.
 *
 * Note that you should NOT inherit from this class, but just include
 * an instance as a member of your own class.
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
 * 05/01/1999:
 *	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPCONFIGUREIMPL_H
#define ARPKERNEL_ARPCONFIGUREIMPL_H

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _MESSENGER_H
#include <app/Messenger.h>
#endif

#ifndef _INVOKER_H
#include <app/Invoker.h>
#endif

// Forward references
class BCheckBox;
class BTextControl;

class ArpConfigureImpl
{
public:
	ArpConfigureImpl(BMessenger configTarget,
					 BView* configRoot,
					 BMessage& settings);
	ArpConfigureImpl(ArpDirectConfigurableI* configTarget,
					 BView* configRoot,
					 BMessage& settings);
	virtual ~ArpConfigureImpl();

	// ------ Attaching/Detaching Controls And Window ------
	
	// Call these in your own AttachedToWindow()/DetachedFromWindow()
	// functions.  Note that your configuration view -must- implement
	// the related BView functions, so that these can be called by you.
	void AttachedToWindow();
	void DetachedFromWindow();
	
	// Send a message to our owning ArpConfigurePanel, asking
	// it to rebuild the list of views.
	void RebuildPanel() const;
	
	// ------ High-Level Updating And Reporting Controls ------
	
	enum { CONFIG_REPORT_MSG = 'aCRP' };
	
	// Call these functions to get the BMessage object that you
	// will pass to the control's constructor.  For example:
	//   new BCheckBox(myRect, "MyParameterName",
	//                 "Select This Paramater",
	//                 mImpl.AttachCheckBox("MyParameterName"));
	
	// Create the BMessage report for an int32 or bool parameter
	// that is controlled by a check box.  If this is an int32
	// parameter, you must supply the bitmask 'mask' to indicate
	// which bit(s) are set/cleared by the checkbox.  Otherwise,
	// the checkbox will be directly tied to a bool parameter.
	BMessage* AttachCheckBox(const char* param,
							 int32 mask = 0, const char* view_name = NULL);
	
	// Create the BMessage report for a string, int32, or float
	// parameter that is controlled by a text control.
	BMessage* AttachTextControl(const char* param);
	
	// Create the BMessage report for an int32 or float parameter
	// that is controlled by a generic BControl subclass.
	BMessage* AttachControl(const char* param);
	
	struct view_context {
		const BMessage* message;		// Attached message
		const char* param;				// Name of parameter for this context
		type_code type;					// Type of parameter in settings
		BView* param_view;				// View with same name as parameter
		ArpConfigureImpl* caller;		// Who is calling you
		BView* root;					// The top configuration view
		const BMessage* settings;		// Current base-line settings
		void* data;						// User data
	};
	
	typedef status_t (*report_func)(const view_context& context);
	typedef status_t (*update_func)(const view_context& context,
									const BMessage& update);
	
	// Generic attachment function, for any kind of view.  You must
	// implement the report_value and update_value functions to
	// interact with the configurable object.
	BMessage* AttachView(const char* param,
						 report_func report, update_func update,
						 const char* view_name = NULL, void* data = 0,
						 BMessage* base=NULL);
	
	// Call this in your own MessageReceived() function.  It returns
	// B_OK if the given message was handled by ArpConfigureImpl, in
	// which case you should return.  If any other value is returned,
	// you should handle the message yourself.
	status_t MessageReceived(const BMessage* message);
	
	// ------ Low-Level Updating And Reporting Controls ------
	
	status_t RefreshControls(const BMessage& settings);
	status_t RefreshControl(const BMessage& settings, const BMessage& info);
	
	status_t SendConfiguration(const BMessage* config,
							   bool updateLocal=true) const;
	
private:
	void UpdateTargets(BView* parent, BHandler* target) const;
	status_t GetViewContext(const BMessage& message, view_context* context,
							report_func* report, update_func* update) const;
	
	BMessenger mConfigTarget;
	ArpDirectConfigurableI* mDirectTarget;
	BMessenger mConfigPanel;
	BView* mConfigRoot;
	BMessage& mSettings;
	
	ArpVectorI<BMessage*>* mAttachments;
};

#endif
