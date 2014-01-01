/* GlStrainedParamList.h
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
 * 2003.03.10			hackborn@angryredplanet.com
 * Extracted from EnabledParamList, added labels to each entry
 */
#ifndef GLPUBLIC_GLSTRAINEDPARAMLIST_H
#define GLPUBLIC_GLSTRAINEDPARAMLIST_H

#include <be/support/SupportDefs.h>
#include <ArpCore/ArpChar.h>
#include <GlPublic/GlParamListI.h>
class GlMidiEvent;
class GlRootRef;
class _StrainedParamListData;

class GlStrainedParam
{
public:
	gl_node_id			nid;
	const GlParam*		p;
	int32				index;
	const GlParamType*	pt;
	const BString16*	label;
	int32				control, midi;
	
	GlStrainedParam();

	void				Clear();
};

/***************************************************************************
 * GL-STRAINED-PARAM-LIST
 * A list of params that have been run through the strainer for a
 * particular node, so any additional data, like label info, is included.
 * Each entry in the list is either a param OR a param type, keyed by node.
 * This is necessary for the UI to create and edit controls for params types
 * that don't have associated params.
 ****************************************************************************/
class GlStrainedParamList : public GlParamListI
{
public:
	/* Constructor flags.
	 */
	enum {
		NO_DEFAULT_LABEL_F			= 0x00000001	// When this is on, At()
		// will only return label if the entry has been assigned a label
		// by the strainer that owns it (otherwise, a label is always returned,
		// even if it means pulling it from the param type).
	};
	
	GlStrainedParamList(uint32 flags = 0);
	GlStrainedParamList(const GlStrainedParamList& o);
	virtual ~GlStrainedParamList();

	uint32				Size() const;
	void				SetFlags(uint32 flags);
	
	virtual status_t	Add(gl_node_id nid, const GlParam* param, int32 index,
							const BString16* label, int32 control, int32 midi);
	virtual status_t	Add(gl_node_id nid, const GlParamType* paramType,
							const BString16* label, int32 control, int32 midi);

	/* The param may or may not exist, but the paramType will always
	 * be there.
	 */
	status_t			At(uint32 index, GlStrainedParam& param) const;
	const GlParam*		At(const gl_param_key& key) const;

	void				MakeEmpty();

private:
	_StrainedParamListData*	mData;
	uint32			mFlags;
	
public:
	void			Print() const;
};


#endif
