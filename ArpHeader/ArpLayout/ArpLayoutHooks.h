/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * ----------------------------------------------------------------------
 *
 * ArpLayoutHooks.h
 *
 * The head file contains macros for common function calls needed
 * to "connect up" an existing BHandler or BView based object to
 * the ArpBaseLayout class.  Yes, it's hideous and wretched and
 * ugly (and looks -way- too much like Windows)...  but I can't think
 * of a better way to do it.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * One could argue it's all one big bug...
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * July 31, 1999:
 *	• Created from ArpViewWrapper.h
 *
 */

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_ARPLAYOUTHOOKS_H
#define ARPLAYOUT_ARPLAYOUTHOOKS_H

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef _HANDLER_H
#include <app/Handler.h>
#endif

// -------------------- ARPLAYOUT_HANDLERHOOKS --------------------

/**	These are the things you almost always have to do when mixing in
	with a BHandler or any derived classs.  Your mix-in class should
	look something like this:

	@formatted
		class myClass : public BHandler, public ArpBaseLayout
		{
		public:
			myClass() { ... }

			ARPLAYOUT_HANDLERHOOKS(BHandler)

			...
		}
	@unformatted
	
	@deffunc ARPLAYOUT_HANDLERHOOKS ARPLAYOUT_HANDLERHOOKS(ParentClass)
 **/

#define ARPLAYOUT_HANDLERHOOKS(ParentClass)								\
	virtual void MessageReceived(BMessage *message)						\
	{																	\
		if( LayoutMessageReceived(message) == B_OK ) return;			\
		ParentClass::MessageReceived(message);							\
	}																	\
	virtual BHandler*													\
	ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,	\
					 int32 form, const char *property)					\
	{																	\
		BHandler* ret = 0;												\
		if( LayoutResolveSpecifier(&ret, msg, index, specifier,			\
								   form, property) == B_OK ) {			\
			return ret;													\
		}																\
		return ParentClass::ResolveSpecifier(msg, index, specifier,		\
											 form, property);			\
	}																	\
	virtual BHandler* LayoutHandler()									\
	{																	\
		return this;													\
	}																	\
	virtual const BHandler* LayoutHandler() const						\
	{																	\
		return this;													\
	}																	\

// -------------------- ARPLAYOUT_SUITEHOOKS --------------------

/**	If your class doesn't implement its own message suite, then
	you can include this macro to use the standard implementation
	of mixing in with ArpLayoutable.  Otherwise, you will need
	to provide your own implementation to GetSupportedSuites()
	that does your own suite as well as the mix-in code here.

	@see ARPLAYOUT_HANDLERHOOKS
	@deffunc ARPLAYOUT_SUITEHOOKS ARPLAYOUT_SUITEHOOKS(ParentClass)
 **/
 
#define ARPLAYOUT_SUITEHOOKS(ParentClass)								\
	virtual status_t GetSupportedSuites(BMessage *data)					\
	{																	\
		LayoutGetSupportedSuites(data);									\
		return ParentClass::GetSupportedSuites(data);					\
	}																	\

// -------------------- ARPLAYOUT_ARCHIVEHOOKS --------------------

/**	If your class doesn't have anything special it needs to do
	for archiving, you can include this macro to use the standard
	implementation of mixing in with ArpLayoutable.  Otherwise, you
	will need to provide your own implementation of Archive()
	that does your own data as well as the mix-in code here.

	@see ARPLAYOUT_HANDLERHOOKS
	@deffunc ARPLAYOUT_ARCHIVEHOOKS ARPLAYOUT_ARCHIVEHOOKS(ThisClass, ParentClass, ViewDeep)
 **/

#define ARPLAYOUT_ARCHIVEHOOKS(ThisClass, ParentClass, ViewDeep)		\
	ThisClass(BMessage* data, bool final=true)							\
		: ParentClass(data), ArpBaseLayout(data, false)					\
	{																	\
		initialize();													\
		if( final ) InstantiateParams(data);							\
	}																	\
	virtual status_t Archive(BMessage* data, bool deep=true) const		\
	{																	\
		status_t status;												\
		status = ParentClass::Archive(data, ViewDeep ? deep:false);		\
		if( status == B_NO_ERROR ) {									\
			status = ArpBaseLayout::Archive(data, deep);				\
		}																\
		return status;													\
	}																	\

// -------------------- ARPLAYOUT_VIEWHOOKS --------------------

/**	These are the things you almost always have to do when mixing in
	with a BView or any derived classs.  Your mix-in class should
	look something like this:
	
	@formatted
		class myClass : public BView, public ArpBaseLayout
		{
		public:
			myClass() { ... }

			ARPLAYOUT_HANDLERHOOKS(BView)
			ARPLAYOUT_VIEWHOOKS(BView)

			...
		}
	@unformatted
	@see ARPLAYOUT_HANDLERHOOKS
	@deffunc ARPLAYOUT_VIEWHOOKS ARPLAYOUT_VIEWHOOKS(ParentClass)
 **/
 
#define ARPLAYOUT_VIEWHOOKS(ParentClass)								\
	virtual void Draw(BRect updateRect)									\
	{																	\
		DrawLayout(this, updateRect);									\
		ParentClass::Draw(updateRect);									\
	}																	\
	virtual void MakeFocus(bool focusState=true)						\
	{																	\
		ParentClass::MakeFocus(focusState);								\
		SetFocusShown(focusState);										\
	}																	\
	virtual void AttachedToWindow()										\
	{																	\
		ParentClass::AttachedToWindow();								\
		AttachLayoutWindow(Window());									\
	}																	\
	virtual void DetachedFromWindow()									\
	{																	\
		ParentClass::DetachedFromWindow();								\
		AttachLayoutWindow(0);											\
	}																	\
	virtual BView*	OwnerView()											\
	{																	\
		return this;													\
	}																	\

#endif
