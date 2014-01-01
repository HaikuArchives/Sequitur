/* AmStdFilters.cpp
 */

#define _BUILDING_AmKernel 1

#ifndef AMKERNEL_AMSTDFILTERS_H
#include "AmKernel/AmStdFilters.h"
#endif

#ifndef AMKERNEL_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#include <ArpLayout/ArpViewWrapper.h>

#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmFilterHolder.h"

#include <Autolock.h>
#include <CheckBox.h>
//#ifndef _MIDI_SYNTH_H
#include <MidiSynth.h>
//#endif
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <MidiRoster.h>

#include <experimental/BitmapTools.h>

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef ARPKERNEL_ARPBITMAPTOOLS_H
#include <ArpKernel/ArpBitmapTools.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

ArpMOD();

static const char*	STR_CHANNEL_NUM		= "Num";

static const char*	S_CHANNEL			= "channel";
static const char*	S_TYPE_MASK			= "type_mask";
static const char*	S_NOTES_OFF_CC		= "notes_off_cc";
static const char*	S_MIDI_CLOCK		= "midi_clock";

static const char*	DEVICE_MFG_STR		= "device_mfg";
static const char*	DEVICE_PROD_STR		= "device_product";
static const char*	DEVICE_LABEL_STR	= "device_label";

enum {
	NOTE_TYPE				= 0x0001,
	CHANNELPRESSURE_TYPE	= 0x0002,
	CONTROLCHANGE_TYPE		= 0x0004,
	KEYPRESSURE_TYPE		= 0x0008,
	PITCHBEND_TYPE			= 0x0010,
	PROGRAMCHANGE_TYPE		= 0x0020,
	SYSTEMCOMMON_TYPE		= 0x0040,
	SYSTEMEXCLUSIVE_TYPE	= 0x0080,
	SYSTEMREALTIME_TYPE		= 0x0100,
	TEMPOCHANGE_TYPE		= 0x0200,
	
	DEFAULT_DESTINATION_TYPES	=
		NOTE_TYPE|CHANNELPRESSURE_TYPE|CONTROLCHANGE_TYPE|
		KEYPRESSURE_TYPE|PITCHBEND_TYPE|PROGRAMCHANGE_TYPE|
		SYSTEMCOMMON_TYPE|SYSTEMEXCLUSIVE_TYPE|SYSTEMREALTIME_TYPE|
		TEMPOCHANGE_TYPE,
	DEFAULT_SOURCE_TYPES	=
		NOTE_TYPE|CHANNELPRESSURE_TYPE|CONTROLCHANGE_TYPE|
		KEYPRESSURE_TYPE|PITCHBEND_TYPE|PROGRAMCHANGE_TYPE|
		SYSTEMEXCLUSIVE_TYPE
};

/*************************************************************************
 * COMMON FUNCTIONALITY
 *************************************************************************/

static void add_channel_item(	char* channelName,
								int32 channelNum,
								int32 curChannel,
								BMenu* menu)
{
	BMessage	*msg;
	BMenuItem	*item;
	if ((msg = new BMessage('zzzz')) == 0) return;
	msg->AddInt32(STR_CHANNEL_NUM, channelNum);
	item = new BMenuItem(channelName, msg, 0, 0);
	if (channelNum == curChannel) item->SetMarked(true);
	menu->AddItem(item);
}

static int32 run_channel_popup(BRect frame, BPoint where, int32 curChannel, bool released)
{
	BPopUpMenu*		popUp; 
	if ((popUp = new BPopUpMenu("", true, true, B_ITEMS_IN_COLUMN))
			== 0) return -1;
	add_channel_item("1", 0, curChannel, popUp);
	add_channel_item("2", 1, curChannel, popUp);
	add_channel_item("3", 2, curChannel, popUp);
	add_channel_item("4", 3, curChannel, popUp);
	add_channel_item("5", 4, curChannel, popUp);
	add_channel_item("6", 5, curChannel, popUp);
	add_channel_item("7", 6, curChannel, popUp);
	add_channel_item("8", 7, curChannel, popUp);
	add_channel_item("9", 8, curChannel, popUp);
	add_channel_item("10", 9, curChannel, popUp);
	add_channel_item("11", 10, curChannel, popUp);
	add_channel_item("12", 11, curChannel, popUp);
	add_channel_item("13", 12, curChannel, popUp);
	add_channel_item("14", 13, curChannel, popUp);
	add_channel_item("15", 14, curChannel, popUp);
	add_channel_item("16", 15, curChannel, popUp);

	BMenuItem	*selected = released
						  ? popUp->Go(where, false, true, false)
						  : popUp->Go(where, false, false, frame, false);
	if ((selected == 0) || (selected->Message() == 0)) return -1;
	int32	answer;
	if ((selected->Message()->FindInt32(STR_CHANNEL_NUM, &answer)) != B_OK) return -1;
	return answer;
}

static BBitmap* create_overlay(	BBitmap** base, ArpCRef<AmDeviceI> device,
								AmFilterAddOn* addon, int32 channel,
								BPoint requestedSize)
{
	if (*base) {
		return new BBitmap(*base);
	}
	
	if (device) {
		const BBitmap*	bm = device->Icon(requestedSize);
		if (bm) *base = new BBitmap(bm);
	}
	if (!*base && addon)
		*base = addon->FinalImage(requestedSize);
	if (!*base) return NULL;
	
	const BBitmap*	overlay = 0;
	ArpImageManagerI&	im = ImageManager();
	switch( channel ) {
		case 0:		overlay = im.FindBitmap(CHANNEL_01_IMAGE_STR);	break;
		case 1:		overlay = im.FindBitmap(CHANNEL_02_IMAGE_STR);	break;
		case 2:		overlay = im.FindBitmap(CHANNEL_03_IMAGE_STR);	break;
		case 3:		overlay = im.FindBitmap(CHANNEL_04_IMAGE_STR);	break;
		case 4:		overlay = im.FindBitmap(CHANNEL_05_IMAGE_STR);	break;
		case 5:		overlay = im.FindBitmap(CHANNEL_06_IMAGE_STR);	break;
		case 6:		overlay = im.FindBitmap(CHANNEL_07_IMAGE_STR);	break;
		case 7:		overlay = im.FindBitmap(CHANNEL_08_IMAGE_STR);	break;
		case 8:		overlay = im.FindBitmap(CHANNEL_09_IMAGE_STR);	break;
		case 9:		overlay = im.FindBitmap(CHANNEL_10_IMAGE_STR);	break;
		case 10:	overlay = im.FindBitmap(CHANNEL_11_IMAGE_STR);	break;
		case 11:	overlay = im.FindBitmap(CHANNEL_12_IMAGE_STR);	break;
		case 12:	overlay = im.FindBitmap(CHANNEL_13_IMAGE_STR);	break;
		case 13:	overlay = im.FindBitmap(CHANNEL_14_IMAGE_STR);	break;
		case 14:	overlay = im.FindBitmap(CHANNEL_15_IMAGE_STR);	break;
		case 15:	overlay = im.FindBitmap(CHANNEL_16_IMAGE_STR);	break;
	}

	if( !overlay ) return new BBitmap(*base);
	overlay_bitmap( *base, overlay );
//	delete overlay;
	
	return new BBitmap(*base);
}

static uint8 gUglyIcon[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x3f, 0x3f, 0x00, 0xff, 0xff, 
	0xff, 0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 0xff, 
	0xff, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0xff, 
	0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 
	0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 
	0x00, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x00, 
	0x00, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x00, 
	0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 
	0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 
	0xff, 0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0xff, 
	0xff, 0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x00, 0xff, 
	0xff, 0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff
};

