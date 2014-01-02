/* GlNode.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 04.18.99		hackborn
 * Created this file.
 */
 
#ifndef GLPUBLIC_GLNODE2_H
#define GLPUBLIC_GLNODE2_H

#include <ArpCore/ArpCoreDefs.h>
#include <ArpCore/ArpSynchronization.h>
#include <ArpCore/String16.h>
#include <app/Message.h>
#include <interface/Rect.h>
#include <interface/View.h>
#include <ArpSupport/ArpSafeDelete.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlNodeData.h>
#include <GlPublic/GlParamList.h>
#include <GlPublic/GlParamStrainer.h>
#include <GlPublic/GlParamTypeList.h>
#include <GlPublic/GlRootRef.h>
class ArpBitmap;
class GlAlgo;
class GlAlgo1d;
class GlAlgo2d;
class GlAlgoIm;
class GlChain;
class GlControlChannel;
class GlNode;
class GlNodeAddOn;
class GlNodePrivate;
class GlNodeVisual;
class GlParamListI;
class GlRecorder;
class GlStrainedParamList;
class _GlStrainList;

/* NewView() view types.  Subclasses can add new views by adding
 * on to GL_NODE_VIEWS (or whatever the subclass above them has
 * added to it).
 */
enum {
	GL_INSPECTOR_VIEW		= 1,
	GL_CONFIG_VIEW			= 2,
	GL_NODE_VIEWS
};

/* Get/SetProperty() codes.  Subclasses can add new properties by
 * adding on to GL_NODE_PROPS (or whatever the subclass above
 * them has added to it).
 */
enum {
	GL_NODE_PROPS			= 0
};

/* GetParams() flags
 */
enum {
	GL_STRAIN_PARAMS		= 0x00000001		// Use my strainer on my params.  This is
												// really only for the config view.
};

/* NewView() flags
 */
enum {
	GL_NEW_VIEW_PREVIEW		= 0x00000001		// Node view should display the Preview() image
};

/* Flags()
 */
enum {
	GL_RECORDER_F			= 0x00000001,		// The node can be recorded into
	GL_PIXEL_TARGETS_F		= 0x00000002		// The node makes use of pixel targets
};

/* Tags()
 */
enum {
	GL_MARKED_F				= 0x00000001		// The node has been marked externally
};

struct gl_new_view_params
{
	size_t					size;				// amount of data in structure
	uint32					flags;				// state flags
	int32					viewType;
	BRect					frame;
	GlRootRef				ref;
	GlControlChannel*		channel;			// An object to add a control target to, and
												// also to send control commands to

	gl_new_view_params();
	gl_new_view_params(const gl_new_view_params& o);
	~gl_new_view_params();
	gl_new_view_params& operator=(const gl_new_view_params& o);

private:
	bool operator==(const gl_new_view_params& o) const;
	bool operator!=(const gl_new_view_params& o) const;
};

/***************************************************************************
 * GL-ABSTRACT-NODE
 ***************************************************************************/
class GlAbstractNode : public ArpSafeDelete
{
public:
	gl_node_id				Id() const;
	
	/* Generic interface for getting/setting non-param values in the node.
	 * Subclasses must augment to allow access to their properties.  To
	 * define a code, add to GL_NODE_PROPS (or whatever the class above
	 * you has added to that).
	 */
	virtual status_t		GetProperty(int32 code, GlParamWrap& wrap) const;
	virtual status_t		SetProperty(int32 code, const GlParamWrap& wrap);

	uint32					ChainSize() const;
	const GlChain*			ChainAt(uint32 index) const;
	GlChain*				ChainAt(uint32 index);
	const GlChain*			FindChain(gl_chain_id id, bool recurse = true) const;
	GlChain*				FindChain(gl_chain_id id, bool recurse = true);
	const GlChain*			FindChain(int32 key) const;
	GlChain*				FindChain(int32 key);
	/* It's nice to have a cid, but it can be 0.
	 */
	const GlNode*			FindNode(gl_chain_id cid, gl_node_id nid, bool recurse = true) const;
	GlNode*					FindNode(gl_chain_id cid, gl_node_id nid, bool recurse = true);

	virtual GlAlgo*			Generate(const gl_generate_args& args) const = 0;
	virtual BView*			NewView(gl_new_view_params& params) const;
	/* Any class that augments the Read/Write methods should NOT
	 * pass the config to inherited in the constructor, instead
	 * calling ReadFrom() itself.  Otherwise, the augmented Read
	 * is never called.
	 */
	virtual status_t		ReadFrom(const BMessage& config);
	virtual status_t		WriteTo(BMessage& config) const;

	virtual void			MakeEmpty();
	
protected:
	GlAbstractNode();
	GlAbstractNode(const BMessage* config);
	GlAbstractNode(const GlAbstractNode& o);
	virtual ~GlAbstractNode();

	/* Conveniences for accessing the data generated by my
	 * chain.  Each method processes the chain for the given
	 * key and answers the requested data type, if any.
	 */
	GlAlgo*					GenerateChainAlgo(int32 key, const gl_generate_args& args) const;
	GlAlgo1d*				GenerateChain1d(int32 key, const gl_generate_args& args) const;
	GlAlgo2d*				GenerateChain2d(int32 key, const gl_generate_args& args) const;

