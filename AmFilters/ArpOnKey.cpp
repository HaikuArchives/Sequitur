/* ArpOnKeyFilter.cpp
 */
#include "ArpOnKey.h"

#include <cstdio>
#include <cstdlib>
#include <interface/CheckBox.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/StringView.h>
#include <translation/TranslationUtils.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "AmPublic/AmFilterConfig.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const int32		OTHER_SCALE				= -1;
static const int32		MAJOR_SCALE				= 0;				// Corresponds with the first scale in gScales
static const uint32		CHOOSE_WRONG_NOTE_MSG	= 'icwn';
static const uint32		CHOOSE_ROOT_MSG			= 'icrt';
static const uint32		CHOOSE_SCALE2_MSG		= 'ics2';
static const char*		WRONG_NOTES_STR			= "wrong notes";
static const char*		ROOT_STR				= "root";
static const char*		SCALE_STR				= "scale";
static const char*		OTHER_STR				= "Other";
static const char*		KEY_C_STR				= "c_key";
static const char*		KEY_CSHARP_STR			= "c#_key";
static const char*		KEY_D_STR				= "d_key";
static const char*		KEY_DSHARP_STR			= "d#_key";
static const char*		KEY_E_STR				= "e_key";
static const char*		KEY_F_STR				= "f_key";
static const char*		KEY_FSHARP_STR			= "f#_key";
static const char*		KEY_G_STR				= "g_key";
static const char*		KEY_GSHARP_STR			= "g#_key";
static const char*		KEY_A_STR				= "a_key";
static const char*		KEY_ASHARP_STR			= "a#_key";
static const char*		KEY_B_STR				= "b_key";

static const char* wrongNoteNames[] = {
	"Discard",
	"Shift down",
	"Shift up",
	"Shift closest",
	0
};

static const char* gRootNames[] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", 0
};

static bool gScales[][12] = {
//	  c			c#		d		d#		e		f		f#		g		g#		a		a#		b	
	{ true,		false,	true,	false,	true,	true,	false,	true,	false,	true,	false,	true },		// Major
	{ true,		false,	true,	true,	false,	true,	false,	true,	true,	false,	true,	false },	// Natural Minor
	{ true,		false,	true,	true,	false,	true,	false,	true,	true,	false,	false,	true },		// Harmonic Minor
	{ true,		false,	true,	true,	false,	true,	false,	true,	false,	true,	false,	true },		// Melodic Minor (Asc)
	{ true,		false,	true,	true,	false,	true,	false,	true,	true,	false,	true,	false },	// Melodic Minor (Desc)
	{ true,		true,	false,	false,	true,	false,	true,	false,	true,	false,	true,	true },		// Enigmatic
	{ true,		true,	true,	true,	true,	true,	true,	true,	true,	true,	true,	true },		// Chromatic
	{ true,		false,	true,	true,	false,	true,	true,	false,	true,	true,	false,	true },		// Diminished
	{ true,		false,	true,	false,	true,	false,	true,	false,	true,	false,	true,	false },	// Whole Tone
	{ true,		false,	true,	false,	true,	false,	false,	true,	false,	true,	false,	false },	// Pentatonic Major
	{ true,		false,	false,	true,	false,	true,	false,	true,	false,	false,	true,	false },	// Pentatonic Minor
	{ true,		false,	false,	true,	false,	false,	true,	false,	false,	true,	false,	false },	// 3 Semitone
	{ true,		false,	false,	false,	true,	false,	false,	false,	true,	false,	false,	false },	// 4 Semitone
};

static const char* gScaleNames[] = {
	"Major",			"Natural minor",		"Harmonic minor",		"Melodic minor (asc)",		"Melodic minor (desc)",
	"Enigmatic",		"Chromatic",			"Diminished",			"Whole tone",				"Pentatonic major",
	"Pentatonic minor",	"3 Semitone",			"4 Semitone",			0
};

static void get_scale(bool* scale, int32 rootConstant, int32 scaleConstant)
{
	int32	index = rootConstant;
	for( int32 count = 0; count < 12; count++ ) {
		scale[index] = gScales[scaleConstant][count];
		index++;
		if( index >= 12 ) index = 0;
	}
}

