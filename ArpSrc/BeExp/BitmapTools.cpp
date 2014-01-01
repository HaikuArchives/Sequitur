/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "BitmapTools.h"
#include "ColorTools.h"

#include <Bitmap.h>
#include <Screen.h>

#include <ByteOrder.h>
#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

namespace BExperimentalPrivate {

	class color_scaler : public pixel_access
	{
	public:
		color_scaler(BBitmap* out, const BBitmap* in)
			: pixel_access(in->ColorSpace()),
			  fOutWidth(int32(out->Bounds().Width()+1.5)),
			  fOutHeight(int32(out->Bounds().Height()+1.5)),
			  fInWidth(int32(in->Bounds().Width()+1.5)),
			  fInHeight(int32(in->Bounds().Height()+1.5)),
			  fOutBPR(out->BytesPerRow()), fInBPR(in->BytesPerRow()),
			  fOutBits((uint8*)out->Bits()),
			  fInBits((const uint8*)in->Bits()),
			  fXPos(0), fYPos(0),
			  fXOff(0), fYOff(0), fXLast(0), fYLast(0)
		{
		}
		
		int32 next_row()
		{
			fXPos++;
			fXLast = fXOff;
			fXOff = (fInWidth*fXPos) / fOutWidth;
			return fXOff - 1;
		}
		
		int32 next_col()
		{
			fYPos++;
			fXPos = 0;
			fXOff = fXLast = 0;
			fYLast = fYOff;
			fYOff = (fInHeight*fYPos) / fOutHeight;
			return fYOff - 1;
		}
		
		int32 rows() const				{ return fOutWidth; }
		int32 columns() const			{ return fOutHeight; }
		
		int32 left() const				{ return fXLast; }
		int32 right() const				{ return fXOff > fXLast ? fXOff-1 : fXLast; }
		int32 top() const				{ return fYLast; }
		int32 bottom() const			{ return fYOff > fYLast ? fYOff-1 : fYLast; }
		
		bool has_pixels() const			{ return fXOff>fXLast && fYOff>fYLast; }
		
		void start_coloring()
		{
			fRed = fGreen = fBlue = fAlpha = fColorCount = fAlphaCount = 0;
		}
		
		void add_color(rgb_color color)
		{
			fRed += (color.red*color.alpha)/255;
			fGreen += (color.green*color.alpha)/255;
			fBlue += (color.blue*color.alpha)/255;
			fAlpha += color.alpha;
			fColorCount += color.alpha;
			fAlphaCount += 1;
		}
		
		rgb_color final_color() const
		{
			if( fColorCount == 0 || fAlphaCount == 0 ) return B_TRANSPARENT_COLOR;
			rgb_color col;
			col.red = (fRed*255)/fColorCount;
			col.green = (fGreen*255)/fColorCount;
			col.blue = (fBlue*255)/fColorCount;
			col.alpha = fAlpha/fAlphaCount;
			return col.alpha != 0 ? col : B_TRANSPARENT_COLOR;
		}
		
		status_t run()
		{
			uint8* sOut = fOutBits;
			const uint8* sIn = fInBits;
			
			// yes, this is horribly horribly inefficient.  oh well.
			for( int32 y=0; y<columns(); y++ ) {
				next_col();
				uint8* pOut = sOut;
				for( int32 x=0; x<rows(); x++ ) {
					next_row();
					PRINT(("Scaling (%d, %d) to (%d, %d)\n",
							left(), top(), x, y));
					start_coloring();
					const uint8* lIn = sIn + (left()*bpp()) + top()*fInBPR;
					const uint8* rIn = lIn + (right()-left())*bpp();
					const uint8* bIn = lIn + (bottom()-top())*fInBPR;
					PRINT(("In Pos = %p, Right = %p, Bottom = %p\n",
							lIn, rIn, bIn));
					while( lIn <= bIn ) {
						const uint8* pIn = lIn;
						PRINT(("Starting row %p to %p\n", lIn, rIn));
						while( pIn<=rIn ) {
							rgb_color color = read(pIn);
							pIn += bpp();
							add_color(color);
						}
						lIn += fInBPR;
						rIn += fInBPR;
					}
					write(pOut, final_color());
					pOut += bpp();
				}
				sOut += fOutBPR;
			}
			
			return B_OK;
		}
		
	private:
		int32 fOutWidth, fOutHeight;
		int32 fInWidth, fInHeight;
		
