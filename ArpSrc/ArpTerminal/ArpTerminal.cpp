/*
	
	ArpTerminal.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPTERMINAL_ARPTERMINAL_H
#include "ArpTerminal/ArpTerminal.h"
#endif

#ifndef ARPTERMINAL_ARPTERMINALMSG_H
#include "ArpTerminal/ArpTerminalMsg.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef _AUTOLOCK_H
#include <be/support/Autolock.h>
#endif

#ifndef _CLIPBOARD_H
#include <be/app/Clipboard.h>
#endif

#ifndef _MENU_ITEM_H
#include <be/interface/MenuItem.h>
#endif

#ifndef _TEXT_VIEW_H
#include <be/interface/TextView.h>
#endif

#ifndef _SCROLL_VIEW_H
#include <be/interface/ScrollView.h>
#endif

#ifndef _SCROLL_BAR_H
#include <be/interface/ScrollBar.h>
#endif

#ifndef _BUTTON_H
#include <be/interface/Button.h>
#endif

#ifndef _BOX_H
#include <be/interface/Box.h>
#endif

#ifndef _WINDOW_H
#include <be/interface/Window.h>
#endif

#ifndef _APPLICATION_H
#include <be/app/Application.h>
#endif

#ifndef _UTF8_H
#include <be/support/UTF8.h>
#endif

#ifndef _PATH_H
#include <be/storage/Path.h>
#endif

#ifndef _ENTRY_H
#include <be/storage/Entry.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>

ArpMOD();

enum {
	TERM_CANCELPASTE = '.cps',
	TERM_DOPASTE = '.dps'
};

ArpTerminal::ArpTerminal(BRect frame, const char* name, ulong resizeMode, ulong flags)
	: ArpCoreTerminal(frame,name,resizeMode,flags),
	  curEmulator(NULL), defEmulator(*this),
	  pasteVerify(true), rmbPaste(true), wasNavigable(true),
	  pasteView(NULL), pasteText(NULL),
	  pasteScroll(NULL), cancelButton(NULL), pasteButton(NULL),
	  selHMode(0), selHShown(false)
{
	SetEmulator((ArpEmulatorInterface*)NULL);
}

ArpTerminal::~ArpTerminal()
{
	if( curEmulator != &defEmulator ) delete curEmulator;
}

bool ArpTerminal::CheckEmulationMenu(BMenu* inmenu) const
{
	const ArpEmulationType* type = Emulator().EmulationType();
	bool okay = false;
	if( inmenu && type ) {
		for( int i=0; i<inmenu->CountItems(); i++ ) {
			BMenuItem* item = inmenu->ItemAt(i);
			if( item ) {
				BMessage* msg = item->Message();
				if( msg && msg->what == TERM_SELECT_EMULATION_MSG ) {
					const char* str;
					if( !msg->FindString("name",0,&str) ) {
						if( !strcmp(type->Name,str) ) {
							item->SetMarked(true);
							okay = true;
						} else item->SetMarked(false);
					}
				}
			}
		}
	}
	return okay;
}

ArpEmulatorInterface* ArpTerminal::SetEmulator(ArpEmulatorInterface* emulator)
{
	ArpEmulatorInterface* last = curEmulator;
	curEmulator = emulator;
	if( !curEmulator ) curEmulator = &defEmulator;
	const char* lastname = last ? last->EmulationType()->Name : NULL;
	const char* curname = curEmulator->EmulationType()->Name;
	if( curname && (!lastname || strcmp(curname,lastname) != 0) ) {
		TermEmulationChanged(curname);
	}
	return ( last == &defEmulator ) ? NULL : last;
}

ArpEmulatorInterface& ArpTerminal::Emulator(void)
{
	return *curEmulator;
}

const ArpEmulatorInterface& ArpTerminal::Emulator(void) const
{
	return *curEmulator;
}

bool ArpTerminal::IsDefaultEmulator(void) const
{
	return (!curEmulator ||
		((const ArpEmulatorInterface*)curEmulator) == &defEmulator);
}

void ArpTerminal::SetPasteVerified(bool state)
{
	pasteVerify = state;
}

bool ArpTerminal::PasteVerified(void) const
{
	return pasteVerify;
}

void ArpTerminal::SetRMBPaste(bool state)
{
	rmbPaste = state;
}

bool ArpTerminal::RMBPaste(void) const
{
	return rmbPaste;
}

void ArpTerminal::TermScrollRegion(int32 top, int32 bottom, int32 amount)
{
	int32 hrow1=0, hcol1=0, hrow2=0, hcol2=0;
	TermGetHighlight(NULL,&hrow1,&hcol1,&hrow2,&hcol2);
	int32 hused=0;
	if( hInitRow == hrow1 && hInitCol == hcol1 ) hused=1;
	else if( hInitRow == hrow2 && hInitCol == hcol2 ) hused=2;
	ArpCoreTerminal::TermScrollRegion(top,bottom,amount);
	TermGetHighlight(NULL,&hrow1,&hcol1,&hrow2,&hcol2);
	if( hused == 1 ) {
		hInitRow = hrow1;
		hInitCol = hcol1;
	}
	if( hused == 2 ) {
		hInitRow = hrow2;
		hInitCol = hcol2;
	}
}

void ArpTerminal::TermSetHighlight(TermHighlightType type,
									int32 row1, int32 col1,
									int32 row2, int32 col2)
{
	if( type == TERM_HIGHLIGHTOFF ) {
		selHMode = type;
		selHShown = false;
		hInitPoint = BPoint();
	} else {
		selHMode = type;
		selHShown = true;
	}
	ArpCoreTerminal::TermSetHighlight(type,row1,col1,row2,col2);
}

void ArpTerminal::TermSendRemote(const ichar * d, size_t len)
{
	Emulator().EmulateToTTY(d,len);
}

void ArpTerminal::EmulateToRemote(const ichar* d, size_t len)
{
	Emulator().EmulateToRemote(d, len);
}

void ArpTerminal::DoPaste(const char* text, size_t size)
{
	if( text && size > 0 ) {
		DidInput();
		if( strcmp(TermEnterStream(), "\n") != 0 ) {
			// Have to convert LF in input to appropriate stream.
			char buffer[512];
			const size_t strLen = strlen(TermEnterStream());
			size_t strPos = 0;
			size_t in, out;
			while( size > 0 ) {
				in=0;
				out=0;
				while( out < (sizeof(buffer)-1-strLen*2) && in < size ) {
					if( text[in] == TermEnterStream()[strPos] ) {
						buffer[out++] = TermEnterStream()[strPos++];
					} else {
						if( strPos > 0 ) {
							memcpy(&buffer[out], TermEnterStream()+strPos,
								   strLen-strPos);
							out += strLen-strPos;
						}
						if( text[in] == '\n' ) {
							memcpy(&buffer[out], TermEnterStream(), strLen);
							out += strLen;
						} else {
							buffer[out++] = text[in];
						}
					}
					if( strPos >= strLen ) strPos = 0;
					in++;
				}
				if( out > 0 ) ConvertInput((const ichar*)buffer, out);
				size -= in;
				text += in;
				ArpD(cdb << ADH << in << " in chars to " << out
						<< " out chars; " << size << " left." << endl);
			}
		} else {
			ConvertInput((const ichar*)text, size);
		}
	}
}

void ArpTerminal::MakeRefsString(BString* strout,
								 const BMessage* from) const
{
	*strout = "";
	int32 buttons=0;
	from->FindInt32("buttons", &buttons);
	ArpD(cdb << ADH << "Got button = " << buttons << endl);
	
	type_code type;
	int32 count;
	if( from->GetInfo("refs", &type, &count) != B_OK ) return;
	if( type != B_REF_TYPE ) return;
	
	for( int i=0; i<count; i++ ) {
		entry_ref ref;
		BEntry entry;
		BPath path;
		status_t res = from->FindRef("refs", i, &ref);
		ArpD(cdb << ADH << "Got ref = " << ref << endl);
		if( !res ) {
			entry.SetTo(&ref);
			res = entry.InitCheck();
			ArpD(cdb << ADH << "Got BEntry, res = " << res << endl);
		}
		if( !res ) {
			res = entry.GetPath(&path);
			ArpD(cdb << ADH << "Got BPath, res = " << res << endl);
		}
		if( !res ) {
			const char* pathName = path.Path();
			ArpD(cdb << ADH << "Adding path = " << pathName << endl);
			if( *strout != "" ) *strout << " ";
			BString esc;
			esc.CharacterEscape(pathName, " \n\t\"'\\()$#&*;<>`", '\\');
			*strout << esc;
		}
	}
}

void ArpTerminal::MessageReceived(BMessage *message)
{
	if( !message ) return;
	
	ArpD(cdb << ADH << "ArpTerminal::MessageReceived: " << *message << endl);
	if( message->WasDropped() ) {
		ArpD(cdb << ADH << "This was a dropped message." << endl);
		switch( message->what ) {
			case B_SIMPLE_DATA:
				if( message->HasRef("refs") ) {
					BString str;
					MakeRefsString(&str, message);
					if( str != "" ) {
						ArpD(cdb << ADH << "Pasting refs = "
									<< str.String() << endl);
						DoPaste(str.String(), strlen(str.String()));
					}
					break;
				}
				
				// fall through to handle SIMPLE and MIME messages
				// the same
			case B_MIME_DATA: {
				PasteFromMessage(message);
			} break;
		
			default:
				inherited::MessageReceived(message);
		}
		
		return;
	}
	
	switch( message->what ) {
		case B_PASTE: {
			Paste(be_clipboard);
		} break;
		case B_COPY: {
			Copy(be_clipboard);
		} break;
		case B_CUT: {
			Cut(be_clipboard);
		} break;
		case B_SELECT_ALL: {
			SelectAll();
		} break;
		case TERM_DOPASTE: {
			if( pasteView && !pasteView->IsHidden() ) {
				pasteView->Hide();
			}
			if( pasteText ) {
				const char* text = pasteText->Text();
				ssize_t size = pasteText->TextLength();
				DoPaste(text,size);
			}
			MakeFocus(TRUE);
			break;
		}
			
		case TERM_CANCELPASTE:
			MakeFocus(TRUE);
			break;
		
		default:
			inherited::MessageReceived(message);
	}
}

void ArpTerminal::TermSizeChanged(int32 rows, int32 cols)
{
	ArpCoreTerminal::TermSizeChanged(rows,cols);
	BMessage msg(B_VIEW_RESIZED);
	if( msg.AddInt64("when",real_time_clock_usecs()) != B_OK ) return;
	if( msg.AddInt32("width",Frame().Width()) != B_OK ) return;
	if( msg.AddInt32("height",Frame().Height()) != B_OK ) return;
	Emulator().EmulateToRemote(&msg);
}

void ArpTerminal::TermEmulationChanged(const char* /*name*/)
{
}