	/* If I have a chain that matches the key, then assign the incoming
	 * chain's label to it and answer it.  Otherwise add the incoming chain.
	 * If I couldn't add it for some reason, delete it.
	 * If the added arg is supplied, it will be sent to true when the
	 * chain is added -- i.e. it wasn't already there.
	 */
	GlChain*				VerifyChain(GlChain* chain, bool* added = 0);
	/* A blind add -- I can have multiple chains with the same key
	 * if they're dynamic.
	 */
	status_t				AddChain(GlChain* chain);
	status_t				RemoveChain(gl_chain_id cid);
	/* A little hack so that, when copied, the chains can be given
	 * the correct parent node.  See the comment on mParent in GlChain
	 * for a little info.
	 */
	void					SetParentHack(GlNode* node);

	void					PrintChains(uint32 tabs = 0) const;
		
private:
	GlNodePrivate*			mData;
};

/***************************************************************************
 * GL-NODE
 ***************************************************************************/
class GlNode : public GlAbstractNode
{
public:
	GlNode(const GlNodeAddOn* addon, const BMessage* config);
	GlNode(const GlNode& o);

	const GlNodeAddOn*			AddOn() const;
	virtual uint32				Io() const;
	virtual uint32				Flags() const;
	/* Tags are an alternative to flags -- flags are generated
	 * dynamically, but tags can be set externally and persist.
	 */
	uint32						Tags() const;
	void						SetTags(uint32 tags);
	const GlChain*				Parent() const;
	GlChain*					Parent();
	const GlParamList&			Params() const;
	GlParamList&				Params();
	/* Some subclasses store a list of their own maps.
	 */
	virtual status_t			SetMapParam(const GlAlgo1d* map, int32 index,
											const char* name);
	/* The strainer masks which params are currently active.
	 */
	const GlParamStrainer&		ParamStrainer() const;
	GlParamStrainer&			ParamStrainer();

	virtual GlNode*				Clone() const = 0;

	virtual status_t			ReadFrom(const BMessage& config);
	virtual status_t			WriteTo(BMessage& config) const;

	virtual BView*				NewView(gl_new_view_params& params) const;
	virtual status_t			GetParams(GlParamListI& list, uint32 flags = 0) const;
	virtual status_t			ParamChanged(gl_param_key key);
	virtual status_t			ChainChanged(gl_chain_id id, int32 dynamic = 0);
	
	virtual GlNodeVisual*		NewVisual(const GlRootRef& ref) const;
	virtual GlRecorder*			NewRecorder(const GlRootRef& ref);
	
	virtual void				MakeEmpty();
	/* These are typically passthroughs to the AddOn but subclasses
	 * can override.
	 */
	virtual const ArpBitmap*	Image() const;
	virtual float				GetMatch(	const BString16& creator, int32 key,
											int32* major = 0, int32* minor = 0) const;

protected:
	virtual ~GlNode();
	friend class GlChain;

	GlChain*					mParent;
	GlParamList					mParams;
	GlParamStrainer				mStrainer;

	mutable ArpBitmap*			mImage;

	status_t					GetConfigParams(GlParamListI& list) const;
	status_t					GetInspectParams(GlParamListI& list) const;
	/* Add all my params that are in the strainer, and search
	 * through all my chains for their params, too.
	 */
	status_t					StrainParams(_GlStrainList* strainer) const;

	void						SetParent(GlChain* chain = 0);

private:
	typedef GlAbstractNode		inherited;
	const GlNodeAddOn*			mBaseAddon;
	uint32						mTags;
	
	BView*						NewInspectorView(const gl_new_view_params& params) const;
	BView*						NewConfigView(const gl_new_view_params& params) const;

public:
	void						Print(uint32 tabs = 0) const;
};

/***************************************************************************
 * GL-NODE-ADD-ON
 ***************************************************************************/
class GlNodeAddOn
{
public:
	virtual ~GlNodeAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const = 0;
//	virtual status_t		Archive(BMessage& config) const;

	gl_node_add_on_id		Id() const;
	/* Answer one or a combination of GL_IMAGE_IN, etc.
	 */
	virtual uint32			Io() const = 0;
	virtual uint32			Flags() const;

	/* Creator:Key uniquely identifies a node.
	 */
	BString16				Creator() const;
	int32					Key() const;
	BString16				Category() const;
	BString16				Label() const;
	void					GetVersion(int32* major, int32* minor) const;
	const GlParamTypeList&	ParamTypes() const;
	/* Answer a small image that represents the node.
	 */
	const ArpBitmap*		Image() const;

	/* A shortcut to the algo generated by the node.  By default,
	 * I just instantiate a node then get its algo, however some
	 * algos are fundamental to the system, so their addons might
	 * override this to generate the algo directly and save a
	 * little overhead.
	 */
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

	float					GetMatch(	const BString16& creator, int32 key,
										int32* major = 0, int32* minor = 0) const;

protected:
	friend class GlNode;
	friend class GlGlobalsImpl;		// Not necessary, but it prevents copying all
									// the category strings.
	/* The creator and key together must form a unique key.
	 */
	GlNodeAddOn(const BString16& creator, int32 key,
				const BString16* category, const BString16* label,
				int32 majorVersion, int32 minorVersion);

	BString16				mCreator;
	int32					mKey;
	BString16				mCategory;
	BString16				mLabel;
	int32					mMajor, mMinor;		// Version
	/* Answer the type if added, 0 otherwise.
	 */
	const GlParamType*		AddParamType(GlParamType* paramType);
	/* Return the default node image run through my node.  Anyone who
	 * wants otherwise must override this.
	 */
	virtual GlImage*		NewImage() const;
		
private:
	GlParamTypeList			mParamTypes;
	mutable ArpBitmap*		mImage;
};

#endif