		size_t fOutBPR, fInBPR;
		
		uint8* fOutBits;
		const uint8* fInBits;
		
		int32 fXPos, fYPos;
		
		int32 fXOff, fYOff;
		int32 fXLast, fYLast;
		
		uint32 fRed;
		uint32 fGreen;
		uint32 fBlue;
		uint32 fAlpha;
		uint32 fColorCount;
		uint32 fAlphaCount;
	};

	// ----------------------------------------------------------------------
	
	class color_setter
	{
	public:
		color_setter(BBitmap* out, const BBitmap* in)
			: fOutAccess(out->ColorSpace()), fInAccess(in->ColorSpace()),
			  fWidth(int32(out->Bounds().Width()+1.5)),
			  fHeight(int32(out->Bounds().Height()+1.5)),
			  fOutBPR(out->BytesPerRow()), fInBPR(in->BytesPerRow()),
			  fOutLength(out->BitsLength()), fInLength(in->BitsLength()),
			  fOutBits((uint8*)out->Bits()),
			  fInBits((const uint8*)in->Bits())
		{
		}
		
		bool valid() const
		{
			return fOutAccess.valid() && fInAccess.valid();
		}
		
		status_t run()
		{
			if( !valid() ) return B_BAD_VALUE;
			
			uint8*			dest = fOutBits;
			uint8*			const dest_end = fOutBits+fOutLength;
			const uint8*	data = fInBits;
			const uint8*	const data_end = fInBits+fInLength;
			
			const size_t	data_jump = fInBPR - fWidth*fInAccess.bpp();
			const size_t	dest_jump = fOutBPR - fWidth*fOutAccess.bpp();
			size_t			line_pos = 0;
			
			while( dest < dest_end && data < data_end ) {
		
				fOutAccess.write(dest, fInAccess.read(data));
				data += fInAccess.bpp();
				dest += fOutAccess.bpp();
				
				if( ++line_pos >= (size_t)fWidth ) {
					data += data_jump;
					dest += dest_jump;
					line_pos = 0;
				}
			}
			
			return data == data_end ? B_OK : B_ERROR;
		}
		
		status_t run_dither()
		{
			if( !valid() ) return B_BAD_VALUE;
			
			uint8*			dest = fOutBits;
			uint8*			const dest_end = fOutBits+fOutLength;
			const uint8*	data = fInBits;
			const uint8*	const data_end = fInBits+fInLength;
			
			const size_t	data_jump = fInBPR - fWidth*fInAccess.bpp();
			const size_t	dest_jump = fOutBPR - fWidth*fOutAccess.bpp();
			size_t			line_pos = 0;
			
			rgb_color		a_color;
			int32			error_r = 0;
			int32			error_g = 0;
			int32			error_b = 0;
			int32			error_a = 0;
			
			while( dest < dest_end && data < data_end ) {
		
				a_color = fInAccess.read(data);
				data += fInAccess.bpp();
				
				error_r += a_color.red;
				error_g += a_color.green;
				error_b += a_color.blue;
				error_a += a_color.alpha;
				
				a_color.red = (error_r > 255 ? 255 : (error_r < 0 ? 0 : error_r));
				a_color.green = (error_g > 255 ? 255 : (error_g < 0 ? 0 : error_g));
				a_color.blue = (error_b > 255 ? 255 : (error_b < 0 ? 0 : error_b));
				a_color.alpha = (error_a > 255 ? 255 : (error_a < 0 ? 0 : error_a));
				
				fOutAccess.write(dest, a_color);
				
				if( ++line_pos >= (size_t)fWidth ) {
					data += data_jump;
					dest += fOutAccess.bpp() + dest_jump;
					line_pos = 0;
					error_r = error_g = error_b = error_a = 0;
					
				} else {
					a_color = fOutAccess.read(dest);
					dest += fOutAccess.bpp();
					error_r -= a_color.red;
					error_g -= a_color.green;
					error_b -= a_color.blue;
					error_a -= a_color.alpha;
				}
			}
			
			return data == data_end ? B_OK : B_ERROR;
		}
		
	private:
		pixel_access fOutAccess;
		pixel_access fInAccess;
		
		int32 fWidth, fHeight;
		
		size_t fOutBPR, fInBPR;
		size_t fOutLength, fInLength;
		
		uint8* fOutBits;
		const uint8* fInBits;
	};
	
