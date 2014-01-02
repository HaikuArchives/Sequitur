#include "ArpKernel/ArpBitmapTools.h"

#include <BeExp/BitmapTools.h>
#include <Bitmap.h>

#include <stdio.h>

static inline void overlay_color_func(rgb_color* target, const rgb_color other, uint8 amount)
{
	const uint8 alphaMix = (uint8)( ((int16(other.alpha)-int16(255-target->alpha))*amount)/255
									+ (255-target->alpha) );
	target->red = (uint8)( ((int16(other.red)-int16(target->red))*alphaMix)/255
								+ target->red );
	target->green = (uint8)( ((int16(other.green)-int16(target->green))*alphaMix)/255
								+ target->green );
	target->blue = (uint8)( ((int16(other.blue)-int16(target->blue))*alphaMix)/255
								+ target->blue );
	target->alpha = other.alpha > target->alpha ? other.alpha : target->alpha;
}

rgb_color overlay_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	overlay_color_func(&color1, color2, amount);
	return color1;
}


namespace ARP {
	static color_space fix_color_space(color_space space)
	{
		switch (space) {
			case B_RGB32:		return B_RGBA32;
			case B_RGB32_BIG:	return B_RGBA32_BIG;
			case B_RGB15:		return B_RGBA15;
			case B_RGB15_BIG:	return B_RGBA15_BIG;
			default:			return space;
		}
	}
	
	class color_overlay
	{
	public:
		color_overlay(BBitmap* out, const BBitmap* in, uint8 amount)
			: fOutAccess(fix_color_space(out->ColorSpace())),
			  fInAccess(fix_color_space(in->ColorSpace())),
			  fAmount(amount),
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
			return false;
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
				fOutAccess.write(dest,
					overlay_color(fOutAccess.read(dest), fInAccess.read(data), fAmount));
				
				data += fInAccess.bpp();
				dest += fOutAccess.bpp();
				
				if( ++line_pos >= fWidth ) {
					data += data_jump;
					dest += dest_jump;
					line_pos = 0;
				}
			}
			
			return data == data_end ? B_OK : B_ERROR;
		}
		
	private:
		pixel_access fOutAccess;
		pixel_access fInAccess;
		
		uint8 fAmount;
		
		size_t fWidth, fHeight;
		
		size_t fOutBPR, fInBPR;
		size_t fOutLength, fInLength;
		
		uint8* fOutBits;
		const uint8* fInBits;
	};
}

using namespace ARP;


status_t overlay_bitmap(BBitmap* behind, const BBitmap* overlay, uint8 amount)
{
	color_overlay setter(behind, overlay, amount);
	return setter.run();
}

// ---------------------------------------------------------------------------

static inline void tint_color_func(rgb_color* target, const rgb_color other)
{
	target->red = (uint8)( ((int16(other.red)-int16(target->red))*other.alpha)/255
								+ target->red );
	target->green = (uint8)( ((int16(other.green)-int16(target->green))*other.alpha)/255
								+ target->green );
	target->blue = (uint8)( ((int16(other.blue)-int16(target->blue))*other.alpha)/255
								+ target->blue );
}

rgb_color tint_color(rgb_color color1, rgb_color color2)
{
	tint_color_func(&color1, color2);
	return color1;
}


namespace ARP {
	class color_tint
	{
	public:
		color_tint(BBitmap* out, rgb_color tint)
			: fOutAccess(fix_color_space(out->ColorSpace())),
			  fTint(tint),
			  fWidth(int32(out->Bounds().Width()+1.5)),
			  fHeight(int32(out->Bounds().Height()+1.5)),
			  fOutBPR(out->BytesPerRow()),
			  fOutLength(out->BitsLength()),
			  fOutBits((uint8*)out->Bits())
		{
		}
		
		bool valid() const
		{
			return fOutAccess.valid();
			return false;
		}
		
		status_t run()
		{
			if( !valid() ) return B_BAD_VALUE;
			
			uint8*			dest = fOutBits;
			uint8*			const dest_end = fOutBits+fOutLength;
			
			const size_t	dest_jump = fOutBPR - fWidth*fOutAccess.bpp();
			size_t			line_pos = 0;
			
			while( dest < dest_end ) {
				fOutAccess.write(dest, tint_color(fOutAccess.read(dest), fTint));
				
				dest += fOutAccess.bpp();
				
				if( ++line_pos >= fWidth ) {
					dest += dest_jump;
					line_pos = 0;
				}
			}
			
			return B_OK;
		}
		
	private:
		pixel_access fOutAccess;
		pixel_access fInAccess;
		
		rgb_color fTint;
		
		size_t fWidth, fHeight;
		
		size_t fOutBPR;
		size_t fOutLength;
		
		uint8* fOutBits;
	};
}

using namespace ARP;


status_t tint_bitmap(BBitmap* base, rgb_color tint)
{
	color_tint setter(base, tint);
	return setter.run();
}
