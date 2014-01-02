#ifndef GLPUBLIC_GLNODEDATA_H
#define GLPUBLIC_GLNODEDATA_H

#include <support/SupportDefs.h>
#include <ArpCore/StlVector.h>
#include <ArpCore/String16.h>
class GlCache1d;
class GlImage;
class GlMask;
class GlNodeDataList;
class GlPath;

/*******************************************************
 * GL-NODE-DATA
 * Node data is data that is passed from node to node
 * during processing.
 *******************************************************/
class GlNodeData
{
public:
	virtual ~GlNodeData();

	enum NodeDataType {
		NO_TYPE				= 0,
		ERROR_TYPE			= 1,			// Signifies an error
		LIST_TYPE			= 2,			// Stores a data list
		BACKPOINTER_TYPE	= 3,			// Stores backpointer data
		CACHE1D_TYPE		= 9,			// Stores a GlCache1d
		IMAGE_TYPE			= 12,			// Stores a GlImage
		TEXT_TYPE			= 13,			// Stores text
		LINE_TYPE			= 14,			// Stores a GlPath
		MASK_TYPE			= 15,			// Stores an ArpMask
		RESERVED_TYPE		= 100000		// Reserved for plugins to add their
											// own types
	};
	virtual NodeDataType	Type() const = 0;
	/* Delete all my data recursively.  Doesn't delete me.
	 */
	virtual void			DeleteContents();

	virtual GlNodeData*		Copy() const = 0;
	/* Not every data type is parseable, but some are.
	 * This is a convenience to parse them without
	 * mucking about with the actual data.
	 */
	virtual status_t		Parse(bool recurse = true);
	
protected:
	GlNodeData(const GlNodeData& o);
	GlNodeData();

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const = 0;
};

/*******************************************************
 * GL-NODE-DATA-ERROR
 * This object stores an error.
 *******************************************************/
class GlNodeDataError : public GlNodeData
{
public:
	GlNodeDataError();
	GlNodeDataError(const BString16& err);
	GlNodeDataError(const GlNodeDataError& o);
	virtual ~GlNodeDataError();

	virtual NodeDataType	Type() const { return ERROR_TYPE; }
	virtual GlNodeData*		Copy() const;

	BString16				Error() const;
	void					SetError(const BString16& err);

private:
	typedef GlNodeData		inherited;
	BString16				mError;

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};

/*******************************************************
 * GL-NODE-DATA-LIST
 * This object stores node data, and is the mechanism
 * that transports the data between nodes and across
 * tags.  I DO NOT OWN MY DATA.  I WILL NOT DELETE IT
 * ON DESTRUCTION.  Use DeleteContents() before destruction
 * if you want that.
 *******************************************************/
class GlNodeDataList : public GlNodeData
{
public:
	GlNodeDataList();
	GlNodeDataList(const GlNodeDataList& o);
	/* The constructor does not free any of my data; call
	 * DeleteData() first for that.
	 */
	virtual ~GlNodeDataList();

	virtual NodeDataType	Type() const { return LIST_TYPE; }
	/* Delete all the contents of the supplied port.  A NULL target
	 * will delete all contents.
	 */
	status_t				DeleteContents(NodeDataType type = NO_TYPE);
	status_t				DeleteContents(uint32 start, uint32 stop);
	/* The data is removed but not deleted.
	 */
	status_t				RemoveData(GlNodeData* data);
	/* All data on the port is removed but not deleted.  NULL means all ports.
	 */
	status_t				RemoveContents(NodeDataType type = NO_TYPE);

	virtual status_t		Parse(bool recurse = true);

	/* Copies recursively.
	 */
	virtual GlNodeData*		Copy() const;

	uint32					Size() const;
	uint32					Size(NodeDataType type) const;

	BString16				ErrorAt(uint32 index);
	GlNodeData*				DataAt(uint32 index, NodeDataType type = NO_TYPE) const;
	/* Convenience -- most clients will just iterate over a particular
	 * type of data in the list and process each item.  A target of NULL
	 * will answer every item, regardless of target.
	 */
	GlCache1d*				Cache1dAt(uint32 index);
	GlImage*				ImageAt(uint32 index);

	GlPath*					LineAt(uint32 index);
	BString16*				TextAt(uint32 index);
	/* The data list becomes the owner of any data supplied.
	 */
	status_t				AddData(GlNodeData* data);
	status_t				AddCache1d(GlCache1d* im);
	status_t				AddImage(GlImage* im);

	status_t				AddLine(GlPath* p);
	status_t				AddText(const BString16& text);
	/* Give ownership of the requested data to the client.
	 */
	GlCache1d*				DetachCache1d();
	GlImage*				DetachImage();

	GlPath*					DetachLine();

	status_t				ReplaceData(uint32 index, GlNodeData* data);
	/* All data in list will be added to me and removed from list.
	 */
	status_t				MergeList(GlNodeDataList* list);
	
	bool					Empty() const;

	/* Data lists are used to communicate errors to the framework.
	 * These methods delete all data installed in the list and add
	 * in the appropriate errors.
	 */
	status_t				AsNoMemoryError();
	status_t				AsError(const BString16& err);

private:
	typedef GlNodeData		inherited;
	vector<GlNodeData*>		m_Data;