	// ---------------------- RGBA32
	
	static rgb_color rgba32_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		color.blue = *pixel++;
		color.green = *pixel++;
		color.red = *pixel++;
		color.alpha = *pixel;
		return color;
	}
	
	static void rgba32_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*pixel++ = color.blue;
		*pixel++ = color.green;
		*pixel++ = color.red;
		*pixel = color.alpha;
	}
	
	// ---------------------- RGBA32 Big
	
	static rgb_color rgba32big_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		color.alpha = *pixel++;
		color.red = *pixel++;
		color.green = *pixel++;
		color.blue = *pixel;
		return color;
	}
	
	static void rgba32big_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*pixel++ = color.alpha;
		*pixel++ = color.red;
		*pixel++ = color.green;
		*pixel = color.blue;
	}
	
	// ---------------------- RGB32
	
	static rgb_color rgb32_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		color.blue = *pixel++;
		color.green = *pixel++;
		color.red = *pixel;
		color.alpha = 255;
		return color;
	}
	
	static void rgb32_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*pixel++ = color.blue;
		*pixel++ = color.green;
		*pixel++ = color.red;
		*pixel = 255;
	}
	
	// ---------------------- RGB32 Big
	
	static rgb_color rgb32big_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		color.alpha = 255; pixel++;
		color.red = *pixel++;
		color.green = *pixel++;
		color.blue = *pixel;
		return color;
	}
	
	static void rgb32big_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*pixel++ = 255;
		*pixel++ = color.red;
		*pixel++ = color.green;
		*pixel = color.blue;
	}
	
	// ---------------------- RGB16
	
	static rgb_color rgb16_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		const uint16 c = B_LENDIAN_TO_HOST_INT16(*(uint16*)pixel);
		color.red = (((c>>11)&0x1f)*0xff)/0x1f;
		color.green = (((c>>5)&0x3f)*0xff)/0x3f;
		color.blue = ((c&0x1f)*0xff)/0x1f;
		color.alpha = 255;
		return color;
	}
	
	static void rgb16_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*(uint16*)pixel = B_HOST_TO_LENDIAN_INT16(
				(((uint16)color.red & 0xf8) << 8) |
				(((uint16)color.green & 0xfc) << 3) |
				((color.blue & 0xf8) >> 3)
			);
	}
	
	// ---------------------- RGB16 Big
	
	static rgb_color rgb16big_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		const uint16 c = B_BENDIAN_TO_HOST_INT16(*(uint16*)pixel);
		color.red = (((c>>11)&0x1f)*0xff)/0x1f;
		color.green = (((c>>5)&0x3f)*0xff)/0x3f;
		color.blue = ((c&0x1f)*0xff)/0x1f;
		color.alpha = 255;
		return color;
	}
	
	static void rgb16big_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*(uint16*)pixel = B_HOST_TO_BENDIAN_INT16(
				(((uint16)color.red & 0xf8) << 8) |
				(((uint16)color.green & 0xfc) << 3) |
				((color.blue & 0xf8) >> 3)
			);
	}
	
	// ---------------------- RGBA15
	
	static rgb_color rgba15_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		const uint16 c = B_LENDIAN_TO_HOST_INT16(*(uint16*)pixel);
		color.red = (((c>>10)&0x1f)*0xff)/0x1f;
		color.green = (((c>>5)&0x1f)*0xff)/0x1f;
		color.blue = ((c&0x1f)*0xff)/0x1f;
		color.alpha = c&0x8000 ? 255 : 0;
		return color;
	}
	
	static void rgba15_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*(uint16*)pixel = B_HOST_TO_LENDIAN_INT16(
				(((uint16)color.red & 0xf8) << 7) |
				(((uint16)color.green & 0xf8) << 2) |
				((color.blue & 0xf8) >> 3) |
				(color.alpha >= 128 ? 0x8000 : 0)
			);
	}

	// ---------------------- RGBA15 Big
	
	static rgb_color rgba15big_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		const uint16 c = B_BENDIAN_TO_HOST_INT16(*(uint16*)pixel);
		color.red = (((c>>10)&0x1f)*0xff)/0x1f;
		color.green = (((c>>5)&0x1f)*0xff)/0x1f;
		color.blue = ((c&0x1f)*0xff)/0x1f;
		color.alpha = c&0x8000 ? 255 : 0;
		return color;
	}
	
	static void rgba15big_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*(uint16*)pixel = B_HOST_TO_BENDIAN_INT16(
				(((uint16)color.red & 0xf8) << 7) |
				(((uint16)color.green & 0xf8) << 2) |
				((color.blue & 0xf8) >> 3) |
				(color.alpha >= 128 ? 0x8000 : 0)
			);
	}

	// ---------------------- RGB15
	
	static rgb_color rgb15_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		const uint16 c = B_LENDIAN_TO_HOST_INT16(*(uint16*)pixel);
		color.red = (((c>>10)&0x1f)*0xff)/0x1f;
		color.green = (((c>>5)&0x1f)*0xff)/0x1f;
		color.blue = ((c&0x1f)*0xff)/0x1f;
		color.alpha = 255;
		return color;
	}
	
	static void rgb15_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*(uint16*)pixel = B_HOST_TO_LENDIAN_INT16(
				(((uint16)color.red & 0xf8) << 7) |
				(((uint16)color.green & 0xf8) << 2) |
				((color.blue & 0xf8) >> 3) |
				0x8000
			);
	}

	// ---------------------- RGB15 Big
	
	static rgb_color rgb15big_pixel_reader(const uint8* pixel, const color_map*)
	{
		rgb_color color;
		const uint16 c = B_BENDIAN_TO_HOST_INT16(*(uint16*)pixel);
		color.red = (((c>>10)&0x1f)*0xff)/0x1f;
		color.green = (((c>>5)&0x1f)*0xff)/0x1f;
		color.blue = ((c&0x1f)*0xff)/0x1f;
		color.alpha = 255;
		return color;
	}
	
	static void rgb15big_pixel_writer(uint8* pixel, const rgb_color color, const color_map*)
	{
		*(uint16*)pixel = B_HOST_TO_BENDIAN_INT16(
				(((uint16)color.red & 0xf8) << 7) |
				(((uint16)color.green & 0xf8) << 2) |
				((color.blue & 0xf8) >> 3) |
				0x8000
			);
	}

	// ---------------------- CMAP8
	
	static rgb_color cmap8_pixel_reader(const uint8* pixel, const color_map* cmap)
	{
		if( *pixel == B_TRANSPARENT_MAGIC_CMAP8 ) return B_TRANSPARENT_COLOR;
		return cmap->color_list[*pixel];
	}
	
	static void cmap8_pixel_writer(uint8* pixel, const rgb_color color, const color_map* cmap)
	{
		if( color.alpha < 128 ) {
			*pixel = B_TRANSPARENT_MAGIC_CMAP8;
			return;
		}
		const int index = ((color.red & 0xf8) << 7) |
						  ((color.green & 0xf8) << 2) |
						  ((color.blue & 0xf8) >> 3) ;
		*pixel = cmap->index_map[index];
	}

}	// namespace BExperimentalPrivate

