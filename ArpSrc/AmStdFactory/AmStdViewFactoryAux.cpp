/* AmStdViewFactoryAux.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const char*		DEFAULT_STR					= "Default";
static const uint32		CHANGE_FAC_VIEW_HEIGHT_MSG	= 'iCVH';

/*************************************************************************
 * AM-GRID-BACKGROUND
 *************************************************************************/
AmPropGridBackground::AmPropGridBackground(AmTrackWinPropertiesI& trackWinProps)
		: mProps(trackWinProps)
{
}

AmTime AmPropGridBackground::GridTime() const
{
	return mProps.GridTime();
}

const AmTimeConverter& AmPropGridBackground::TimeConverter() const
{
	return mProps.TimeConverter();
}

/*************************************************************************
 * AM-STD-FACTORY-PREF-VIEW
 *************************************************************************/
AmStdFactoryPrefView::AmStdFactoryPrefView(	BRect f, BMessage* prefs,
											const BString& facSig,
											const BString& facKey)
		: inherited(f, "pref", B_FOLLOW_ALL, 0), mFacSig(facSig),
		  mFacKey(facKey), mPrefs(prefs), mHeightCtrl(0)
{
}

void AmStdFactoryPrefView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(Prefs().Color(AM_AUX_WINDOW_BG_C));
	if (mHeightCtrl) mHeightCtrl->SetTarget(this);
}

void AmStdFactoryPrefView::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case CHANGE_FAC_VIEW_HEIGHT_MSG:
			if (mHeightCtrl) {
				mPrefs.SetInt32Preference(	mFacSig.String(), mFacKey.String(),
											AM_HEIGHT_PREF_STR, mHeightCtrl->Value());
			}
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmStdFactoryPrefView::AddViews()
{
	AddHeightView(BPoint(0, 0));
}

float AmStdFactoryPrefView::AddY() const
{
	if (!mHeightCtrl) return 0;
	return mHeightCtrl->Frame().bottom + Prefs().Size(SPACE_Y);
}

static float view_font_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight(&fh);
	return fh.ascent + fh.descent + fh.leading;
}

class _StdFacHeightFormat : public ArpIntFormatterI
{
public:
	_StdFacHeightFormat()	{ ; }
	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number < AM_MIN_FAC_VIEW_HEIGHT) out << DEFAULT_STR;
		else out << number;
	}
};

void AmStdFactoryPrefView::AddHeightView(BPoint where)
{
	const char*			label = "Height:";
//	float				sx = Prefs().Size(SPACE_X), sy = Prefs().Size(SPACE_Y);
	float				fH = view_font_height(this),
						iH = Prefs().Size(INT_CTRL_Y);
	float				max = (fH > iH) ? fH : iH;

	float				lW = StringWidth(label) + 9,
						iW = StringWidth(DEFAULT_STR) + 5;

	BRect				f(where.x, where.y, where.x + lW + iW, where.y + max);
	mHeightCtrl = new ArpIntControl(f, "fac_height", label, new BMessage(CHANGE_FAC_VIEW_HEIGHT_MSG) );
	if (!mHeightCtrl) return;

	mHeightCtrl->SetLimits(AM_MIN_FAC_VIEW_HEIGHT - 1, 200);
	mHeightCtrl->SetValue(AM_MIN_FAC_VIEW_HEIGHT - 1);
	mHeightCtrl->SetEnabled(true);
	mHeightCtrl->SetFormatter(new _StdFacHeightFormat());

	int32				i32;
	if (mPrefs.GetInt32Preference(mFacSig.String(), mFacKey.String(), AM_HEIGHT_PREF_STR, &i32) != B_OK)
		i32 = AM_MIN_FAC_VIEW_HEIGHT - 1;
	mHeightCtrl->SetValue(i32);

	AddChild(mHeightCtrl);
}