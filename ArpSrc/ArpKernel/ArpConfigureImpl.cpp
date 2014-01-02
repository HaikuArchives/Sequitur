/*
	
	ArpConfigureImpl.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef _MENUFIELD_H
#include <interface/MenuField.h>
#endif

#ifndef _POPUPMENU_H
#include <interface/PopUpMenu.h>
#endif

#ifndef _MENUITEM_H
#include <interface/MenuItem.h>
#endif

#ifndef _COLORCONTROL_H
#include <interface/ColorControl.h>
#endif

#ifndef _STRINGVIEW_H
#include <interface/StringView.h>
#endif

#ifndef _TEXTCONTROL_H
#include <interface/TextControl.h>
#endif

#ifndef _CHECKBOX_H
#include <interface/CheckBox.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREIMPL_H
#include <ArpKernel/ArpConfigureImpl.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef ARPCOLLECTIONS_ARPPTRVECTOR_H
#include <ArpCollections/ArpPtrVector.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

ArpMOD();

static const char* S_PARAMETER = "arp:param";
static const char* S_REPORT = "arp:report";
static const char* S_UPDATE = "arp:update";
static const char* S_VIEW_NAME = "arp:view_name";
static const char* S_DATA = "arp:data";

static void print_float(char* buffer, double value)
{
	sprintf(buffer, "%g", value);
	if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
		!strchr(buffer, 'E') ) {
		strncat(buffer, ".0", sizeof(buffer)-1);
	}
}

ArpConfigureImpl::ArpConfigureImpl(BMessenger configTarget,
									BView* configRoot,
									BMessage& settings)
	: mConfigTarget(configTarget), mDirectTarget(NULL),
	  mConfigRoot(configRoot),
	  mSettings(settings),
	  mAttachments(0)
{
	mAttachments = new ArpPtrVector<BMessage*>;
	
	ArpD(cdb << ADH << "Configuring settings: " << mSettings << endl);
}

ArpConfigureImpl::ArpConfigureImpl(ArpDirectConfigurableI* directTarget,
									BView* configRoot,
									BMessage& settings)
	: mConfigTarget(BMessenger()), mDirectTarget(directTarget),
	  mConfigRoot(configRoot),
	  mSettings(settings),
	  mAttachments(0)
{
	mAttachments = new ArpPtrVector<BMessage*>;
	
	ArpD(cdb << ADH << "Configuring settings: " << mSettings << endl);
}

ArpConfigureImpl::~ArpConfigureImpl()
{
	if( mAttachments ) {
		for( size_t i=0; i<mAttachments->size(); i++ ) {
			delete mAttachments->at(i);
		}
	}
	delete mAttachments;
	mAttachments = 0;
}

void ArpConfigureImpl::AttachedToWindow()
{
	if( !mConfigRoot ) return;
	
	UpdateTargets(mConfigRoot, mConfigRoot);
	
	BMessenger w(mConfigRoot);
	if (mDirectTarget) mDirectTarget->AddWatcher(w);
	else {
		BMessage add_watch(ARP_ADD_WATCHER_MSG);
		add_watch.AddMessenger("watch", w);
		mConfigTarget.SendMessage(&add_watch);
	}
}

void ArpConfigureImpl::DetachedFromWindow()
{
	if( !mConfigRoot ) return;
	
	BMessenger w(mConfigRoot);
	if (mDirectTarget) mDirectTarget->RemWatcher(w);
	else {
		BMessage rem_watch(ARP_REM_WATCHER_MSG);
		rem_watch.AddMessenger("watch", w);
		mConfigTarget.SendMessage(&rem_watch);
	}
}

void ArpConfigureImpl::RebuildPanel() const
{
	if( mConfigPanel.IsValid() ) {
		ArpD(cdb << ADH << "Asking configuration panel to rebuild." << endl);
		BMessage rebuild(ARP_REBUILD_PANEL_MSG);
		mConfigPanel.SendMessage(&rebuild);
	} else {
		ArpD(cdb << ADH << "*** Asked for panel rebuild, but bad messenged." << endl);
	}
}

static status_t check_box_report(const ArpConfigureImpl::view_context& context)
{
	ArpD(cdb << ADH << "ArpConfigureImpl: report " << context.param << endl);
	BCheckBox* view = dynamic_cast<BCheckBox*>(context.param_view);
	if( !view ) return B_ERROR;
	
	BMessage settings;
	status_t res;
	if( context.type == B_BOOL_TYPE ) {
		bool state = view->Value() ? true : false;
		res = settings.AddBool(context.param, state);
	} else if( context.type == B_INT32_TYPE ) {
		const int32 mask = (int32)context.data;
		int32 flags;
		if( context.settings->FindInt32(context.param, &flags) != B_OK ) {
			flags = 0;
		}
		flags = (flags&~mask) | (view->Value() ? mask : 0);
		ArpD(cdb << ADH << "Checkbox report mask=" << mask
				<< ", new flags=" << flags << endl);
		res = settings.AddInt32(context.param, flags);
	} else {
		ArpD(cdb << ADH << "Unable to handle parameter type: " << context.type << endl);
		res = B_ERROR;
	}
	
	if( res == B_OK ) res = context.caller->SendConfiguration(&settings);
	
	return res;
}

static status_t check_box_update(const ArpConfigureImpl::view_context& context,
								 const BMessage& update)
{
	ArpD(cdb << ADH << "ArpConfigureImpl: update " << context.param << endl);
	BCheckBox* view = dynamic_cast<BCheckBox*>(context.param_view);
	if( !view ) return B_ERROR;
	
	status_t res;
	if( context.type == B_BOOL_TYPE ) {
		bool state;
		if( (res=update.FindBool(context.param, &state)) == B_OK ) {
			view->SetValue(state ? B_CONTROL_ON : B_CONTROL_OFF);
		}
	} else if( context.type == B_INT32_TYPE ) {
		const int32 mask = (int32)context.data;
		int32 flags;
		if( (res=update.FindInt32(context.param, &flags)) == B_OK ) {
			ArpD(cdb << ADH << "Checkbox update mask=" << mask
					<< ", new flags=" << flags
					<< ", value=" << (int32)((flags&mask) ? B_CONTROL_ON : B_CONTROL_OFF)
					<< endl);
			view->SetValue((flags&mask) ? B_CONTROL_ON : B_CONTROL_OFF);
		}
	} else {
		ArpD(cdb << ADH << "Unable to handle parameter type: " << context.type << endl);
		res = B_ERROR;
	}
	
	return res;
}

BMessage* ArpConfigureImpl::AttachCheckBox(const char* param,
										int32 mask, const char* view_name)
{
	return AttachView(param, &check_box_report, &check_box_update,
					  view_name, (void*)mask);
}

static status_t text_control_report(const ArpConfigureImpl::view_context& context)
{
	ArpD(cdb << ADH << "ArpConfigureImpl: report " << context.param << endl);
	BTextControl* view = dynamic_cast<BTextControl*>(context.param_view);
	if( !view ) return B_ERROR;
	
	BMessage settings;
	status_t res = B_OK;
	if( context.type == B_STRING_TYPE ) {
		const char* str = view->Text();
		if( !str ) str = "";
		res = settings.AddString(context.param, str);
	} else if( context.type == B_INT32_TYPE ) {
		ArpString str(view->Text());
		bool valid = false;
		int32 val = str.AsInt(10,&valid);
		if( !valid ) res = B_ERROR;
		if( res == B_OK ) {
			res = settings.AddInt32(context.param, val);
		}
	} else if( context.type == B_FLOAT_TYPE ) {
		ArpString str(view->Text());
		bool valid = false;
		double val = str.AsDouble(&valid);
		if( !valid ) res = B_ERROR;
		if( res == B_OK ) {
			res = settings.AddFloat(context.param, (float)val);
		}
	} else if( context.type == B_DOUBLE_TYPE ) {
		ArpString str(view->Text());
		bool valid = false;
		double val = str.AsDouble(&valid);
		if( !valid ) res = B_ERROR;
		if( res == B_OK ) {
			res = settings.AddDouble(context.param, val);
		}
	} else {
		ArpD(cdb << ADH << "Unable to handle parameter type: " << context.type << endl);
		res = B_ERROR;
	}
	
	if( res == B_OK ) res = context.caller->SendConfiguration(&settings);
	
	return res;
}

static status_t text_control_update(const ArpConfigureImpl::view_context& context,
							   const BMessage& update)
{
	ArpD(cdb << ADH << "ArpConfigureImpl: update " << context.param << endl);
	BTextControl* view = dynamic_cast<BTextControl*>(context.param_view);
	if( !view ) return B_ERROR;
	
	status_t res;
	if( context.type == B_STRING_TYPE ) {
		const char* str;
		if( (res=update.FindString(context.param, &str)) == B_OK ) {
			if( str ) view->SetText(str);
			else view->SetText("");
		}
	} else if( context.type == B_INT32_TYPE ) {
		int32 val;
		if( (res=update.FindInt32(context.param, &val)) == B_OK ) {
			ArpString str(val);
			view->SetText(str);
		}
	} else if( context.type == B_FLOAT_TYPE ) {
		float val;
		if( (res=update.FindFloat(context.param, &val)) == B_OK ) {
			char buf[32];
			print_float(buf, val);
			view->SetText(buf);
		}
	} else if( context.type == B_DOUBLE_TYPE ) {
		double val;
		if( (res=update.FindDouble(context.param, &val)) == B_OK ) {
			char buf[32];
			print_float(buf, val);
			view->SetText(buf);
		}
	} else {
		ArpD(cdb << ADH << "Unable to handle parameter type: " << context.type << endl);
		res = B_ERROR;
	}
	
	return res;
}

BMessage* ArpConfigureImpl::AttachTextControl(const char* param)
{
	return AttachView(param, &text_control_report, &text_control_update);
}

static status_t control_report(const ArpConfigureImpl::view_context& context)
{
	ArpD(cdb << ADH << "ArpConfigureImpl: report " << context.param << endl);
	BControl* view = dynamic_cast<BControl*>(context.param_view);
	if( !view ) return B_ERROR;
	
	BMessage settings;
	status_t res;
	if( context.type == B_BOOL_TYPE ) {
		bool state = view->Value() ? true : false;
		res = settings.AddBool(context.param, state);
	} else if( context.type == B_INT32_TYPE ) {
		res = settings.AddInt32(context.param, view->Value());
	} else if( context.type == B_FLOAT_TYPE ) {
		res = settings.AddFloat(context.param, view->Value());
	} else {
		ArpD(cdb << ADH << "Unable to handle parameter type: " << context.type << endl);
		res = B_ERROR;
	}
	
	if( res == B_OK ) res = context.caller->SendConfiguration(&settings);
	
	return res;
}

static status_t control_update(const ArpConfigureImpl::view_context& context,
								 const BMessage& update)
{
	ArpD(cdb << ADH << "ArpConfigureImpl: update " << context.param << endl);
	BControl* view = dynamic_cast<BControl*>(context.param_view);
	if( !view ) return B_ERROR;
	
	status_t res;
	if( context.type == B_BOOL_TYPE ) {
		bool state;
		if( (res=update.FindBool(context.param, &state)) == B_OK ) {
			view->SetValue(state ? B_CONTROL_ON : B_CONTROL_OFF);
		}
	} else if( context.type == B_INT32_TYPE ) {
		int32 value;
		if( (res=update.FindInt32(context.param, &value)) == B_OK ) {
			view->SetValue(value);
		}
	} else if( context.type == B_FLOAT_TYPE ) {
		float value;
		if( (res=update.FindFloat(context.param, &value)) == B_OK ) {
			view->SetValue((int32)(value+.5));
		}
	} else {
		ArpD(cdb << ADH << "Unable to handle parameter type: " << context.type << endl);
		res = B_ERROR;
	}
	
	return res;
}

BMessage* ArpConfigureImpl::AttachControl(const char* param)
{
	return AttachView(param, &control_report, &control_update);
}

BMessage* ArpConfigureImpl::AttachView(const char* param,
									   report_func report,
									   update_func update,
									   const char* view_name,
									   void* data,
									   BMessage* base)
{
	BMessage* msg = base;
	if( msg ) msg->what = CONFIG_REPORT_MSG;
	else msg = new BMessage(CONFIG_REPORT_MSG);
	
	if( msg ) {
		status_t res;
		res = msg->AddString(S_PARAMETER, param);
		if( res == B_OK ) {
			res = msg->AddPointer(S_REPORT, (void*)report);
		}
		if( res == B_OK ) {
			res = msg->AddPointer(S_UPDATE, (void*)update);
		}
		if( res == B_OK && view_name ) {
			res = msg->AddString(S_VIEW_NAME, view_name);
		}
		if( res == B_OK && data ) {
			res = msg->AddPointer(S_DATA, data);
		}
		if( res != B_OK ) {
			ArpD(cdb << ADH << "Error creating TextControl msg: "
					<< res << endl);
			delete msg;
			return 0;
		}
	}
	
	BMessage* attach;
	
	try {
		attach = new BMessage(*msg);
	} catch(...) {
		delete msg;
		throw;
	}
	
	try {
		mAttachments->push_back(attach);
	} catch(...) {
		delete attach;
		delete msg;
		throw;
	}
	
	return msg;
}

status_t ArpConfigureImpl::MessageReceived(const BMessage* message)
{
	if( !message ) return B_ERROR;
	
	if( message->what == CONFIG_REPORT_MSG ) {
		ArpD(cdb << ADH << "Reporting configuration change: " << *message
						<< endl);
		view_context context;
		report_func report;
		status_t res = GetViewContext(*message, &context, &report, NULL);
		if( res != B_OK ) return res;
		return (*report)(context);
	
	} else if( message->what == ARP_PUT_CONFIGURATION_MSG ) {
		ArpD(cdb << ADH << "*** Configuration changed." << endl);
		BMessage settings;
		status_t res = message->FindMessage("settings", &settings);
		if( res != B_OK ) return res;
		ArpUpdateMessage(mSettings, settings);
		ArpD(cdb << ADH << "New settings: " << mSettings << endl);
		return RefreshControls(settings);
	
	} else if( message->what == ARP_SET_CONFIG_PANEL_MSG ) {
		ArpD(cdb << ADH << "Setting config panel target: "
				<< *message << endl);
		message->FindMessenger("panel", &mConfigPanel);
		return B_OK;
	}
	
	return B_ERROR;
}

status_t ArpConfigureImpl::RefreshControls(const BMessage& settings)
{
	if( !mAttachments ) return B_ERROR;
	
	status_t res = B_OK;
	for( size_t i=0; i<mAttachments->size(); i++ ) {
		if( (*mAttachments)[i] != 0 ) {
			status_t ret = RefreshControl(settings, *((*mAttachments)[i]));
			if( res == B_OK ) res = ret;
		}
	}
	return res;
}

status_t ArpConfigureImpl::RefreshControl(const BMessage& settings,
										const BMessage& message)
{
	view_context context;
	update_func update;
	status_t res = GetViewContext(message, &context, NULL, &update);
	if( res != B_OK ) return res;
	return (*update)(context, settings);
}

void ArpConfigureImpl::UpdateTargets(BView* parent, BHandler* target) const
{
	BView* child = parent->ChildAt(0);
	while( child ) {
		BView* cur = child;
		child = child->NextSibling();
		
		BPopUpMenu* menu = dynamic_cast<BPopUpMenu*>(cur);
		if( menu ) {
			menu->SetTargetForItems(target);
			continue;
		}
		
		BInvoker* invoke = dynamic_cast<BInvoker*>(cur);
		if( invoke ) {
			invoke->SetTarget(target);
			continue;
		}
		
		UpdateTargets(cur, target);
	}
}

status_t ArpConfigureImpl::GetViewContext(const BMessage& message,
										  view_context* context,
										  report_func* report,
										  update_func* update) const
{
	context->message = &message;
	context->caller = const_cast<ArpConfigureImpl*>(this);
	context->root = mConfigRoot;
	status_t res = message.FindString(S_PARAMETER, &context->param);
	if( res != B_OK ) return res;
	int32 count;
	if( mSettings.GetInfo(context->param, &context->type, &count) != B_OK ) {
		ArpD(cdb << ADH << "Parameter not in settings!" << endl);
		context->type = 0;
	}
	const char* view_name;
	if( message.FindString(S_VIEW_NAME, &view_name) != B_OK ) {
		view_name = context->param;
	}
	context->param_view = mConfigRoot
						? mConfigRoot->FindView(view_name)
						: NULL;
	context->settings = &mSettings;
	if( message.FindPointer(S_DATA, &context->data) != B_OK ) {
		context->data = NULL;
	}
	if( report ) {
		res = message.FindPointer(S_REPORT, (void**)report);
		if( res != B_OK ) return res;
	}
	if( update ) {
		res = message.FindPointer(S_UPDATE, (void**)update);
		if( res != B_OK ) return res;
	}
	return res;
}

status_t ArpConfigureImpl::SendConfiguration(const BMessage* config,
											 bool updateLocal) const
{
	status_t res = B_OK;
	if( mDirectTarget ) res = mDirectTarget->PutConfiguration(config);
	else {
		BMessage update(ARP_PUT_CONFIGURATION_MSG);
		res = update.AddMessage("settings", config);
		if( res == B_OK ) {
			ArpD(cdb << ADH << "Sending update message: " << update << endl);
			res = mConfigTarget.SendMessage(&update,
											(BHandler*)NULL, 0);
		}
	}
	
	if( res == B_OK && updateLocal ) {
		ArpUpdateMessage(mSettings, *config);
	}
	
	return res;
}
