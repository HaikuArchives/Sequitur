/* ArpMenuControl.h
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
 * 2002.07.26			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPINTERFACE_ARPMENUCONTROL_H
#define ARPINTERFACE_ARPMENUCONTROL_H

#include <be/app/Message.h>
#include <be/app/Messenger.h>
#include <be/interface/MenuField.h>
#include <ArpCore/String16.h>

class _ArpMenuControlData;

extern const char*		ARP_MENU_I_STR;
extern const char*		ARP_MENU_ID_STR;
extern const char*		ARP_MENU_ITEM_STR;
extern const char*		ARP_MENU_ON_STR;

/*******************************************************
 * ARP-MENU-CONTROL
 * Display a list from the items supplied in the message.
 * The message should have a series of strings, all with
 * the name "item".  The message can optionally contain
 * a series of int32s named "i".  There must be the same
 * number as strings.  If these are included, then the
 * CurrentIndex() and SetCurrentIndex() takes ints that
 * correspond to the supplied items for their values.
 *******************************************************/
class ArpMenuControl : public BMenuField
{
public:
	ArpMenuControl(	BRect frame, const char* name,
					const BString16* label, uint32 message,
					const BMessage& items);
	virtual ~ArpMenuControl();
	
	/* Answer the index of the current menu item.  Answer
	 * is < 0 if no item is selected.
	 */
	int32			CurrentIndex() const;
	void			SetCurrentIndex(int32 index);
	/* The id is an optional, alternative way of identifying
	 * the menu items.  If the items arg in the constructor
	 * contains an int32 labeled "id" for each string labeled
	 " "item", then the ids are active and you can manipulate
	 * them with these methods.
	 */
	status_t			GetCurrentId(int32* id) const;
	status_t			SetCurrentId(int32 id);

	status_t			ReplaceItems(const BMessage& items);

	void				SetCachedTarget(BMessenger target);

private:
	typedef BMenuField	inherited;
	_ArpMenuControlData*	mData;
	uint32				mCachedMsgWhat;
	BMessenger			mCachedTarget;

	status_t			InstallItems(BMenu* m, const BMessage& items);
};

#endif
