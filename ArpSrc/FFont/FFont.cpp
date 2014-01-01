/*
 * FFont.h: Flattenable font implementation.
 *
 * Written by Dianne Hackborn <hackbod@lucent.com>, with contributions
 * from Marco Nelissen, Jon Watte, Attila Mezei, Jon Ragnarsson,
 * Rainer Riedl, David Mirchell, Ingmar Krusch, and others on the
 * Be Developer Talk mailing list.
 *
 * This code is released to the public domain, and may be used however
 * you see fit.  However, if you make enhancements, I ask that you get
 * in contact with me (Dianne), so that they can be rolled into the
 * official version and made available to others.  I can be contacted
 * through e-mail at either <hackbod@lucent.com> or
 * <hackbod@enterct.com>.
 *
 */
 
#ifndef _FFONT_H
#include <FFont/FFont.h>
#endif

#ifndef _BYTEORDER_H
#include <be/support/ByteOrder.h>
#endif

#include <string.h>

// ----------------------- Constructors -----------------------

FFont::FFont()
	: attrMask(B_FONT_ALL) { }
FFont::FFont(const BFont &font)
	: BFont(font), attrMask(B_FONT_ALL) { }
FFont::FFont(const BFont *font)
	: BFont(font), attrMask(B_FONT_ALL) { }

FFont::FFont(const FFont &font)
	: BFont(font), attrMask(font.attrMask) { }
FFont::FFont(const FFont *font)
	: BFont(font), attrMask(font ? font->attrMask : B_FONT_ALL) { }

// ----------------------- Assignment -----------------------

FFont& FFont::operator=(const BFont& font)
{
	this->BFont::operator=(font);
	attrMask = B_FONT_ALL;
	return *this;
}

FFont& FFont::operator=(const FFont& font)
{
	this->BFont::operator=(font);
	attrMask = font.attrMask;
	return *this;
}

// ----------------------- Attribute Mask -----------------------

void FFont::SetMask(uint32 mask)
{
	attrMask = mask;
}

uint32 FFont::Mask(void) const
{
	return attrMask;
}

void FFont::UpdateTo(BFont* font, uint32 mask) const
{
	if( !font ) return;
	
	mask &= attrMask;
	if( (mask&B_FONT_ALL) == B_FONT_ALL ) {
		// Quickly copy everything.
		*font = *(BFont*)this;
		return;
	}
	
	// Only copy specific fields.
	if( mask & B_FONT_FAMILY_AND_STYLE ) {
		font->SetFamilyAndStyle(FamilyAndStyle());
	}
	if( mask & B_FONT_SIZE ) {
		font->SetSize(Size());
	}
	if( mask & B_FONT_SHEAR ) {
		font->SetShear(Shear());
	}
	if( mask & B_FONT_ROTATION ) {
		font->SetRotation(Rotation());
	}
	if( mask & B_FONT_SPACING ) {
		font->SetSpacing(Spacing());
	}
	if( mask & B_FONT_ENCODING ) {
		font->SetEncoding(Encoding());
	}
	if( mask & B_FONT_FACE ) {
		font->SetFace(Face());
	}
	if( mask & B_FONT_FLAGS ) {
		font->SetFlags(Flags());
	}
}

void FFont::UpdateTo(FFont* font, uint32 mask) const
{
	if( !font ) return;
	
	UpdateTo((BFont*)font,mask);
	
	font->attrMask |= mask&attrMask;
}

void FFont::UpdateFrom(const FFont* font, uint32 mask)
{
	if( !font ) return;
	font->UpdateTo(this,mask);
}

// ----------------------- BFlattenable Interface -----------------------

bool FFont::IsFixedSize() const
{ return true; }

type_code FFont::TypeCode() const
{ return FONT_TYPE; }

ssize_t FFont::FlattenedSize() const
{ return sizeof(flat_font_data); }

bool FFont::AllowsTypeCode(type_code code) const
{ return code == FONT_TYPE; }