void ArpTerminal::ScrollTo(BPoint where)
{
	BRect bnd = Bounds();
	bool hidden = true;
	if( bnd.top != where.y && pasteView ) {
		hidden = pasteView->IsHidden();
		pasteView->Hide();
		//if( !hidden ) Draw(Bounds());
	}
	ArpCoreTerminal::ScrollTo(where);
	if( bnd.top != where.y && pasteView && !hidden ) {
		SetPastePos();
		pasteView->Show();
	} else if( bnd.top != where.y && pasteView ) {
		pasteView->Show();
	}
}

void ArpTerminal::DidInput(void)
{
	ArpD(cdb << ADH << "ArpTerminal::DidInput()" << endl);
	be_app->ObscureCursor();
	if( (((int)AutoScrollMode())&AUTOSCROLL_INPUT) != 0 ) {
		int32 row=0, col=0;
		TermGetCursorPos(&row,&col);
		MakePosVisible(row,col);
	}
}

void ArpTerminal::KeyDown(const char *bytes, int32 numBytes)
{
	ArpD(cdb << ADH << "ArpTerminal::KeyDown(char=" << *bytes
		<< " (" << (int)(*bytes) << "), count=" << numBytes << ")"
		<< ", msg=" << *Window()->CurrentMessage() << endl);
		
	BMessage* msg = Window()->CurrentMessage();
	int32 mods = 0;
	int8 key = 0;
	int32 raw_key = 0;
	msg->FindInt32("modifiers",&mods);
	msg->FindInt8("byte",&key);
	msg->FindInt32("raw_char",&raw_key);
	
	if( Emulator().EmulateToRemote(msg) ) {
		DidInput();
		return;
	}
	
	BScrollBar* vbar = ScrollBar(B_VERTICAL);
	BScrollBar* hbar = ScrollBar(B_HORIZONTAL);
	float hstep=0, hjump=0, hmin=0, hmax=0, hval=0;
	if( hbar ) {
		hbar->GetSteps(&hstep,&hjump);
		hbar->GetRange(&hmin,&hmax);
		hval = hbar->Value();
	}
	float vstep=0, vjump=0, vmin=0, vmax=0, vval=0;
	if( vbar ) {
		vbar->GetSteps(&vstep,&vjump);
		vbar->GetRange(&vmin,&vmax);
		vval = vbar->Value();
	}
	
	// Check for cursor keys.  Use the modifier mapping to arrive
	// at the cursor key, unless the original key was alphanumeric --
	// then use the unmodified key.  We need to do this because the
	// below key constants map to values < 0x20, but we don't want
	// to interpret control keys as these (such as Ctrl-D as B_END).
	if( raw_key >= 0x20 ) key = raw_key;
	switch( key ) {
		case B_HOME: {
			if( vbar ) vbar->SetValue(vmin);
			return;
		}
		case B_PAGE_UP: {
			if( vbar ) {
				vval -= vjump;
				if( vval < vmin ) vval = vmin;
				vbar->SetValue(vval);
			}
			return;
		}
		case B_UP_ARROW: {
			if( vbar ) {
				vval -= vstep;
				if( vval < vmin ) vval = vmin;
				vbar->SetValue(vval);
			}
			return;
		}
		case B_DOWN_ARROW: {
			if( vbar ) {
				vval += vstep;
				if( vval > vmax ) vval = vmax;
				vbar->SetValue(vval);
			}
			return;
		}
		case B_PAGE_DOWN: {
			if( vbar ) {
				vval += vjump;
				if( vval > vmax ) vval = vmax;
				vbar->SetValue(vval);
			}
			return;
		}
		case B_END: {
			if( vbar ) vbar->SetValue(vmax);
			return;
		}
		case B_LEFT_ARROW: {
			if( hbar ) {
				hval -= hstep;
				if( hval < hmin ) hval = hmin;
				hbar->SetValue(hval);
			}
			return;
		}
		case B_RIGHT_ARROW: {
			if( hbar ) {
				hval += hstep;
				if( hval > hmax ) hval = hmax;
				hbar->SetValue(hval);
			}
			return;
		}
	}
	
	ArpD(cdb << ADH << "ArpTerminal: Sending key: " << bytes << endl);
	
	if( numBytes > 0 ) {
		BString altBytes;
		if( (TermMode()&TERM_MODESWAPBSDEL) != 0 && numBytes == 1 ) {
			if( *bytes == ANSI_BS ) {
				altBytes << (char)ANSI_DEL;
				bytes = altBytes.String();
			} else  if( *bytes == ANSI_DEL ) {
				altBytes << (char)ANSI_BS;
				bytes = altBytes.String();
			}
		}
		if( key == B_RETURN || key == B_ENTER ) {
			altBytes = TermEnterStream();
			bytes = altBytes.String();
			numBytes = altBytes.Length();
		}
		// If it is a regular key, the emulator always gets it.
		EmulateToRemote((const ichar*)bytes, numBytes);
		DidInput();
		return;
	}
	
	// If something else, pass up to superclass.  This allows it
	// to handle standard key sequences, particularily TAB focus
	// sequences.
	ArpCoreTerminal::KeyDown(bytes,numBytes);
}