static void add_menu_item(const char* label, uint32 msgWhat, const char* msgKey, int32 msgIndex, int32 initialIndex, BMenu* toMenu, bool enabled = true)
{
	if( !toMenu ) return;
	BMessage*	msg = new BMessage( msgWhat );
	if( !msg ) return;
	if( msg->AddInt32( msgKey, msgIndex ) != B_OK ) {
		delete msg;
		return;
	}
	BMenuItem*	item = new BMenuItem( label, msg );
	if( !item ) {
		delete msg;
		return;
	}
	if( msgIndex == initialIndex ) item->SetMarked( true );
	item->SetEnabled( enabled );
	toMenu->AddItem(item);
}

/*************************************************************************
 * _ON-KEY-CONTROL
 * A control that displays a quick-access menu and text control for editing
 * some On Key filter properties.
 *************************************************************************/
class _OnKeyControl : public AmFilterConfig
{
public:
	_OnKeyControl(	AmFilterHolderI* target,
					const BMessage& initSettings,
					const char* initialName );
	/* Find the current root and scale values and send out an
	 * update with those values, along with the on/off value for
	 * each key.
	 */
	void			SetScale();
	void			UpdateScale(const BMessage& from);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfig	inherited;
		
	void		AddViews(const BMessage& initSettings, const char* initialName);
	/* Given the root and scale constants, find the actual scale and add
	 * each of its keys to the update message.
	 */
	status_t	AddKeys(int32 rootConstant, int32 scaleConstant, BMessage& upd) const;
};

/*****************************************************************************
 * ARP-ON-KEY-FILTER-SETTINGS
 *****************************************************************************/
class ArpOnKeyFilterSettings : public AmFilterConfigLayout
{
public:
	ArpOnKeyFilterSettings(	AmFilterHolderI* target,
							const BMessage& initSettings);

	virtual	void AttachedToWindow();
	virtual	void MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	ArpMenuField		*mWrongNoteMenu, *mRootMenu, *mScaleMenu;
	BPopUpMenu			*mWrongNotePopUpMenu, *mRootPopUpMenu, *mScalePopUpMenu;
	BCheckBox			*mCBox, *mCSharpBox, *mDBox, *mDSharpBox, *mEBox, *mFBox,
						*mFSharpBox, *mGBox, *mGSharpBox, *mABox, *mASharpBox, *mBBox;

	void RefreshControls(const BMessage& settings);
	bool GetScale(bool* scale) const;
	void SetCheckBoxes();
	void UpdateCheckBox(BCheckBox* box, bool* scale, uint32 index, const char* str);
	/* Called always and only when the user clicks directly on a checkbox.  The
	 * result is that the scale becomes "Other".
	 */
	void CheckBoxInvoked();
};

/*****************************************************************************
 * ARP-ON-KEY-FILTER
 *****************************************************************************/
ArpOnKeyFilter::ArpOnKeyFilter(	ArpOnKeyFilterAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mWrongNotes(SHIFT_CLOSEST),
		  mRoot(0), mScale(MAJOR_SCALE)
{
	/* The mRoot was set to C, mScale was set to Major, so mKey has
	 * to be instantiated with the same scale.
	 */
	for( uint32 k = 0; k < 12; k++ ) mKey[k] = gScales[MAJOR_SCALE][k];
	if( config ) PutConfiguration( config );
}

ArpOnKeyFilter::~ArpOnKeyFilter()
{
}

AmEvent* ArpOnKeyFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	
	event->SetNextFilter(mHolder->FirstConnection() );
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
		if (!note) return event;
		uint8		newNote;
		if (NoteResult(note->Note(), &newNote) ) {
			note->SetNote(newNote);
			return event;
		}
		/* Send all discarded notes out the second output, if any.
		 */
		AmFilterHolderI*	nextHolder;
		if (mWrongNotes == DISCARD && (nextHolder = mHolder->ConnectionAt(1)) != 0) {
			event->SetNextFilter(nextHolder);
			return event;
		}
		event->Delete();
		return 0;
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff* note = dynamic_cast<AmNoteOff*>(event);
		if (!note) return event;
		uint8		newNote;
		if (NoteResult(note->Note(), &newNote) ) {
			note->SetNote(newNote);
			return event;
		}
		/* Send all discarded notes out the second output, if any.
		 */
		AmFilterHolderI*	nextHolder;
		if (mWrongNotes == DISCARD && (nextHolder = mHolder->ConnectionAt(1)) != 0) {
			event->SetNextFilter(nextHolder);
			return event;
		}
		event->Delete();
		return 0;
	}
	return event;
}