static BBitmap* create_icon(BMidiEndpoint* device)
{
	BBitmap* icon = NULL;
	ArpImageManagerI&	im = ImageManager();
	
	if (device) {
		BMessage prop;
		if (device->GetProperties(&prop) == B_OK) {
			const void* data;
			ssize_t size;
			if (prop.FindData("be:small_icon", 'MICN', &data, &size) == B_OK ||
					prop.FindData("be:mini_icon", 'MICN', &data, &size) == B_OK) {
				if (size == sizeof(gUglyIcon) && memcmp(data, gUglyIcon, size) == 0) {
					icon = const_cast<BBitmap*>( im.FindBitmap( MIDI_PORT_FILTER_IMAGE_STR ) );
					if( icon ) icon = new BBitmap(icon);
				} else if (size == 256) {
					BBitmap raw(BRect(0, 0, 15, 15), 0, B_CMAP8);
					raw.SetBits(data, size, 0, B_CMAP8);
					BBitmap sized(BRect(0, 0, 19, 19), 0, B_CMAP8);
					memset(sized.Bits(), B_TRANSPARENT_MAGIC_CMAP8,
						   sized.BitsLength());
					copy_bitmap(&sized, &raw,
								BRect(0, 0, 15, 15), BPoint(2, 2));
					icon = new BBitmap(BRect(0, 0, 19, 19), 0, B_RGBA32);
					set_bitmap(icon, &sized, false);
				}
			} else if (prop.FindData("be:large_icon", 'ICON', &data, &size) == B_OK) {
				if (size == 1024) {
					BBitmap raw(BRect(0, 0, 31, 31), 0, B_CMAP8);
					raw.SetBits(data, size, 0, B_CMAP8);
					BBitmap sized(BRect(0, 0, 19, 19), 0, B_CMAP8);
					scale_bitmap(&sized, &raw);
					icon = new BBitmap(BRect(0, 0, 19, 19), 0, B_RGBA32);
					set_bitmap(icon, &sized, false);
				}
			}
		}
	}
	if (!icon) {
		icon = const_cast<BBitmap*>( im.FindBitmap(MIDI_PORT_FILTER_IMAGE_STR) );
		if( icon ) icon = new BBitmap(icon);
	}
	
	return icon;
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------

static const uint32		CHANNEL_MENU_MSG	= 'ichm';

static void strip_dev_midi(BString& name)
{
	if (name.Length() < 11) return;
	if (name.Compare("/dev/midi/", 10) != 0) return;
	name.RemoveFirst("/dev/midi/");
}


class ArpEndpointFilterSettings : public AmFilterConfigLayout
{
public:
	ArpEndpointFilterSettings(AmFilterHolderI* target,
						  const BMessage& initSettings,
						  bool isSource)
		: AmFilterConfigLayout(target, initSettings),
		  mChannelPopUpMenu(0)
	{
		BString		shortName( target->Filter()->AddOn()->Name() );
		strip_dev_midi(shortName);
		SetName( shortName.String() );

		int32	initChannel;
		if( initSettings.FindInt32(S_CHANNEL, &initChannel) != B_OK ) initChannel = 0;
		mChannelPopUpMenu = new BPopUpMenu("channel_popup");
		for( int32 k = 0; k < 16; k++ ) {
			BString		label;
			label << k + 1;
			BMessage*	msg = new BMessage(CHANNEL_MENU_MSG);
			if( msg ) {
				msg->AddInt32(S_CHANNEL, k);
				BMenuItem*	item = new BMenuItem( label.String(), msg );
				if( item ) {
					mChannelPopUpMenu->AddItem( item );
					if( k == initChannel ) item->SetMarked(true);
				}
			}
		}
		try {
			ArpRunningBar* top = new ArpRunningBar("TopVBar");
			top->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			);
			top
				->AddLayoutChild((new ArpTextControl(
										SZ_FILTER_LABEL, "Label:","",
										mImpl.AttachTextControl(SZ_FILTER_LABEL)))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "8")
						.SetString(ArpTextControl::PrefTextStringP, "8888888888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpMenuField("channel_menu", "Channel: ",
													mChannelPopUpMenu))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpBox("box",
											 isSource ? "Record Events" : "Perform Events"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->AddLayoutChild((new ArpRunningBar("BoxVGroup1"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, 0)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "notes", "Notes",
										mImpl.AttachCheckBox(S_TYPE_MASK, NOTE_TYPE, "notes"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "kpressure", "Key Pressure",
										mImpl.AttachCheckBox(S_TYPE_MASK, KEYPRESSURE_TYPE, "kpressure"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "cpressure", "Channel Pressure",
										mImpl.AttachCheckBox(S_TYPE_MASK, CHANNELPRESSURE_TYPE, "cpressure"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "pbend", "Pitch Bend",
										mImpl.AttachCheckBox(S_TYPE_MASK, PITCHBEND_TYPE, "pbend"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "cchange", "Control Change",
										mImpl.AttachCheckBox(S_TYPE_MASK, CONTROLCHANGE_TYPE, "cchange"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
						)
						->AddLayoutChild((new ArpRunningBar("BoxVGroup1"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, 0)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "pchange", "Program Change",
										mImpl.AttachCheckBox(S_TYPE_MASK, PROGRAMCHANGE_TYPE, "pchange"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "scommon", "System Common",
										mImpl.AttachCheckBox(S_TYPE_MASK, SYSTEMCOMMON_TYPE, "scommon"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "sexclusive", "System Exclusive",
										mImpl.AttachCheckBox(S_TYPE_MASK, SYSTEMEXCLUSIVE_TYPE, "sexclusive"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "srealtime", "System Realtime",
										mImpl.AttachCheckBox(S_TYPE_MASK, SYSTEMREALTIME_TYPE, "srealtime"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "tchange", "Tempo Change",
										mImpl.AttachCheckBox(S_TYPE_MASK, TEMPOCHANGE_TYPE, "tchange"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
						)
					)
				);
			if (!isSource) {
				top	->AddLayoutChild((new ArpViewWrapper(
						new BCheckBox(BRect(0,0,10,10), "midiClock", "Send MIDI Clock (Global for Port)",
								mImpl.AttachCheckBox(S_MIDI_CLOCK, 0, "midiClock"),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					);
#if 0
				top	->AddLayoutChild((new ArpViewWrapper(
						new BCheckBox(BRect(0,0,10,10), "notesOffCc", "Send Notes Off Control Change",
								mImpl.AttachCheckBox(S_NOTES_OFF_CC, 0, "notesOffCc"),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					);
#endif
			}
			AddLayoutChild(top);
		} catch(...) {
			throw;
		}
		
		Implementation().RefreshControls(mSettings);
	}

	void AttachedToWindow()
	{
		inherited::AttachedToWindow();
		if( mChannelPopUpMenu )	mChannelPopUpMenu->SetTargetForItems(this);
	}
	
	void MessageReceived(BMessage *msg)
	{
		switch( msg->what ) {
			case CHANNEL_MENU_MSG:
				{
					if( mChannelPopUpMenu ) {
						BMenuItem* item = mChannelPopUpMenu->FindMarked();
						BMessage* msg = item ? item->Message() : 0;
						if( msg ) {
							Implementation().SendConfiguration( msg );
							mSettings.Update(*msg);
						}
					}
				}
			case ARP_PUT_CONFIGURATION_MSG:
				{
					BMessage	settings;
					if( msg->FindMessage( "settings", &settings ) == B_OK )
						RefreshControls( settings );
				}
				// Note: no break on purpose
				break;
			default:
				inherited::MessageReceived( msg );
		}
}

protected:
	typedef AmFilterConfigLayout inherited;

	void RefreshControls(const BMessage& settings)
	{
		int32		value;
		BMenuItem*	item;
		if( mChannelPopUpMenu && settings.FindInt32(S_CHANNEL, &value) == B_OK ) {
			for( int32 k = 0; (item = mChannelPopUpMenu->ItemAt(k)); k++ ) {
				if( k == value ) item->SetMarked(true);
				else item->SetMarked(false);
			}
		}
	}
	
private:
	BPopUpMenu*		mChannelPopUpMenu;
};

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------

/*************************************************************************
 * AM-LOCAL-CONSUMER
 * A BMidiLocalConsumer for transforming data into events.
 *************************************************************************/
class AmEventProducer : public BMidiLocalConsumer
{
public:
	AmEventProducer(AmProducerFilter* owner);
	virtual ~AmEventProducer();

	void SetChannel(int32 channel);
	void SetTypeMask(int32 mask);
	
	virtual	void NoteOff(uchar channel, uchar note, uchar velocity, 
						 bigtime_t time);
	virtual	void NoteOn(uchar channel, uchar note, uchar velocity, 
						bigtime_t time);
	virtual	void KeyPressure(uchar channel, uchar note, uchar pressure, 
							 bigtime_t time);
	virtual	void ControlChange(uchar channel, uchar controlNumber, 
							   uchar controlValue, 
							   bigtime_t time);
	virtual	void ProgramChange(uchar channel, uchar programNumber, 
							   bigtime_t time);
	virtual	void ChannelPressure(uchar channel, uchar pressure, 
								 bigtime_t time);
	virtual	void PitchBend(uchar channel, uchar lsb, uchar msb, 
						   bigtime_t time);
	virtual	void SystemExclusive(void* data, size_t dataLength, 
								 bigtime_t time);
	virtual	void SystemCommon(uchar statusByte, uchar data1, uchar data2, 
							  bigtime_t time);
	virtual void AllNotesOff(bool justChannel, 
							 bigtime_t time);

private:
	AmProducerFilter*	mOwner;
	int32				mChannel;
	int32				mTypeMask;
};

/*************************************************************************
 * ARP-MIDI-PRODUCER-FILTER
 *************************************************************************/
AmProducerFilter::AmProducerFilter(	AmProducerFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config )
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder),
	  mChannel(0), mTypeMask(DEFAULT_SOURCE_TYPES),
	  mConsumer(NULL), mProducer(0),
	  mImageBitmap(NULL), mInputHolder(0)
{
	SetFlag(WATCH_DEVICE_FLAG, true);
	
	mProducer = addon->Producer();
	if (mProducer) {
		mProducer->Acquire();
		mConsumer = new AmEventProducer(this);
		if (mProducer != NULL && mConsumer != NULL) {
			mProducer->Connect(mConsumer);
		}
	}
	
	mConsumer->SetChannel(mChannel);
	mConsumer->SetTypeMask(mTypeMask);

	if (config) PutConfiguration(config);
	else CacheFilters();
}

AmProducerFilter::~AmProducerFilter()
{
#ifdef AM_LOGGING
	BString		str("AmProducerFilter::~AmProducerFilter() ");
	str << Label() << "\n";
	AM_BLOG(str);
#endif
	delete mImageBitmap;
	UncacheFilters();
	if ( (mProducer != 0) && (mConsumer != 0) ) mProducer->Disconnect(mConsumer);
	if (mProducer !=0) mProducer->Release();
	if (mConsumer !=0) mConsumer->Release();
}

BString AmProducerFilter::Label() const
{
	if (HasLabel()) return AmFilterI::Label();

	if (mProducer) {
		am_studio_endpoint	endpoint(	mProducer->Name(), AM_PRODUCER_TYPE,
										mProducer->ID(), mChannel);
		BString		mfg, prod, label;
		if (AmGlobals().GetDeviceInfo(endpoint, mfg, prod, label) == B_OK) {
			if (label.Length() > 0) return label;
			BString		s(mfg);
			if (mfg.Length() > 0 && prod.Length() > 0) s << " ";
			s << prod;
			if (s.Length() > 0) return s;
		}
	}
		
	return Name();
}

status_t AmProducerFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err=values->AddInt32(S_CHANNEL, mChannel)) != B_OK) return err;
	if ((err=values->AddInt32(S_TYPE_MASK, mTypeMask)) != B_OK) return err;

	if (mProducer) {
		am_studio_endpoint		endpoint(	mProducer->Name(), AM_PRODUCER_TYPE,
											mProducer->ID(), mChannel);
		BString			mfg, product, label;
		if (AmGlobals().GetDeviceInfo(endpoint, mfg, product, label) == B_OK) {
			if (mfg.Length() > 0)
				if ((err = values->AddString(DEVICE_MFG_STR, mfg)) != B_OK) return err;
			if (product.Length() > 0)
				if ((err = values->AddString(DEVICE_PROD_STR, product)) != B_OK) return err;
			if (label.Length() > 0)
				if ((err = values->AddString(DEVICE_LABEL_STR, label)) != B_OK) return err;
		}
	}

	return B_OK;
}

status_t AmProducerFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32 i;
	if (values->FindInt32(S_CHANNEL, &i) == B_OK) SetChannel(i);
	if (values->FindInt32(S_TYPE_MASK, &i) == B_OK) {
		mTypeMask = i;
		mConsumer->SetTypeMask(mTypeMask);
	}
	CacheFilters();
	/* For backwards compatibility I have to blatantly set this
	 * -- any filters created with previous versions of sequitur
	 * will wipe this out.  Not sure if there are any cases where
	 * I don't want it true.
	 */
	SetFlag(WATCH_DEVICE_FLAG, true);
	return B_OK;
}

status_t AmProducerFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpEndpointFilterSettings(mHolder, config, true));
	return B_OK;
}

status_t AmProducerFilter::GetNextConfiguration(BMessage* value) const
{
	if (mChannel < 15) {
		return value->AddInt32(S_CHANNEL, mChannel+1);
	}
	return B_ERROR;
}

status_t AmProducerFilter::GetToolTipText(BString* out) const
{
	BString		label = Label();
	if (label.Length() > 0) {
		*out = label.String();
		return B_OK;
	}
	AmFilterAddOn* addon = AddOn();
	if (addon) {
		*out = addon->Name();
		return B_OK;
	}
	return B_ERROR;
}

void AmProducerFilter::SetChannel(int32 channel)
{
	BAutolock _l(mAccess);
	if (mChannel != channel) {
		delete mImageBitmap;
		mImageBitmap = NULL;
		mChannel = channel;
		mConsumer->SetChannel(mChannel);
		if (mHolder) mHolder->FilterChangeNotice();
	}
}

ArpCRef<AmDeviceI> AmProducerFilter::Device() const
{
	BAutolock _l(mAccess);
	if (!mProducer) return NULL;
	am_studio_endpoint		endpoint(	mProducer->Name(), AM_PRODUCER_TYPE,
										mProducer->ID(), mChannel);
	return AmGlobals().DeviceAt(endpoint);
}

void AmProducerFilter::DeviceChanged()
{
	CacheFilters();
}

int32 AmProducerFilter::HintChannel() const
{
	return mChannel;
}

AmEvent* AmProducerFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!mInputFilter) return event;
	/* I need to nil out the next filter of any generated events,
	 * or else they'll disappear somewhere in the system.
	 */
	event = mInputFilter->HandleEvent(event, params);
	AmEvent*	e = event;
	while (e) { e->SetNextFilter(0); e = e->NextEvent(); }
	return event;
}

void AmProducerFilter::MouseAction(	BRect frame, BPoint where,
									uint32 buttons, bool released)
{
	int32 channel = run_channel_popup(frame, where, mChannel, released);
	if( channel >= 0 && mHolder ) {
		BMessage	msg;
		if( msg.AddInt32(S_CHANNEL, channel) == B_OK ) mHolder->PutConfiguration(&msg);
	}
}

BBitmap* AmProducerFilter::Image(BPoint requestedSize) const
{
	BAutolock _l(mAccess);
	return create_overlay(&mImageBitmap, Device(), AddOn(), mChannel, requestedSize);
}

AmTime AmProducerFilter::MakeTime(bigtime_t sysTime) const
{
//	return mHolder->RealtimeToPulse(sysTime);
	AmTime	time = (sysTime <= 0) ? system_time() : sysTime;
	return mHolder->RealtimeToPulse(time);
}

void AmProducerFilter::GenerateEvents(AmEvent* chain)
{
	ArpD(chain->Print());
	mHolder->GenerateEvents(chain);
}

void AmProducerFilter::CacheFilters()
{
	UncacheFilters();
	if (!mHolder) return;
	ArpCRef<AmDeviceI>		device = mHolder->TrackDevice();
	if (!device) return;
	BString					key = device->InputFilterKey();
	if (key.Length() < 1) return;
	ArpRef<AmFilterAddOn>	addon = am_find_filter_addon(key.String());
	if (!addon) return;
	/* FIX:  Do I need to create a dummy filter holder?
	 */
	track_id				tid = (mHolder) ? mHolder->TrackId() : 0;
	pipeline_id				pid = (mHolder) ? mHolder->PipelineId() : 0;
	mInputHolder = new AmFilterHolder(0, tid, pid, 0);
	if (!mInputHolder) return;
	mInputFilter = addon->NewInstance(mInputHolder, 0);
	if (mInputFilter) mInputFilter->TurnOffWtfHack();
}

void AmProducerFilter::UncacheFilters()
{
	mInputFilter = 0;
	if (mInputHolder) {
		mInputHolder->Delete();
		mInputHolder = 0;
	}
}

void AmProducerFilter::HandleMMC(uint8 device, uint8 command)
{
	if (!mHolder) return;
	AmSongRef		ref(AmGlobals().SongRef(mHolder->MatrixId()));
	if (!ref.IsValid()) return;

	BMessage		msg;	
	msg.what = 0;
	// d[1] is device ID, ignored for now
	switch(command) {
		case 1:				// Stop
			msg.what = AM_STOP_MMC;
			break;
		case 2:				// Play
			msg.what = AM_PLAY_MMC;
			break;
		case 4:				// Fast forward
			msg.what = AM_FASTFORWARD_MMC;
			break;
		case 5:				// Rewind
			msg.what = AM_REWIND_MMC;
			break;
		case 6:				// Record
			msg.what = AM_RECORD_MMC;
			break;
		default: break;
	}

	if (msg.what != 0) ref.WindowMessage(&msg);
}

// #pragma mark -

/*************************************************************************
 * ARP-MIDI-CONSUMER-FILTER-ADDON
 *************************************************************************/

AmProducerFilterAddOn::AmProducerFilterAddOn(const void* cookie,
											 BMidiProducer* consumer,
											 int32 producerIndex)
	: AmFilterAddOn(cookie),
	  mProducer(consumer), mProducerIndex(producerIndex), mIcon(NULL)
{
	if (mProducer) mProducer->Acquire();
	mIcon = create_icon(mProducer);
}

AmProducerFilterAddOn::~AmProducerFilterAddOn()
{
	if (mProducer) mProducer->Release();
	mProducer = NULL;
	delete mIcon;
	mIcon = NULL;
}

BString AmProducerFilterAddOn::Name() const
{
	if (mProducer) {
		if (mProducerIndex > 0) {
			BString		n = mProducer->Name();
			n << " " << mProducerIndex;
			return n;
		}
		return mProducer->Name();
	}
	return "--unknown--";
}

status_t AmProducerFilterAddOn::GetLabel(	BString& outLabel, bool useName,
											BMessage* archivedFilter) const
{
	if (mProducer) {
		uint8		channel = 0;
		if (archivedFilter) {
			int32	i;
			if (archivedFilter->FindInt32(S_CHANNEL, &i) == B_OK) channel = uint8(i);
		}
		am_studio_endpoint	endpoint(	mProducer->Name(), AM_PRODUCER_TYPE,
										mProducer->ID(), channel);
		BString		mfg, prod, label;
		if (AmGlobals().GetDeviceInfo(endpoint, mfg, prod, label) == B_OK) {
			if (label.Length() > 0) outLabel << label;
			else {
				outLabel << mfg;
				if (mfg.Length() > 0 && prod.Length() > 0) outLabel << " ";
				outLabel << prod;
			}
			if (outLabel.Length() > 0) return B_OK;
		}
	}
	return AmFilterAddOn::GetLabel(outLabel, useName);
}

BString AmProducerFilterAddOn::Key() const
{
	return PRODUCER_CLASS_NAME_STR;
}

BString AmProducerFilterAddOn::ShortDescription() const
{
	return "Receive events from the MIDI producers";
}

void AmProducerFilterAddOn::LongDescription(BString& name, BString& str) const
{
	name << "MIDI In";
	str << "<p>I correspond to a single BeOS MIDI producer.  The most common type of MIDI
		producer is a hardware MIDI port installed on your system, although any application
		that generates MIDI data can publish a MIDI producer.  Any MIDI In filter that is
		a result of an application will appear and disappear as the application is opened
		and closed.  A BeOS MIDI producer can supply its own icon (software synthesizers
		frequently do), although any that don\'t will receive the standard icon with a
		unique colour.</p>

		<P>These filters can only be placed in the first slot of the input pipeline.  Once
		the filter is in the pipeline, you can change its MIDI channel by clicking anywhere
		on the icon and selecting a new channel from the popup menu.  Alternatively, you can
		open the properties window for the filter and change the channel there.  The properties
		window also allows you to turn off MIDI event types, which prevents them from appearing
		in a track.</p>
		
		<p>There are a variety of ways to add MIDI ports to your system:  Some soundcards
		have supported MIDI ports, or drivers have been published for several popular USB-based
		MIDI interfaces (such as the Yamaha UX256).</p>";
}

void AmProducerFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BString AmProducerFilterAddOn::KeyForType(type inType) const
{
	if (inType == Type()) return Key();
	if (inType == DESTINATION_FILTER) return CONSUMER_CLASS_NAME_STR;
	return NULL;
}

BBitmap* AmProducerFilterAddOn::Image(BPoint requestedSize) const
{
	if (mIcon) return new BBitmap(mIcon);
	return NULL;
}

static bool devices_match(	const char* mfg1, const char* prod1,
							const BString& mfg2, const BString& prod2)
{
	if (!mfg1 && !prod1) return false;
	if (mfg2 != mfg1) return false;
	if (prod2 != prod1) return false;
	return true;
}

float AmProducerFilterAddOn::CheckInstantiation(const BMessage* config) const
{
	float q = AmFilterAddOn::CheckInstantiation(config);
	if (q < 0) return q;
	/* First try and match against the current device for my producer, if any.
	 */
	if (mProducer) {
		int32			channel;
		const char*		mfg1;
		const char*		prod1;
		const char*		label1;
		if (config->FindInt32(S_CHANNEL, &channel) != B_OK) channel = -1;
		if (config->FindString(DEVICE_MFG_STR, &mfg1) != B_OK) mfg1 = NULL;
		if (config->FindString(DEVICE_PROD_STR, &prod1) != B_OK) prod1 = NULL;
		if (config->FindString(DEVICE_LABEL_STR, &label1) != B_OK) label1 = NULL;
		if (mfg1 || prod1) {
			am_studio_endpoint	endpoint(	mProducer->Name(), AM_PRODUCER_TYPE,
											mProducer->ID(), channel);
			BString		mfg2, prod2, label2;
			if (AmGlobals().GetDeviceInfo(endpoint, mfg2, prod2, label2) == B_OK) {
				if (devices_match(mfg1, prod1, mfg2, prod2) == true) {
					if (label1 && label2 == label1) return 1;
					else return 0.95;
				}
			}
		}
	}
	/* Next try to match the name of the producer -- i.e. the MIDI port name.
	 * Note that a matching device should have the highest possible match
	 * value, so everyone else gets scaled down a little.
	 */
	float		scale = 0.65;
	const char* producer;
	if (config->FindString(SZ_FILTER_NAME, &producer) != B_OK) {
		return -1;
	}
	
	BString		name = Name();
	if (strcmp(name.String(), producer) == 0) return 1 * scale;
	
	/* It wasn't a perfect match, so return a match value based on the
	 * percentage of matched characters.
	 */
	const int32 N = strlen(producer);
	int32 matches = 0;
	for (int32 i=0; i<N && name[i]; i++) {
		if (producer[i] == name[i]) matches++;
	}
	
	return (q + (matches*(1-q))/N) * scale;
}

AmFilterI* AmProducerFilterAddOn::NewInstance(
										AmFilterHolderI* holder,
										const BMessage* config)
{
	return new AmProducerFilter( this, holder, config );
}

BMidiProducer* AmProducerFilterAddOn::Producer() const
{
	return mProducer;
}

int32 AmProducerFilterAddOn::ProducerID() const
{
	if (mProducer) return mProducer->ID();
	return -1;
}

// #pragma mark -

/*************************************************************************
 * AM-EVENT-PRODUCER
 *************************************************************************/
AmEventProducer::AmEventProducer(AmProducerFilter* owner)
		: mOwner(owner)
{
}

AmEventProducer::~AmEventProducer()
{
	AM_LOG("AmEventProducer::~AmEventProducer()\n");
}

void AmEventProducer::SetChannel(int32 channel)
{
	mChannel = channel;
}

void AmEventProducer::SetTypeMask(int32 mask)
{
	mTypeMask = mask;
}

void AmEventProducer::NoteOff(	uchar channel, uchar note, uchar velocity, 
								bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::NoteOff ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " note " << (int)note << " velocity " << (int)velocity << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&NOTE_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmNoteOff* event = new AmNoteOff(note, velocity, pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::NoteOff() (end)\n");
}

void AmEventProducer::NoteOn(	uchar channel, uchar note, uchar velocity, 
								bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::NoteOn ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " note " << (int)note << " velocity " << (int)velocity << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&NOTE_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	if (velocity > 0) {
		AmNoteOn* event = new AmNoteOn(note, velocity, pulse);
		event->SetHasDuration(false);
		mOwner->GenerateEvents(event);
	} else {
		AmNoteOff* event = new AmNoteOff(note, 127, pulse);
		mOwner->GenerateEvents(event);
	}
	AM_LOG("AmEventProducer::NoteOn() (end)\n");
}

void AmEventProducer::KeyPressure(	uchar channel, uchar note, uchar pressure, 
									bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::KeyPressure ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " note " << (int)note << " pressure " << (int)pressure << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&KEYPRESSURE_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	(void)time;
	AM_LOG("AmEventProducer::KeyPressure() (end)\n");
}

void AmEventProducer::ControlChange(	uchar channel, uchar controlNumber, 
										uchar controlValue, 
										bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::ControlChange ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " number " << (int)controlNumber << " value " << (int)controlValue << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&CONTROLCHANGE_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmControlChange* event = new AmControlChange(controlNumber, controlValue,
												 pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::ControlChange() (end)\n");
}

void AmEventProducer::ProgramChange(	uchar channel, uchar programNumber, 
										bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::ProgramChange ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " number " << (int)programNumber << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&PROGRAMCHANGE_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmProgramChange* event = new AmProgramChange(programNumber, pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::ProgramChange() (end)\n");
}

void AmEventProducer::ChannelPressure(	uchar channel, uchar pressure, 
										bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::ChannelPressure ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " pressure " << (int)pressure << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&CHANNELPRESSURE_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmChannelPressure* event = new AmChannelPressure(pressure, pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::ChannelPressure() (end)\n");
}

void AmEventProducer::PitchBend(	uchar channel, uchar lsb, uchar msb, 
									bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::PitchBend ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "channel " << (int)channel << " lsb " << (int)lsb << " msb " << (int)msb  << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (channel != mChannel || (mTypeMask&PITCHBEND_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmPitchBend* event = new AmPitchBend(lsb, msb, pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::PitchBend() (end)\n");
}

void AmEventProducer::SystemExclusive(	void* data, size_t dataLength, 
										bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::SystemExclusive ");
	if (mOwner) str << mOwner->Label() << " ";
	char		hex[32];
	for (size_t k = 0; k < dataLength; k++) {
		sprintf(hex, "%x ", ((uchar*)data)[k]);
		str << hex;
	}
	str << "\n";
	AM_BLOG(str);
#endif
	if ((mTypeMask&SYSTEMEXCLUSIVE_TYPE) == 0) return;
	bool			handleMMC = true;
	if (handleMMC && dataLength == 4 && ((uint8*)data)[0] == 0x7f && ((uint8*)data)[2] == 6) {
		mOwner->HandleMMC(((uint8*)data)[1], ((uint8*)data)[3]);
		return;
	}

	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmSystemExclusive* event = new AmSystemExclusive((uint8*)data, dataLength,
													 pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::SystemExclusive() (end)\n");
}

void AmEventProducer::SystemCommon(	uchar statusByte, uchar data1, uchar data2, 
									bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::SystemCommon ");
	if (mOwner) str << mOwner->Label() << " ";
	str << "status " << (int)statusByte << " data1 " << (int)data1 << " data2 " << (int)data2  << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if ((mTypeMask&SYSTEMCOMMON_TYPE) == 0) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	AmSystemCommon* event = new AmSystemCommon(statusByte, data1, data2, pulse);
	mOwner->GenerateEvents(event);
	AM_LOG("AmEventProducer::SystemCommon() (end)\n");
}
							  
void AmEventProducer::AllNotesOff(	bool justChannel, 
									bigtime_t time)
{
#ifdef AM_LOGGING
	BString		str("\nAmEventProducer::AllNotesOff ");
	if (mOwner) str << mOwner->Label() << " ";
	str <<"justChannel " << (int)justChannel  << " time " << time << "\n";
	AM_BLOG(str);
#endif
	if (justChannel != mChannel) return;
	AmTime pulse = mOwner->MakeTime(time);
	if (pulse < 0) return;
	(void)time;
	AM_LOG("AmEventProducer::AllNotesOff() (end)\n");
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------

/*************************************************************************
 * ARP-SYNTH-CONSUMER
 * A BMidiLocalConsumer for interfacing with the BMidiSynth.
 *************************************************************************/
class ArpSynthConsumer : public BMidiLocalConsumer
{
public:
	ArpSynthConsumer();
	virtual ~ArpSynthConsumer();

	virtual	void NoteOff(uchar channel, uchar note, uchar velocity, 
						 bigtime_t time);
	virtual	void NoteOn(uchar channel, uchar note, uchar velocity, 
						bigtime_t time);
	virtual	void KeyPressure(uchar channel, uchar note, uchar pressure, 
							 bigtime_t time);
	virtual	void ControlChange(uchar channel, uchar controlNumber, 
							   uchar controlValue, 
							   bigtime_t time);
	virtual	void ProgramChange(uchar channel, uchar programNumber, 
							   bigtime_t time);
	virtual	void ChannelPressure(uchar channel, uchar pressure, 
								 bigtime_t time);
	virtual	void PitchBend(uchar channel, uchar lsb, uchar msb, 
						   bigtime_t time);
	virtual void AllNotesOff(bool justChannel, 
							 bigtime_t time);

private:
	BMidiSynth*		mSynth;
};

// #pragma mark -

/*************************************************************************
 * ARP-MIDI-CONSUMER-FILTER
 *************************************************************************/

AmConsumerFilter::AmConsumerFilter(	AmConsumerFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config )
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder), mDevice(NULL),
	  mChannel(0), mTypeMask(DEFAULT_DESTINATION_TYPES),
	  mNotesOffCc(true), mProducer(0), mImageBitmap(NULL)
{
	mConsumer = addon->Consumer();
	if (mConsumer) {
		mConsumer->Acquire();
		mProducer = new BMidiLocalProducer();
		if (mProducer != NULL && mConsumer != NULL) {
			mProducer->Connect(mConsumer);
		}
	}
	
	if (config) PutConfiguration(config);
	for (int32 k = 0; k < 128; k++) mNoteOns[k] = 0;
}

AmConsumerFilter::~AmConsumerFilter()
{
	mAddOn->InstanceGone();
	delete mImageBitmap;
	if ( (mProducer != 0) && (mConsumer != 0) ) mProducer->Disconnect(mConsumer);
	if (mProducer !=0 ) {
//printf("CONSUMER FILTER RELEASED PRODUCER %s\n", mProducer->Name() );
		mProducer->Release();
}
	if (mConsumer !=0 ) {
//printf("CONSUMER FILTER RELEASED CONSUMER %s\n", mConsumer->Name() );
		mConsumer->Release();
}
}

BString AmConsumerFilter::Label() const
{
	if (HasLabel()) return AmFilterI::Label();

	if (mConsumer) {
		am_studio_endpoint	endpoint(	mConsumer->Name(), AM_CONSUMER_TYPE,
										mConsumer->ID(), mChannel);
		BString		mfg, prod, label;
		if (AmGlobals().GetDeviceInfo(endpoint, mfg, prod, label) == B_OK) {
			if (label.Length() > 0) return label;
			BString		s(mfg);
			if (mfg.Length() > 0 && prod.Length() > 0) s << " ";
			s << prod;
			if (s.Length() > 0) return s;
		}
	}
		
	return Name();
}

status_t AmConsumerFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err=values->AddInt32(S_CHANNEL, mChannel)) != B_OK) return err;
	if ((err=values->AddInt32(S_TYPE_MASK, mTypeMask)) != B_OK) return err;
	if ((err=values->AddBool(S_NOTES_OFF_CC, mNotesOffCc)) != B_OK) return err;
	if ((err=values->AddBool(S_MIDI_CLOCK, mAddOn->ClockEnabled())) != B_OK) return err;

	if (mConsumer) {
		am_studio_endpoint		endpoint(	mConsumer->Name(), AM_CONSUMER_TYPE,
											mConsumer->ID(), mChannel);
		BString			mfg, product, label;
		if (AmGlobals().GetDeviceInfo(endpoint, mfg, product, label) == B_OK) {
			if (mfg.Length() > 0)
				if ((err = values->AddString(DEVICE_MFG_STR, mfg)) != B_OK) return err;
			if (product.Length() > 0)
				if ((err = values->AddString(DEVICE_PROD_STR, product)) != B_OK) return err;
			if (label.Length() > 0)
				if ((err = values->AddString(DEVICE_LABEL_STR, label)) != B_OK) return err;
		}
	}
	return B_OK;
}

status_t AmConsumerFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	bool b;
	if (values->FindInt32(S_CHANNEL, &i) == B_OK) SetChannel(i);
	if (values->FindInt32(S_TYPE_MASK, &i) == B_OK) mTypeMask = i;
	if (values->FindBool(S_NOTES_OFF_CC, &b) == B_OK) mNotesOffCc = b;
	if (values->FindBool(S_MIDI_CLOCK, &b) == B_OK) mAddOn->SetClockEnabled(b);
	return B_OK;
}

status_t AmConsumerFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpEndpointFilterSettings(mHolder, config, false));
	return B_OK;
}

status_t AmConsumerFilter::GetNextConfiguration(BMessage* value) const
{
	if (mChannel < 15) {
		return value->AddInt32(S_CHANNEL, mChannel+1);
	}
	return B_ERROR;
}

status_t AmConsumerFilter::GetToolTipText(BString* out) const
{
	BString		label = Label();
	if (label.Length() > 0) {
		*out = label.String();
		return B_OK;
	}
	AmFilterAddOn* addon = AddOn();
	if (addon) {
		*out = addon->Name();
		return B_OK;
	}
	return B_ERROR;
}

void AmConsumerFilter::SetChannel(int32 channel)
{
	BAutolock _l(mAccess);
	if (mChannel != channel) {
		delete mImageBitmap;
		mImageBitmap = NULL;
		mChannel = channel;
		if (mHolder) mHolder->FilterChangeNotice();
	}
}

ArpCRef<AmDeviceI> AmConsumerFilter::Device() const
{
	BAutolock _l(mAccess);
	if (!mConsumer) return NULL;
	if (mAddOn && mAddOn->Name() == BE_MIDI_SYNTH_STR) {
		return AmGlobals().DeviceNamed(NULL, "Be MIDI Synth");
	}

	am_studio_endpoint		endpoint(	mConsumer->Name(), AM_CONSUMER_TYPE,
										mConsumer->ID(), mChannel);
	return AmGlobals().DeviceAt(endpoint);
}

int32 AmConsumerFilter::HintChannel() const
{
	return mChannel;
}

void AmConsumerFilter::Stop(uint32 context)
{
	if (mProducer) {
		if ((context&MASK_CONTEXT) == TRANSPORT_CONTEXT) {
//printf("stop TRANSPORT_CONTEXT\n");
			// Turn sustain off.
			mProducer->SprayControlChange(mChannel, 0x40, 0, 0);
			for (uint8 k = 0; k < 128; k++) {
				if (mNoteOns[k] > 0) {
					mProducer->SprayNoteOff(mChannel, k, 127, 0);
					mNoteOns[k] = 0;
				}
			}
		}
		if ((context&MASK_CONTEXT) == PANIC_CONTEXT) {
//printf("stop PANIC_CONTEXT\n");
			for (uchar i=0; i<128; i++) {
				mProducer->SprayNoteOff(mChannel, i, 127, 0);
				mNoteOns[i] = 0;
			}
			// Turn sustain off.
			mProducer->SprayControlChange(mChannel, 0x40, 0, 0);
			// All notes off. -- This is the culprit with the E5K mixer bug.
			mProducer->SprayControlChange(mChannel, 0x7B, 0, 0);
			// All sounds off. -- This is also a culprit.
			mProducer->SprayControlChange(mChannel, 0x78, 0, 0);
		}
	}
}

#if 0
void AmConsumerFilter::Stop(uint32 context)
{
	if (mProducer) {
		if ((context&MASK_CONTEXT) == TRANSPORT_CONTEXT ||
				(context&MASK_CONTEXT) == PANIC_CONTEXT) {
			// Turn sustain off.
			mProducer->SprayControlChange(mChannel, 0x40, 0, 0);
			// All notes off. -- This is the culprit with the E5K mixer bug.
			if (mNotesOffCc) {
				mProducer->SprayControlChange(mChannel, 0x7B, 0, 0);
				// All sounds off. -- This is also a culprit.
				mProducer->SprayControlChange(mChannel, 0x78, 0, 0);
			}
		}
		if ((context&MASK_CONTEXT) == PANIC_CONTEXT) {
			for (uchar i=0; i<128; i++) {
				mProducer->SprayNoteOff(mChannel, i, 127, 0);
			}
		}
	}
}
#endif

AmEvent* AmConsumerFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if( !event || !mProducer ) return event;
	
	bigtime_t time = (params && params->size >= AM_SIZE_TO(am_filter_params, performance_time))
				   ? params->performance_time : 0;
	
	switch( event->Type() ) {
		case AmEvent::NOTEON_TYPE: {
			AmNoteOn* noteon = dynamic_cast<AmNoteOn*>(event);
			if( noteon && (mTypeMask&NOTE_TYPE) != 0 ) {
				ArpD(cdb << ADH << "Note=" << noteon->Note()
						<< ", Velocity=" << noteon->Velocity() << endl);
				mProducer->SprayNoteOn(mChannel, noteon->Note(), noteon->Velocity(), time);
				mNoteOns[noteon->Note()] = 1;
				if (noteon->Duration() > 0) {
					try {
						AmNoteOff* noteoff =
							new AmNoteOff(	noteon->Note(),
											noteon->ReleaseVelocity(),
											noteon->EndTime());
						AmEvent* offev = dynamic_cast<AmEvent*>(noteoff);
						if( offev ) {
							event->AppendEvent(offev);
							offev->SetNextFilter(mHolder);
						}
					} catch (bad_alloc& e) {
					}
				}
			}
		} break;
		case AmEvent::NOTEOFF_TYPE: {
			AmNoteOff* noteoff = dynamic_cast<AmNoteOff*>(event);
			if( noteoff && (mTypeMask&NOTE_TYPE) != 0 ) {
				mProducer->SprayNoteOff(mChannel, noteoff->Note(), noteoff->Velocity(), time);
			}
		} break;
		case AmEvent::CHANNELPRESSURE_TYPE: {
			AmChannelPressure* cp = dynamic_cast<AmChannelPressure*>(event);
			if( cp && (mTypeMask&CHANNELPRESSURE_TYPE) != 0 ) {
				mProducer->SprayChannelPressure(mChannel, cp->Pressure(), time);
			}
		} break;
		case AmEvent::TEMPOCHANGE_TYPE: {
			AmTempoChange* tc = dynamic_cast<AmTempoChange*>(event);
			if( tc && (mTypeMask&TEMPOCHANGE_TYPE) != 0 ) {
				mProducer->SprayTempoChange(mChannel, int32(tc->Tempo()) );
			}
		} break;
		case AmEvent::CONTROLCHANGE_TYPE: {
			AmControlChange* cc = dynamic_cast<AmControlChange*>(event);
			if( cc && (mTypeMask&CONTROLCHANGE_TYPE) != 0 ) {
				mProducer->SprayControlChange(mChannel, cc->ControlNumber(),
									  cc->ControlValue(), time);
			}
		} break;
		case AmEvent::PITCHBEND_TYPE: {
			AmPitchBend* pb = dynamic_cast<AmPitchBend*>(event);
			if( pb && (mTypeMask&PITCHBEND_TYPE) != 0 ) {
				mProducer->SprayPitchBend(mChannel, pb->Lsb(), pb->Msb(), time);
			}
		} break;
		case AmEvent::PROGRAMCHANGE_TYPE: {
			AmProgramChange* pc = dynamic_cast<AmProgramChange*>(event);
			if( pc && (mTypeMask&PROGRAMCHANGE_TYPE) != 0 ) {
				mProducer->SprayProgramChange(mChannel, pc->ProgramNumber(), time);
			}
		} break;
		case AmEvent::SYSTEMCOMMON_TYPE: {
			AmSystemCommon* sc = dynamic_cast<AmSystemCommon*>(event);
			if( sc && (mTypeMask&SYSTEMCOMMON_TYPE) != 0 ) {
				mProducer->SpraySystemCommon(sc->Status(), sc->Data1(), sc->Data2(), time);
			}
		} break;
		case AmEvent::SYSTEMEXCLUSIVE_TYPE: {
			AmSystemExclusive* sc = dynamic_cast<AmSystemExclusive*>(event);
			if( sc && (mTypeMask&SYSTEMEXCLUSIVE_TYPE) != 0 ) {
				sc->SetChannel(mChannel);
				mProducer->SpraySystemExclusive(const_cast<uint8*>(sc->Data()),
												sc->Length(), time);
			}
		} break;
		default: {
			// that's all that we know how to handle.
		} break;
	}
	
	return event;
}

void AmConsumerFilter::MouseAction(BRect frame, BPoint where,
									uint32 buttons, bool released)
{
	int32 channel = run_channel_popup(frame, where, mChannel, released);
	if( channel >= 0 && mHolder ) {
		BMessage	msg;
		if( msg.AddInt32(S_CHANNEL, channel) == B_OK ) mHolder->PutConfiguration(&msg);
	}
}

BBitmap* AmConsumerFilter::Image(BPoint requestedSize) const
{
	BAutolock _l(mAccess);
	return create_overlay(&mImageBitmap, Device(), AddOn(), mChannel, requestedSize);
}

void AmConsumerFilter::SetDevice(ArpCRef<AmDeviceI> device)
{
debugger("Not yet");
	mDevice = device;
	delete mImageBitmap;
	mImageBitmap = NULL;
	const BBitmap*	bitmap = device->Icon(BPoint(20, 20));
	if (bitmap) mImageBitmap = new BBitmap(bitmap);
	if (mHolder) mHolder->FilterChangeNotice();
}

// #pragma mark -

/*************************************************************************
 * ARP-MIDI-CONSUMER-FILTER-ADDON
 *************************************************************************/

AmConsumerFilterAddOn::AmConsumerFilterAddOn(const void* cookie,
											 BMidiConsumer* consumer,
											 int32 consumerIndex)
	: AmFilterAddOn(cookie),
	  mProducer(NULL), mConsumer(consumer), mConsumerIndex(consumerIndex),
	  mIcon(NULL), mInstanceCount(0), mClockEnabled(false)
{
	if (mConsumer) {
		mConsumer->Acquire();
		mProducer = new BMidiLocalProducer();
		if (mProducer != NULL && mConsumer != NULL) {
			mProducer->Connect(mConsumer);
		}
	}
	mIcon = create_icon(mConsumer);
}

AmConsumerFilterAddOn::~AmConsumerFilterAddOn()
{
	if ( (mProducer != 0) && (mConsumer != 0) ) mProducer->Disconnect(mConsumer);
	if (mProducer !=0 ) mProducer->Release();
	if (mConsumer !=0 ) mConsumer->Release();
	mConsumer = NULL;
	delete mIcon;
	mIcon = NULL;
}

BString AmConsumerFilterAddOn::Name() const
{
	if (mConsumer) {
		if (mConsumerIndex > 0) {
			BString		n = mConsumer->Name();
			n << " " << mConsumerIndex;
			return n;
		}
		return mConsumer->Name();
	}
	
	// This is for lazy creation of the midi synth player,
	// which takes a long time to start up.
	return BE_MIDI_SYNTH_STR;
}

status_t AmConsumerFilterAddOn::GetLabel(	BString& outLabel, bool useName,
											BMessage* archivedFilter) const
{
	if (mConsumer) {
		uint8		channel = 0;
		if (archivedFilter) {
			int32	i;
			if (archivedFilter->FindInt32(S_CHANNEL, &i) == B_OK) channel = uint8(i);
		}
		am_studio_endpoint	endpoint(	mConsumer->Name(), AM_CONSUMER_TYPE,
										mConsumer->ID(), channel);
		BString		mfg, prod, label;
		if (AmGlobals().GetDeviceInfo(endpoint, mfg, prod, label) == B_OK) {
			if (label.Length() > 0) outLabel << label;
			else {
				outLabel << mfg;
				if (mfg.Length() > 0 && prod.Length() > 0) outLabel << " ";
				outLabel << prod;
			}
			if (outLabel.Length() > 0) return B_OK;
		}
	}
	return AmFilterAddOn::GetLabel(outLabel, useName);
}

BString AmConsumerFilterAddOn::Key() const
{
	return CONSUMER_CLASS_NAME_STR;
}

BString AmConsumerFilterAddOn::ShortDescription() const
{
	if( strcmp( Name().String(), BE_MIDI_SYNTH_STR ) == 0 )
		return "Perform all events with the BeOS synthesizer";
	else
		return "Perform all events with the MIDI consumer";
}

void AmConsumerFilterAddOn::LongDescription(BString& name, BString& str) const
{
	if (strcmp( Name().String(), BE_MIDI_SYNTH_STR ) == 0) {
		name << "Be MIDI Synth";
		str << "<p>I am a filter for the standard Be-supplied software General MIDI synthesizer.
			I will only appear if you have a BeOS-compatible soundcard. Once the I have
			been placed in the output pipeline, you can select from amongst the sixteen MIDI
			channels by accessing the properties. As a convenience, two- and three- button
			mice can access the channels through a special right-click popup menu.</p>
			
			<P>This filter uses the factory supplied Be MIDI Synth device.  If you wish
			to change any device properties (such as patch or controller names) for tracks
			that use the Be MIDI Synth, change that device.&nbsp;";

	} else {
		name << "MIDI Out";
		str << "<p>I correspond to a single BeOS MIDI consumer.  The most common type of MIDI
			consumer is a hardware MIDI port installed on your system, although any application
			that receives MIDI data can publish a MIDI consumer.  Any MIDI Out filter that is
			a result of an application will appear and disappear as the application is opened
			and closed.  A BeOS MIDI consumer can supply its own icon (software synthesizers
			frequently do), although any that don\'t will receive the standard icon with a
			unique colour.</p>

			<p>These filters can only be placed in the last slot of the output pipeline.  Once
			the filter is in the pipeline, you can change its MIDI channel by clicking anywhere
			on the icon and selecting a new channel from the popup menu.  Alternatively, you can
			open the properties window for the filter and change the channel there.</p>
		
			<p>The properties window allows you to turn off MIDI event types, which prevents
			them from being performed.  Additionally, the Send MIDI Clock checkbox, when
			on, will send MIDI clock signals to the MIDI consumer.  This can be used for
			any MIDI device which syncs to MIDI clock.  For example, many devices with an
			arpeggiator can sync to MIDI clock so the sequencer\'s tempo controllers the
			tempo of the arpeggiation.  <i>Note that you only need to turn this option on
			for a single channel of a single MIDI consumer.</i></p>
		
			<p>There are a variety of ways to add MIDI ports to your system:  Some soundcards
			have supported MIDI ports, or drivers have been published for several popular USB-based
			MIDI interfaces (such as the Yamaha UX256).</p>";
	}
}

void AmConsumerFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BString AmConsumerFilterAddOn::KeyForType(type inType) const
{
	if (inType == Type()) return Key();
	if (inType == SOURCE_FILTER) return PRODUCER_CLASS_NAME_STR;
	return NULL;
}

BBitmap* AmConsumerFilterAddOn::Image(BPoint requestedSize) const
{
	if (mIcon) return new BBitmap(mIcon);
	return NULL;
}

float AmConsumerFilterAddOn::CheckInstantiation(const BMessage* config) const
{
	float q = AmFilterAddOn::CheckInstantiation(config);
	if (q < 0) return q;
	/* First try and match against the current device for my consumer, if any.
	 */
	if (mConsumer) {
		int32			channel;
		const char*		mfg1;
		const char*		prod1;
		const char*		label1;
		if (config->FindInt32(S_CHANNEL, &channel) != B_OK) channel = -1;
		if (config->FindString(DEVICE_MFG_STR, &mfg1) != B_OK) mfg1 = NULL;
		if (config->FindString(DEVICE_PROD_STR, &prod1) != B_OK) prod1 = NULL;
		if (config->FindString(DEVICE_LABEL_STR, &label1) != B_OK) label1 = NULL;
		if (mfg1 || prod1) {
			am_studio_endpoint	endpoint(	mConsumer->Name(), AM_CONSUMER_TYPE,
											mConsumer->ID(), channel);
			BString		mfg2, prod2, label2;
			if (AmGlobals().GetDeviceInfo(endpoint, mfg2, prod2, label2) == B_OK) {
				if (devices_match(mfg1, prod1, mfg2, prod2) == true) {
					if (label1 && label2 == label1) return 1;
					else return 0.95;
				}
			}
		}
	}
	/* Next try to match the name of the consumer -- i.e. the MIDI port name.
	 * Note that a matching device should have the highest possible match
	 * value, so everyone else gets scaled down a little.
	 */
	float		scale = 0.65;
	const char* consumer;
	if (config->FindString(SZ_FILTER_NAME, &consumer) != B_OK) {
		if (config->FindString("consumer", &consumer) != B_OK) return -1;
	}

	BString		name = Name();
	if (strcmp(name.String(), consumer) == 0) return 1 * scale;
	/* It wasn't a perfect match, so return a match value based on the
	 * percentage of matched characters.
	 */
	const int32 N = strlen(consumer);
	int32 matches = 0;
	for (int32 i=0; i<N && name[i]; i++) {
		if (consumer[i] == name[i]) matches++;
	}
	
	return (q + (matches*(1-q))/N) * scale;
}

AmFilterI* AmConsumerFilterAddOn::NewInstance(
										AmFilterHolderI* holder,
										const BMessage* config)
{
	mAccess.Lock();
	mInstanceCount++;
	const bool first = (mClockEnabled && mInstanceCount == 1);
	mAccess.Unlock();
	if (first) StartClock();
	return new AmConsumerFilter( this, holder, config );
}

void AmConsumerFilterAddOn::Clock(AmTime /*time*/)
{
	if (mProducer) mProducer->SpraySystemRealTime(0xF8);
}

BMidiConsumer* AmConsumerFilterAddOn::Consumer() const
{
	if (!mConsumer) {
		BAutolock _l(mAccess);
		if (!mConsumer) mConsumer = new ArpSynthConsumer();
	}
	return mConsumer;
}

int32 AmConsumerFilterAddOn::ConsumerID() const
{
	// The point of this function is to not load in the
	// soft synth consumer if it isn't already.
	if (mConsumer) return mConsumer->ID();
	return -1;
}

void AmConsumerFilterAddOn::InstanceGone()
{
	mAccess.Lock();
	mInstanceCount--;
	const bool last = mInstanceCount == 0;
	mAccess.Unlock();
	if (last) StopClock();
}

void AmConsumerFilterAddOn::SetClockEnabled(bool state)
{
	mAccess.Lock();
	if (mClockEnabled == state) {
		mAccess.Unlock();
		return;
	}
	mClockEnabled = state;
	mAccess.Unlock();
	if (state) StartClock();
	else StopClock();
}

bool AmConsumerFilterAddOn::ClockEnabled() const
{
	return mClockEnabled;
}

// #pragma mark -

/*************************************************************************
 * ARP-SYNTH-CONSUMER
 *************************************************************************/
ArpSynthConsumer::ArpSynthConsumer()
		: mSynth(0)
{
	SetName(BE_MIDI_SYNTH_STR);
	mSynth = new BMidiSynth;
	if (mSynth != 0) {
		mSynth->EnableInput(true, true);
		mSynth->SetVolume(1.0);
	}
}

ArpSynthConsumer::~ArpSynthConsumer()
{
	delete mSynth;
}

void ArpSynthConsumer::NoteOff(	uchar channel, uchar note, uchar velocity, 
								bigtime_t time)
{
	if (mSynth != 0) mSynth->NoteOff(channel + 1, note, velocity);
}

void ArpSynthConsumer::NoteOn(	uchar channel, uchar note, uchar velocity, 
								bigtime_t time)
{
	if (mSynth != 0) mSynth->NoteOn(channel + 1, note, velocity);
}

void ArpSynthConsumer::KeyPressure(	uchar channel, uchar note, uchar pressure, 
									bigtime_t time)
{
	if (mSynth != 0) mSynth->KeyPressure(channel + 1, note, pressure);
}

void ArpSynthConsumer::ControlChange(	uchar channel, uchar controlNumber, 
										uchar controlValue, 
										bigtime_t time)
{
	if (mSynth != 0) mSynth->ControlChange(channel + 1, controlNumber, controlValue);
}

void ArpSynthConsumer::ProgramChange(	uchar channel, uchar programNumber, 
										bigtime_t time)
{
	if (mSynth != 0) mSynth->ProgramChange(channel + 1, programNumber);
}

void ArpSynthConsumer::ChannelPressure(	uchar channel, uchar pressure, 
										bigtime_t time)
{
	if (mSynth != 0) mSynth->ChannelPressure(channel + 1, pressure);
}

void ArpSynthConsumer::PitchBend(	uchar channel, uchar lsb, uchar msb, 
									bigtime_t time)
{
	if (mSynth != 0) mSynth->PitchBend(channel + 1, lsb, msb);
}

void ArpSynthConsumer::AllNotesOff(	bool justChannel, 
									bigtime_t time)
{
	if (mSynth != 0) mSynth->AllNotesOff(justChannel);
}

// #pragma mark -

/*************************************************************************
 * AM-NULL-INPUT-FILTER
 *************************************************************************/
AmNullInputFilter::AmNullInputFilter(	AmNullInputAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* config )
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

AmNullInputFilter::~AmNullInputFilter()
{
}

AmEvent* AmNullInputFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event || !mHolder) return event;
	event->SetNextFilter(mHolder->FirstConnection() );
	return event;
}

// #pragma mark -

/*************************************************************************
 * AM-NULL-INPUT-ADD-ON
 *************************************************************************/
AmNullInputAddOn::AmNullInputAddOn(const void* cookie)
	: AmFilterAddOn(cookie)
{
}

AmNullInputAddOn::~AmNullInputAddOn()
{
}

void AmNullInputAddOn::LongDescription(BString& name, BString& str) const
{
	name << "Tool Input";
	str << "<p>A source for the pipeline page of the Edit Tool and Edit
		Multi Filter windows.</p>
		
		<p>The Edit Tool window can have any number of Tool Input filters
		(although only one per pipeline).  Each filter corresponds to one
		of the active tracks.  For example, the first Tool Input filter
		in the tool corresponds to the primary track, the second corresponds
		to the second ordered track (if any), etc.</p>
		
		<p>The Edit Multi Filter window can only have a single Tool Input
		filter.  All previous filters in the pipeline with the multi filter
		send their output to this Tool Input filter.</p>";
}

void AmNullInputAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* AmNullInputAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = ImageManager().FindBitmap("Null Input");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* AmNullInputAddOn::NewInstance(	AmFilterHolderI* holder,
											const BMessage* config)
{
	return new AmNullInputFilter(this, holder, config);
}

// #pragma mark -

/*************************************************************************
 * AM-NULL-OUTPUT-FILTER
 *************************************************************************/
AmNullOutputFilter::AmNullOutputFilter(	AmNullOutputAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* config )
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

AmNullOutputFilter::~AmNullOutputFilter()
{
}

AmEvent* AmNullOutputFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return event;
}

// #pragma mark -

/*************************************************************************
 * AM-NULL-OUTPUT-ADD-ON
 *************************************************************************/
AmNullOutputAddOn::AmNullOutputAddOn(const void* cookie)
	: AmFilterAddOn(cookie)
{
}

AmNullOutputAddOn::~AmNullOutputAddOn()
{
}

void AmNullOutputAddOn::LongDescription(BString& name, BString& str) const
{
	name << "Tool Output";
	str << "<p>A target for the pipeline page of the Edit Tool and Edit
		Multi Filter windows.</p>
		
		<p>The Edit Tool window can have any number of Tool Output filters
		(although only one per pipeline).  Each filter corresponds to one
		of the active tracks.  For example, the first Tool Output filter
		in the tool corresponds to the primary track, the second corresponds
		to the second ordered track (if any), etc.</p>
		
		<p>The Edit Multi Filter window can also have any number of Tool
		Output filters, again at most one per pipeline.  The number of
		Tool Output filters determines the maximum connections the multi
		filter can have, and each Tool Output filter corresponds with any
		connection filters, in order.</p>";
}

void AmNullOutputAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* AmNullOutputAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = ImageManager().FindBitmap("Null Output");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* AmNullOutputAddOn::NewInstance(	AmFilterHolderI* holder,
											const BMessage* config)
{
	return new AmNullOutputFilter(this, holder, config);
}


