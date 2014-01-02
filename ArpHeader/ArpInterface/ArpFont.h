/* ArpFont.h
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
 * 2002.07.18			hackborn@angryredplanet.com
 * Created this file.
 */
 
#ifndef ARPINTERFACE_ARPFONT_H
#define ARPINTERFACE_ARPFONT_H

#include <ArpCore/String16.h>
#include <app/Message.h>
#include <interface/Font.h>

enum ArpFontStyle {
	ARP_PLAIN_STYLE			= 0x00000000,
	ARP_BOLD_STYLE			= 0x00000001,
	ARP_ITALIC_STYLE		= 0x00000002,
	ARP_UNDERLINE_STYLE		= 0x00000004
};

/***************************************************************************
 * ARP-FONT
 ****************************************************************************/
class ArpFont
{
public:
	ArpFont();
	ArpFont(const BString16* family, int32 style);
	ArpFont(const ArpFont& o);
	virtual ~ArpFont();

	ArpFont&			operator=(const ArpFont& o);

	BString16		Family() const;
	int32			Style() const;
	float			Size() const;
	/* Style can be either -1 for 'leave style alone', 0 for regular, or
	 * one of the ArpFontStyle enumerations.
	 */
	void			SetFamilyAndStyle(const BString16* family, int32 style = ARP_PLAIN_STYLE);
	void			SetSize(float size);
	status_t		GetHeight(	float* outAscent, float* outDescent,
								float* outLeading) const;
		
	status_t		WriteTo(BMessage& msg) const;
	status_t		ReadFrom(const BMessage& msg);
	
private:
	friend class	ArpPainter;
	
	BString16		mFamily;
	int32			mStyle;
	float			mSize;
	
	BFont			mFont;

	void			CacheFont();
	void			UncacheFont();
};


#endif