void ArpTerminal::MouseDown(BPoint point)
{
	if( !IsFocus() ) {
		ArpCoreTerminal::MouseDown(point);
		return;
	}
	ArpCoreTerminal::MouseDown(point);
	ArpD(cdb << ADH << "ArpTerminal: Mouse down: " << point << endl);
	BMessage* msg = Window()->CurrentMessage();
	if( !msg ) return;
	
	if( rmbPaste ) {
		int32 button=0;
		msg->FindInt32("buttons",&button);
		if( button == B_SECONDARY_MOUSE_BUTTON ) {
			QuickPaste(be_clipboard);
			return;
		}
	}
	
	int32 clicks=0;
	msg->FindInt32("clicks",&clicks);
	
	int32 pointCol = IRawXToCol(point.x);
	int32 pointRow = IRawYToRow(point.y);
	if( clicks <= 1 && TermInHighlight(pointRow, pointCol) ) {
		TermHighlightType hmode;
		int32 trow, tcol;
		int32 brow, bcol;
		TermGetHighlight(&hmode,&trow,&tcol,&brow,&bcol);
		if( hmode != TERM_HIGHLIGHTOFF ) {
			BMessage drag(B_MIME_DATA);
			CopyToMessage(&drag, trow, tcol, brow, bcol);
			// Figure out left and right columns of highlighted
			// region.
			int32 selbcol = bcol;
			if( trow != brow ) tcol = LeftDataCol();
			bcol = LeftDataCol();
			for( int i=trow; i<brow; i++ ) {
				const RowInfo* ri = GetRowInfo(i);
				if( ri && ri->UsedCols() > bcol ) {
					bcol = ri->UsedCols();
				}
			}
			if( selbcol > bcol ) {
				const RowInfo* ri = GetRowInfo(brow);
				if( ri ) {
					if( ri->UsedCols() > selbcol ) bcol = selbcol;
					else if( ri->UsedCols() > bcol ) {
						bcol = ri->UsedCols();
					}
				}
			}
			BRect rect(ColToX(tcol), RowToY(trow),
					   ColToX(bcol)+CharWidth()-1,
					   RowToY(brow)+CharHeight()-1);
			DragMessage(&drag, rect);
			return;
		}
	}
	
	SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS,
					  B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
	StartMouseTracker();	
	if( Emulator().EmulateToRemote(msg) ) return;
	TermSetHighlight(TERM_HIGHLIGHTOFF,0,0,0,0);
	selHMode = ((clicks-1)%3)+1;
	hInitPoint = point;
	hInitCol = pointCol;
	hInitRow = pointRow;
	if( selHMode != TERM_HIGHLIGHTCHAR ) {
		TermSetHighlight(selHMode,hInitRow,hInitCol,
								hInitRow,hInitCol);
		selHShown = true;
	}
}

