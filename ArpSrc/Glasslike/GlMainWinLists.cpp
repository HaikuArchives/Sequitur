#include <be/StorageKit.h>
#include <be/support/Autolock.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpInterface/ViewTools.h>
#include <GlPublic/GlImage.h>
#include <Glasslike/GlMainWinLists.h>

/***************************************************************************
 * GL-MAIN-LABEL-ENTRY
 ***************************************************************************/
GlMainLabelEntry::GlMainLabelEntry(const BString16& l, gl_id i)
		: label(l), id(i), code(0)
{
}

GlMainLabelEntry::GlMainLabelEntry(const BString16& l, int32 c)
		: label(l), id(0), code(c)
{
}

// #pragma mark -

/***************************************************************************
 * GL-MAIN-LABELS
 ***************************************************************************/
GlMainLabels::GlMainLabels()
{
}

GlMainLabels::~GlMainLabels()
{
	Free();
}

void GlMainLabels::DrawOn(BRect clip, BView* v, gl_chain_id active)
{
	v->SetHighColor(100, 100, 100);

	for (uint32 k = 0; k < e.size(); k++) {
		GlMainLabelEntry*	le = e[k];
		if (le && le->label.String()) {
			if (active == le->id) v->SetHighColor(0, 0, 0);
			v->DrawString(le->label.String(), le->frame.LeftBottom());
			if (active == le->id) v->SetHighColor(100, 100, 100);
		}
	}
}

const GlMainLabelEntry* GlMainLabels::EntryAt(BPoint pt, int32* outIndex) const
{
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k]->frame.Contains(pt)) {
			if (outIndex) *outIndex = k;
			return e[k];
		}
	}
	return 0;
}

GlMainLabelEntry* GlMainLabels::EntryAt(BPoint pt, int32* outIndex)
{
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k]->frame.Contains(pt)) {
			if (outIndex) *outIndex = k;
			return e[k];
		}
	}
	return 0;
}

void GlMainLabels::Free()
{
	for (uint32 k = 0; k < e.size(); k++) delete e[k];
	e.resize(0);
}

// #pragma mark -

/***************************************************************************
 * GL-MAIN-IMAGE-ENTRY
 ***************************************************************************/
GlMainImageEntry::GlMainImageEntry(	gl_id inId, const ArpBitmap* img,
									uint32 cs, uint32 f, BString16* l)
		: image(img), id(inId), chainSize(cs), flags(f)
{
	if (l) label = *l;
}

// #pragma mark -

/***************************************************************************
 * GL-MAIN-IMAGES
 ***************************************************************************/
GlMainImages::GlMainImages()
{
}

GlMainImages::~GlMainImages()
{
	DeleteCache();
}

const GlMainImageEntry* GlMainImages::At(int32 index) const
{
	ArpVALIDATE(index >= 0 && index < int32(e.size()), return 0);
	return e[index];
}

GlMainImageEntry* GlMainImages::At(int32 index)
{
	ArpVALIDATE(index >= 0 && index < int32(e.size()), return 0);
	return e[index];
}

const GlMainImageEntry* GlMainImages::EntryAt(BPoint pt, int32* outIndex) const
{
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k]->frame.Contains(pt)) {
			if (outIndex) *outIndex = k;
			return e[k];
		}
	}
	return 0;
}

GlMainImageEntry* GlMainImages::EntryAt(BPoint pt, int32* outIndex)
{
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k]->frame.Contains(pt)) {
			if (outIndex) *outIndex = k;
			return e[k];
		}
	}
	return 0;
}

const GlMainImageEntry* GlMainImages::EntryAt(gl_id id, int32* outIndex) const
{
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k] && id == e[k]->id) {
			if (outIndex) *outIndex = k;
			return e[k];
		}
	}
	return 0;
}

GlMainImageEntry* GlMainImages::EntryAt(gl_id id, int32* outIndex)
{
	for (uint32 k = 0; k < e.size(); k++) {
		if (e[k] && id == e[k]->id) {
			if (outIndex) *outIndex = k;
			return e[k];
		}
	}
	return 0;
}

void GlMainImages::DeleteCache()
{
	for (uint32 k = 0; k < e.size(); k++) delete e[k];
	e.resize(0);
}