using namespace BExperimentalPrivate;

// ----------------------------------------------------------------------

pixel_access::pixel_access()
{
	fColorMap = 0;
	fBPP = 1;
	fReader = (pixel_reader)NULL;
	fWriter = (pixel_writer)NULL;
}

pixel_access::pixel_access(color_space space)
{
	fColorMap = system_colors();
	set_to(space);
}

status_t pixel_access::set_to(color_space space)
{
	switch( space ) {
		case B_RGBA32:
		{
			fBPP = 4;
			fReader = rgba32_pixel_reader;
			fWriter = rgba32_pixel_writer;
		} break;
		
		case B_RGBA32_BIG:
		{
			fBPP = 4;
			fReader = rgba32big_pixel_reader;
			fWriter = rgba32big_pixel_writer;
		} break;
		
		case B_RGB32:
		{
			fBPP = 4;
			fReader = rgb32_pixel_reader;
			fWriter = rgb32_pixel_writer;
		} break;
		
		case B_RGB32_BIG:
		{
			fBPP = 4;
			fReader = rgb32big_pixel_reader;
			fWriter = rgb32big_pixel_writer;
		} break;
		
		case B_RGB16:
		{
			fBPP = 2;
			fReader = rgb16_pixel_reader;
			fWriter = rgb16_pixel_writer;
		} break;
		
		case B_RGB16_BIG:
		{
			fBPP = 2;
			fReader = rgb16big_pixel_reader;
			fWriter = rgb16big_pixel_writer;
		} break;
		
		case B_RGBA15:
		{
			fBPP = 2;
			fReader = rgba15_pixel_reader;
			fWriter = rgba15_pixel_writer;
		} break;
		
		case B_RGBA15_BIG:
		{
			fBPP = 2;
			fReader = rgba15big_pixel_reader;
			fWriter = rgba15big_pixel_writer;
		} break;
		
		case B_RGB15:
		{
			fBPP = 2;
			fReader = rgb15_pixel_reader;
			fWriter = rgb15_pixel_writer;
		} break;
		
		case B_RGB15_BIG:
		{
			fBPP = 2;
			fReader = rgb15big_pixel_reader;
			fWriter = rgb15big_pixel_writer;
		} break;
		
		case B_CMAP8:
		{
			fBPP = 1;
			fReader = cmap8_pixel_reader;
			fWriter = cmap8_pixel_writer;
		} break;
		
		default: {
			TRESPASS();
			fBPP = 1;
			fReader = (pixel_reader)NULL;
			fWriter = (pixel_writer)NULL;
			return B_BAD_VALUE;
		}
	}
	
	return B_OK;
}