void ArpTerminal::MouseMoved(BPoint point, uint32 transit,
							 const BMessage* message)
{
	ArpCoreTerminal::MouseMoved(point, transit, message);
	
	if( message ) {
		ArpD(cdb << ADH << "Mouse Moved: point=" << point
			<< ", transit=" << transit << ", msg="
			<< *message << endl);
	}
}

void ArpTerminal::TrackMouseMoved(BPoint where, uint32 buttons)
{
	ArpD( printf("Track move: x=%f, y=%f, buttons=%lx\n",
			where.x, where.y, buttons));
	if( selHMode ) {
		if( !selHShown && fabs(hInitPoint.x-where.x) < 3
			&& fabs(hInitPoint.x-where.y) < 3 ) return;
		BRect bnd = Bounds();
		if( where.x < bnd.left-1 ) where.x = bnd.left-1;
		if( where.x > bnd.right+1 ) where.x = bnd.right+1;
		if( where.y < bnd.top-1 ) where.y = bnd.top-1;
		if( where.y > bnd.bottom+1 ) where.y = bnd.bottom+1;
		int32 cur_col = IRawXToCol(where.x);
		int32 cur_row = IRawYToRow(where.y);
		TermSetHighlight(selHMode,hInitRow,hInitCol,
								cur_row,cur_col);
	} else {
		BMessage msg(B_MOUSE_MOVED);
		if( msg.AddInt64("when",real_time_clock_usecs()) != B_OK ) return;
		if( msg.AddPoint("where",where) != B_OK ) return;
		if( msg.AddInt32("buttons",buttons) != B_OK ) return;
		if( msg.AddInt32("modifiers",modifiers()) != B_OK ) return;
		Emulator().EmulateToRemote(&msg);
	}
}