status_t
FFont::Flatten(void *buffer, ssize_t size) const
{
	if( size < (ssize_t)sizeof(flat_font_data) ) return B_BAD_VALUE;
		
	// Easy reference to the buffer.
	flat_font_data* fdat = (flat_font_data*)buffer;
	memset(fdat,0,sizeof(*fdat));
	
	// Stash away name of family and style.
	GetFamilyAndStyle(&fdat->family,&fdat->style);
	
	// This is used as a temporary when byte-swapping floats.
	// Note that we are assuming a float is 4 bytes.
	union {
		uint32 aslong;
		float asfloat;
	} swap;
	
	// Byte-swap size, shear, and rotation into the flattened
	// structure.  This is written for clarity more than speed,
	// since the additional overhead will be entirely subsumed
	// by everything else going on.
	swap.asfloat = Size();
	swap.aslong = htonl(swap.aslong);
	fdat->size = swap.asfloat;
	swap.asfloat = Shear();
	swap.aslong = htonl(swap.aslong);
	fdat->shear = swap.asfloat;
	swap.asfloat = Rotation();
	swap.aslong = htonl(swap.aslong);
	fdat->rotation = swap.asfloat;
	
	// Byte-swap the remaining data into the flattened structure.
	fdat->flags = htonl(Flags());
	fdat->face = htons(Face());
	fdat->spacing = Spacing();
	fdat->encoding = Encoding();
	fdat->mask = htonl(Mask());
	
	return B_NO_ERROR;
}

status_t
FFont::Unflatten(type_code c, const void *buf, ssize_t size)
{
	if( c != FONT_TYPE ) return B_BAD_TYPE;
	
	// Make sure buffer contains all data.  If we later want
	// to add new attributes, we can make a more sophisticated
	// check to allow partial structures.
	if( size < (ssize_t)sizeof(flat_font_data) ) return B_BAD_VALUE;
	
	// Easy reference to the buffer.
	flat_font_data* fdat = (flat_font_data*)buf;

	// Initialize from default font, just in case.
	*this = FFont();
	
	// Set up family and style for font.
	SetFamilyAndStyle(fdat->family,fdat->style);
	
	// This is used as a temporary when byte-swapping floats.
	// Note that this assumes a float is 4 bytes.
	union {
		uint32 aslong;
		float asfloat;
	} swap;
	
	// Byte-swap size, shear, and rotation out of the flattened
	// structure.  This is written for clarity more than speed,
	// since the additional overhead will be entirely subsumed
	// by everything else going on.
	swap.asfloat = fdat->size;
	swap.aslong = ntohl(swap.aslong);
	SetSize(swap.asfloat);
	swap.asfloat = fdat->shear;
	swap.aslong = ntohl(swap.aslong);
	SetShear(swap.asfloat);
	swap.asfloat = fdat->rotation;
	swap.aslong = ntohl(swap.aslong);
	SetRotation(swap.asfloat);
	
	// Byte-swap the remaining data from the flattened structure.
	SetFlags(ntohl(fdat->flags));
	SetFace(ntohs(fdat->face));
	SetSpacing(fdat->spacing);
	SetEncoding(fdat->encoding);
	SetMask(ntohl(fdat->mask));
	
	return B_NO_ERROR;
}

// -------------------- BMessage Convenience Functions --------------------

status_t
AddMessageFont(BMessage* msg, const char *name, const FFont *font)
{
	if( !msg || !font ) return B_BAD_VALUE;
	// Have to make a temporary because AddFlat()'s BFlattenable
	// argument isn't typed as const.  Why is that?!?
	FFont ffont(font);
	return msg->AddFlat(name, &ffont);
}

status_t
FindMessageFont(const BMessage* msg, const char *name,
                int32 index, FFont *font)
{
	if( !msg || !font ) return B_BAD_VALUE;
	status_t result = msg->FindFlat(name, index, font);
	return result;
}

status_t
AddMessageFont(BMessage* msg, const char *name, const BFont *font)
{
	if( !msg || !font ) return B_BAD_VALUE;
	FFont ffont(font);
	return msg->AddFlat(name, &ffont);
}

status_t
FindMessageFont(const BMessage* msg, const char *name,
                int32 index, BFont *font)
{
	if( !msg || !font ) return B_BAD_VALUE;
	// Since the basic BFont doesn't keep track of the attribute mask,
	// make a temporary so that we can perform a partial attribute
	// update into the destination BFont.
	FFont ffont;
	status_t result = msg->FindFlat(name, index, &ffont);
	if( result == B_NO_ERROR ) ffont.UpdateTo(font);
	return result;
}
