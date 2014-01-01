#include <ArpInterface/ArpPopUpMenu.h>

/***************************************************************************
 * ARP-POP-UP-MENU
 ***************************************************************************/
ArpPopUpMenu::ArpPopUpMenu(const BString16* title)
		: inherited(title)
{
}

void ArpPopUpMenu::GoAndDeliver(const BPoint& where,
								BView& owner)
{
	BRect		frame(where, where);
	SetAsyncAutoDestruct(true);
	Go(	owner.ConvertToScreen(where), true, false,
		owner.ConvertToScreen(frame), true);
}
