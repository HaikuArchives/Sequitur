/* AmToolSeedViews.h
 * Copyright (c)2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
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
 * 2001.02.26			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMKERNEL_AMTOOLSEEDVIEWS_H
#define AMKERNEL_AMTOOLSEEDVIEWS_H

#include <be/interface/CheckBox.h>
#include <be/interface/RadioButton.h>
#include <be/interface/View.h>
#include "AmPublic/AmToolRef.h"
class ArpFloatControl;

/***************************************************************************
 * AM-TOOL-SEED-VIEW
 * The abstract superclass for all tool seed views.
 ***************************************************************************/
class AmToolSeedView : public BView
{
public:
	AmToolSeedView();
	virtual ~AmToolSeedView();

	void				SetToolRef(AmToolRef toolRef);
	/* This method requires that the tool ref already be set, and it locks
	 * that ref.
	 */
	status_t			SetInfo(const BString& factoryKey,
								const BString& viewKey,
								const BString& seedKey);

	virtual void		AttachedToWindow();
	virtual	void		MessageReceived(BMessage *msg);

	virtual	status_t	SetConfiguration(const BMessage* config);
	virtual	status_t	GetConfiguration(BMessage* config) const;

protected:
	AmToolRef			mToolRef;
	BCheckBox*			mScrollBox;
//	BCheckBox*			mScrollXBox;
//	BCheckBox*			mScrollYBox;

	status_t			WriteToToolRef();

	/* Answer what my current Flags() state should be based on my
	 * widgets.  Subclasses can add to this if they alter the flags state.
	 */
	virtual uint32		BuildFlags() const;
	virtual BPoint		AddViews();
	
private:
	typedef BView		inherited;
	BString				mFactoryKey, mViewKey, mSeedKey;
};

/***************************************************************************
 * AM-BOX-SEED-VIEW
 ***************************************************************************/
class AmBoxSeedView : public AmToolSeedView
{
public:
	AmBoxSeedView();
};

/***************************************************************************
 * AM-BEZIER-SEED-VIEW
 ***************************************************************************/
class AmBezierSeedView : public AmToolSeedView
{
public:
	AmBezierSeedView();
	virtual ~AmBezierSeedView();

	virtual void		AttachedToWindow();

	virtual	status_t	SetConfiguration(const BMessage* config);
	virtual	status_t	GetConfiguration(BMessage* config) const;

protected:
	virtual BPoint		AddViews();

private:
	typedef AmToolSeedView	inherited;
	ArpFloatControl*		mPt1StartX;
	ArpFloatControl*		mPt1StartY;
	ArpFloatControl*		mPt1EndX;
	ArpFloatControl*		mPt1EndY;
	ArpFloatControl*		mPt2StartX;
	ArpFloatControl*		mPt2StartY;
	ArpFloatControl*		mPt2EndX;
	ArpFloatControl*		mPt2EndY;
	ArpFloatControl*		mFrame;
	BCheckBox*				mCreateBox;
	BRadioButton*			mMoveCtrl;
	BRadioButton*			mTransformCtrl;
};

/***************************************************************************
 * AM-MOVE-SEED-VIEW
 ***************************************************************************/
class AmMoveSeedView : public AmToolSeedView
{
public:
	AmMoveSeedView();
};

/***************************************************************************
 * AM-TOUCH-SEED-VIEW
 ***************************************************************************/
class AmTouchSeedView : public AmToolSeedView
{
public:
	AmTouchSeedView();
};

/***************************************************************************
 * AM-TRANSFORM-SEED-VIEW
 ***************************************************************************/
class AmTransformSeedView : public AmToolSeedView
{
public:
	AmTransformSeedView();
	virtual ~AmTransformSeedView();

	virtual void		AttachedToWindow();

	virtual	status_t	SetConfiguration(const BMessage* config);

protected:
	virtual uint32		BuildFlags() const;
	virtual BPoint		AddViews();

private:
	typedef AmToolSeedView	inherited;
	BRadioButton*		mOneByOneCtrl;
	BRadioButton*		mEnMasseCtrl;
};

/***************************************************************************
 * AM-CREATE-SEED-VIEW
 ***************************************************************************/
class AmCreateSeedView : public AmToolSeedView
{
public:
	AmCreateSeedView();
	virtual ~AmCreateSeedView();

	virtual void		AttachedToWindow();

	virtual	status_t	SetConfiguration(const BMessage* config);
//	virtual	status_t	GetConfiguration(BMessage* config) const;

protected:
	virtual uint32		BuildFlags() const;
	virtual BPoint		AddViews();

private:
	typedef AmToolSeedView	inherited;
	BRadioButton*		mMoveCtrl;
	BRadioButton*		mAddNCtrl;
	BRadioButton*		mTransformCtrl;
	BRadioButton*		mRefilterCtrl;
};

/***************************************************************************
 * AM-REFILTER-SEED-VIEW
 ***************************************************************************/
class AmRefilterSeedView : public AmToolSeedView
{
public:
	AmRefilterSeedView();
	virtual ~AmRefilterSeedView();

	virtual void		AttachedToWindow();

	virtual	status_t	SetConfiguration(const BMessage* config);
//	virtual	status_t	GetConfiguration(BMessage* config) const;

protected:
	virtual uint32		BuildFlags() const;
	virtual BPoint		AddViews();

private:
	typedef AmToolSeedView	inherited;
	BCheckBox*		mRestoreBox;
};

#endif
