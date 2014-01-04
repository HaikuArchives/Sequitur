#include <stdio.h>
#include <interface/Window.h>
#include <ArpInterface/ArpPrefs.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlRootNode.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlRootInfoInspector.h>
#define WM_USER 'WMUS'
static const uint32		CREATOR_MSG		= WM_USER + 1;
static const uint32		KEY_MSG			= WM_USER + 2;
static const uint32		CATEGORY_MSG	= WM_USER + 3;
static const uint32		LABEL_MSG		= WM_USER + 4;
//static const uint32		PATH_MSG		= WM_USER + 5;

static int32 gl_key_from_string(const BString16* str);
static BString16 gl_string_from_key(int32 key);

/***************************************************************************
 * GL-ROOT-INSPECTOR
 ***************************************************************************/
GlRootInfoInspector::GlRootInfoInspector(BRect frame, const GlRootNode* root)
		: inherited(frame), mRef(root),
		  mCreator(0), mKey(0), mCategory(0), mLabel(0), mPath(0)
{
	float				w = 200;
	float				fh = ViewFontHeight();
	float				sx = 2, sy = 2;
	float				div = StringWidth(SZ(SZ_Category)->String()) + 10;
	/* Add the node label.
	 */	
	BString16		lab(root->Label());
	lab << " Info";
	BRect			r(sx, sy, w - sx, sy + fh);
	AddLabel(r, "label", lab);

	ShiftTextDown(r);
	mCreator = AddTextControl(r, "cr", SZ(SZ_Creator), CREATOR_MSG, CREATOR_MSG, root->Creator(), div);
	ShiftTextDown(r);
	BString16		keyStr = gl_string_from_key(root->Key());
	mKey = AddTextControl(r, "ky", SZ(SZ_Key), KEY_MSG, KEY_MSG, keyStr.String(), div);
	ShiftTextDown(r);
	mCategory = AddTextControl(r, "ct", SZ(SZ_Category), CATEGORY_MSG, CATEGORY_MSG, root->Category(), div);
	ShiftTextDown(r);
	mLabel = AddTextControl(r, "lb", SZ(SZ_Label), LABEL_MSG, LABEL_MSG, root->Label(), div);
}

status_t GlRootInfoInspector::ControlMessage(uint32 what)
{
	switch (what) {
		case CREATOR_MSG:
			SetString(mCreator);
			return B_OK;
		case CATEGORY_MSG:
			SetString(mCategory);
			return B_OK;
		case LABEL_MSG:
			SetString(mLabel);
			return B_OK;
#if 0
		case PATH_MSG:
			SetAll();
			return B_OK;
#endif
		case KEY_MSG:
			SetKey();
			return B_OK;
	}
	return B_ERROR;
}

void GlRootInfoInspector::SetString(BTextControl* ctrl)
{
	ArpVALIDATE(ctrl, return);
	GlRootNode*			root = mRef.WriteLock();
	if (root) {
		BString16		str((ctrl->Text()) ? ctrl->Text() : "");
		if (ctrl == mCreator) root->SetCreator(str);
		else if (ctrl == mCategory) root->SetCategory(str);
		else if (ctrl == mLabel) root->SetLabel(str);
	}
	mRef.WriteUnlock(root);
}

void GlRootInfoInspector::SetKey()
{
	if (!mKey) return;
	int32				key = gl_key_from_string(new BString16(mKey->Text()));
	BString16			keyStr = gl_string_from_key(key);
	if (keyStr != mKey->Text())
		mKey->SetText(keyStr.String());
	GlRootNode*			root = mRef.WriteLock();
	if (root) root->SetKey(key);
	mRef.WriteUnlock(root);
}

// #pragma mark -

/***************************************************************************
 * Misc functions
 ***************************************************************************/
static int32 gl_key_from_string(const BString16* str)
{
	ArpVALIDATE(str, return 0);
	return str->AsInt32();
#if 0
	if (!str) return 0;
	int32		len = strlen(str);
	int32		byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
	if (len >= 1) byte0 = str[0];
	if (len >= 2) byte1 = str[1];
	if (len >= 3) byte2 = str[2];
	if (len >= 4) byte3 = str[3];
	return (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
#endif
}

static BString16 gl_string_from_key(int32 key)
{
	BString16	str;
	str << key;
	return str;
#if 0
	char		str[5];
	str[0] = (key >> 24);
	str[1] = (key >> 16) & 0xff;
	str[2] = (key >> 8) & 0xff;
	str[3] = (key) & 0xff;
	str[4] = '\0';
	return BString16(str);
#endif
}