pixel_access::~pixel_access()
{
}

bool pixel_access::valid() const
{
	return (fColorMap != 0) && (fReader != 0) && (fWriter != 0);
}

// ----------------------------------------------------------------------

namespace BExperimental {

// ----------------------------------------------------------------------

status_t mix_bitmaps(BBitmap* out,
					 const BBitmap* b1, const BBitmap* b2, uint8 amount)
{
	if( out->BitsLength() != b1->BitsLength() ||
		out->BitsLength() != b2->BitsLength() ||
		out->ColorSpace() != b1->ColorSpace() ||
		out->ColorSpace() != b2->ColorSpace() ) {
		TRESPASS();
		return B_BAD_VALUE;
	}
	
	switch( out->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		case B_RGB24:
		case B_GRAY8:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB24_BIG:
		{
			uint8* sOut = (uint8*)out->Bits();
			uint8* eOut = sOut + out->BitsLength();
			uint8* sB1 = (uint8*)b1->Bits();
			uint8* sB2 = (uint8*)b2->Bits();
			
			while( sOut < eOut ) {
				*sOut = (uint8)( ( ((uint16)*sB1)*(255-amount)
								 + ((uint16)*sB2)*(amount)
								 ) / 255 );
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		case B_RGB16:
		{
			uint16* sOut = (uint16*)out->Bits();
			uint16* eOut = sOut + out->BitsLength()/2;
			uint16* sB1 = (uint16*)b1->Bits();
			uint16* sB2 = (uint16*)b2->Bits();
			
			while( sOut < eOut ) {
				const uint16 b1 = *sB1;
				const uint16 b2 = *sB2;
				uint16 b = ( (b1&0x1F)*(255-amount)
								+ (b2&0x1F)*(amount) ) / 255;
				uint16 g = ( ((b1>>5)&0x3F)*(255-amount)
								+ ((b2>>5)&0x3F)*(amount) ) / 255;
				uint16 r = ( ((b1>>11)&0x1F)*(255-amount)
								+ ((b2>>11)&0x1F)*(amount) ) / 255;
				*sOut = b | (g<<5) | (r<<11);
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		case B_RGB15:
		case B_RGBA15:
		{
			uint16* sOut = (uint16*)out->Bits();
			uint16* eOut = sOut + out->BitsLength()/2;
			uint16* sB1 = (uint16*)b1->Bits();
			uint16* sB2 = (uint16*)b2->Bits();
			
			while( sOut < eOut ) {
				const uint16 b1 = *sB1;
				const uint16 b2 = *sB2;
				if( b1 == B_TRANSPARENT_MAGIC_RGBA15 ) {
					*sOut = b2;
				} else if( b2 == B_TRANSPARENT_MAGIC_RGBA15 ) {
					*sOut = b1;
				} else {
					uint16 b = ( (b1&0x1F)*(255-amount)
									+ (b2&0x1F)*(amount) ) / 255;
					uint16 g = ( ((b1>>5)&0x1F)*(255-amount)
									+ ((b2>>5)&0x1F)*(amount) ) / 255;
					uint16 r = ( ((b1>>10)&0x1F)*(255-amount)
									+ ((b2>>10)&0x1F)*(amount) ) / 255;
					*sOut = b | (g<<5) | (r<<10) | 0x8000;
				}
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		case B_CMAP8:
		{
			uint8* sOut = (uint8*)out->Bits();
			uint8* eOut = sOut + out->BitsLength();
			uint8* sB1 = (uint8*)b1->Bits();
			uint8* sB2 = (uint8*)b2->Bits();
			
			BScreen s;
			const color_map* cm = system_colors();
			
			while( sOut < eOut ) {
				if( *sB1 == B_TRANSPARENT_MAGIC_CMAP8 ) {
					*sOut = *sB2;
				} else if( *sB2 == B_TRANSPARENT_MAGIC_CMAP8 ) {
					*sOut = *sB1;
				} else {
					rgb_color c1 = cm->color_list[*sB1];
					rgb_color c2 = cm->color_list[*sB2];
					c1.red = (uint8)( ( ((uint16)c1.red)*(255-amount)
										+ ((uint16)c2.red)*(amount)
									) / 255 );
					c1.green = (uint8)( ( ((uint16)c1.green)*(255-amount)
										+ ((uint16)c2.green)*(amount)
									) / 255 );
					c1.blue = (uint8)( ( ((uint16)c1.blue)*(255-amount)
										+ ((uint16)c2.blue)*(amount)
									) / 255 );
					*sOut = cm->index_map[
								((c1.red&0xF8)<<7)
							|	((c1.green&0xF8)<<2)
							|	((c1.blue&0xF8)>>3)
							];
				}
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		default:
			TRESPASS();
			return B_BAD_VALUE;
	}
	
	return B_OK;
}

// ----------------------------------------------------------------------

status_t scale_bitmap(BBitmap* dest, const BBitmap* in)
{
	BBitmap* out = dest;
	if( out->ColorSpace() != in->ColorSpace() ) {
		out = new BBitmap(dest->Bounds(), in->ColorSpace());
	}
	
	color_scaler scale(out, in);
	if( !scale.valid() ) return B_BAD_VALUE;
	
	status_t err = scale.run();
	
	if( out != dest ) {
		if( err == B_OK ) err = set_bitmap(dest, out);
		delete out;
	}
	
	return err;
}

status_t blend_bitmap_color(BBitmap* dest, rgb_color color, uint8 amount)
{
	pixel_access pa(dest->ColorSpace());
	
	if (!pa.valid()) return B_BAD_VALUE;

	const BRect b(dest->Bounds());
	int32 columns = int32(b.Width()+1)*pa.bpp();
	
	uint8* data = (uint8*)dest->Bits();
	uint8* end = ((uint8*)dest->Bits()) + dest->BitsLength() - columns;
	
	while (data <= end) {
		uint8* pos = data;
		uint8* end = data + columns;
		while (pos < end) {
			rgb_color c = pa.read(pos);
			// Skip the common case of no change.
			if (c.alpha != 255 || amount != 0) {
				// Another common case of complete replacement.
				if (c.alpha == 0 && color.alpha == 255) {
					pa.write(pos, color);
				} else {
					c = blend_color(c, color, amount);
					if (c.alpha < color.alpha) c.alpha = color.alpha;
					pa.write(pos, c);
				}
			}
			pos += pa.bpp();
		}
		data = end;
	}
	
	return B_OK;
}

uint8* make_alpha_channel(const BBitmap* src, rgb_color background,
						  uint8 slop)
{
	pixel_access pa(src->ColorSpace());
	if (!pa.valid()) return NULL;
	
	const BRect b(src->Bounds());
	int32 columns = int32(b.Width()+1);
	int32 lines = int32(b.Height()+1);
	
	uint8* alpha = (uint8*)malloc(columns*lines);
	if (!alpha) return NULL;
	
	uint8* data = (uint8*)src->Bits();
	uint8* end = ((uint8*)src->Bits()) + src->BitsLength();
	
	if (background.alpha == 0) background = B_TRANSPARENT_COLOR;
	
	uint8* apos = alpha;
	
	for (int32 y=0; y<lines && data < end; y++) {
		uint8* sd = data;
		uint8* se = data + columns*pa.bpp();
		while (sd < se) {
			rgb_color c(pa.read(sd));
			if (c.alpha == 0) c = B_TRANSPARENT_COLOR;
			if (slop == 0) {
				*apos = (c == background ? 0 : 255);
			} else {
				int32 diff	= abs((int32)c.red-(int32)background.red)
							+ abs((int32)c.green-(int32)background.green)
							+ abs((int32)c.blue-(int32)background.blue)
							+ abs((int32)c.alpha-(int32)background.alpha);
				diff = (diff+3)/4;
				if (diff <= slop) *apos = (uint8)((diff*255)/slop);
				else *apos = 255;
			}
			sd += pa.bpp();
			apos++;
		}
		data += src->BytesPerRow();
	}
	
	return alpha;
}

static
status_t copy_bitmap_generic(BBitmap* dest,
							 const BBitmap* src, BRect srcRect, BPoint destPnt,
							 drawing_mode mode, uint8* alpha)
{
	pixel_access dpa(dest->ColorSpace());
	pixel_access spa(src->ColorSpace());
	
	if (!dpa.valid() || !spa.valid()) return B_BAD_VALUE;
	
	const size_t alphaBytesPerRow = (int32)(src->Bounds().Width())+1;
	if (alpha) {
		alpha += (int32)srcRect.left + ((int32)srcRect.top)*alphaBytesPerRow;
	}
	
	int32 srcLineStart = int32(srcRect.left)*spa.bpp();
	int32 destLineStart = int32(destPnt.x)*dpa.bpp();
	int32 columns = int32(srcRect.Width()+1)*spa.bpp();
	int32 lines = int32(srcRect.Height()+1);
	
	uint8* srcData = ((uint8*)src->Bits()) + srcLineStart
				   + int32(srcRect.top)*src->BytesPerRow();
	uint8* srcEnd = ((uint8*)src->Bits()) + src->BitsLength();
	uint8* destData = ((uint8*)dest->Bits()) + destLineStart
					+ int32(destPnt.y)*dest->BytesPerRow();
	uint8* destEnd = ((uint8*)dest->Bits()) + dest->BitsLength();
	
	if (mode == B_OP_COPY) {
		for (int32 y=0; y<lines && srcData < srcEnd && destData < destEnd; y++) {
			uint8* sd = srcData;
			uint8* ed = sd + columns;
			uint8* dd = destData;
			while (sd < ed) {
				const rgb_color c(spa.read(sd));
				dpa.write(dd, c);
				sd += spa.bpp();
				dd += dpa.bpp();
			}
			srcData += src->BytesPerRow();
			destData += dest->BytesPerRow();
		}
	} else if (mode == B_OP_OVER) {
		for (int32 y=0; y<lines && srcData < srcEnd && destData < destEnd; y++) {
			uint8* sd = srcData;
			uint8* ed = sd + columns;
			uint8* dd = destData;
			uint8* ad = alpha;
			while (sd < ed) {
				if (ad) {
					if (*ad++ >= 128) {
						const rgb_color c(spa.read(sd));
						dpa.write(dd, c);
					}
				} else {
					const rgb_color c(spa.read(sd));
					if (c.alpha >= 128) dpa.write(dd, c);
				}
				sd += spa.bpp();
				dd += dpa.bpp();
			}
			if (alpha) alpha += alphaBytesPerRow;
			srcData += src->BytesPerRow();
			destData += dest->BytesPerRow();
		}
	} else {
		for (int32 y=0; y<lines && srcData < srcEnd && destData < destEnd; y++) {
			uint8* sd = srcData;
			uint8* ed = sd + columns;
			uint8* dd = destData;
			uint8* ad = alpha;
			while (sd < ed) {
				const rgb_color sc(spa.read(sd));
				const uint8 amount = alpha ? (*ad) : sc.alpha;
				const rgb_color dc(dpa.read(dd));
				const rgb_color final(blend_color(dc, sc, amount));
				dpa.write(dd, final.alpha > 0 ? final : B_TRANSPARENT_COLOR);
				ad++;
				sd += spa.bpp();
				dd += dpa.bpp();
			}
			if (alpha) alpha += alphaBytesPerRow;
			srcData += src->BytesPerRow();
			destData += dest->BytesPerRow();
		}
	}
	
	return B_OK;
}

status_t copy_bitmap(BBitmap* dest,
					 const BBitmap* src, BRect srcRect, BPoint destPnt,
					 drawing_mode mode, uint8* alpha)
{
	srcRect.left = floor(srcRect.left+.5);
	srcRect.top = floor(srcRect.top+.5);
	srcRect.right = floor(srcRect.right+.5);
	srcRect.bottom = floor(srcRect.bottom+.5);
	destPnt.x = floor(destPnt.x+.5);
	destPnt.y = floor(destPnt.y+.5);
	
	const BRect destRect = dest->Bounds();
	
	if( destPnt.x < 0 ) {
		srcRect.left -= destPnt.x;
		destPnt.x = 0;
	}
	if( destPnt.y < 0 ) {
		srcRect.top -= destPnt.y;
		destPnt.y = 0;
	}
	if( srcRect.left < 0 ) {
		destPnt.x -= srcRect.left;
		srcRect.left = 0;
	}
	if( srcRect.top < 0 ) {
		destPnt.y -= srcRect.top;
		srcRect.top = 0;
	}
	if( (destPnt.x+srcRect.Width()) > destRect.right ) {
		srcRect.right = destRect.right-destPnt.x;
	}
	if( (destPnt.y+srcRect.Height()) > destRect.bottom ) {
		srcRect.bottom = destRect.bottom-destPnt.y;
	}
	
	if( dest->ColorSpace() != src->ColorSpace() || mode != B_OP_COPY || alpha ) {
		return copy_bitmap_generic(dest, src, srcRect, destPnt, mode, alpha);
	}
	
	int32 bytesPerPixel = 1;
	
	switch( dest->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			bytesPerPixel = 4;
			break;
			
		case B_RGB24:
		case B_RGB24_BIG:
			bytesPerPixel = 3;
			break;
			
		case B_RGB16:
		case B_RGB15:
		case B_RGBA15:
			bytesPerPixel = 2;
			break;
			
		case B_GRAY8:
		case B_CMAP8:
			bytesPerPixel = 1;
			break;
			
		default:
			return copy_bitmap_generic(dest, src, srcRect, destPnt, mode, alpha);
	}
	
	int32 srcLineStart = int32(srcRect.left)*bytesPerPixel;
	int32 destLineStart = int32(destPnt.x)*bytesPerPixel;
	int32 lineBytes = int32(srcRect.right-srcRect.left+1)*bytesPerPixel;
	if( srcLineStart+lineBytes > src->BytesPerRow() ) {
		lineBytes = src->BytesPerRow()-srcLineStart;
	}
	if( destLineStart+lineBytes > dest->BytesPerRow() ) {
		lineBytes = dest->BytesPerRow()-destLineStart;
	}
	if( lineBytes <= 0 ) return B_OK;
	
	uint8* srcData = ((uint8*)src->Bits()) + srcLineStart
				   + int32(srcRect.top)*src->BytesPerRow();
	uint8* srcEnd = ((uint8*)src->Bits()) + src->BitsLength();
	uint8* destData = ((uint8*)dest->Bits()) + destLineStart
					+ int32(destPnt.y)*dest->BytesPerRow();
	uint8* destEnd = ((uint8*)dest->Bits()) + dest->BitsLength();
	
	while( srcData < srcEnd && destData < destEnd ) {
		memcpy(destData, srcData, lineBytes);
		srcData += src->BytesPerRow();
		destData += dest->BytesPerRow();
	}
	
	return B_OK;
}

// ---------------------------------------------------------------------

static status_t set_bits_x_to_x(BBitmap* dest_bm,
								const BBitmap* src_bm)
{
	uint8*		dest = (uint8*)dest_bm->Bits();
	uint8*		dest_end = dest+dest_bm->BitsLength();
	const uint8*data = (uint8*)src_bm->Bits();
	const uint8*data_end = data+src_bm->BitsLength();
	const size_t line_len = dest_bm->BytesPerRow() < src_bm->BytesPerRow()
						  ? dest_bm->BytesPerRow() : src_bm->BytesPerRow();
	
	while( dest < dest_end && data < data_end ) {
		memcpy(dest, data, line_len);
		dest += dest_bm->BytesPerRow();
		data += src_bm->BytesPerRow();
	}
	
	return data == data_end ? B_OK : B_ERROR;
}

status_t set_bitmap(BBitmap* dest, const BBitmap* src, bool dither)
{
	if( dest->ColorSpace() == src->ColorSpace() ) {
		return set_bits_x_to_x(dest, src);
	}
	
	color_setter setter(dest, src);
	status_t err = dither ? setter.run_dither() : setter.run();
	if( err == B_OK ) return err;
	
	TRESPASS();
	
	dest->SetBits(src->Bits(), src->BitsLength(), 0, src->ColorSpace());
	return B_OK;
}

}	// namespace BExperimental