void ArpTerminal::TrackMousePulse(BPoint where, uint32 buttons)
{
	if( selHMode ) {
		BRect bnd = Bounds();
		float w = bnd.Width();
		float h = bnd.Height();
		bool changed = false;
		if( where.x < bnd.left && bnd.left > 0 ) {
			bnd.left -= bnd.left-where.x;
			if( bnd.left < 0 ) bnd.left = 0;
			changed = true;
		}
		float right = DataWidth()*CharWidth() - w - 1;
		if( where.x >= bnd.right && bnd.left < right ) {
			bnd.left += where.x-bnd.right;
			if( bnd.left > right ) bnd.left = right;
			changed = true;
		}
		if( where.y < bnd.top && bnd.top > 0 ) {
			bnd.top -= bnd.top-where.y;
			if( bnd.top < 0 ) bnd.top = 0;
			changed = true;
		}
		float bottom = DataHeight()*CharHeight() - h - 1;
		if( where.y >= bnd.bottom && bnd.top < bottom ) {
			bnd.top += where.y-bnd.bottom;
			if( bnd.top > bottom ) bnd.top = bottom;
			changed = true;
		}
		if( changed ) {
			ScrollTo(BPoint(bnd.left,bnd.top));
			TrackMouseMoved(where,buttons);
		}
	}
}

void ArpTerminal::TrackMouseUp(BPoint where, uint32 buttons)
{
	ArpD(printf("Track release: x=%f, y=%f, buttons=%lx\n",
			where.x, where.y, buttons));
	if( selHMode ) {
		// We know that 'where' is no different than the
		// last 'where' in TrackMouseMoved().  Because, well, we
		// WROTE ArpMouseTracker. :p
		if( !selHShown ) TermSetHighlight(TERM_HIGHLIGHTOFF,
											0,0,0,0);
	} else {
		BMessage msg(B_MOUSE_UP);
		if( msg.AddInt64("when",real_time_clock_usecs()) != B_OK ) return;
		if( msg.AddPoint("where",where) != B_OK ) return;
		if( msg.AddInt32("buttons",buttons) != B_OK ) return;
		if( msg.AddInt32("modifiers",modifiers()) != B_OK ) return;
		Emulator().EmulateToRemote(&msg);
	}
}

void ArpTerminal::MakeFocus(bool focusState)
{
	ArpCoreTerminal::MakeFocus(focusState);
	if( focusState ) {
		if( pasteView && !pasteView->IsHidden() ) {
			pasteView->Hide();
		}
		if( pasteText ) {
			pasteText->SetText(NULL);
		}
		SetFlags( (Flags()&~B_NAVIGABLE)
				| (wasNavigable ? B_NAVIGABLE : 0) );
	}
}

void ArpTerminal::Cut(BClipboard* clipboard)
{
	Copy(clipboard);
}

