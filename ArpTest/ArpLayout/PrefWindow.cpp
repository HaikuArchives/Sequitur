/*
 * Copyright (c)1997 by Dianne Hackborn.
 * All rights reserved.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@enteract.com>.
 *
 */

#ifndef PREFWINDOW_H
#include "PrefWindow.h"
#endif

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include <ArpLayout/ArpRunningBar.h>
#endif

#ifndef ARPLAYOUT_ARPSCROLLAREA_H
#include <ArpLayout/ArpScrollArea.h>
#endif

#ifndef ARPLAYOUT_ARPVIEWWRAPPER_H
#include <ArpLayout/ArpViewWrapper.h>
#endif

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include <ArpLayout/ViewStubs.h>
#endif

#include <interface/Button.h>
#include <interface/CheckBox.h>
#include <interface/ColorControl.h>
#include <interface/PopUpMenu.h>
#include <interface/TextControl.h>
#include <interface/MenuItem.h>
#include <support/Autolock.h>

#include <float.h>
#include <cstring>
#include <cstdlib>

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

ArpMOD();

enum {
	COLOR_MSG = '.col',
	COLORVAR_MSG = '.cvr',
	FONTNAME_MSG = '.fnm',
	FONTSTYLE_MSG = '.fst',
	FONTSIZE_MSG = '.fsz',
	FONTVAR_MSG = '.fvr',
	REVERT_MSG = '.rev',
	APPLY_MSG = '.apl',
};

#if 0
static void print_ltree(ArpLayoutable* root, int indent)
{
	char ibuff[100];
	int32 i;
	if( indent > 98 ) indent=0;
	for( i=0; i<indent; i++ ) ibuff[i] = ' ';
	ibuff[i] = 0;
	
	if( !root ) {
		printf("%s<NULL>\n",&ibuff[0]);
		return;
	}
	printf("%s%s\n",&ibuff[0],class_name(root));
	
	int num = root->CountLayoutChildren();
	for( i=0; i<num; i++ ) {
		print_ltree(root->LayoutChildAt(i),indent+2);
	}
}

static void print_vtree(BView* root, int indent)
{
	char ibuff[100];
	int32 i;
	if( indent > 98 ) indent=0;
	for( i=0; i<indent; i++ ) ibuff[i] = ' ';
	ibuff[i] = 0;
	
	if( !root ) {
		printf("%s<NULL>\n",&ibuff[0]);
		return;
	}
	printf("%s%s\n",&ibuff[0],class_name(root));
	
	int num = root->CountChildren();
	for( i=0; i<num; i++ ) {
		print_vtree(root->ChildAt(i),indent+2);
	}
}
#endif