BView* ArpOnKeyFilter::NewEditView(BPoint requestedSize) const
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return NULL;
	return new _OnKeyControl( mHolder, config, Label().String() );
}

status_t ArpOnKeyFilter::GetToolTipText(BString* out) const
{
	if( mRoot >=0 && mScale >= 0 ) {
		*out = gRootNames[mRoot];
		*out << " ";
		*out << gScaleNames[mScale];
		AmFilterAddOn* addon = AddOn();
		if (addon) {
			*out << " (" << addon->Name() << ")";
		}
		return B_OK;
	}
	return inherited::GetToolTipText(out);
}


bool ArpOnKeyFilter::NoteResult(uint8 oldNote, uint8* newNote)
{
	uint8		n = oldNote % 12;
	bool		answer = mKey[n];
	*newNote = oldNote;
	if (mWrongNotes == SHIFT_DOWN && !answer) {
		int32		dist = DistanceToNewNote(mKey, n, -1);
		if (oldNote + dist < 0) answer = false;
		else {
			*newNote = (uint8)(oldNote + dist);
			answer = true;
		}
	} else if (mWrongNotes == SHIFT_UP && !answer) {
		int32		dist = DistanceToNewNote(mKey, n, 1);
		if (oldNote + dist > 127) answer = false;
		else {
			*newNote = (uint8)(oldNote + dist);
			answer = true;
		}
	} else if (mWrongNotes == SHIFT_CLOSEST && !answer) {
		int32		downDist = DistanceToNewNote( mKey, n, -1 );
		int32		upDist = DistanceToNewNote( mKey, n, 1 );
		if( (oldNote + downDist < 0) && (oldNote + upDist > 127) ) answer = false;
		else {
			answer = true;
			if( oldNote + downDist < 0 ) *newNote = (uint8)(oldNote + upDist);
			else if( oldNote + upDist > 127 ) *newNote = (uint8)(oldNote + downDist);
			else {
				if( abs(downDist) <= upDist ) *newNote = (uint8)(oldNote + downDist);
				else *newNote = (uint8)(oldNote + upDist);
			}
		}
	}
	return answer;
}

int32 ArpOnKeyFilter::DistanceToNewNote(bool scale[], uint8 note, int32 step)
{
	int32	dist = 0;
	int32	next = note;
	while( !scale[next] ) {
		next = next + step;
		dist += step;
		if( next < 0 ) next = 11;
		if( next > 11 ) next = 0;
	}
	return dist;
}

status_t ArpOnKeyFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if( values->AddInt32( WRONG_NOTES_STR, mWrongNotes)	!= B_OK ) return B_ERROR;
	if( values->AddInt32( ROOT_STR, mRoot)				!= B_OK ) return B_ERROR;
	if( values->AddInt32( SCALE_STR, mScale)			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_C_STR, mKey[0] )			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_CSHARP_STR, mKey[1] )		!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_D_STR, mKey[2] )			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_DSHARP_STR, mKey[3] )		!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_E_STR, mKey[4] )			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_F_STR, mKey[5] )			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_FSHARP_STR, mKey[6] )		!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_G_STR, mKey[7] )			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_GSHARP_STR, mKey[8] )		!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_A_STR, mKey[9] )			!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_ASHARP_STR, mKey[10] )		!= B_OK ) return B_ERROR;
	if( values->AddBool( KEY_B_STR, mKey[11] )			!= B_OK ) return B_ERROR;
	return B_OK;
}

