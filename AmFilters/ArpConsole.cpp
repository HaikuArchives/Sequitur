#include "ArpConsole.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <be/interface/CheckBox.h>
#include <ArpLayout/ArpViewWrapper.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();

static const char*					_MASK_STR = "mask";

static AmStaticResources gRes;

enum {
	_NOTES				= 0x00000001,
	_CHANNELPRESSURE	= 0x00000002,
	_CONTROLCHANGE		= 0x00000004,
	_KEYPRESSURE		= 0x00000008,
	_PITCHBEND			= 0x00000010,
	_PROGRAMCHANGE		= 0x00000020,
	_SYSTEMCOMMON		= 0x00000040,
	_SYSTEMEXCLUSIVE	= 0x00000080,
	_SYSTEMREALTIME		= 0x00000100,
	_TEMPO				= 0x00000200,
	_OTHERS				= 0x00000400,

	_PARAMS				= 0x00010000,
	_SECTIONS			= 0x00020000,
	
	_INIT =
		_NOTES|_CHANNELPRESSURE|_CONTROLCHANGE|_KEYPRESSURE|
		_PITCHBEND|_PROGRAMCHANGE|_SYSTEMCOMMON|_SYSTEMEXCLUSIVE|
		_SYSTEMREALTIME|_TEMPO|_OTHERS
};

/*****************************************************************************
 * ARP-CONSOLE-FILTER
 *****************************************************************************/
ArpConsoleFilter::ArpConsoleFilter(ArpConsoleFilterAddOn* addon,
								   AmFilterHolderI* holder,
								   const BMessage* config)
	: AmFilterI(addon),
	  mDevice(NULL), mAddOn(addon), mHolder(holder), mTypeMask(_INIT)
{
	if (config) PutConfiguration(config);
}

ArpConsoleFilter::~ArpConsoleFilter()
{
}

AmEvent* ArpConsoleFilter::StartSection(AmTime firstTime, AmTime lastTime,
										const am_filter_params* params)
{
	if (mTypeMask&_SECTIONS) {
		printf("Filter %s: Start section %lld to %lld\n", Label().String(), firstTime, lastTime);
		print_params(params);
	}
	return NULL;
}

AmEvent* ArpConsoleFilter::FinishSection(AmTime firstTime, AmTime lastTime,
										 const am_filter_params* params)
{
	if (mTypeMask&_SECTIONS) {
		printf("Filter %s: Finish section %lld to %lld\n", Label().String(), firstTime, lastTime);
		print_params(params);
	}
	return NULL;
}

void ArpConsoleFilter::Stop(uint32 context)
{
	if (mTypeMask&_SECTIONS) {
		printf("Filter %s: Stop section\n", Label().String());
	}
}

static int32 _print(int32 types, int32 mask, AmEvent* event, BString& str)
{
	if (!(types&mask)) return 0;
	if (str.Length() > 0) printf("%s", str.String());
	event->Print();
	return 1;
}

AmEvent* ArpConsoleFilter::HandleEvent(AmEvent* e, const am_filter_params* params)
{
	if (!e) {
		printf("NULL event!\n");
		return e;
	}
	BString				str("Filter ");
	str << Label().String() << ": ";
	int32				count = 0;
	switch (e->Type()) {
		case e->NOTEOFF_TYPE:
		case e->NOTEON_TYPE:			count += _print(mTypeMask, _NOTES, e, str);				break;
		case e->CHANNELPRESSURE_TYPE:	count += _print(mTypeMask, _CHANNELPRESSURE, e, str);	break;
		case e->CONTROLCHANGE_TYPE:		count += _print(mTypeMask, _CONTROLCHANGE, e, str);		break;
		case e->KEYPRESSURE_TYPE:		count += _print(mTypeMask, _KEYPRESSURE, e, str);		break;
		case e->PITCHBEND_TYPE:			count += _print(mTypeMask, _PITCHBEND, e, str);			break;
		case e->PROGRAMCHANGE_TYPE:		count += _print(mTypeMask, _PROGRAMCHANGE, e, str);		break;
		case e->SYSTEMCOMMON_TYPE:		count += _print(mTypeMask, _SYSTEMCOMMON, e, str);		break;
		case e->SYSTEMEXCLUSIVE_TYPE:	count += _print(mTypeMask, _SYSTEMEXCLUSIVE, e, str);	break;
		case e->SYSTEMREALTIME_TYPE:	count += _print(mTypeMask, _SYSTEMREALTIME, e, str);	break;
		case e->TEMPOCHANGE_TYPE:		count += _print(mTypeMask, _TEMPO, e, str);				break;
		default:						count += _print(mTypeMask, _OTHERS, e, str);			break;
	}
	if (mTypeMask&_PARAMS) {
		if (count < 1 && str.Length() > 0) printf("%s", str.String());
		print_params(params);
	}
	if (mHolder) e->SetNextFilter(mHolder->FirstConnection());
	return e;
}

