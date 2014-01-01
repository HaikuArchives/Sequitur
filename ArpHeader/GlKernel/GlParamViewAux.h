/* GlParamViewAux.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.02.24			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLPARAMVIEWAUX_H
#define GLKERNEL_GLPARAMVIEWAUX_H

#include <GlPublic/GlControlTarget.h>
#include <GlPublic/GlControlView.h>
#include <GlPublic/GlDefs.h>
class GlControlChannel;
class GlNode;
class GlParamWrap;
class _ParamViewMidiList;

/***************************************************************************
 * _PARAM-VIEW-TARGET
 ***************************************************************************/
class _ParamViewTarget : public GlControlTarget
{
public:
	_ParamViewTarget(const GlRootRef& ref, void* sender);
	virtual ~_ParamViewTarget();

	virtual status_t		ParamEvent(	gl_param_key key, int32 code,
										const GlParamWrap& wrap);
	virtual status_t		MidiEvent(	GlMidiEvent event, int32 letter,
										bigtime_t time);

	virtual void			Populate(GlControlTargetPopulator& p);
	virtual void			SetState(GlControlState& s) const;

	/* Midi handling
	 */
	status_t				AddMidiParam(	int32 midi, gl_param_key key, 
											const GlParam* p);
	status_t				AddMidiParam(	int32 midi, gl_param_key key, 
											const GlParamType* pt);

private:
	GlRootRef				mRef;
	void*					mSender;
	_ParamViewMidiList*		mMidi[GL_MIDI_SIZE];

	_ParamViewMidiList*		MidiListAt(int32 midi, bool create);
};

/***************************************************************************
 * _PARAM-VIEW-ENTRY
 ***************************************************************************/
class _ParamViewEntry
{
public:
	gl_param_key		key;
	int32				mColumn;
	
	_ParamViewEntry(gl_param_key k, uint32 what);
	_ParamViewEntry(gl_param_key k, uint32 changingMsg, uint32 changedMsg);
	virtual ~_ParamViewEntry();

	virtual uint32		MatchesWhat(uint32 what) const;

	virtual status_t	Callback(GlControlChannel* channel, uint32 code) = 0;
	virtual status_t	EnableControl(bool enable) = 0;
	virtual status_t	SetControl(GlParamWrap& wrap) = 0;
/* FIX: Make this pure virtual and finish everyone.
 */
	virtual status_t	UpdateControl(const GlNode& node) { return B_ERROR; }
	virtual float		RightEdge() const = 0;
	virtual void		SetLeftEdge(float left) = 0;

private:
	uint32				mChangingMsg, mChangedMsg;
};

/***************************************************************************
 * _PARAM-VIEW-BOOL-ENTRY
 ***************************************************************************/
class _ParamViewBoolEntry : public _ParamViewEntry
{
public:
	BCheckBox*			mCtrl;

	_ParamViewBoolEntry(BCheckBox* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual status_t	UpdateControl(const GlNode& node);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-COLOR-ENTRY
 ***************************************************************************/
class _ParamViewColorEntry : public _ParamViewEntry
{
public:
	ArpColourControl*	mCtrl;

	_ParamViewColorEntry(ArpColourControl* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-FILE-NAME-ENTRY
 ***************************************************************************/
class _ParamViewFileNameEntry : public _ParamViewEntry
{
public:
	ArpFileNameControl*	mCtrl;

	_ParamViewFileNameEntry(ArpFileNameControl* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-FLOAT-ENTRY
 ***************************************************************************/
class _ParamViewFloatEntry : public _ParamViewEntry
{
public:
	ArpFloatControl*	mCtrl;

	_ParamViewFloatEntry(	ArpFloatControl* ctrl, gl_param_key key,
							uint32 changingMsg, uint32 changedMsg);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual status_t	UpdateControl(const GlNode& node);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-FONT-ENTRY
 ***************************************************************************/
class _ParamViewFontEntry : public _ParamViewEntry
{
public:
	ArpFontControl*	mCtrl;

	_ParamViewFontEntry(ArpFontControl* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-INT32-ENTRY
 ***************************************************************************/
class _ParamViewInt32Entry : public _ParamViewEntry
{
public:
	ArpIntControl*	mCtrl;

	_ParamViewInt32Entry(ArpIntControl* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual status_t	UpdateControl(const GlNode& node);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-MENU-ENTRY
 ***************************************************************************/
class _ParamViewMenuEntry : public _ParamViewEntry
{
public:
	ArpMenuControl*	mCtrl;

	_ParamViewMenuEntry(ArpMenuControl* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual status_t	UpdateControl(const GlNode& node);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};

/***************************************************************************
 * _PARAM-VIEW-POINT-ENTRY
 ***************************************************************************/
class _ParamViewPointEntry : public _ParamViewEntry
{
public:
	ArpFloatControl*	mXCtrl;
	ArpFloatControl*	mYCtrl;

	_ParamViewPointEntry(	ArpFloatControl* xCtrl, uint32 xChanging, uint32 xChanged,
							ArpFloatControl* yCtrl, uint32 yChanging, uint32 yChanged,
							gl_param_key key);

	virtual uint32		MatchesWhat(uint32 what) const;
	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
	uint32					mXChanging, mXChanged,
							mYChanging, mYChanged;
};

/***************************************************************************
 * _PARAM-VIEW-POINT3D-ENTRY
 ***************************************************************************/
class _ParamViewPoint3dEntry : public _ParamViewEntry
{
public:
	ArpFloatControl*	mXCtrl;
	ArpFloatControl*	mYCtrl;
	ArpFloatControl*	mZCtrl;

	_ParamViewPoint3dEntry(	ArpFloatControl* xCtrl, uint32 xChanging, uint32 xChanged,
							ArpFloatControl* yCtrl, uint32 yChanging, uint32 yChanged,
							ArpFloatControl* zCtrl, uint32 zChanging, uint32 zChanged,
							gl_param_key key);

	virtual uint32		MatchesWhat(uint32 what) const;
	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
	uint32					mXChanging, mXChanged,
							mYChanging, mYChanged,
							mZChanging, mZChanged;
};

/***************************************************************************
 * _PARAM-VIEW-REL-ABS-ENTRY
 ***************************************************************************/
class _ParamViewRelAbsEntry : public _ParamViewEntry
{
public:
	ArpFloatControl*	mRel;
	ArpIntControl*		mAbs;

	_ParamViewRelAbsEntry(	ArpFloatControl* rel, uint32 relChanging, uint32 relChanged,
							ArpIntControl* abs, uint32 absChanging, uint32 absChanged,
							gl_param_key key);

	virtual uint32		MatchesWhat(uint32 what) const;
	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
	uint32					mRelChanging, mRelChanged,
							mAbsChanging, mAbsChanged;
};

/***************************************************************************
 * _PARAM-VIEW-TEXT-ENTRY
 ***************************************************************************/
class _ParamViewTextEntry : public _ParamViewEntry
{
public:
	BTextControl*	mCtrl;

	_ParamViewTextEntry(BTextControl* ctrl, gl_param_key k, uint32 what);

	virtual status_t	Callback(GlControlChannel* channel, uint32 code);
	virtual status_t	EnableControl(bool enable);
	virtual status_t	SetControl(GlParamWrap& wrap);
	virtual float		RightEdge() const;
	virtual void		SetLeftEdge(float left);

private:
	typedef _ParamViewEntry	inherited;
};


#endif