status_t ArpOnKeyFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	int32		i;
	bool		b;
	if( values->FindInt32( WRONG_NOTES_STR, &i) == B_OK )	mWrongNotes = i;
	if( values->FindInt32( ROOT_STR, &i) == B_OK )			mRoot = i;
	if( values->FindInt32( SCALE_STR, &i) == B_OK )			mScale = i;
	if( values->FindBool( KEY_C_STR, &b) == B_OK )			mKey[0] = b;
	if( values->FindBool( KEY_CSHARP_STR, &b) == B_OK )		mKey[1] = b;
	if( values->FindBool( KEY_D_STR, &b) == B_OK )			mKey[2] = b;
	if( values->FindBool( KEY_DSHARP_STR, &b) == B_OK )		mKey[3] = b;
	if( values->FindBool( KEY_E_STR, &b) == B_OK )			mKey[4] = b;
	if( values->FindBool( KEY_F_STR, &b) == B_OK )			mKey[5] = b;
	if( values->FindBool( KEY_FSHARP_STR, &b) == B_OK )		mKey[6] = b;
	if( values->FindBool( KEY_G_STR, &b) == B_OK )			mKey[7] = b;
	if( values->FindBool( KEY_GSHARP_STR, &b) == B_OK )		mKey[8] = b;
	if( values->FindBool( KEY_A_STR, &b) == B_OK )			mKey[9] = b;
	if( values->FindBool( KEY_ASHARP_STR, &b) == B_OK )		mKey[10] = b;
	if( values->FindBool( KEY_B_STR, &b) == B_OK )			mKey[11] = b;
	return B_OK;
}

status_t ArpOnKeyFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpOnKeyFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-ON-KEY-FILTER-ADDON
 *****************************************************************************/
void ArpOnKeyFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I take all incoming notes and makes them conform to a desired key. \n"
	"You can choose from several predefined scales or make your own. Incoming notes \n"
	"that aren't in the selected scale can either be discarded -- they are never heard \n"
	"from again -- shifted down to the next note that is in the scale, shifted up, or \n"
	"shifted up or down, depending upon which shift is closest.</p> \n"
"\n"
	"<p>This filter can have either one or two connections.  If I have two connections \n"
	"and I am in Discard mode, then all discarded events are sent to the second connection.</p>\n";
}

void ArpOnKeyFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpOnKeyFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpOnKeyFilterAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpOnKeyFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpOnKeyFilterAddOn(cookie);
	return NULL;
}

/*************************************************************************
 * _ON-KEY-CONTROL
 *************************************************************************/
_OnKeyControl::_OnKeyControl(	AmFilterHolderI* target,
								const BMessage& initSettings,
								const char* initialName )
		: inherited(target, initSettings, be_plain_font,
					B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW )
{
	AddViews(initSettings, initialName);
	float	right = 0, bottom = 0;
	BView*	view;
	for( view = ChildAt(0); view != 0; view = view->NextSibling() ) {
		BRect	f = view->Frame();
		if( f.right > right ) right = f.right;
		if( f.bottom > bottom ) bottom = f.bottom;
	}
	ResizeTo( right, bottom );
}

void _OnKeyControl::SetScale()
{
	/* Get the current root and scale values from the menu fields.
	 */
	BMenuField*		field = dynamic_cast<BMenuField*>( FindView( "root_field" ) );
	BMenu*			rootMenu = field ? field->Menu() : NULL;
	if( !rootMenu ) return;
	field = dynamic_cast<BMenuField*>( FindView( "scale_field" ) );
	BMenu*			scaleMenu = field ? field->Menu() : NULL;
	if( !scaleMenu ) return;
	
	BMenuItem*		item = rootMenu->FindMarked();
	BMessage*		rootMsg = item ? item->Message() : 0;
	item = scaleMenu->FindMarked();
	BMessage*		scaleMsg = item ? item->Message() : 0;

	int32			root, scale;
	if( rootMsg->FindInt32(ROOT_STR, &root) != B_OK ) return;
	if( scaleMsg->FindInt32(SCALE_STR, &scale) != B_OK ) return;
	/* Fill the update message out with the root, scale, and keys.
	 */
	BMessage upd;
	if( upd.AddInt32(ROOT_STR, root) == B_OK
			&& upd.AddInt32(SCALE_STR, scale) == B_OK
			&& AddKeys( root, scale, upd ) == B_OK ) {
		Implementation().SendConfiguration(&upd);
	}
}