void ArpTerminal::Copy(BClipboard* clipboard)
{
	TermHighlightType hmode;
	int32 trow, tcol;
	int32 brow, bcol;
	TermGetHighlight(&hmode,&trow,&tcol,&brow,&bcol);
	if( hmode != TERM_HIGHLIGHTOFF ) {
		if( !clipboard ) clipboard = be_clipboard;
		if( !clipboard->Lock() ) return;
		clipboard->Clear();
		BMessage* clip = clipboard->Data();
		
		CopyToMessage(clip, trow, tcol, brow, bcol);
		
		clipboard->Commit();
		clipboard->Unlock();
	}
}

void ArpTerminal::CopyToMessage(BMessage* clip,
								int32 trow, int32 tcol,
								int32 brow, int32 bcol)
{
	const style_t RUNSTYLES = TERM_STYLEBOLD | TERM_STYLEITALIC
				| (TERM_STYLECOLOR_MASK<<TERM_STYLEFOREGROUND_POS);
	
	BString oldEnter = TermEnterStream();
	TermSetEnterStream("\n");
	
	size_t tsize=0;
	TermGetTextRange(trow,tcol,brow,bcol,&tsize,NULL,NULL);
	if( tsize > 0 && clip ) {
		ichar* text = (ichar*)malloc(sizeof(ichar)*tsize);
		style_t* style = (style_t*)malloc(sizeof(style)*tsize);
		if( text ) {
			TermGetTextRange(trow,tcol,brow,bcol,
							&tsize,text,style);
			ArpD(fwrite(text,tsize,1,stdout));
			ArpD(printf("\n"));
			size_t uniSize = tsize;
			if( GetEncodingConv() >= 0 ) {
				size_t newsize = tsize*6;
				int32 usedsize = 0;
				ichar* newtext = (ichar*)malloc(sizeof(ichar)*newsize);
				while( newtext && usedsize < (int32)tsize ) {
					usedsize = tsize;
					int32 dstsize = newsize;
					if( convert_to_utf8(GetEncodingConv(),
							(const char*)text,&usedsize,
							(char*)newtext,&dstsize,NULL) != B_OK ) {
						ArpD(cdb << ADH << "Error converting!  char="
								<< (char)(*text) << " (" << (int)(*text)
								<< ")" << endl);
						memcpy((char*)newtext,(char*)text,tsize);
						usedsize = tsize;
						uniSize = tsize;
					} else if( usedsize <= 0 ) {
						ArpD(cdb << ADH << "Huh?!?  Couldn't convert any characters!" << endl);
						memcpy((char*)newtext,(char*)text,tsize);
						usedsize = tsize;
						uniSize = tsize;
					} else if( usedsize < (int32)tsize ) {
						free(newtext);
						newsize *= 2;
						newtext = (ichar*)malloc(sizeof(ichar)*newsize);
					}
					uniSize = dstsize;
				}
				free(text);
				text = newtext;
			}
			clip->AddData("text/plain",B_MIME_TYPE,text,
							sizeof(ichar)*uniSize);
			if( style ) {
				int32 scount=0;
				style_t* cur_style = style;
				style_t last_style = ~(*cur_style);
				ArpD(
					printf("Finding styles: tsize=%ld, styleptr=%lx, first_style=%x\n",
					tsize, (uint32)cur_style, *cur_style));
				for( size_t i=0; i<tsize; i++ ) {
					//int32 next_style = (*cur_style)&RUNSTYLES;
					if( last_style != ((*cur_style)&RUNSTYLES) ) {
						scount++;
						last_style = (*cur_style)&RUNSTYLES;
					}
					cur_style++;
				}
				ArpD( printf("Found %ld styles\n",scount));
				text_run_array* textruns = (text_run_array*)
						malloc(sizeof(text_run_array)
									+ sizeof(text_run)*scount);
				if( textruns ) {
					textruns->count = 0;
					text_run* run = &textruns->runs[0];
					cur_style = style;
					last_style = ~(*cur_style);
					for( size_t i=0, j=0; i<tsize && scount>0; i++ ) {
						if( last_style != ((*cur_style)&RUNSTYLES) ) {
							scount--;
							last_style = (*cur_style)&RUNSTYLES;
							run->offset = j;
							run->font = GetStyledFont(last_style);
							run->font.SetEncoding(B_UNICODE_UTF8);
							run->color = TermTextForeground(
											(last_style>>TERM_STYLEFOREGROUND_POS)
											& TERM_STYLECOLOR_MASK);
							textruns->count++;
							run++;
						}
						if( j<uniSize ) j++;
						while( (text[j]&0xC0)==0x80 && j<uniSize ) j++;
						cur_style++;
					}
					clip->AddData(
						"application/x-vnd.Be-text_run_array",
						B_MIME_TYPE,textruns,
						sizeof(text_run_array)
							+ sizeof(text_run)*(textruns->count-1));
					free(textruns);
				}
			}
			free(text);
		}
		if( style ) free(style);
	}
	
	ArpD(cdb << ADH << "Paste message:" << endl << *clip << endl);
	
	TermSetEnterStream(oldEnter.String());
}

