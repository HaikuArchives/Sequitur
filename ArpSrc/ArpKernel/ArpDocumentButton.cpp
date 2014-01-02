/*
	
	ArpDocumentButton.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#include "ArpKernel/ArpDocumentButton.h"

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _SCREEN_H
#include <interface/Screen.h>
#endif

#ifndef _APPLICATION_H
#include <app/Application.h>
#endif

#ifndef _ROSTER_H
#include <app/Roster.h>
#endif

#ifndef _APPFILEINFO_H
#include <storage/AppFileInfo.h>
#endif

#ifndef _FILE_H
#include <storage/File.h>
#endif

#ifndef _TRANSLATION_UTILS_H
#include <translation/TranslationUtils.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <stdio.h>
#include <string.h>

ArpMOD();

ArpDocumentButton::ArpDocumentButton(BRect frame, const char *name, BBitmap* icon,
									 BMessage* pressMsg,
									 uint32 resizeMask, uint32 flags)
	: BControl(frame, name, "", pressMsg, resizeMask, flags),
	  mDocIcon(icon), mSmallIcon(0),
	  mAction(Inactive),
	  mPressed(false), mMenued(false), mDropped(false)
{
	SetHandlerForReply(this);
	
	if( mDocIcon == 0 ) {
		app_info ai;
		BFile file;
		BAppFileInfo afi;
		be_app->GetAppInfo(&ai);
		file.SetTo(&ai.ref, B_READ_ONLY);
		afi.SetTo(&file);
		mDocIcon = new BBitmap(BRect(0,0,15,15), B_CMAP8);
		if( afi.GetIcon(mDocIcon, B_MINI_ICON) != B_OK ) {
			delete mDocIcon;
			mDocIcon = 0;
		}
	}
	
	mSmallIcon = BTranslationUtils::GetBitmap('bmp ', 1);
		
}

ArpDocumentButton::~ArpDocumentButton()
{
	FreeMemory();
}

void ArpDocumentButton::FreeMemory()
{
	delete mDocIcon;
	mDocIcon = 0;
}

void ArpDocumentButton::SetDragPrototype(const BMessage& prototype)
{
	mDragPrototype = prototype;
}

const BMessage& ArpDocumentButton::DragPrototype() const
{
	return mDragPrototype;
}

void ArpDocumentButton::SetDropPrototype(const BMessage& prototype)
{
	mDropPrototype = prototype;
}

const BMessage& ArpDocumentButton::DropPrototype() const
{
	return mDropPrototype;
}

void ArpDocumentButton::AttachedToWindow()
{
	inherited::AttachedToWindow();
	#if 0
	if( ViewColor() != B_TRANSPARENT_COLOR ) SetLowColor(ViewColor());
	else SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
	#endif
	SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
	SetViewColor(B_TRANSPARENT_COLOR);
}

void ArpDocumentButton::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
}

void ArpDocumentButton::GetPreferredSize(float* width, float* height)
{
	if( width ) {
		*width = FrameWidth*2 + BorderWidth*2;
		if( mDocIcon ) *width += mDocIcon->Bounds().Width()+1;
	}
	if( height ) {
		*height = FrameHeight*2 + BorderHeight*2;
		if( mDocIcon ) *height += mDocIcon->Bounds().Height()+1;
	}
}

void ArpDocumentButton::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	ArpD(cdb << ADH << "ArpDocumentButton::MessageReceived: " << *message << endl);
	if( message->WasDropped() ) {
		ArpD(cdb << ADH << "This was a dropped message." << endl);
		mDropped = false;
		Invalidate();
		switch( message->what ) {
			case B_SIMPLE_DATA: {
				entry_ref ref;
				BMessage data;
				if( GetDropRef(&ref, message, mDropped) == B_OK ) {
					// Forward to target as an "open" action.
					ArpD(cdb << ADH << "Sending to target as B_REFS_RECEIVED."
								<< endl);
					BMessage openMsg(*message);
					openMsg.what = B_REFS_RECEIVED;
					Invoke(&openMsg);
				} else if( GetDropAction(&data, message, mDropped) == B_OK ) {
					// Only if we are not the one who sent the message...
					BMessenger source;
					if( message->FindMessenger("ARP:source", &source) == B_OK ) {
						if( source == BMessenger(this) ) return;
					}
					// Tell the sender what format we would like the data in, and
					// have it send the final reply containing that data to our
					// target.
					ArpD(cdb << ADH << "Replying with desired action: " << data
								<< endl);
					message->SendReply(&data, Messenger());
				}
			} break;
			default:
				ArpD(cdb << ADH << "Not a drop I understand." << endl);
				inherited::MessageReceived(message);
		}
		return;
	}
	
	// If this action is one of the allowed ones in our prototype, forward
	// it to the target handler.
	int32 action=0;
	int i;
	for( i=0; mDragPrototype.FindInt32("be:actions", i, &action)==B_OK; i++ ) {
		if( static_cast<type_code>(action) == message->what ) {
			ArpD(cdb << ADH << "Forwarding this action to target." << endl);
			// Forward message on to target, making that target's reply
			// go back to the originator.
			InvokeReply(message);
			//Messenger().SendMessage(message, message->ReturnAddress());
			//Invoke(message);
			return;
		}
	}
	
	// Forward any other data received to the target.  Using the official
	// drag and drop protocol, this should never happen -- we redirect the
	// data message directly to our target, when responding to the initial
	// drop.  However, just in case, we also catch data messages here.
	if( message->what == B_MIME_DATA ) {
		ArpD(cdb << ADH << "Forwarding this data to target." << endl);
		InvokeReply(message);
		return;
	}
	
	ArpD(cdb << ADH << "Not a message I understand." << endl);
	inherited::MessageReceived(message);
}

void ArpDocumentButton::Draw(BRect)
{
	enum {
		ShineColor = 0,
		MedShineColor = 1,
		ShadowColor = 2,
		MedShadowColor = 3,
		NumColors = 4
	};
	
	static float ColorTints[NumColors] = {
		.2, B_DARKEN_2_TINT,
		B_DARKEN_4_TINT, B_DARKEN_1_TINT
	};
	
	static int32 StandardMap[NumColors] = {
		ShineColor, MedShineColor,
		ShadowColor, MedShadowColor
	};
	
	static int32 PressedMap[NumColors] = {
		ShadowColor, MedShadowColor,
		ShineColor, MedShineColor
	};
	
	rgb_color bgcolor = LowColor();
	if( mDropped ) bgcolor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	
	BRect bounds(Bounds());
	
	ArpD(cdb << ADH << "Drawing ArpDocument Button..." << endl);
	ArpD(cdb << ADH << "Initial view bounds=" << bounds << endl);
	
	int32* cmap = mPressed ? PressedMap : StandardMap;
	
	BeginLineArray(8);
	rgb_color curcol;
	
	if( bounds.IsValid() ) {
		ArpD(cdb << ADH << "Drawing interior frame at bounds=" << bounds
						<< endl);
		curcol = tint_color(bgcolor, ColorTints[cmap[MedShineColor]]);
		AddLine( BPoint(bounds.left, bounds.bottom),
				 BPoint(bounds.left, bounds.top),
				 curcol );
		AddLine( BPoint(bounds.left, bounds.top),
				 BPoint(bounds.right, bounds.top),
				 curcol );
		
		curcol = tint_color(bgcolor, ColorTints[cmap[ShadowColor]]);
		AddLine( BPoint(bounds.right, bounds.top),
				 BPoint(bounds.right, bounds.bottom),
				 curcol );
		AddLine( BPoint(bounds.right, bounds.bottom),
				 BPoint(bounds.left, bounds.bottom),
				 curcol );
		
		bounds.InsetBy(1, 1);
	}
	
	if( bounds.IsValid() ) {
		ArpD(cdb << ADH << "Drawing interior frame at bounds=" << bounds
						<< endl);
		curcol = tint_color(bgcolor, ColorTints[cmap[ShineColor]]);
		if( IsFocus() ) curcol = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		AddLine( BPoint(bounds.left, bounds.bottom),
				 BPoint(bounds.left, bounds.top),
				 curcol );
		AddLine( BPoint(bounds.left, bounds.top),
				 BPoint(bounds.right, bounds.top),
				 curcol );
		
		curcol = tint_color(bgcolor, ColorTints[cmap[MedShadowColor]]);
		if( IsFocus() ) curcol = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		AddLine( BPoint(bounds.right, bounds.top),
				 BPoint(bounds.right, bounds.bottom),
				 curcol );
		AddLine( BPoint(bounds.right, bounds.bottom),
				 BPoint(bounds.left, bounds.bottom),
				 curcol );
					
		bounds.InsetBy(1, 1);
	}
	
	EndLineArray();
	
	BRect ibounds;
	BBitmap* shownBitmap = 0;
	
	if( mPressed ) bgcolor = tint_color(LowColor(), B_DARKEN_MAX_TINT);
	else if( mMenued ) bgcolor = tint_color(LowColor(), B_DARKEN_2_TINT);
	
	if( bounds.IsValid() ) {
		
		shownBitmap = mDocIcon;
		if( shownBitmap ) ibounds = shownBitmap->Bounds();
		if( mSmallIcon
			&& ( !ibounds.IsValid()
				|| ibounds.Width() > bounds.Width()
				|| ibounds.Height() > bounds.Height() ) ) {
			shownBitmap = mSmallIcon;
			ibounds = shownBitmap->Bounds();
		}
		
		ArpD(cdb << ADH << "Current view bounds=" << bounds
						<< ", icon bounds=" << ibounds << endl);

		SetHighColor(bgcolor);
		const float xdiff = bounds.Width() - ibounds.Width();
		const float xoff = floor(xdiff/2);
		if( xdiff > 0 ) {
			// Fill in background to left and right of bitmap.
			if( xoff > 0 ) {
				FillRect(BRect(bounds.left, bounds.top,
							   bounds.left+xoff-1, bounds.bottom));
			}
			FillRect(BRect(bounds.right-(xdiff-xoff)+1, bounds.top,
						   bounds.right, bounds.bottom));
			bounds.left += xoff;
			bounds.right -= (xdiff-xoff);
			ArpD(cdb << ADH << "Fill view X to bounds=" << bounds << endl);
		} else {
			#if 1
			ibounds.left -= xoff;
			ibounds.right += (xdiff-xoff);
			#endif
			ArpD(cdb << ADH << "Indent icon X to bounds=" << ibounds << endl);
		}
		
		const float ydiff = bounds.Height() - ibounds.Height();
		const float yoff = floor(ydiff/2);
		if( ydiff > 0 ) {
			// Fill in background to left and right of bitmap.
			if( yoff > 0 ) {
				FillRect(BRect(bounds.left, bounds.top,
							   bounds.right, bounds.top+yoff-1));
			}
			FillRect(BRect(bounds.left, bounds.bottom-(ydiff-yoff)+1,
						   bounds.right, bounds.bottom));
			bounds.top += yoff;
			bounds.bottom -= (ydiff-yoff);
			ArpD(cdb << ADH << "Fill view Y to bounds=" << bounds << endl);
		} else {
			#if 1
			ibounds.top -= yoff;
			ibounds.bottom += (ydiff-yoff);
			#endif
			ArpD(cdb << ADH << "Indent icon Y to bounds=" << ibounds << endl);
		}
	}
	
	if( bounds.IsValid() && ibounds.IsValid() && shownBitmap ) {
		ArpD(cdb << ADH << "Drawing icon at bounds=" << bounds
						<< ", icon bounds=" << ibounds << endl);
		SetHighColor(bgcolor);
		SetLowColor(bgcolor);
		FillRect(bounds);
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		DrawBitmapAsync(shownBitmap, ibounds, bounds);
		SetDrawingMode(B_OP_COPY);
	}
}

void ArpDocumentButton::MouseDown(BPoint point)
{
	ArpD(cdb << ADH << "Mouse down received." << endl);
	
	BMessage* msg = Window()->CurrentMessage();
	if( !msg ) return;
	int32 button=0;
	msg->FindInt32("buttons",&button);
	
	SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS);
	mAction = ButtonPressed;
	mPressed = IsPointIn(point);
	
#if 0		// popup menu not currently implemented
	if( button == B_PRIMARY_MOUSE_BUTTON ) {
		
	} else {
		mAction = MenuPopped;
		mMenued = true;

	}
#endif
	
	Invalidate();
}

void ArpDocumentButton::MouseMoved(BPoint point, uint32 transit,
							 const BMessage* message)
{
	BMessage* curMsg = Window()->CurrentMessage();
	
	if( message ) {
		// Only update button when drag enters or leaves the view.
		if( transit != B_ENTERED_VIEW && transit != B_EXITED_VIEW ) return;
		
		BMessenger source;
		if( message->FindMessenger("ARP:source", &source) == B_OK ) {
			if( source == BMessenger(this) ) return;
		}
		bool newDropped = false;
		if( transit == B_ENTERED_VIEW ) {
			// Is this a message we can handle?
			bool control=false;
			if( curMsg ) {
				int32 modifiers=0;
				curMsg->FindInt32("modifiers",&modifiers);
				if( (modifiers&B_LEFT_CONTROL_KEY) != 0 ) control=true;
				ArpD(cdb << ADH << "Drag entered view, ctrl=" << control
							<< ", mouse move=" << *curMsg << endl);
			}
			if( control || GetDropRef(0,message) == B_OK
						|| GetDropAction(0,message) == B_OK ) {
				newDropped = true;
			}
		}
		if( newDropped != mDropped ) {
			ArpD(if( message ) cdb << "Dragged message: " << *message << endl);
			mDropped = newDropped;
			Invalidate();
		}
	}
	
	if( mAction == ButtonPressed ) {
		bool newPressed = IsPointIn(point);
		if( newPressed != mPressed ) {
			if( !newPressed ) {
				BMessage msg(mDragPrototype);
				int32 buttons=0;
				if( curMsg && curMsg->FindInt32("buttons",&buttons)==B_OK ) {
					msg.AddInt32("buttons", buttons);
				}
				msg.AddMessenger("ARP:source", BMessenger(this));
				BBitmap* shownBitmap = mDocIcon;
				if( !shownBitmap ) shownBitmap = mSmallIcon;
				if( shownBitmap ) {
					BBitmap * bmp = new BBitmap(shownBitmap->Bounds(),
												shownBitmap->ColorSpace(),
												false);
					memcpy(bmp->Bits(), shownBitmap->Bits(), 
						   shownBitmap->BitsLength());
					BPoint pt(shownBitmap->Bounds().Width()/2,
							  shownBitmap->Bounds().Height()/2);
					DragMessage(&msg, bmp, B_OP_BLEND, pt, this);
				} else {
					BRect rect(Bounds());
					rect.OffsetBy(-rect.Width()/2, -rect.Height()/2);
					DragMessage(&msg, rect, this);
				}
				mAction = Inactive;
			}
			mPressed = newPressed;
			Invalidate();
		}
	}
}

status_t ArpDocumentButton::InvokeReply(BMessage* msg)
{
	if( !msg ) return B_BAD_VALUE;
	
	ArpD(cdb << ADH << "Called InvokeReply() on: " << *msg << endl);
	
	// Is the target in the same looper as this handler?  If so, we can't
	// send this message to it and wait for a reply -- instead, we just
	// directly call the target's handler.
	
	BLooper* myLooper = Looper();
	if( myLooper ) {
		BLooper* yourLooper = 0;
		BHandler* yourHandler = Target(&yourLooper);
		if( myLooper == yourLooper ) {
			ArpD(cdb << ADH << "Directly calling target handler." << endl);
			if( yourHandler ) yourHandler->MessageReceived(msg);
			else yourLooper->DispatchMessage(msg, 0);
			return B_OK;
		}
	}
	
	// Otherwise, send the message to our target, and wait for a reply.
	// Then take this reply and use it as the reply to the original message.
	BMessage reply;
	ArpD(cdb << ADH << "Sending message to target." << endl);
	status_t err = Messenger().SendMessage(msg, &reply);
	ArpD(cdb << ADH << "Reply status=" << err << ", msg=" << reply << endl);
	if( !err ) {
		msg->SendReply(&reply, this);
	}
	
	return err;
}

status_t ArpDocumentButton::GetDropRef(entry_ref* out, const BMessage* in,
									   bool always) const
{
	if( !in ) return B_BAD_VALUE;
	
	ArpD(cdb << ADH << "Checking reference in message: " << *in << endl);
	ArpD(cdb << ADH << "Prototype is: " << mDropPrototype << endl);
	
	entry_ref ref;
	status_t ret;
	if( (ret=in->FindRef("refs", &ref)) != B_OK ) {
		ArpD(cdb << ADH << "No 'refs' value found." << endl);
		return ret;
	}
	
	if( always ) {
		ArpD(cdb << ADH << "Always returning reference." << endl);
		if( out ) *out = ref;
		return B_OK;
	}
	
	// Check if this file is a MIME type we understand.
	
	BEntry entry(&ref, true);
	if( (ret=entry.InitCheck()) != B_OK ) {
		ArpD(cdb << ADH << "Unable to create BEntry from reference." << endl);
		return ret;
	}
	BNode node(&entry);
	if( (ret=node.InitCheck()) != B_OK ) {
		ArpD(cdb << ADH << "Unable to create BNode from BEntry." << endl);
		return ret;
	}
	
	char mimetype[B_MIME_TYPE_LENGTH];
	if( (ret=node.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0,
						   mimetype, sizeof(mimetype))) < 0 ) {
		ArpD(cdb << ADH << "Unable to retrieve MIME type from node." << endl);
		return ret;
	}
	
	ArpD(cdb << ADH << "Node's MIME type is: " << &mimetype[0] << endl);
	
	const char* str;
	for( int i=0; mDropPrototype.FindString("be:filetypes", i, &str)==B_OK; i++ ) {
		// TO DO: Be able to match with MIME supertypes in the drop prototype.
		ArpD(cdb << ADH << "Comparing with prototype: " << str << endl);
		if( strcasecmp(str, mimetype) == 0 ) {
			ArpD(cdb << ADH << "Match!" << endl);
			if( out ) *out = ref;
			return B_OK;
		}
	}
	
	ArpD(cdb << ADH << "No matches for node's MIME type!" << endl);
	return B_NOT_ALLOWED;
}

status_t ArpDocumentButton::GetDropAction(BMessage* out, const BMessage* in,
										  bool always) const
{
	if( !in ) return B_BAD_VALUE;
	
	ArpD(cdb << ADH << "Checking known type in message: " << *in << endl);
	ArpD(cdb << ADH << "Prototype is: " << mDropPrototype << endl);
	
	int i, j;
	
	// First look for a type we can handle.
	const char* dropType=0;
	const char* inType=0;
	
	for( i=0; !inType && mDropPrototype.FindString("be:types", i, &dropType)==B_OK; i++ ) {
		for( j=0; in->FindString("be:types", j, &inType)==B_OK; j++ ) {
			ArpD(cdb << ADH << "Comparing type proto #" << i << " (" << dropType
						<< ") with given #" << j << " (" << inType << ")..."
						<< endl);
			// TO DO: Be able to match with MIME supertypes in the drop prototype.
			if( strcasecmp(dropType, inType) == 0 ) {
				ArpD(cdb << ADH << "Match!" << endl);
				break;
			}
			inType = 0;
		}
	}
	
	// No matching type found?
	if( !dropType || !inType ) {
		ArpD(cdb << ADH << "No matching type was found in the drop." << endl);
		return B_NOT_ALLOWED;
	}
	
	// Next look for an action we can handle.
	int32 dropAction=0;
	int32 inAction=0;
	for( i=0; !inAction && mDropPrototype.FindInt32("be:actions", i, &dropAction)==B_OK; i++ ) {
		for( j=0; in->FindInt32("be:actions", j, &inAction)==B_OK; j++ ) {
			ArpD(cdb << ADH << "Comparing action proto #" << i << " (" << dropAction
						<< ") with given #" << j << " (" << inAction << ")..."
						<< endl);
			if( dropAction == inAction ) {
				ArpD(cdb << ADH << "Match!" << endl);
				break;
			}
			inAction = 0;
		}
		// If no actions supplied in drop message, default to B_COPY_TARGET.
		if( !inAction && j == 0 && dropAction == B_COPY_TARGET ) {
			ArpD(cdb << ADH << "No actions in drop; assuming COPY." << endl);
			inAction = B_COPY_TARGET;
			break;
		}
	}
	
	// No matching action found?
	if( dropAction == 0 || inAction == 0 ) {
		ArpD(cdb << ADH << "No matching action was found in the drop." << endl);
		return B_NOT_ALLOWED;
	}
	
	// Place all the information we found into the output message.
	if( out ) {
		out->what = dropAction;
		out->AddString("be:types", dropType);
		out->AddInt32("be:actions", dropAction);
		const char* clipname=0;
		if( in->FindString("be:clip_name", &clipname) == B_OK ) {
			out->AddString("be:clip_name", clipname);
		}
	}
	
	if( strcasecmp(dropType, B_FILE_MIME_TYPE) == 0 ) {
		// Transfer as a file.  Look for a file type we can handle.
		dropType = inType = 0;
		for( i=0; !inType && mDropPrototype.FindString("be:filetypes", i, &dropType)==B_OK; i++ ) {
			for( j=0; in->FindString("be:filetypes", j, &inType)==B_OK; j++ ) {
				// TO DO: Be able to match with MIME supertypes in the drop prototype.
				ArpD(cdb << ADH << "Comparing file proto #" << i << " (" << dropType
							<< ") with given #" << j << " (" << inType << ")..."
							<< endl);
				if( strcasecmp(dropType, inType) == 0 ) {
					ArpD(cdb << ADH << "Match!" << endl);
					break;
				}
				inType = 0;
			}
		}
		
		// No matching type found?
		if( !dropType || !inType ) {
			ArpD(cdb << ADH << "No matching file type was found in the drop." << endl);
			return B_NOT_ALLOWED;
		}
	
		if( out ) out->AddString("be:filetypes", dropType);
	}
	
	ArpD(if( out ) cdb << "Reply message to drop: " << *out << endl; else cdb << "This is a good message drop." << endl);
	
	return B_OK;
}

void ArpDocumentButton::MouseUp(BPoint point)
{
	ArpD(cdb << ADH << "ArpDocumentButton: Mouse Up." << endl);
	if( mAction == ButtonPressed ) {
		if( IsPointIn(point) ) {
			ArpD(cdb << ADH << "Invoking button action." << endl);
			Invoke();
		}
	}
	
	mPressed = false;
	mMenued = false;
	mAction = Inactive;
	Invalidate();
}

bool ArpDocumentButton::IsPointIn(BPoint point)
{
	return Bounds().Contains(point);
}