void _OnKeyControl::UpdateScale(const BMessage& from)
{
	int32 root;
	if (from.FindInt32(ROOT_STR, &root) == B_OK) {
		BMenuField*		field = dynamic_cast<BMenuField*>( FindView( "root_field" ) );
		BMenu*			menu = field ? field->Menu() : NULL;
		if( menu ) {
			for (int32 i=0; i<menu->CountItems(); i++) {
				BMenuItem* it = menu->ItemAt(i);
				if (!it) continue;
				BMessage* msg = it->Message();
				if (!msg) continue;
				int32 s;
				if (msg->FindInt32(ROOT_STR, &s) != B_OK) continue;
				if (s == root) {
					it->SetMarked(true);
					break;
				}
			}
		}
	}

	int32 scale;
	if (from.FindInt32(SCALE_STR, &scale) == B_OK) {
		BMenuField*		field = dynamic_cast<BMenuField*>( FindView( "scale_field" ) );
		BMenu*			menu = field ? field->Menu() : NULL;
		if( menu ) {
			for (int32 i=0; i<menu->CountItems(); i++) {
				BMenuItem* it = menu->ItemAt(i);
				if (!it) continue;
				BMessage* msg = it->Message();
				if (!msg) continue;
				int32 s;
				if (msg->FindInt32(SCALE_STR, &s) != B_OK) continue;
				if (s == scale) {
					it->SetMarked(true);
					break;
				}
			}
		}
	}
}
	
void _OnKeyControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( Parent() ) SetViewColor( Parent()->ViewColor() );

	BMenuField*		field = dynamic_cast<BMenuField*>( FindView( "root_field" ) );
	if( field ) field->Menu()->SetTargetForItems( this );
	field = dynamic_cast<BMenuField*>( FindView( "scale_field" ) );
	if( field ) field->Menu()->SetTargetForItems( this );
}

void _OnKeyControl::MessageReceived(BMessage *msg)
{
	switch( msg->what ) {
		case CHOOSE_ROOT_MSG:
			SetScale();
			break;
		case CHOOSE_SCALE2_MSG:
			SetScale();
			break;
		case ARP_PUT_CONFIGURATION_MSG: {
			BMessage settings;
			msg->FindMessage("settings", &settings);
			UpdateScale(settings);
		} break;
		
		default:
			inherited::MessageReceived( msg );
	}
}

void _OnKeyControl::AddViews(const BMessage& initSettings, const char* initialName)
{
	font_height		fheight;
	GetFontHeight( &fheight );
	float			labelW = StringWidth("Scale") + 10;
	float			menuW = StringWidth("Melodic Minor (Desc)") + 25;
	float			height = fheight.ascent + fheight.descent + fheight.leading + 1;
	float			menuH = height + 8;
	float			top = 0;
	BMenu*			rootMenu = new BPopUpMenu("root_popup");
	BMenu*			scaleMenu = new BPopUpMenu("scale_popup");
	if( !rootMenu || !scaleMenu ) {
		delete rootMenu;
		delete scaleMenu;
		return;
	}
	int32	initialRoot = 0, initialScale2 = 0, i;
	initSettings.FindInt32( ROOT_STR, &initialRoot );
	initSettings.FindInt32( SCALE_STR, &initialScale2 );

	for( i = 0; gRootNames[i] != 0; i++ )
		add_menu_item(gRootNames[i], CHOOSE_ROOT_MSG, ROOT_STR, i, initialRoot, rootMenu);

	for( i = 0; gScaleNames[i] != 0; i++ )
		add_menu_item(gScaleNames[i], CHOOSE_SCALE2_MSG, SCALE_STR, i, initialScale2, scaleMenu);
	if( scaleMenu ) scaleMenu->AddSeparatorItem();
	add_menu_item(OTHER_STR, CHOOSE_SCALE2_MSG, SCALE_STR, OTHER_SCALE, initialScale2, scaleMenu, false);

	BRect			f(0, top, labelW + menuW, top + menuH);
	BMenuField*		field = new BMenuField( f, "root_field", "Root:", rootMenu, true );
	if( !field ) delete rootMenu;
	else {
		AddChild( field );
		field->SetDivider( labelW );
	}

	f.OffsetBy(0, menuH + 4);
	field = new BMenuField( f, "scale_field", "Scale:", scaleMenu, true );
	if( !field ) delete scaleMenu;
	else {
		AddChild( field );
		field->SetDivider( labelW );
	}

#if 0	
	if( initialName ) {
		float			labelW = StringWidth( initialName );
		frame.Set( 0, 1, labelW, labelH + 1 );
		BStringView*	sv = new BStringView( frame, initialName, initialName );
		if( sv ) {
			sv->SetFontSize( labelH );
			AddChild( sv );
		}
	}
#endif
}

