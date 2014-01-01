/* GlRealtimeParamList.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2003.02.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLREALTIMEPARAMLIST_H
#define GLPUBLIC_GLREALTIMEPARAMLIST_H

#include <GlPublic/GlActions.h>
#include <GlPublic/GlParamListI.h>
class GlControlState;
class GlMidiEvent;
class GlRealtimeEntryList;
class GlRootNode;
class GlAlgo2d;
class _GlRtplData;

/***************************************************************************
 * GL-REALTIME-PARAM-LIST
 * A param list that stores direct indexes into parse nodes for fast
 * access.
 ***************************************************************************/
class GlRealtimeParamList : public GlParamListI
{
public:
	GlRealtimeParamList();
	virtual ~GlRealtimeParamList();

	virtual status_t		Add(gl_node_id nid, const GlParam* param, int32 index,
								const BString16* label, int32 control, int32 midi);
	virtual status_t		Add(gl_node_id nid, const GlParamType* paramType,
								const BString16* label, int32 control, int32 midi);

	status_t				SetValue(const gl_param_key& key, const GlParamWrap& wrap);
	status_t				SetValue(int32 midi, const GlMidiEvent& e);
	/* Must be run before using the list -- this fills in all
	 * the targets for each param.
	 */
	status_t				Build(GlAlgo2d* s);
	status_t				UpdateSource(GlNode* root);

	void					SetState(GlControlState& s) const;

private:
	_GlRtplData*			mData;
	
	GlRealtimeEntryList*	mParams;
	GlRealtimeEntryList*	mMidi[GL_MIDI_SIZE];

	/* If a root is assigned during Build(), then calling
	 * SetValue() will bypass the current target and instead find
	 * a node in the root with a matching NodeId().  I do not own
	 * the root -- it must exist for as long as I have it.
	 */
	GlAlgo*					mRoot;
	GlParseFromNodeAction	mRootAction;
	
	GlRealtimeEntryList*	ParamList(bool create);
	GlRealtimeEntryList*	MidiListAt(int32 midi, bool create);

public:
	void					Print(uint32 tabs = 0) const;
};

#endif
