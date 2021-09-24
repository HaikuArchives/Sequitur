/*
 * FFont.h: Flattenable font interface.
 *
 * This code defines a subclass of BFont called FFont, which adds
 * a BFlattenable interface to the base BFont class.  This makes it
 * easy to pass BFont objects through messages, and assures that
 * all programs using this interface will be able to interoperate.
 *
 * There are also a few additional features; see FFont.html for more
 * information.
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
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• None known.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 1.0 (Sept. 18, 1997):
 *	First complete release, based on discussion on the Be Developer Talk
 *	mailing list.
 *
 */

#ifndef _ARP_FFONT_H
#define _ARP_FFONT_H

#ifndef _FONT_H
#include <Font.h>
#endif

#ifndef _FLATTENABLE_H
#include <Flattenable.h>
#endif

#ifndef _MESSAGE_H
#include <Message.h>
#endif

#ifndef _VIEW_H
#include <View.h>
#endif

#include <cstdio>

class FFont : public BFont, public BFlattenable
{
public:
	// Duplicate constructors from BFont.
	FFont();
	FFont(const BFont &font);
	FFont(const BFont *font);

	// New constructors that copy from another complete FFont object.
	FFont(const FFont &font);
	FFont(const FFont *font);
	
	// Update copy operators.
	FFont& operator=(const BFont &font); 
	FFont& operator=(const FFont &font); 
	
	// Internal mask control.
	void SetMask(uint32 mask);
	uint32 Mask(void) const;
	
	// Copy to/from another font, respecting the existing and given
	// mask bits.  This works the same way as the 'mask' parameter
	// of BView::SetFont().  That is,
	//
	//   "UpdateTo" copies this object into the given font object,
	//   only changing the those fields for which this object's
	//   Mask bit is set and the parameter 'mask' bit is set.
	//   If 'font' were a BView, it would be the same as doing
	//     view->SetFont(this, mask & this->Mask());
	//
	//   "UpdateFrom" does the same things as "UpdateTo", but in
	//   reverse: it copies the fields from the given 'font' into
	//   this font, only changing those for which both the mask
	//   bit in 'font' is set, and the bit in 'mask' is set.
	
	void UpdateTo(BFont* font, uint32 mask = B_FONT_ALL) const;
	void UpdateTo(FFont* font, uint32 mask = B_FONT_ALL) const;
	void UpdateFrom(const FFont* font, uint32 mask = B_FONT_ALL);
	
	// Now define the new BFlattenable interface.  First the
	// type code that a font is identified with.
	enum {
		FONT_TYPE = 'FONt'
	};

	// The BFlattenable methods.
	virtual	bool		IsFixedSize() const;
	virtual	type_code	TypeCode() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual	bool		AllowsTypeCode(type_code code) const;
	virtual	status_t	Flatten(void *buffer, ssize_t size) const;
	virtual	status_t	Unflatten(type_code c, const void *buf,
                                 ssize_t size);

	// Last is the definition of how a font is flattened,
	// which you should not normally have to worry about.
	//
	// **NOTE** Do -not- make subclasses of FFont that add
	// additional data to the end of this flattened representation.
	// The FONt data type reserves that space for any additions
	// it makes to its own data, so that it can preserve compatibility
	// with older code.
	struct flat_font_data {
		int32 mask;
		float size;
		float shear;
		float rotation;
		uint32 flags;
		uint16 face;
		uint8 spacing;
		uint8 encoding;
		font_family family;
		font_style style;
	};

	static void Test(void);
	
private:
	uint32 attrMask;
};

/*
 * Here is a simple interface for putting FFont objects into and
 * retrieving them from a BMessage.  They work just as true
 * BMessage methods would -- AddFont() and FindFont() -- except
 * that the BMessage object must be explicitly supplied.
 *
 * If FindMessageFont() encounters an error, 'font' will remain
 * unchanged.
 */
 
status_t
AddMessageFont(BMessage* msg, const char *name, const FFont *font);

status_t
FindMessageFont(const BMessage* msg, const char *name,
                int32 index, FFont *font);

/*
 * Finally, a similar interface for working with basic BFont
 * objects.  Since a BFont does not contain the FFont's special
 * usage mask, we define it to be handled as thus:
 *
 * AddMessageFont() writes a FONt type with a usage mask
 * containing B_FONT_ALL, meaning that all fields are defined.
 *
 * FindMessageFont() reads the FONt mask data, and uses
 * FFont::UpdateTo() to copy only those fields defined into
 * the given BFont object.
 */

status_t
AddMessageFont(BMessage* msg, const char *name, const BFont *font);

status_t
FindMessageFont(const BMessage* msg, const char *name,
                int32 index, BFont *font);

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

inline void FFont::Test(void)
{
	FFont font;
	printf("Default font:\n(A) ");
	font.PrintToStream();
	
	FFont font2;
	font2.SetSize(100);
	font2.SetShear(20);
	font2.SetRotation(180);
	font2.SetMask(font2.Mask() & ~(B_FONT_SHEAR|B_FONT_ROTATION));
	printf("Modified font:\n(B) ");
	font2.PrintToStream();
	
	FFont font3(font);
	printf("Initial font:\n(A) ");
	font3.PrintToStream();
	
	font3.UpdateFrom(&font2);
	printf("Update initial from modified, except shear and rotation:\n(C) ");
	font3.PrintToStream();
	
	font3 = font;
	printf("Initial font:\n(A) ");
	font3.PrintToStream();
	
	font2.UpdateTo(&font3);
	printf("Update modified into initial, except shear and rotation:\n(C) ");
	font3.PrintToStream();
	
	BMessage msg;
	AddMessageFont(&msg,"test",&font2);
	printf("Modifed placed into message:\n");
	msg.PrintToStream();
	
	FindMessageFont(&msg,"test",0,&font3);
	printf("Modifed retrieved from message:\n(B) ");
	font3.PrintToStream();
	
	BFont font4;
	FindMessageFont(&msg,"test",0,&font4);
	printf("Modifed retrieved into initial:\n(C) ");
	font4.PrintToStream();
}
#endif
