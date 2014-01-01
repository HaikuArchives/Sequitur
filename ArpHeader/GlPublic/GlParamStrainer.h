/* GlParamStrainer.h
 * Copyright (c)2003 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.02.16			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLPARAMSTRAINER_H
#define GLPUBLIC_GLPARAMSTRAINER_H

#include <ArpCore/ArpChar.h>
#include <GlPublic/GlNodeReaderWriter.h>
class _ParamStrainerData;
class _ParamStrainerTypeEntry;
class _GlStrainList;

/***************************************************************************
 * GL-PARAM-STRAINER
 * I am a list of active params.  I am stored in nodes, so that each node
 * can filter out params from previous nodes.
 ****************************************************************************/
class GlParamStrainer : public GlNodeReaderWriter
{
public:
	GlParamStrainer();
	virtual ~GlParamStrainer();

	status_t			InitCheck() const;
	const BString16*	Label(		gl_param_key key) const;
	status_t			SetLabel(	gl_param_key key, const BString16* label);
	int32				Control(	gl_param_key key) const;
	status_t			SetControl(	gl_param_key key, int32 control);
	int32				Midi(		gl_param_key key) const;
	status_t			SetMidi(	gl_param_key key, int32 midi);

	/* Enabled(), Label() and Midi() are just conveniences for this method.
	 */
	status_t			NewGetAt(gl_param_key key, int32* ptIndex,
								const BString16** outLabel, int32* outControl, 
								int32* outMidi) const;
								
	status_t			GetAt(	gl_param_key key, int32* ptIndex,
								bool* outEnabled, const BString16** outLabel,
								int32* outMidi) const;
	status_t			GetAt(	gl_node_id nid, uint32 paramTypeIndex,
								int32* outPtKey, int32* outI, bool* outEnabled,
								const BString16** outLabel, int32* outMidi) const;

	/* Answer the number of all params, across all nodes.
	 */
	uint32				Size() const;
	/* Answer the number of param type keys found at the node id.
	 */
	uint32				SizeAt(gl_node_id nid) const;

	virtual status_t	ReadNode(const GlNode* node, const BMessage& config);
	virtual status_t	WriteNode(const GlNode* node, BMessage& config) const;

protected:
	friend class GlNode;
	status_t			Load(	gl_node_id nid, _GlStrainList& list,
								const GlParamList& params,
								const GlParamTypeList& paramTypes) const;

private:
	_ParamStrainerData*		mData;

	const _ParamStrainerTypeEntry*	FindAt(const gl_param_key& key) const;
	_ParamStrainerTypeEntry*		MakeAt(const gl_param_key& key);

public:
	void					Print() const;
};

#endif
