/* AmViewProperty.cpp
 */
#include "ArpLayout/ArpScrollArea.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmKernel/fake_file.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include <experimental/ResourceSet.h>
#include <png.h>

class _FakeLayoutSettings : public AmFilterConfigLayout
{
public:
	_FakeLayoutSettings(	AmFilterHolderI* target,
							const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		Implementation().RefreshControls(mSettings);
	}
};


void do_fakeness() {

	ArpRangeControl		fakeControl( BRect(0, 0, 0, 0), "ctrl", 0 );

	const BMessage		fakeMessage('fuck');
	_FakeLayoutSettings	fakeSettings( 0, fakeMessage );

	BView				fakeView( BRect(0, 0, 0, 0), "name", 0, 0 );
	ArpViewWrapper		fakeViewWrapper( &fakeView );
	ArpScrollArea		fakeScrollArea;
	
	ArpKnobControl		knob( BRect(0, 0, 0, 0), "name", 0, 0, 0 );
	
	BResourceSet 		blah;
	
	png_structp			dest_png;
	png_infop			dest_info;
	
	// Horrible awful quick hack to get stuff from libpng.a pulled in.
	dest_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	dest_info = png_create_info_struct(dest_png);
}