void ArpTerminal::ShowPasteReq(const char* text, ssize_t size)
{
	if( !text || size <= 0 ) return;
	
	BRect bnd = Bounds();
	BRect frm = BRect(0,0,bnd.Width(),bnd.Height());
	if( !pasteView ) {
#if 0
			pasteText = new BTextView(frm,"pasteText",frm,
							B_FOLLOW_BOTTOM|B_FOLLOW_H_CENTER,
							B_WILL_DRAW|B_NAVIGABLE|B_FRAME_EVENTS);
#endif
			pasteText = new BTextView(frm,"pasteText",frm,
							B_FOLLOW_ALL,
							B_WILL_DRAW|B_NAVIGABLE|B_FRAME_EVENTS);
			if( pasteText ) {
				pasteScroll = new BScrollView("pasteScroll",
								pasteText,
								B_FOLLOW_BOTTOM|B_FOLLOW_H_CENTER,
								B_WILL_DRAW,
								true, true, B_FANCY_BORDER);
			} else pasteScroll = NULL;
			cancelButton = new BButton(BRect(),"cancelButton",
							"Cancel",new BMessage(TERM_CANCELPASTE),
							B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT);
			pasteButton = new BButton(BRect(),"pasteButton",
							"Paste",new BMessage(TERM_DOPASTE),
							B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT);
			pasteView = new BBox(bnd,"pasteView",
								B_WILL_DRAW|B_NAVIGABLE_JUMP
								|B_FULL_UPDATE_ON_RESIZE
								|B_FRAME_EVENTS);
			if( !pasteText || !pasteScroll ||
					!cancelButton || !pasteButton || !pasteView ) {
				delete pasteText;
				delete cancelButton;
				delete pasteButton;
				delete pasteView;
				pasteText = NULL;
				pasteScroll = NULL;
				cancelButton = pasteButton = NULL;
				pasteView = NULL;
			} else {
				cancelButton->SetTarget(this);
				pasteButton->SetTarget(this);
				pasteText->SetWordWrap(false);
				pasteView->Hide();
				AddChild(pasteView);
				pasteView->SetViewColor(ArpColor(0xe0,0xe0,0xe0));
				pasteScroll->SetViewColor(ArpColor(0xe0,0xe0,0xe0));
				pasteText->SetViewColor(ArpColor(0xff,0xff,0xff));
				pasteView->AddChild(pasteScroll);
				pasteView->AddChild(cancelButton);
				pasteView->AddChild(pasteButton);
			}
	}
	
	if( !pasteView ) {
		DoPaste(text,size);
		return;
	}
	if( !pasteView->IsHidden() ) pasteView->Hide();
	wasNavigable = (Flags()&B_NAVIGABLE) != 0;
	SetFlags(Flags()&~B_NAVIGABLE);
	pasteScroll->ResizeTo(frm.right,frm.bottom);
	pasteText->SetTextRect(frm);
	pasteText->SetText(text,size);
	int32 lcnt = pasteText->CountLines();
	float textWidth=0, textHeight=0;
	for( int32 i=0; i<lcnt; i++ ) {
		textHeight += pasteText->LineHeight(i);
		float myw = pasteText->LineWidth(i);
		if( myw > textWidth ) textWidth = myw;
		if( textHeight > bnd.Height() ) {
			i += lcnt/50;
		}
	}
	float textRealWidth=textWidth;
	BScrollBar* scroll = pasteScroll->ScrollBar(B_VERTICAL);
	if( scroll ) {
		float myw=0, myh=0;
		scroll->GetPreferredSize(&myw, &myh);
		textWidth += myw;
	}
	scroll = pasteScroll->ScrollBar(B_HORIZONTAL);
	if( scroll ) {
		float myw=0, myh=0;
		scroll->GetPreferredSize(&myw, &myh);
		textHeight += myh;
	}
	textWidth += 4;
	textHeight += 4;
	float cancelWidth=0, cancelHeight=0;
	float pasteWidth=0, pasteHeight=0;
	cancelButton->GetPreferredSize(&cancelWidth,&cancelHeight);
	pasteButton->GetPreferredSize(&pasteWidth,&pasteHeight);
	float fullWidth=textWidth, fullHeight=textHeight;
	fullHeight += (cancelHeight > pasteHeight)
					? cancelHeight : pasteHeight;
	fullWidth = (cancelWidth+pasteWidth) > fullWidth ?
				(cancelWidth+pasteWidth) : fullWidth;
	fullHeight += 12;
	fullWidth += 12;
	if( fullHeight > (frm.bottom-CharHeight()) )
		fullHeight = frm.bottom-CharHeight();
	if( fullWidth > frm.right-CharWidth() )
		fullWidth = frm.right-CharWidth();
	pasteView->ResizeTo(fullWidth,fullHeight);
	SetPastePos();
	cancelButton->MoveTo(4,fullHeight-cancelHeight-4);
	cancelButton->ResizeTo(cancelWidth,cancelHeight);
	pasteButton->MoveTo(fullWidth-pasteWidth-4,
						fullHeight-pasteHeight-4);
	pasteButton->ResizeTo(pasteWidth,pasteHeight);
	frm.right = fullWidth-8;
	frm.bottom = fullHeight - 12
			- (cancelHeight>pasteHeight
						? cancelHeight : pasteHeight);
	pasteScroll->MoveTo(4,4);
	pasteScroll->ResizeTo(frm.right,frm.bottom);
	BRect tfrm(frm);
	tfrm.right = tfrm.left+textRealWidth;
	pasteText->SetTextRect(tfrm);
	pasteView->Show();
	pasteView->Invalidate();
	pasteScroll->Invalidate();
	pasteText->Invalidate();
	pasteText->SetText(text,size);
	pasteButton->MakeFocus(true);
	ArpD(printf("View shown, loc=(%f,%f)-(%f,%f)\n",
			pasteView->Frame().left, pasteView->Frame().top,
			pasteView->Frame().right, pasteView->Frame().bottom));
}

