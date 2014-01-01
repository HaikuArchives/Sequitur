/* SeqWindowStateI.cpp
 */
#include <stdio.h>
#include <be/interface/Screen.h>
#include "ArpKernel/ArpDebug.h"
#include "Sequitur/SeqWindowStateI.h"

/*************************************************************************
 * SEQ-WINDOW-STATE-I
*************************************************************************/
status_t SeqWindowStateI::GetDimensions(BMessage* config, BWindow* window) const
{
	ArpASSERT(config && window);
	BScreen 	screen(window);
	BRect		f = window->Frame(), sf = screen.Frame();

	status_t	err;
	if ((err = config->AddBool("f_minimized", window->IsMinimized() )) != B_OK) return err;
	if ((err = config->AddFloat("f_left", f.left / sf.Width() )) != B_OK) return err;
	if ((err = config->AddFloat("f_top", f.top / sf.Height() )) != B_OK) return err;
	if ((err = config->AddFloat("f_right", f.right / sf.Width() )) != B_OK) return err;
	if ((err = config->AddFloat("f_bottom", f.bottom / sf.Height() )) != B_OK) return err;
	return B_OK;
}

status_t SeqWindowStateI::SetDimensions(const BMessage* config, BWindow* window)
{
	ArpASSERT(config && window);
	bool		minimized;
	if (config->FindBool("f_minimized", &minimized) != B_OK) minimized = false;
	window->Minimize(minimized);

	BScreen		s(window);
	if (!s.IsValid() ) return B_ERROR;
	BRect		sf = s.Frame();
	float		l, t, r, b;
	if (config->FindFloat("f_left", &l) == B_OK
			&& config->FindFloat("f_top", &t) == B_OK
			&& config->FindFloat("f_right", &r) == B_OK
			&& config->FindFloat("f_bottom", &b) == B_OK ) {
		BRect	f( l * sf.Width(), t * sf.Height(), r * sf.Width(), b * sf.Height() );
		window->MoveTo(f.LeftTop() );
		window->ResizeTo(f.Width(), f.Height() );
//		window->MoveTo( l * sf.Width(), t * sf.Height() );
	}

	return B_OK;
}
