/* GlParamView.h
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
#ifndef GLPUBLIC_GLPARAMVIEW_H
#define GLPUBLIC_GLPARAMVIEW_H

#include <ArpCore/ArpChar.h>
#include <GlPublic/GlControlTarget.h>
#include <GlPublic/GlControlView.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParamWrap.h>
#include <GlPublic/GlRootRef.h>
class GlControlChannel;
class GlFloatParamType;
class GlNode;
class GlNodeVisual;
class GlParam;
class GlParamType;
class GlStrainedParam;
class _ParamViewData;
class _ParamViewTarget;

/* A simple interface for controlling which controls are added
 * during the AddParamControls() call.
 */
class GlAddParamFilter
{
public:
	virtual bool		Allow(	gl_node_id nid, const GlParam* p,
								const GlParamType* pt, int32 index);
	virtual bool		AllowKey(gl_param_key key);
};

/***************************************************************************
 * GL-PARAM-VIEW
 * This abstract class creates controls out of params and adds them to
 * itself, then provides subclass notification about changed params.
 ***************************************************************************/
class GlParamView : public GlControlView,
					public GlControlTargetPopulator
{
public:
	// During init -- this is really ugly.
	BRect				mRect;

	GlParamView(const gl_new_view_params& params,
				gl_chain_id cid, gl_node_id nid);
	virtual ~GlParamView();

	virtual void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual void			MessageReceived(BMessage* msg);

	/* GL-CONTROL-TARGET-POPULATOR interface
	 */
	virtual GlRecorderBitmapView*	RecorderView(const char* name = 0);	

	/* Use (and modify) the mRect for param frames.
	 */
	status_t				AddParamControls(	const GlStrainedParamList& params,
												BRect* f = 0, GlAddParamFilter* filter = 0);

	/* This is similar to calling AddBitmapView() in GlControlView, but
	 * a visual view does more -- it causes a GlNodeVisual to be generated
	 * from the node, and it uses that to generate a bitmap.  Also, the
	 * visuals can be numbered -- if you supply a value of 0 or greater
	 * to visualIndex, that value will be passed off to the visual's
	 * Visual() method.  The node is required to generate the visual.  That's
	 * pretty ugly but oh well.
	 */
	ArpBitmapView*			AddVisualView(	const BRect& frame, const char* name,
											GlNodeVisual* visual, int32 visualIndex = 0);

protected:
	GlRootRef			mRef;
	gl_chain_id			mCid;
	gl_node_id			mNid;
	GlControlChannel*	mChannel;
	// During init
	uint32				mWhat;

	virtual	status_t	ControlMessage(uint32 what);
	virtual status_t	UpdateControls();
	
	status_t			ParamControlKeyAt(	uint32 index, gl_param_key* outKey) const;
	status_t			EnableParamControl(	gl_param_key key, bool enable = true);
	status_t			SetParamControl(	gl_param_key key, GlParamWrap& wrap);

	/* The pt is always valid but the param might be NULL.  If there's a param
	 * use its node ID, otherwise the supplied nid.
	 */
	status_t			AddParamControl(GlStrainedParam& p, float div);
	status_t			AddParamControlsFinished();
	/* Layout convenience.
	 */
	float				FloatW(const BString16* label, const GlFloatParamType* pt);

private:
	typedef GlControlView	inherited;
	_ParamViewData*			mData;
	GlNodeVisual*			mVisual;
	// During init
	int32					mRow;
	gl_node_id				mRowNid;
	float					mRowLeft;
	enum {
		START_ROW			= 1,
		STACK_ROW			= 2
	};
	uint32					mRowF;
	int32					mRowColumn;
	_ParamViewTarget*		mTarget;
		
	status_t				AddBoolParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddColorParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddFileNameParam(GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddFloatParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddFontParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddInt32Param(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddMenuParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddPointParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddPoint3dParam(GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddRelAbsParam(	GlStrainedParam& p, gl_param_key key, float div);
	status_t				AddTextParam(	GlStrainedParam& p, gl_param_key key, float div);

	void					AddMidiParam(	int32 midi, gl_param_key key, const GlParam* p,
											const GlParamType* pt);
};


#endif
