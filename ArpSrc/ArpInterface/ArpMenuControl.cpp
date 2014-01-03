#include <stdio.h>
#include <interface/Menu.h>
#include <interface/MenuItem.h>
#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpMenuControl.h>
#include <ArpCore/String16.h>

const char*			ARP_MENU_I_STR				= "i";
const char*			ARP_MENU_ID_STR				= "id";
const char*			ARP_MENU_ITEM_STR			= "item";
const char*			ARP_MENU_ON_STR				= "on";

class _ArpMenuControlData
{
public:
	vector<int32>		ints;
	_ArpMenuControlData()			{ }
};

/*******************************************************
 * ARP-MENU-CONTROL
 *******************************************************/
ArpMenuControl::ArpMenuControl(	BRect frame,
								const char* name,
								const BString16* label,
								uint32 message,
								const BMessage& items)
		: inherited(frame, name, label, new BMenu("Empty"), true),
		  mData(0), mCachedMsgWhat(message)
{
	BMenu*			m = Menu();
	if (m) {
		m->SetLabelFromMarked(true);
		InstallItems(m, items);
	}
}

ArpMenuControl::~ArpMenuControl()
{
	delete mData;
}

int32 ArpMenuControl::CurrentIndex() const
{
	BMenu*		m = Menu();
	if (!m) return -1;
	BMenuItem*	item = m->FindMarked();
	if (!item) return -1;
	/* Translate from the int data if I can
	 */
	int32		i = m->IndexOf(item);
	if (!mData)	return i;
	if (i >= int32(mData->ints.size())) return -1;
	return mData->ints[i];
}

void ArpMenuControl::SetCurrentIndex(int32 index)
{
	BMenu*		m = Menu();
	if (!m) return;
	if (mData) {
		for (uint32 k = 0; k < mData->ints.size(); k++) {
			if (mData->ints[k] == index) {
				index = k;
				break;
			}
		}
	}
	BMenuItem*	item = m->ItemAt(index);
	if (item) item->SetMarked(true);
}

status_t ArpMenuControl::GetCurrentId(int32* id) const
{
	BMenu*		m = Menu();
	if (!m) return B_ERROR;
	BMenuItem*	item = m->FindMarked();
	if (!item || !item->Message()) return B_ERROR;
	int32		i32;
	status_t	err = item->Message()->FindInt32(ARP_MENU_ID_STR, &i32);
	if (err != B_OK) return err;
	*id = i32;
	return B_OK;
}

status_t ArpMenuControl::SetCurrentId(int32 id)
{
	BMenu*		m = Menu();
	if (!m) return B_ERROR;
	BMenuItem*	item;
	for (int32 k = 0; (item = m->ItemAt(k)) != 0; k++) {
		if (item->Message()) {
			int32	i32;
			if (item->Message()->FindInt32(ARP_MENU_ID_STR, &i32) == B_OK && i32 == id) {
				item->SetMarked(true);
				return B_OK;
			}
		}
	}
	return  B_ERROR;
}

status_t ArpMenuControl::ReplaceItems(const BMessage& items)
{
	BMenu*			m = Menu();
	if (!m) return B_ERROR;
	m->RemoveItems(0, m->CountItems(), true);
	return InstallItems(m, items);
}

status_t ArpMenuControl::InstallItems(BMenu* m, const BMessage& items)
{
	delete mData;
	mData = 0;
	BString16		si;
	uint32			k;
	for (k = 0; items.FindString16(ARP_MENU_ITEM_STR, k, &si) == B_OK; k++) {
		if (si == "sep") m->AddSeparatorItem();
		else {
			BMenuItem*	item = new BMenuItem(si.String(), new BMessage(mCachedMsgWhat));
			if (item) {
				bool	b;
				if (items.FindBool(ARP_MENU_ON_STR, k, &b) == B_OK && !b)
					item->SetEnabled(false);
				int32	i32;
				if (items.FindInt32(ARP_MENU_ID_STR, k, &i32) == B_OK) {
					if (item->Message()) item->Message()->AddInt32(ARP_MENU_ID_STR, i32);
				}
				m->AddItem(item);
			}
		}
	}
	if (mCachedTarget.IsValid()) m->SetTargetForItems(mCachedTarget);

	int32			i32;
	for (uint32 k2 = 0; items.FindInt32(ARP_MENU_I_STR, k2, &i32) == B_OK; k2++) {
		if (!mData) mData = new _ArpMenuControlData();
		if (mData) mData->ints.push_back(i32);
	}

	if (mData && mData->ints.size() != k) {
		delete mData;
		mData = 0;
	}

	return B_OK;
}

void ArpMenuControl::SetCachedTarget(BMessenger target)
{
	mCachedTarget = target;
}