#define PREFIX "\t -> "

int32 ArpConsoleFilter::print_params(const am_filter_params* params) const
{
	if (!params) {
		printf(PREFIX "NO PARAMS.\n");
		return 1;
	}
	
	if (params->cur_tempo || params->cur_signature) {
		if (params && params->cur_tempo) {
			if (params->cur_tempo->NextEvent() != NULL) {
				printf(PREFIX "Tempos:\n"); params->cur_tempo->PrintChain(0, PREFIX);
			} else {
				printf(PREFIX "Tempo: "); params->cur_tempo->Print();
			}
		} else {
			printf(PREFIX "NO TEMPO.\n");
		}
		if (params && params->cur_signature) {
			if (params->cur_signature->NextEvent() != NULL) {
				printf(PREFIX "Signatures:\n"); params->cur_signature->PrintChain(0, PREFIX);
			} else {
				printf(PREFIX "Signature: "); params->cur_signature->Print();
			}
		} else {
			printf(PREFIX "NO SIGNATURE.\n");
		}
	} else {
		printf(PREFIX "NO TEMPO OR SIGNATURE.\n");
	}
	return 1;
}

status_t ArpConsoleFilter::GetConfiguration(BMessage* values) const
{
	status_t		err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	return values->AddInt32(_MASK_STR, mTypeMask);
}

status_t ArpConsoleFilter::PutConfiguration(const BMessage* values)
{
	AmFilterI::PutConfiguration(values);
	int32			m;
	if (values->FindInt32(_MASK_STR, &m) != B_OK) m = _INIT;
	mTypeMask = m;
	return B_OK;
}

class ArpConsoleSettings : public AmFilterConfigLayout
{
public:
	ArpConsoleSettings(	AmFilterHolderI* target,
						const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
//		initSettings.PrintToStream();
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
				->AddLayoutChild((new ArpBox("box", "Events"))
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
										mImpl.AttachCheckBox(_MASK_STR, _NOTES, "notes"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "kpressure", "Key Pressure",
										mImpl.AttachCheckBox(_MASK_STR, _KEYPRESSURE, "kpressure"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "cpressure", "Channel Pressure",
										mImpl.AttachCheckBox(_MASK_STR, _CHANNELPRESSURE, "cpressure"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "pbend", "Pitch Bend",
										mImpl.AttachCheckBox(_MASK_STR, _PITCHBEND, "pbend"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "cchange", "Control Change",
										mImpl.AttachCheckBox(_MASK_STR, _CONTROLCHANGE, "cchange"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
						)
						->AddLayoutChild((new ArpRunningBar("BoxVGroup2"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, 0)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "pchange", "Program Change",
										mImpl.AttachCheckBox(_MASK_STR, _PROGRAMCHANGE, "pchange"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "scommon", "System Common",
										mImpl.AttachCheckBox(_MASK_STR, _SYSTEMCOMMON, "scommon"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "sexclusive", "System Exclusive",
										mImpl.AttachCheckBox(_MASK_STR, _SYSTEMEXCLUSIVE, "sexclusive"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "srealtime", "System Realtime",
										mImpl.AttachCheckBox(_MASK_STR, _SYSTEMREALTIME, "srealtime"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "tchange", "Tempo Change",
										mImpl.AttachCheckBox(_MASK_STR, _TEMPO, "tchange"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "others", "Others",
										mImpl.AttachCheckBox(_MASK_STR, _OTHERS, "others"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
						)
					)
				)
				->AddLayoutChild((new ArpBox("box", "Filter"))
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
								new BCheckBox(BRect(0,0,10,10), "params", "Params",
										mImpl.AttachCheckBox(_MASK_STR, _PARAMS, "params"),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,1)
									.SetInt32(ArpRunningBar::FillC,ArpWest)
								)
							)
						)
						->AddLayoutChild((new ArpRunningBar("BoxVGroup2"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, 0)
							)
							->AddLayoutChild((new ArpViewWrapper(
								new BCheckBox(BRect(0,0,10,10), "sec", "Sections",
										mImpl.AttachCheckBox(_MASK_STR, _SECTIONS, "sec"),
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
			AddLayoutChild(top);
		} catch(...) {
			throw;
		}
		
		Implementation().RefreshControls(mSettings);
	}
	
private:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpConsoleFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpConsoleSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-CONSOLE-FILTER-ADD-ON
 *****************************************************************************/
void ArpConsoleFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I print in the terminal every MIDI event I receive. I am useful
	for debugging any filters you've written, so you can see the output
	they are providing.  In order for this filter to work, Sequitur must be
	launched from a command line.</P>";
}

void ArpConsoleFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpConsoleFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpConsoleFilterAddOn(cookie);
	return NULL;
}