	void*					TypeAt(NodeDataType type, uint32 index);

	GlNodeData*				DetachAt(NodeDataType type);
	
/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};

/*******************************************************
 * GL-NODE-DATA-IMAGE
 * This object stores a GlImage.
 *******************************************************/
class GlNodeDataImage : public GlNodeData
{
public:
	GlNodeDataImage();
	GlNodeDataImage(GlImage* im);
	GlNodeDataImage(const GlNodeDataImage& o);
	virtual ~GlNodeDataImage();

	virtual NodeDataType	Type() const { return IMAGE_TYPE; }
	virtual GlNodeData*		Copy() const;

	GlImage*				Image() const;
	/* Take ownership of the image.
	 */
	void					SetImage(GlImage* im);		
	/* Release ownership of the image to the caller.
	 */
	GlImage*				DetachImage();

private:
	typedef GlNodeData		inherited;
	GlImage*				mImage;

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};

/*******************************************************
 * GL-NODE-DATA-CACHE-1D
 * This object stores a GlCache1d.
 *******************************************************/
class GlNodeDataCache1d : public GlNodeData
{
public:
	GlNodeDataCache1d();
	GlNodeDataCache1d(GlCache1d* c);
	GlNodeDataCache1d(const GlNodeDataCache1d& o);
	virtual ~GlNodeDataCache1d();

	virtual NodeDataType	Type() const { return CACHE1D_TYPE; }
	virtual GlNodeData*		Copy() const;

	GlCache1d*				Cache() const;
	/* Take and release ownership of the cache
	 */
	void					Take(GlCache1d* c);
	GlCache1d*				Give();

private:
	typedef GlNodeData		inherited;
	GlCache1d*				mCache;

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};

/*******************************************************
 * GL-NODE-DATA-LINE
 * This object stores a GlPath.
 *******************************************************/
class GlNodeDataLine : public GlNodeData
{
public:
	GlNodeDataLine();
	GlNodeDataLine(GlPath* p);
	GlNodeDataLine(const GlNodeDataLine& o);
	virtual ~GlNodeDataLine();

	virtual NodeDataType	Type() const { return LINE_TYPE; }
	virtual GlNodeData*		Copy() const;

	GlPath*					Line() const;
	/* Take ownership of the path.
	 */
	void					SetLine(GlPath* p);
	/* Release ownership of the path to the caller.
	 */
	GlPath*					DetachLine();
	
private:
	typedef GlNodeData		inherited;
	GlPath*					mPath;

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};

/*******************************************************
 * GL-NODE-DATA-MASK
 * This object stores a GlMask.
 *******************************************************/
class GlNodeDataMask : public GlNodeData
{
public:
	GlNodeDataMask();
	GlNodeDataMask(GlMask* m);
	GlNodeDataMask(const GlNodeDataMask& o);
	virtual ~GlNodeDataMask();

	virtual NodeDataType	Type() const { return MASK_TYPE; }
	virtual GlNodeData*		Copy() const;

	GlMask*					Mask() const;
	/* Take ownership of the mask.
	 */
	void					SetMask(GlMask* m);
	/* Release ownership of the mask to the caller.
	 */
	GlMask*					DetachMask();

private:
	typedef GlNodeData		inherited;
	GlMask*					mMask;

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};

/*******************************************************
 * GL-NODE-DATA-TEXT
 * This object stores a string.
 *******************************************************/
class GlNodeDataText : public GlNodeData
{
public:
	GlNodeDataText();
	GlNodeDataText(const BString16& text);
	GlNodeDataText(const GlNodeDataText& o);
	virtual ~GlNodeDataText();

	virtual NodeDataType	Type() const { return TEXT_TYPE; }
	virtual GlNodeData*		Copy() const;

	BString16				Text() const;
	void					SetText(const BString16& text);

private:
	typedef GlNodeData		inherited;
	friend class GlNodeDataList;
	BString16				mText;

/* Debugging
 */
public:
	virtual void			Print(uint32 tabs = 0) const;
};


/*******************************************************
 * GL-NODE-DATA-BACK-POINTER
 * This is an internal housekeeping object.  It stores
 * a reference to another piece of data owned by someone
 * else.  The data is guaranteed to exist, if it's there.
 * This is for In nodes -- a node supplies a back pointer
 * to some of its data for its chain to process, and In
 * nodes can take this data and copy it, inserting a
 * proper object into the list.
 *******************************************************/
class GlNodeDataBackPointer : public GlNodeData
{
public:
	const GlImage*			image;

	GlNodeDataBackPointer();
	GlNodeDataBackPointer(const GlImage* img);
	GlNodeDataBackPointer(const GlNodeDataBackPointer& o);
	virtual ~GlNodeDataBackPointer();

	virtual NodeDataType	Type() const { return BACKPOINTER_TYPE; }
	virtual GlNodeData*		Copy() const;

private:
	typedef GlNodeData		inherited;

public:
	virtual void			Print(uint32 tabs = 0) const;
};

#endif