void ArpTerminal::SetPastePos(void)
{
	if( pasteView ) {
		BRect frm = pasteView->Frame();
		BRect bnd = Bounds();
		int32 row=0, col=0;
		TermGetCursorPos(&row,&col);
		BPoint target(ColToX(col)-bnd.left,RowToY(row)-bnd.top);
#if 0
		printf("Row=%d, Col=%d, RawY=%f, RawX=%f\n",
			row,col,RowToY(row),ColToX(col));
		printf("InitX=%f, InitY=%d, BndX=%f, BndY=%f\n",
			target.x, target.y, bnd.left, bnd.top);
#endif
		if( target.x < (CharWidth()/2) ) target.x = CharWidth()/2;
		if( target.y < (CharHeight()/2) ) target.y = CharHeight()/2;
		if( target.x + frm.Width() > (bnd.Width()-CharWidth()/2) )
			target.x = bnd.Width()-frm.Width()-CharWidth()/2;
		if( target.y + frm.Height() > (bnd.Height()-CharHeight()/2) )
			target.y = bnd.Height()-frm.Height()-CharHeight()/2;
		pasteView->MoveTo(target.x,target.y);
#if 0
		pasteView->MoveTo( (bnd.Width()-frm.Width())/2,
							bnd.Height()-frm.Height()
							 -(CharHeight()/2));
#endif
	}
//	pasteView->MoveTo( ((frm.right-fullWidth)/2),
//						bnd.Height()-fullHeight-(CharHeight()/2));
}

void ArpTerminal::Paste(BClipboard* clipboard)
{
	if( !clipboard ) clipboard = be_clipboard;
	if( !clipboard->Lock() ) return;
	BMessage* clip = clipboard->Data();
	PasteFromMessage(clip);
	clipboard->Unlock();
}

bool ArpTerminal::PasteFromMessage(const BMessage* clip)
{
	const char* text = NULL;
	ssize_t size;
	if( clip->FindData(	"text/plain",B_MIME_TYPE,(const void**)&text,&size)
			== B_OK ) {
		if( pasteVerify ) ShowPasteReq(text,size);
		else DoPaste(text,size);
		return true;
	}
	
	return false;
}

void ArpTerminal::QuickPaste(BClipboard* clipboard)
{
	TermHighlightType hmode;
	int32 trow, tcol;
	int32 brow, bcol;
	TermGetHighlight(&hmode,&trow,&tcol,&brow,&bcol);
	if( hmode != TERM_HIGHLIGHTOFF ) {
		size_t tsize=0;
		TermGetTextRange(trow,tcol,brow,bcol,&tsize,NULL,NULL);
		if( tsize > 0 ) {
			ichar* text = (ichar*)malloc(sizeof(ichar)*tsize);
			if( text ) {
				TermGetTextRange(trow,tcol,brow,bcol,
								&tsize,text,NULL);
				ArpD(fwrite(text,tsize,1,stdout));
				ArpD(printf("\n"));
				EmulateToRemote((const ichar*)text, tsize);
				DidInput();
				free(text);
			}
		}
	} else Paste(clipboard);
}

void ArpTerminal::SelectAll(void)
{
	TermSetHighlight(TERM_HIGHLIGHTLINE,
						TopDataRow(), 0, BottomDataRow(), 32767);
}
