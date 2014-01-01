#ifdef MS_WIN
#pragma warning( disable : 4541 )
#endif

#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlImage.h>
#include "GlPublic/GlMask.h"
#include <GlPublic/GlNodeData.h>
#include "GlPublic/GlPath.h"

/*******************************************************
 * GL-NODE-DATA
 *******************************************************/
GlNodeData::GlNodeData()
{
}

GlNodeData::GlNodeData(const GlNodeData& o)
{
}

GlNodeData::~GlNodeData()
{
}

void GlNodeData::DeleteContents()
{
}

status_t GlNodeData::Parse(bool recurse)
{
	return B_OK;
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-ERROR
 *******************************************************/
GlNodeDataError::GlNodeDataError()
{
}

GlNodeDataError::GlNodeDataError(const BString16& err)
		: mError(err)
{
}

GlNodeDataError::GlNodeDataError(const GlNodeDataError& o)
		: inherited(o), mError(o.mError)
{
}

GlNodeDataError::~GlNodeDataError()
{
}

GlNodeData* GlNodeDataError::Copy() const
{
	return new GlNodeDataError(*this);
}

BString16 GlNodeDataError::Error() const
{
	return mError;
}

void GlNodeDataError::SetError(const BString16& err)
{
	mError = err;
}

void GlNodeDataError::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataError %s\n", mError.String());
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-LIST
 *******************************************************/
GlNodeDataList::GlNodeDataList()
{
}

GlNodeDataList::GlNodeDataList(const GlNodeDataList& o)
		: inherited(o)
{
	for (uint32 k = 0; k < o.m_Data.size(); k++) {
		GlNodeData*		d = o.m_Data[k]->Copy();
		if (d) m_Data.push_back(d);
	}
}

GlNodeDataList::~GlNodeDataList()
{
	DeleteContents();
}

uint32 GlNodeDataList::Size() const
{
	return uint32(m_Data.size());
}

uint32 GlNodeDataList::Size(NodeDataType type) const
{
	uint32		count = 0;
	for (uint32 k = 0; k < m_Data.size(); k++) {
		if (m_Data[k]->Type() == type)
			count++;
	}
	return count;
}

BString16 GlNodeDataList::ErrorAt(uint32 index)
{
	uint32		count = 0;
	for (uint32 k = 0; k < m_Data.size(); k++) {
		if (m_Data[k]->Type() == ERROR_TYPE) {
			if (index == count)
				return ((GlNodeDataError*)m_Data[k])->Error();
			count++;
		}
	}
	return BString16();
}

GlNodeData* GlNodeDataList::DataAt(uint32 index, NodeDataType type) const
{
	if (type == NO_TYPE) {
		if (index >= m_Data.size()) return NULL;
		return const_cast<GlNodeData*>(m_Data[index]);
	}
	uint32		count = 0;
	for (uint32 k = 0; k < m_Data.size(); k++) {
		if (type == m_Data[k]->Type()) {
			if (index == count) {
				return const_cast<GlNodeData*>(m_Data[k]);
			}
			count++;
		}
	}
	return NULL;
}

GlCache1d* GlNodeDataList::Cache1dAt(uint32 index)
{
	return (GlCache1d*)TypeAt(CACHE1D_TYPE, index);
}

GlImage* GlNodeDataList::ImageAt(uint32 index)
{
	return (GlImage*)TypeAt(IMAGE_TYPE, index);
}

GlPath* GlNodeDataList::LineAt(uint32 index)
{
	return (GlPath*)TypeAt(LINE_TYPE, index);
}

BString16* GlNodeDataList::TextAt(uint32 index)
{
	return (BString16*)TypeAt(TEXT_TYPE, index);
}

status_t GlNodeDataList::AddData(GlNodeData* data)
{
	ArpASSERT(data);
	m_Data.push_back(data);
	return B_OK;
}

status_t GlNodeDataList::AddCache1d(GlCache1d* c)
{
	ArpVALIDATE(c, return B_ERROR);
	GlNodeDataCache1d*	data = new GlNodeDataCache1d(c);
	if (!data) return B_NO_MEMORY;
	return AddData(data);
}

status_t GlNodeDataList::AddImage(GlImage* im)
{
	ArpVALIDATE(im, return B_ERROR);
	GlNodeDataImage*	data = new GlNodeDataImage(im);
	if (!data) return B_NO_MEMORY;
	return AddData(data);
}

status_t GlNodeDataList::AddLine(GlPath* p)
{
	ArpVALIDATE(p, return B_ERROR);
	GlNodeDataLine*	data = new GlNodeDataLine(p);
	if (!data) return B_NO_MEMORY;
	return AddData(data);
}

status_t GlNodeDataList::AddText(const BString16& text)
{
	GlNodeDataText*	data = new GlNodeDataText(text);
	if (!data) return B_NO_MEMORY;
	return AddData(data);
}

GlCache1d* GlNodeDataList::DetachCache1d()
{
	GlNodeData*		d = DetachAt(CACHE1D_TYPE);
	if (!d) return 0;
	GlCache1d*		c = ((GlNodeDataCache1d*)d)->Give();
	delete d;
	return c;
}

GlImage* GlNodeDataList::DetachImage()
{
	GlNodeData*		d = DetachAt(IMAGE_TYPE);
	if (!d) return 0;
	GlImage*		i = ((GlNodeDataImage*)d)->DetachImage();
	delete d;
	return i;
}

GlPath* GlNodeDataList::DetachLine()
{
	GlNodeData*		d = DetachAt(LINE_TYPE);
	if (!d) return 0;
	GlPath*			p = ((GlNodeDataLine*)d)->DetachLine();
	delete d;
	return p;
}

status_t GlNodeDataList::ReplaceData(uint32 index, GlNodeData* data)
{
	if (index < 0 || index >= m_Data.size()) return B_ERROR;
	m_Data[index] = data;
	return B_OK;
}

status_t GlNodeDataList::MergeList(GlNodeDataList* list)
{
	if (!list) return B_ERROR;
	int			size = list->Size();
	for (int k = 0; k < size; k++) {
		GlNodeData*		d = list->DataAt(k);
		if (d) AddData(d);
	}
	list->m_Data.resize(0);
	return B_OK;
}

static bool scrub_port(GlNodeDataList* list, GlNodeData::NodeDataType t, bool del)
{
	for (uint32 k = 0; k < list->Size(); k++) {
		GlNodeData*		data = list->DataAt(k);
		if (data && (t == GlNodeData::NO_TYPE || t == data->Type())) {
			if (list->RemoveData(data) == B_OK) {
				if (del) delete data;
				return true;
			}
		}
	}
	return false;
}

status_t GlNodeDataList::DeleteContents(NodeDataType type)
{
	if (type == NO_TYPE) {
		for (uint32 k = 0; k < m_Data.size(); k++) {
			m_Data[k]->DeleteContents();
			delete m_Data[k];
		}
		m_Data.resize(0);
		return B_OK;
	}
	while (scrub_port(this, type, true)) ;
	return B_OK;
}

status_t GlNodeDataList::DeleteContents(uint32 start, uint32 stop)
{
	if (m_Data.size() < 1) return B_ERROR;
	if (stop >= m_Data.size()) stop = uint32(m_Data.size() - 1);
	if (start > stop) return B_ERROR;

	for (uint32 k = start; k <= stop; k++) {
		m_Data[k]->DeleteContents();
		delete m_Data[k];
	}
	m_Data.erase(m_Data.begin() + start, m_Data.begin() + stop);

	return B_OK;
}

status_t GlNodeDataList::RemoveData(GlNodeData* data)
{
	for (uint32 k = 0; k < m_Data.size(); k++) {
		if (m_Data[k] == data) {
			m_Data.erase(m_Data.begin() + k);
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t GlNodeDataList::RemoveContents(NodeDataType type)
{
	if (type == NO_TYPE) {
		m_Data.resize(0);
		return B_OK;
	}
	while (scrub_port(this, type, false)) ;
	return B_OK;
}

status_t GlNodeDataList::Parse(bool recurse)
{
	for (uint32 k = 0; k < m_Data.size(); k++) {
		if (m_Data[k]) {
			if (m_Data[k]->Type() != LIST_TYPE || recurse) {
				status_t	err = m_Data[k]->Parse(recurse);
				if (err != B_OK) return err;
			}
		}
	}
	return B_OK;
}

GlNodeData* GlNodeDataList::Copy() const
{
	return new GlNodeDataList(*this);
}

bool GlNodeDataList::Empty() const
{
	return m_Data.size() < 1;
}

status_t GlNodeDataList::AsNoMemoryError()
{
	return AsError("No memory");
}

status_t GlNodeDataList::AsError(const BString16& err)
{
	DeleteContents();
	GlNodeDataError*		de = new GlNodeDataError(err);
	if (de && AddData(de) != B_OK) delete de;
	return B_ERROR;
}

void* GlNodeDataList::TypeAt(NodeDataType type, uint32 index)
{
	uint32		count = 0;
	for (uint32 k = 0; k < m_Data.size(); k++) {
		ArpASSERT(m_Data[k]);
		if (m_Data[k] && m_Data[k]->Type() == type) {
			if (index == count) {
				// Flippin' Windows...  it likes to throw
				// exceptions on dynamic casts, I've no idea why.
				if (type == CACHE1D_TYPE) {
					GlNodeDataCache1d*		data = (GlNodeDataCache1d*)(m_Data[k]);
					if (data) return (void*)data->Cache();
				} else if (type == IMAGE_TYPE) {
					GlNodeDataImage*		data = (GlNodeDataImage*)(m_Data[k]);
					if (data) return (void*)data->Image();
				} else if (type == MASK_TYPE) {
					GlNodeDataMask*		data = (GlNodeDataMask*)(m_Data[k]);
					if (data) return (void*)data->Mask();
				} else if (type == TEXT_TYPE) {
					GlNodeDataText*		data = (GlNodeDataText*)(m_Data[k]);
					if (data) return (void*)&(data->mText);
				}
				return 0;
			}
			count++;
		}
	}
	return 0;
}

GlNodeData* GlNodeDataList::DetachAt(NodeDataType type)
{
	GlNodeData*		d = DataAt(0, type);
	if (!d) return 0;
	if (RemoveData(d) == B_OK) return d;
	return 0;
}

void GlNodeDataList::Print(uint32 tabs) const
{
	uint32		k;
	for (k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataList size %ld\n", m_Data.size());
	
	for (k = 0; k < m_Data.size(); k++) {
		if (m_Data[k]) m_Data[k]->Print(tabs + 1);
	}
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-IMAGE
 *******************************************************/
GlNodeDataImage::GlNodeDataImage()
		: mImage(0)
{
}

GlNodeDataImage::GlNodeDataImage(GlImage* im)
		: mImage(im)
{
}

GlNodeDataImage::GlNodeDataImage(const GlNodeDataImage& o)
		: inherited(o), mImage(0)
{
	if (o.mImage) mImage = o.mImage->Clone();
}

GlNodeDataImage::~GlNodeDataImage()
{
	delete mImage;
}

GlNodeData* GlNodeDataImage::Copy() const
{
	return new GlNodeDataImage(*this);
}

GlImage* GlNodeDataImage::Image() const
{
	return mImage;
}

void GlNodeDataImage::SetImage(GlImage* im)
{
	delete mImage;
	mImage = im;
}

GlImage* GlNodeDataImage::DetachImage()
{
	GlImage*		ans = mImage;
	mImage = 0;
	return ans;
}

void GlNodeDataImage::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataImage ");
	if (!mImage) printf("NO IMAGE\n");
	else {
		printf("width: %ld height: %ld (%p)\n", mImage->Width(), mImage->Height(), mImage);
	}
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-ALGO-1D
 *******************************************************/
GlNodeDataCache1d::GlNodeDataCache1d()
		: mCache(0)
{
}

GlNodeDataCache1d::GlNodeDataCache1d(GlCache1d* c)
		: mCache(c)
{
}

GlNodeDataCache1d::GlNodeDataCache1d(const GlNodeDataCache1d& o)
		: inherited(o), mCache(0)
{
	if (o.mCache) mCache = o.mCache->Clone();
}

GlNodeDataCache1d::~GlNodeDataCache1d()
{
	delete mCache;
}

GlNodeData* GlNodeDataCache1d::Copy() const
{
	return new GlNodeDataCache1d(*this);
}

GlCache1d* GlNodeDataCache1d::Cache() const
{
	return mCache;
}

void GlNodeDataCache1d::Take(GlCache1d* c)
{
	delete mCache;
	mCache = c;
}

GlCache1d* GlNodeDataCache1d::Give()
{
	GlCache1d*		ans = mCache;
	mCache = 0;
	return ans;
}

void GlNodeDataCache1d::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataCache1d");
	if (mCache) printf(" w %ld h %ld", mCache->w, mCache->h);
	printf("\n");
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-LINE
 *******************************************************/
GlNodeDataLine::GlNodeDataLine()
		: mPath(0)
{
}

GlNodeDataLine::GlNodeDataLine(GlPath* p)
		: mPath(p)
{
}

GlNodeDataLine::GlNodeDataLine(const GlNodeDataLine& o)
		: inherited(o), mPath(0)
{
	if (o.mPath) mPath = o.mPath->Clone();
}

GlNodeDataLine::~GlNodeDataLine()
{
	delete mPath;
}

GlNodeData* GlNodeDataLine::Copy() const
{
	return new GlNodeDataLine(*this);
}

GlPath* GlNodeDataLine::Line() const
{
	return mPath;
}

void GlNodeDataLine::SetLine(GlPath* p)
{
	delete mPath;
	mPath = p;
}

GlPath* GlNodeDataLine::DetachLine()
{
	GlPath*			ans = mPath;
	mPath = 0;
	return ans;
}

void GlNodeDataLine::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	uint32			size = 0;
	if (mPath) size = mPath->size;
	printf("GlNodeDataLine size %ld\n", size);
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-MASK
 *******************************************************/
GlNodeDataMask::GlNodeDataMask()
		: mMask(0)
{
}

GlNodeDataMask::GlNodeDataMask(GlMask* m)
		: mMask(m)
{
}

GlNodeDataMask::GlNodeDataMask(const GlNodeDataMask& o)
		: inherited(o), mMask(0)
{
	ArpASSERT(false);
//	if (o.mMask) mMask = o.mMask->Clone();
}

GlNodeDataMask::~GlNodeDataMask()
{
	delete mMask;
}

GlNodeData* GlNodeDataMask::Copy() const
{
	return new GlNodeDataMask(*this);
}

GlMask* GlNodeDataMask::Mask() const
{
	return mMask;
}

void GlNodeDataMask::SetMask(GlMask* m)
{
	delete mMask;
	mMask = m;
}

GlMask* GlNodeDataMask::DetachMask()
{
	GlMask*			ans = mMask;
	mMask = 0;
	return ans;
}

void GlNodeDataMask::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataMask ");
	if (!mMask) printf("NO MASK\n");
	else printf("\n");
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-TEXT
 *******************************************************/
GlNodeDataText::GlNodeDataText()
{
}

GlNodeDataText::GlNodeDataText(const BString16& text)
		: mText(text)
{
}

GlNodeDataText::GlNodeDataText(const GlNodeDataText& o)
		: inherited(o), mText(o.mText)
{
}

GlNodeDataText::~GlNodeDataText()
{
}

GlNodeData* GlNodeDataText::Copy() const
{
	return new GlNodeDataText(*this);
}

BString16 GlNodeDataText::Text() const
{
	return mText;
}

void GlNodeDataText::SetText(const BString16& text)
{
	mText = text;
}

void GlNodeDataText::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataText '%s'\n", mText.String());
}

// #pragma mark -

/*******************************************************
 * GL-NODE-DATA-BACK-POINTER
 *******************************************************/
GlNodeDataBackPointer::GlNodeDataBackPointer()
		: image(0)
{
}

GlNodeDataBackPointer::GlNodeDataBackPointer(const GlImage* img)
		: image(img)
{
}

GlNodeDataBackPointer::GlNodeDataBackPointer(const GlNodeDataBackPointer& o)
		: inherited(o), image(o.image)
{
}

GlNodeDataBackPointer::~GlNodeDataBackPointer()
{
}

GlNodeData* GlNodeDataBackPointer::Copy() const
{
	return new GlNodeDataBackPointer(*this);
}

void GlNodeDataBackPointer::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlNodeDataBackPointer img %s\n", (image) ? "yes" : "no");
}
