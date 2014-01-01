/* GlMainWinLists.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2003.04.22		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLMAINWINLISTS_H
#define GLASSLIKE_GLMAINWINLISTS_H

#include <ArpCore/StlVector.h>
#include <GlPublic/GlDefs.h>

/********************************************************
 * GL-MAIN-LABEL-ENTRY
 ********************************************************/
class GlMainLabelEntry
{
public:
	BString16					label;
	BRect						frame;
	/* Additional info for certain clients.
	 */
	gl_id						id;
	int32						code;
	
	GlMainLabelEntry(const BString16& l, gl_id i);
	GlMainLabelEntry(const BString16& l, int32 c);
};

/********************************************************
 * GL-MAIN-LABELS
 ********************************************************/
class GlMainLabels
{
public:
	vector<GlMainLabelEntry*>	e;
		
	GlMainLabels();
	~GlMainLabels();

	void						DrawOn(BRect clip, BView* v, gl_chain_id active);
	const GlMainLabelEntry*		EntryAt(BPoint pt, int32* outIndex = 0) const;
	GlMainLabelEntry*			EntryAt(BPoint pt, int32* outIndex = 0);

	void						Free();
};

/********************************************************
 * GL-MAIN-IMAGE-ENTRY
 * I store a cache on either a node or a node add on, depending
 * on the context.
 ********************************************************/
class GlMainImageEntry
{
public:
	const ArpBitmap*			image;
	BRect						frame;
	/* Additional info for certain clients.
	 */
	gl_id						id;
	uint32						chainSize;
	uint32						flags;
	BString16					label;
		
	GlMainImageEntry(	gl_id inId, const ArpBitmap* img,
						uint32 cs, uint32 f, BString16* l = 0);
};

/********************************************************
 * GL-MAIN-IMAGES
 ********************************************************/
class GlMainImages
{
public:
	vector<GlMainImageEntry*>	e;

	GlMainImages();
	~GlMainImages();

	const GlMainImageEntry*	At(int32 index) const;
	GlMainImageEntry*		At(int32 index);
	const GlMainImageEntry*	EntryAt(BPoint pt, int32* outIndex = 0) const;
	GlMainImageEntry*		EntryAt(BPoint pt, int32* outIndex = 0);
	const GlMainImageEntry*	EntryAt(gl_id id, int32* outIndex = 0) const;
	GlMainImageEntry*		EntryAt(gl_id id, int32* outIndex = 0);
	void					DeleteCache();
};


#endif