PrefWindow::PrefWindow(const BMessenger& target,
						const BMessage* initglobs)
	: BWindow(BRect(20,20,100,100),
	  		  "Preferences", B_TITLED_WINDOW, 0),
	  dest(target)
{
	if( initglobs ) globals = *initglobs;
	reversion = globals;
	
	mainmenu = NULL;
	color = NULL;
	color_vars = NULL;
	color_pop = NULL;
	font_vars = NULL;
	font_pop = NULL;
	font_names = NULL;
	font_styles = NULL;
	revert_but = NULL;
	
	(root = new ArpRootLayout(Bounds(), "Root"))
->AddLayoutChild((new ArpRunningBar("TopVBar"))
	->SetParams(ArpMessage()
		.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
	)
	->AddLayoutChild((mainmenu = new ArpMenuBar("MainMenu"))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
	)
	->AddLayoutChild((new ArpRunningBar("ControlHBar"))
		->SetParams(ArpMessage()
			.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
			.SetFloat(ArpRunningBar::InsetLeftP, .5)
			.SetFloat(ArpRunningBar::InsetRightP, .5)
			.SetFloat(ArpRunningBar::InsetTopP, .5)
			.SetFloat(ArpRunningBar::InsetBottomP, .5)
			.SetFloat(ArpRunningBar::IntraSpaceP, .5)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,1)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
		->AddLayoutChild((new ArpBox("ColorBox","Colors"))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
				.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			)
			->AddLayoutChild((new ArpRunningBar("ColorVBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
				->AddLayoutChild((new ArpMenuField("color_menu", "",
										(color_pop=new BPopUpMenu("color_popup"))))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpCenter)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new ArpViewWrapper( color = 
						new BColorControl(BPoint(),B_CELLS_16x16,
											8,"CVarSel",
											new BMessage(COLORVAR_MSG)) ))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpNorth)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
			)
		)
		->AddLayoutChild((new ArpBox("FontBox","Fonts"))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			)
			->AddLayoutChild((new ArpRunningBar("FontVBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
				->AddLayoutChild((new ArpMenuField("font_menu", "",
										(font_pop=new BPopUpMenu("font_popup"))))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpCenter)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new ArpRunningBar("FontStyleHBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
					->AddLayoutChild((new ArpScrollArea("FNameArea"))
						->SetParams(ArpMessage()
							.SetBool(ArpScrollArea::ScrollVerticalP,true)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((font_names = new ArpListView(
								"FNameList",B_SINGLE_SELECTION_LIST))
						)
					)
					->AddLayoutChild((new ArpScrollArea("FStyleArea"))
						->SetParams(ArpMessage()
							.SetBool(ArpScrollArea::ScrollVerticalP,true)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
						)
						->AddLayoutChild((font_styles = new ArpListView(
								"FStyleList",B_SINGLE_SELECTION_LIST))
						)
					)
				)
				->AddLayoutChild((font_size = new ArpTextControl(
										"FontSize","Size: ","",
										new BMessage(FONTSIZE_MSG)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
			)
		)
	)
	->AddLayoutChild((new ArpRunningBar("ButtonHBar"))
		->SetParams(ArpMessage()
			.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
			.SetFloat(ArpRunningBar::InsetLeftP, .5)
			.SetFloat(ArpRunningBar::InsetRightP, .5)
			.SetFloat(ArpRunningBar::InsetTopP, .5)
			.SetFloat(ArpRunningBar::InsetBottomP, .5)
			.SetFloat(ArpRunningBar::IntraSpaceP, .5)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
		->AddLayoutChild((new ArpLayout("BFillLeft"))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
			)
		)
		->AddLayoutChild((revert_but = new ArpButton("Revert","Revert",
										new BMessage(REVERT_MSG)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
			)
		)
		->AddLayoutChild((new ArpButton("Apply","Apply",
							new BMessage(APPLY_MSG)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
			)
		)
	)
);

	// Stick globals into layout
	if( root ) root->UpdateGlobals(&globals);
	
	// Initially, 'Revert' button is disabled
	if( revert_but ) revert_but->SetEnabled(false);
				
	// Place some items into the top menu bar.
	if( mainmenu ) {
		BMenu* first = new BMenu("File");
		BMenuItem *item; 
	
		item = new BMenuItem("Close",
								new BMessage(B_QUIT_REQUESTED)); 
		item->SetTarget(this);
		first->AddItem(item);
	
		mainmenu->AddItem(first);
		mainmenu->SetFlags(mainmenu->Flags()|B_WILL_DRAW);
	}
	
	// Place color variables into their list view
	if( color_vars ) {
		char* name;
		uint32 type;
		int32 count=0;
		for( int i=0; !globals.GetInfo(B_RGB_COLOR_TYPE,i,
									&name,&type,&count); i++ ) {
			if( count > 0 ) {
				color_vars->AddItem(new BStringItem(name));
			}
		}
		color_vars->SetSelectionMessage(new BMessage(COLOR_MSG));
		color_vars->Select(0);
	}
	
	// Place color variables into their popup menu
	if( color_pop ) {
		char* name;
		uint32 type;
		int32 count=0;
		for( int i=0; !globals.GetInfo(B_RGB_COLOR_TYPE,i,
									&name,&type,&count); i++ ) {
			if( count > 0 ) {
				BMessage* ctrlMsg;
				ctrlMsg = new BMessage(COLOR_MSG);
				ctrlMsg->AddString("color_name", name);
				BMenuItem* item = new BMenuItem(name, ctrlMsg);
				if( i == 0 ) item->SetMarked(true);
				color_pop->AddItem(item);
			}
		}
	}
	
	// Place font variables into their list view
	if( font_vars ) {
		char* name;
		uint32 type;
		int32 count=0;
		for( int i=0; !globals.GetInfo(FFont::FONT_TYPE,i,
									&name,&type,&count); i++ ) {
			if( count > 0 ) {
				font_vars->AddItem(new BStringItem(name));
			}
		}
		font_vars->SetSelectionMessage(new BMessage(FONTVAR_MSG));
		font_vars->Select(0);
	}
	
	// Place font variables into their popup menu
	if( font_pop ) {
		char* name;
		uint32 type;
		int32 count=0;
		for( int i=0; !globals.GetInfo(FFont::FONT_TYPE,i,
									&name,&type,&count); i++ ) {
			if( count > 0 ) {
				BMessage* ctrlMsg;
				ctrlMsg = new BMessage(FONTVAR_MSG);
				ctrlMsg->AddString("font_name", name);
				BMenuItem* item = new BMenuItem(name, ctrlMsg);
				if( i == 0 ) item->SetMarked(true);
				font_pop->AddItem(item);
			}
		}
	}
	
	// Place font names into their list view
	if( font_names ) {
		int32 num = count_font_families();
		for( int32 i=0; i<num; i++ ) {
			font_family name;
			if( !get_font_family(i,&name) ) {
				font_names->AddItem(new BStringItem(&name[0]));
			}
		}
		font_names->SetSelectionMessage(new BMessage(FONTNAME_MSG));
	}
	if( font_styles ) {
		font_styles->SetSelectionMessage(new BMessage(FONTSTYLE_MSG));
	}

	if( root ) {
		ArpD(cdb << ADH << "Before attach: win=" << Bounds()
						<< ", root=" << root->Bounds() << std::endl);
		this->AddChild(root);
		ArpD(cdb << ADH << "After attach: win=" << Bounds()
						<< ", root=" << root->Bounds() << std::endl);
		
		float w=100, h=00;
		root->GetPreferredSize(&w, &h);
		MoveTo(30,30);
		ResizeTo(w, h);
		
		ArpD(cdb << ADH << "After resize: win=" << Bounds()
						<< ", root=" << root->Bounds() << std::endl);
		
		#if 0
		printf("LAYOUT TREE:\n");
		print_ltree(root,2);
		printf("VIEW TREE:\n");
		print_vtree(root,2);
		#endif
	}
	
	show_color();
	show_font();
}

PrefWindow::~PrefWindow()
{
}

BStringItem* PrefWindow::find_string_item(BListView* lv,
										const char* match)
{
	if( lv ) {
		if( !match ) {
			int32 sel = lv->CurrentSelection();
			BListItem* li = lv->ItemAt(sel);
			return dynamic_cast<BStringItem*>(li);
		}
		int32 num = lv->CountItems();
		for( int32 i=0; i<num; i++ ) {
			BListItem* li = lv->ItemAt(i);
			BStringItem* item = dynamic_cast<BStringItem*>(li);
			if( item && strcmp(item->Text(),match) == 0 ) {
				return item;
			}
		}
	}
	return NULL;
}

void PrefWindow::show_color(void)
{
	if( color_pop ) {
		BMenuItem* item = color_pop->FindMarked();
		if( item && color ) {
			rgb_color col = { 0,0,0 };
			globals.FindRGBColor(item->Label(),&col);
			color->SetValue(col);
		}
	}
#if 0
	if( color_vars ) {
		BStringItem* item = find_string_item(color_vars,NULL);
		if( item && color ) {
			rgb_color col = { 0,0,0 };
			globals.FindRGBColor(item->Text(),&col);
			color->SetValue(col);
		}
	}
#endif
}

void PrefWindow::show_font(void)
{
	if( !font_pop ) return;
	#if 0
	if( !font_vars ) return;
	#endif
	
	BFont font;
	if( font_pop ) {
		BMenuItem* item = font_pop->FindMarked();
		if( !item || !font_names
			|| globals.FindFont(item->Label(),&font) ) {
			if( font_names ) font_names->DeselectAll();
			if( font_styles ) font_styles->DeselectAll();
			return;
		}
	}
	#if 0
	BStringItem* item = find_string_item(font_vars,NULL);
	if( !item || !font_names
		|| globals.FindFont(item->Text(),&font) ) {
		if( font_names ) font_names->DeselectAll();
		if( font_styles ) font_styles->DeselectAll();
		return;
	}
	#endif
	if( font_size ) {
		char buf[100];
		buf[0] = 0;
		sprintf(&buf[0],"%d",(int)font.Size());
		font_size->SetText(&buf[0]);
	}
	font_family family;
	font_style style;
	font.GetFamilyAndStyle(&family,&style);
	BStringItem* fitem = find_string_item(font_names,
											&family[0]);
	if( !fitem ) {
		if( font_names ) font_names->DeselectAll();
		if( font_styles ) font_styles->DeselectAll();
		return;
	}
	font_names->Select(font_names->IndexOf(fitem));
	font_names->ScrollToSelection();
	show_style(&style);
}

void PrefWindow::show_style(font_style* name)
{
	if( !font_styles ) return;
	
	font_style style;
	if( name ) memcpy(&style,name,sizeof(*name));
	else {
		BStringItem* item = find_string_item(font_styles,NULL);
		style[0] = 0;
		if( item ) {
			strncat(&style[0],item->Text(),sizeof(style));
		}
	}
	
	font_styles->MakeEmpty();
	if( !font_names ) return;
	BStringItem* fami = find_string_item(font_names,NULL);
	if( !fami ) return;
	font_family family;
	family[0] = 0;
	strncat(&family[0],fami->Text(),sizeof(family));
	
	// Place font styles into their list view
	int32 num = count_font_styles(family);
	for( int32 i=0; i<num; i++ ) {
		font_style name;
		if( !get_font_style(family,i,&name) ) {
			font_styles->AddItem(new BStringItem(&name[0]));
		}
	}

	BStringItem* sitem = find_string_item(font_styles,
											&style[0]);
	if( sitem ) {
		font_styles->Select(font_styles->IndexOf(sitem));
	}
	font_styles->InvalidateDimens();
	font_styles->ScrollToSelection();
}

void PrefWindow::MessageReceived(BMessage *message)
{
	if( message ) {
		ArpMessage update;
		update.what = 0;
		ArpD(cdb << ADH << "Received message: " << *message << std::endl);
		switch(message->what) {
			case COLOR_MSG:
				show_color();
				break;
			case COLORVAR_MSG: {
				if( color_pop ) {
					BMenuItem* item = color_pop->FindMarked();
					if( item && color ) {
						update.what = ARP_PREF_MSG;
						rgb_color col = color->ValueAsColor();
						update.SetRGBColor(item->Label(),&col);
						if( revert_but ) revert_but->SetEnabled(true);
					}
				}
				#if 0
				BStringItem* item
					= find_string_item(color_vars,NULL);
				if( item && color ) {
					update.what = ARP_PREF_MSG;
					rgb_color col = color->ValueAsColor();
					update.SetRGBColor(item->Text(),&col);
					if( revert_but ) revert_but->SetEnabled(true);
				}
				#endif
			} break;
			case FONTVAR_MSG:
				show_font();
				break;
			case FONTNAME_MSG:
				show_style(NULL);
			case FONTSTYLE_MSG: {
				const char* fontCat = 0;
				if( font_pop ) {
					BMenuItem* item = font_pop->FindMarked();
					if( item ) fontCat = item->Label();
				}
				#if 0
				BStringItem* item
					= find_string_item(font_vars,NULL);
				if( item ) fontCat = item->Label();
				#endif
				BStringItem* nitem
					= find_string_item(font_names,NULL);
				BStringItem* sitem
					= find_string_item(font_styles,NULL);
				if( fontCat && nitem && sitem ) {
					BFont old_font;
					globals.FindFont(fontCat,&old_font);
					BFont font = old_font;
					font_family family;
					family[0] = 0;
					strncat(&family[0],nitem->Text(),sizeof(family));
					font_style style;
					style[0] = 0;
					strncat(&style[0],sitem->Text(),sizeof(style));
					font.SetFamilyAndStyle(family,style);
					ArpD(cdb << ADH << "old_font = "
									<< old_font << std::endl);
					ArpD(cdb << ADH << "new_font = "
									<< font << std::endl);
					if( font != old_font ) {
						ArpD(cdb << ADH << "Sending update." << std::endl);
						update.what = ARP_PREF_MSG;
						update.SetFont(fontCat,&font);
						if( revert_but ) revert_but->SetEnabled(true);
					}
				}
			} break;
			case FONTSIZE_MSG: {
				const char* fontCat = 0;
				if( font_pop ) {
					BMenuItem* item = font_pop->FindMarked();
					if( item ) fontCat = item->Label();
				}
				#if 0
				BStringItem* item
					= find_string_item(font_vars,NULL);
				if( item ) fontCat = item->Label();
				#endif
				if( fontCat && font_size ) {
					BFont old_font;
					globals.FindFont(fontCat,&old_font);
					BFont font = old_font;
					float size = atof(font_size->Text());
					font.SetSize(size > 1 ? size : 1);
					if( font != old_font ) {
						ArpD(cdb << ADH << "Sending update." << std::endl);
						update.what = ARP_PREF_MSG;
						update.SetFont(fontCat,&font);
						if( revert_but ) revert_but->SetEnabled(true);
					}
				}
			} break;
			case REVERT_MSG: {
				if( !root ) break;
				update = reversion;
				update.what = ARP_PREF_MSG;
				if( revert_but ) revert_but->SetEnabled(false);
			} break;
			case APPLY_MSG: {
				if( !root ) break;
				root->UpdateGlobals(&globals);
				reversion = globals;
				if( revert_but ) revert_but->SetEnabled(false);
			} break;
		}
		if( update.what != 0 ) {
			globals.Update(update);
			dest.SendMessage(&update,this);
			return;
		}
	}
	inherited::MessageReceived(message);
}

void PrefWindow::FrameResized(float width, float height)
{
	inherited::FrameResized(width, height);
}

bool PrefWindow::QuitRequested()
{
	//target.PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}
