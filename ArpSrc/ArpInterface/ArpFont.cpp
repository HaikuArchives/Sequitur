#include <ArpKernel/ArpDebug.h>
#include <ArpInterface/ArpFont.h>

static const char*	FONTFAMILY		= "fam";
static const char*	FONTSTYLE		= "stl";
static const char*	FONTSIZE		= "siz";

static uint16 be_face_from_arp_style(int32 newStyle, int32 oldStyle);
static int32 arp_style_from_be_face(uint16 face);

/***************************************************************************
 * ARP-FONT
 ****************************************************************************/
ArpFont::ArpFont()
		: mFamily("")
{
	UncacheFont();
}

ArpFont::ArpFont(const BString16* family, int32 style)
		: mFamily(""), mStyle(style)
{
	if (family) mFamily = *family;
	mSize = mFont.Size();
	CacheFont();
}

ArpFont::ArpFont(const ArpFont& o)
		: mFamily(o.mFamily), mStyle(o.mStyle), mSize(o.mSize)
{
	CacheFont();
}

ArpFont::~ArpFont()
{
}

ArpFont& ArpFont::operator=(const ArpFont& o)
{
	mFamily = o.mFamily;
	mStyle = o.mStyle;
	mSize = o.mSize;
	CacheFont();
	return *this;
}

BString16 ArpFont::Family() const
{
	return mFamily;
}

int32 ArpFont::Style() const
{
	return mStyle;
}

float ArpFont::Size() const
{
	return mSize;
}

void ArpFont::SetFamilyAndStyle(const BString16* family, int32 style)
{
	if (family) mFamily = family;
	uint16		face = be_face_from_arp_style(style, mStyle);
	mStyle = style;

	mFont.SetFamilyAndFace(&mFamily, face);
}

void ArpFont::SetSize(float size)
{
	mSize = size;
	mFont.SetSize(size);
}

status_t ArpFont::GetHeight(float* outAscent, float* outDescent,
							float* outLeading) const
{
	font_height			fh;
	mFont.GetHeight(&fh);
	if (outAscent) *outAscent = fh.ascent;
	if (outDescent) *outDescent = fh.descent;
	if (outLeading) *outLeading = fh.leading;
	return B_OK;
}

status_t ArpFont::WriteTo(BMessage& msg) const
{
	status_t		err;
	if ((err = msg.AddString16(FONTFAMILY, mFamily)) != B_OK) return err;
	if ((err = msg.AddInt32(FONTSTYLE, mStyle)) != B_OK) return err;
	if ((err = msg.AddFloat(FONTSIZE, mSize)) != B_OK) return err;
	return B_OK;
}

status_t ArpFont::ReadFrom(const BMessage& msg)
{
	BString16		str;
	if (msg.FindString16(FONTFAMILY, &str) == B_OK)
		mFamily = str;
	int32			i32;
	if (msg.FindInt32(FONTSTYLE, &i32) == B_OK)
		mStyle = i32;
	float			f;
	if (msg.FindFloat(FONTSIZE, &f) == B_OK)
		mSize = f;
	CacheFont();
	return B_OK;
}

void ArpFont::CacheFont()
{
	uint16		face = be_face_from_arp_style(mStyle, 0);
	mFont.SetFamilyAndFace(mFamily.String(), face);
	mFont.SetSize(mSize);
}

void ArpFont::UncacheFont()
{
	BString16		family;
	mFont.GetFamilyAndStyle(&family, NULL);
	mFamily = family;
	mStyle = arp_style_from_be_face(mFont.Face());
	mSize = mFont.Size();
}

/***************************************************************************
 * Miscellaneous functions
 ****************************************************************************/
static uint16 be_face_from_arp_style(int32 newStyle, int32 oldStyle)
{
	uint16				face = 0;
	if (newStyle < 0) face = uint16(oldStyle);
	else if (newStyle == 0) face = B_REGULAR_FACE;
	else {
		if (newStyle&ARP_BOLD_STYLE) face |= B_BOLD_FACE;
		if (newStyle&ARP_ITALIC_STYLE) face |= B_ITALIC_FACE;
		if (newStyle&ARP_UNDERLINE_STYLE) face |= B_UNDERSCORE_FACE;
		if (newStyle == 0) face = B_REGULAR_FACE;
	}
	return face;
}

static int32 arp_style_from_be_face(uint16 face)
{
	int32		style = 0;
	if (face&B_BOLD_FACE) style |= ARP_BOLD_STYLE;
	if (face&B_ITALIC_FACE) style |= ARP_ITALIC_STYLE;
	if (face&B_UNDERSCORE_FACE) style |= ARP_UNDERLINE_STYLE;
	return style;
}