status_t _OnKeyControl::AddKeys(int32 rootConstant, int32 scaleConstant, BMessage& upd) const
{
	if( scaleConstant < 0 ) return B_ERROR;
	bool		scale[12];
	get_scale(scale, rootConstant, scaleConstant);
	status_t	err;
	if( (err = upd.AddBool( KEY_C_STR, scale[0] ))			!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_CSHARP_STR, scale[1] ))		!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_D_STR, scale[2] ))			!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_DSHARP_STR, scale[3] ))		!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_E_STR, scale[4] ))			!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_F_STR, scale[5] ))			!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_FSHARP_STR, scale[6] ))		!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_G_STR, scale[7] ))			!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_GSHARP_STR, scale[8] ))		!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_A_STR, scale[9] ))			!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_ASHARP_STR, scale[10] ))	!= B_OK ) return err;
	if( (err = upd.AddBool( KEY_B_STR, scale[11] ))			!= B_OK ) return err;
	return B_OK;
}

/*****************************************************************************
 * ARP-ON-KEY-FILTER-SETTINGS
 *****************************************************************************/
ArpOnKeyFilterSettings::ArpOnKeyFilterSettings(	AmFilterHolderI* target,
												const BMessage& initSettings)
		: inherited(target, initSettings),
		  mWrongNoteMenu(0), mRootMenu(0), mScaleMenu(0),
		  mWrongNotePopUpMenu(0), mRootPopUpMenu(0), mScalePopUpMenu(0),
		  mCBox(0), mCSharpBox(0), mDBox(0), mDSharpBox(0), mEBox(0), mFBox(0),
		  mFSharpBox(0), mGBox(0), mGSharpBox(0), mABox(0), mASharpBox(0), mBBox(0)
{
	int32	initialWrongNote = 0;
	int32	initialRoot = 0;
	int32	initialScale2 = 0;
	int32	i;
	initSettings.FindInt32( WRONG_NOTES_STR, &initialWrongNote );
	initSettings.FindInt32( ROOT_STR, &initialRoot );
	initSettings.FindInt32( SCALE_STR, &initialScale2 );

	mRootPopUpMenu = new BPopUpMenu("root_popup");
	for( i = 0; gRootNames[i] != 0; i++ )
		add_menu_item(gRootNames[i], CHOOSE_ROOT_MSG, ROOT_STR, i, initialRoot, mRootPopUpMenu);

	mScalePopUpMenu = new BPopUpMenu("scale2_popup");
	for( i = 0; gScaleNames[i] != 0; i++ )
		add_menu_item(gScaleNames[i], CHOOSE_SCALE2_MSG, SCALE_STR, i, initialScale2, mScalePopUpMenu);
	if( mScalePopUpMenu ) mScalePopUpMenu->AddSeparatorItem();
	add_menu_item(OTHER_STR, CHOOSE_SCALE2_MSG, SCALE_STR, OTHER_SCALE, initialScale2, mScalePopUpMenu);
	
	mWrongNotePopUpMenu = new BPopUpMenu("wrong_note_popup");
	for( i = 0; wrongNoteNames[i] != 0; i++ )
		add_menu_item(wrongNoteNames[i], CHOOSE_WRONG_NOTE_MSG, WRONG_NOTES_STR, i, initialWrongNote, mWrongNotePopUpMenu);

	try {
		AddLayoutChild((new ArpRunningBar("TopVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpTextControl(
									SZ_FILTER_LABEL, "Label:","",
									mImpl.AttachTextControl(SZ_FILTER_LABEL)))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "8")
					.SetString(ArpTextControl::PrefTextStringP, "8888888888")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpRunningBar("KeyHBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)

				->AddLayoutChild((new ArpRunningBar("KeyCol1"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)

					->AddLayoutChild((new ArpViewWrapper(mCBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_C_STR, "C",
																		mImpl.AttachCheckBox(KEY_C_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)

					->AddLayoutChild((new ArpViewWrapper(mCSharpBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_CSHARP_STR, "C#",
																		mImpl.AttachCheckBox(KEY_CSHARP_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mDBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_D_STR, "D",
																		mImpl.AttachCheckBox(KEY_D_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mDSharpBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_DSHARP_STR, "D#",
																		mImpl.AttachCheckBox(KEY_DSHARP_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mEBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_E_STR, "E",
																		mImpl.AttachCheckBox(KEY_E_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mFBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_F_STR, "F",
																		mImpl.AttachCheckBox(KEY_F_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
				)

				->AddLayoutChild((new ArpRunningBar("KeyCol2"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)

					->AddLayoutChild((new ArpViewWrapper(mFSharpBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_FSHARP_STR, "F#",
																		mImpl.AttachCheckBox(KEY_FSHARP_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)

					->AddLayoutChild((new ArpViewWrapper(mGBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_G_STR, "G",
																		mImpl.AttachCheckBox(KEY_G_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)

					->AddLayoutChild((new ArpViewWrapper(mGSharpBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_GSHARP_STR, "G#",
																		mImpl.AttachCheckBox(KEY_GSHARP_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)

					->AddLayoutChild((new ArpViewWrapper(mABox = new BCheckBox( BRect(0,0,0,0),
																		KEY_A_STR, "A",
																		mImpl.AttachCheckBox(KEY_A_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)

					->AddLayoutChild((new ArpViewWrapper(mASharpBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_ASHARP_STR, "A#",
																		mImpl.AttachCheckBox(KEY_ASHARP_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)

					->AddLayoutChild((new ArpViewWrapper(mBBox = new BCheckBox( BRect(0,0,0,0),
																		KEY_B_STR, "B",
																		mImpl.AttachCheckBox(KEY_B_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)


				)

			)
			->AddLayoutChild((mWrongNoteMenu =
							  new ArpMenuField("wrong_note_menu", "Wrong notes: ",
												mWrongNotePopUpMenu))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((mRootMenu =
							  new ArpMenuField("root_menu", "Root:",
												mRootPopUpMenu))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((mScaleMenu =
							  new ArpMenuField("scale2_menu", "Scale:",
												mScalePopUpMenu))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);

	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}

void ArpOnKeyFilterSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( mWrongNotePopUpMenu )	mWrongNotePopUpMenu->SetTargetForItems(this);
	if( mRootPopUpMenu )		mRootPopUpMenu->SetTargetForItems(this);
	if( mScalePopUpMenu )		mScalePopUpMenu->SetTargetForItems(this);
}
	
void ArpOnKeyFilterSettings::MessageReceived(BMessage *msg)
{
	switch( msg->what ) {
		case CHOOSE_WRONG_NOTE_MSG:
			{
				if( mWrongNotePopUpMenu ) {
					BMenuItem* item = mWrongNotePopUpMenu->FindMarked();
					BMessage* msg = item ? item->Message() : 0;
					if( msg ) {
						Implementation().SendConfiguration( msg );
						mSettings.Update(*msg);
					}
				}
			}
			break;

		case CHOOSE_ROOT_MSG:
			{
				if( mRootPopUpMenu ) {
					BMenuItem* item = mRootPopUpMenu->FindMarked();
					BMessage* msg = item ? item->Message() : 0;
					if( msg ) {
						Implementation().SendConfiguration( msg );
						mSettings.Update(*msg);
					}
					SetCheckBoxes();
				}
			}
			break;
		case CHOOSE_SCALE2_MSG:
			{
				if( mScalePopUpMenu ) {
					BMenuItem* item = mScalePopUpMenu->FindMarked();
					BMessage* msg = item ? item->Message() : 0;
					if( msg ) {
						Implementation().SendConfiguration( msg );
						mSettings.Update(*msg);
					}
					SetCheckBoxes();
				}
			}
			break;
		case ArpConfigureImpl::CONFIG_REPORT_MSG:
			/* This happens in certain cases, like a checkbox was pressed, or
			 * a text control.  Use the parameter string to determine if it was,
			 * in fact, a checkbox, and if so set the root and scale to other.
			 */
			{
				const char*		param;
				if( msg->FindString("arp:param", &param) == B_OK ) {
					if( strcmp(param, KEY_C_STR) == 0 || strcmp(param, KEY_CSHARP_STR)
							|| strcmp(param, KEY_D_STR) == 0 || strcmp(param, KEY_DSHARP_STR)
							|| strcmp(param, KEY_E_STR) == 0 || strcmp(param, KEY_F_STR)
							|| strcmp(param, KEY_FSHARP_STR) == 0 || strcmp(param, KEY_G_STR)
							|| strcmp(param, KEY_GSHARP_STR) == 0 || strcmp(param, KEY_A_STR)
							|| strcmp(param, KEY_ASHARP_STR) == 0 || strcmp(param, KEY_B_STR) ) {
						CheckBoxInvoked();
					}
				}
			}
			// Note: no break on purpose				
		case ARP_PUT_CONFIGURATION_MSG:
			{
				BMessage	settings;
				if( msg->FindMessage( "settings", &settings ) == B_OK )
					RefreshControls( settings );
			}
			// Note: no break on purpose
		default:
			inherited::MessageReceived( msg );
	}
}

void ArpOnKeyFilterSettings::RefreshControls(const BMessage& settings)
{
	int32		value;
	BMenuItem*	item;
	if( mWrongNotePopUpMenu && settings.FindInt32(WRONG_NOTES_STR, &value) == B_OK ) {
		for( int32 k = 0; (item = mWrongNotePopUpMenu->ItemAt(k)); k++ ) {
			if( k == value ) item->SetMarked(true);
			else item->SetMarked(false);
		}
	}
	if( mRootPopUpMenu && settings.FindInt32(ROOT_STR, &value) == B_OK ) {
		for( int32 k = 0; (item = mRootPopUpMenu->ItemAt(k)); k++ ) {
			if( k == value ) item->SetMarked(true);
			else item->SetMarked(false);
		}
	}
	if( mScalePopUpMenu && settings.FindInt32(SCALE_STR, &value) == B_OK ) {
		for( int32 k = 0; (item = mScalePopUpMenu->ItemAt(k)); k++ ) {
			if( k == value ) item->SetMarked(true);
			else item->SetMarked(false);
		}
	}
}

bool ArpOnKeyFilterSettings::GetScale(bool* scale) const
{
	if( !mRootPopUpMenu || !mScalePopUpMenu ) return false;
	int32		rootConstant, scaleConstant;
	BMenuItem* item = mRootPopUpMenu->FindMarked();
	BMessage* msg = item ? item->Message() : 0;
	if( !msg || msg->FindInt32( ROOT_STR, &rootConstant ) != B_OK ) return false;
	item = mScalePopUpMenu->FindMarked();
	msg = item ? item->Message() : 0;
	if( !msg || msg->FindInt32( SCALE_STR, &scaleConstant ) != B_OK ) return false;
	if( scaleConstant < 0 ) return false;
	get_scale(scale, rootConstant, scaleConstant);
	return true;
}
	
void ArpOnKeyFilterSettings::SetCheckBoxes()
{
	bool	scale[12];
	if( !GetScale( scale ) ) return;
	UpdateCheckBox( mCBox,		scale, 0,	KEY_C_STR );
	UpdateCheckBox( mCSharpBox,	scale, 1,	KEY_CSHARP_STR );
	UpdateCheckBox( mDBox,		scale, 2,	KEY_D_STR );
	UpdateCheckBox( mDSharpBox, scale, 3,	KEY_DSHARP_STR );
	UpdateCheckBox( mEBox,		scale, 4,	KEY_E_STR );
	UpdateCheckBox( mFBox,		scale, 5,	KEY_F_STR );
	UpdateCheckBox( mFSharpBox, scale, 6,	KEY_FSHARP_STR );
	UpdateCheckBox( mGBox,		scale, 7,	KEY_G_STR );
	UpdateCheckBox( mGSharpBox, scale, 8,	KEY_GSHARP_STR );
	UpdateCheckBox( mABox,		scale, 9,	KEY_A_STR );
	UpdateCheckBox( mASharpBox, scale, 10,	KEY_ASHARP_STR );
	UpdateCheckBox( mBBox,		scale, 11,	KEY_B_STR );
}

void ArpOnKeyFilterSettings::UpdateCheckBox(BCheckBox* box, bool* scale, uint32 index, const char* str)
{
	if( !box ) return;
	scale[index] ? box->SetValue( B_CONTROL_ON ) : box->SetValue( B_CONTROL_OFF );
	BMessage upd;
	if( upd.AddBool( str, scale[index] ) == B_OK )
		Implementation().SendConfiguration(&upd);
}

void ArpOnKeyFilterSettings::CheckBoxInvoked()
{
	if( !mScalePopUpMenu ) return;
	BMenuItem*	item;
	for( int32 k = 0; (item = mScalePopUpMenu->ItemAt(k)); k++ ) {
		if( strcmp( OTHER_STR, item->Label() ) == 0 ) item->SetMarked(true);
		else item->SetMarked(false);
	}
	BMessage upd;
	if( upd.AddInt32( SCALE_STR, OTHER_SCALE ) == B_OK )
		Implementation().SendConfiguration(&upd);
}
