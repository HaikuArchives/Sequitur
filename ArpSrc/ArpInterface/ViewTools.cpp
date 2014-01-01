#include <be/app/Message.h>
#include <be/interface/Font.h>
#include <be/interface/Window.h>
#include <ArpInterface/ViewTools.h>

int32 arp_get_mouse_buttons(BView& view)
{
	int32		i32;
	if (view.Window()->CurrentMessage()->FindInt32("buttons", &i32) == B_OK)
		return i32;
	return 0;
}

static float arp_get_font_height(const BFont* font)
{
	if (!font) return 0;
	font_height		fh;
	font->GetHeight(&fh);
	return fh.ascent + fh.descent + fh.leading;
}

float arp_get_font_height(const BView* view)
{
	if (!view) return 0;
	BFont	font;
	view->GetFont(&font);
	return arp_get_font_height(&font);
}

float arp_get_string_width(const BView* view, const char* text)
{
	if (!view || !text) return 0;
	return view->StringWidth(text);
}

float arp_get_string_width(const BView* view, const BString16* text)
{
	ArpVALIDATE(view && text, return 0);
	if (!view || !(text->String())) return 0;
	return view->StringWidth(text);
}

void arp_set_frame(BView& view, BRect f)
{
	BRect			oldF = view.Frame();
	if (oldF.left != f.left || oldF.top != f.top)
		view.MoveTo(f.left, f.top);
	if (oldF.Width() != f.Width() || oldF.Height() != f.Height())
		view.ResizeTo(f.Width(), f.Height());
}
