#include "ArpMetronome.h"

#ifndef AMPUBLIC_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif
#include "AmPublic/AmControls.h"

#include <ArpLayout/ArpViewWrapper.h>

#include <be/interface/CheckBox.h>

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

ArpMOD();
static AmStaticResources gRes;

static const char* S_BEAT = "beat_val";
static const char* S_BEATS_PER_MEASURE = "bpm";
static const char* S_MEASURE_NOTE = "measure_note";
static const char* S_MEASURE_VELOCITY = "measure_vel";
static const char* S_BEAT_NOTE = "beat_note";
static const char* S_BEAT_VELOCITY = "beat_vel";
static const char* S_OFFBEAT_NOTE = "offbeat_note";
static const char* S_OFFBEAT_VELOCITY = "offbeat_vel";
static const char* S_FLAG_MASK = "flag_mask";

enum {
	ONLY_WHILE_RECORDING_FLAG	= (1<<0),
	FOLLOW_SONG_SIGNATURE_FLAG	= (1<<1)
};

ArpMetronomeFilter::ArpMetronomeFilter(ArpMetronomeFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  mBeat(4),
	  mBeatsPerMeasure(4),
	  mMeasureNote(34),
	  mMeasureVelocity(127),
	  mBeatNote(33),
	  mBeatVelocity(127),
	  mOffbeatNote(33),
	  mOffbeatVelocity(48),
	  mFlags(ONLY_WHILE_RECORDING_FLAG|FOLLOW_SONG_SIGNATURE_FLAG)
{
	if (config) PutConfiguration(config);
}

ArpMetronomeFilter::~ArpMetronomeFilter()
{
}

AmEvent* ArpMetronomeFilter::StartSection(AmTime firstTime, AmTime lastTime,
										  const am_filter_params* params)
{
	if ( mFlags&ONLY_WHILE_RECORDING_FLAG
			&& params
			&& !(params->flags&AMFF_RECORDING) )
		return NULL;

	AmTime base;
	AmTime beat;
	int32 bpm;
	
	const AmSignature* sig = ((mFlags&FOLLOW_SONG_SIGNATURE_FLAG) != 0 && params)
						   ? params->cur_signature : NULL;
	
	AmEvent* head = NULL;
	AmEvent* tail = NULL;
	while (firstTime <= lastTime) {
		if (sig) {
			const AmSignature* next = dynamic_cast<const AmSignature*>(sig->NextEvent());
			while (next && next->StartTime() <= firstTime) {
				sig = next;
				next = dynamic_cast<const AmSignature*>(sig->NextEvent());
			}
		}
		
		if (sig) {
			base = sig->StartTime();
			beat = (PPQN*4)/sig->BeatValue();
			bpm = sig->Beats();
		} else {
			base = 0;
			beat = (PPQN*4)/mBeat;
			bpm = mBeatsPerMeasure;
		}
		
		firstTime = ((firstTime-base+beat-1)/beat)*beat + base;
	
		const AmTime lastMeasure = ((firstTime-base)/(beat*bpm))*(beat*bpm) + base;
		//const bool measure = (((firstTime-base)%(beat*bpm)) == 0);
		uint8 noteVal;
		uint8 noteVel;
		if (firstTime == lastMeasure) {
			noteVal = mMeasureNote;
			noteVel = mMeasureVelocity;
		} else {
			// TO DO: Fix this to generate the expected beating for things
			// like 6/8 time.
			const bool beat = (((firstTime-lastMeasure)%(PPQN)) == 0);
			if (beat) {
				noteVal = mBeatNote;
				noteVel = mBeatVelocity;
			} else {
				noteVal = mOffbeatNote;
				noteVel = mOffbeatVelocity;
			}
		}
		
		if (noteVel > 0) {
			AmNoteOn* note = new AmNoteOn(noteVal, noteVel, firstTime);
			note->SetDuration(beat/2);
			if (tail) {
				tail->AppendEvent(note);
				tail = note;
			} else {
				head = tail = note;
			}
		}
		
		firstTime += beat;
	}
	return head;
}

AmEvent* ArpMetronomeFilter::FinishSection(AmTime /*firstTime*/, AmTime /*lastTime*/,
										   const am_filter_params* /*params*/)
{
	// Nothing doin'.
	return NULL;
}

AmEvent* ArpMetronomeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	// We don't modify events, we generate them.
	return event;
}

class ArpMetronomeFilterSettings : public AmFilterConfigLayout
{
public:
	ArpMetronomeFilterSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
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
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new ArpViewWrapper(
					new BCheckBox(BRect(0,0,10,10), "only_during_record", "On only while recording",
							mImpl.AttachCheckBox(S_FLAG_MASK, ONLY_WHILE_RECORDING_FLAG, "only_during_record"),
							B_FOLLOW_NONE,
							B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpWest)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new ArpBox("SignatureBox", "Signature"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpRunningBar("SignatureVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "follow_song_signature", "Follow song signature",
									mImpl.AttachCheckBox(S_FLAG_MASK, FOLLOW_SONG_SIGNATURE_FLAG, "follow_song_signature"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
								.SetBool(ArpRunningBar::AlignLabelsC,false)
							)
						)
						->AddLayoutChild((new ArpRunningBar("CustomSignatureHBar"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, .5)
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpFillAll)
							)
							->AddLayoutChild((new ArpTextControl(
													S_BEATS_PER_MEASURE, "Custom signature:","",
													mImpl.AttachTextControl(S_BEATS_PER_MEASURE)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "88")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
							->AddLayoutChild((new ArpTextControl(
													S_BEAT, "/","",
													mImpl.AttachTextControl(S_BEAT)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "88")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
						)
					)
				)
				->AddLayoutChild((new ArpBox("NotesBox", "Notes"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpRunningBar("NotesHBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->AddLayoutChild((new ArpRunningBar("NoteVBar"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, .5)
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpFillAll)
							)



							->AddLayoutChild((new AmKeyControl(
													S_MEASURE_NOTE, "Measure note:",
													mImpl.AttachControl(S_MEASURE_NOTE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
#if 0
							->AddLayoutChild((new ArpTextControl(
													S_MEASURE_NOTE, "Measure note:","",
													mImpl.AttachTextControl(S_MEASURE_NOTE)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "888")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
#endif

							->AddLayoutChild((new AmKeyControl(
													S_BEAT_NOTE, "Beat note:",
													mImpl.AttachControl(S_BEAT_NOTE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
#if 0
							->AddLayoutChild((new ArpTextControl(
													S_BEAT_NOTE, "Beat note:","",
													mImpl.AttachTextControl(S_BEAT_NOTE)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "888")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
#endif

							->AddLayoutChild((new AmKeyControl(
													S_OFFBEAT_NOTE, "Offbeat note:",
													mImpl.AttachControl(S_OFFBEAT_NOTE)))
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
#if 0
							->AddLayoutChild((new ArpTextControl(
													S_OFFBEAT_NOTE, "Offbeat note:","",
													mImpl.AttachTextControl(S_OFFBEAT_NOTE)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "888")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
#endif

						)
						->AddLayoutChild((new ArpRunningBar("VelocityVBar"))
							->SetParams(ArpMessage()
								.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
								.SetFloat(ArpRunningBar::IntraSpaceP, .5)
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpFillAll)
							)
							->AddLayoutChild((new ArpTextControl(
													S_MEASURE_VELOCITY, "Velocity:","",
													mImpl.AttachTextControl(S_MEASURE_VELOCITY)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "888")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
							->AddLayoutChild((new ArpTextControl(
													S_BEAT_VELOCITY, "Velocity:","",
													mImpl.AttachTextControl(S_BEAT_VELOCITY)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "888")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
							->AddLayoutChild((new ArpTextControl(
													S_OFFBEAT_VELOCITY, "Velocity:","",
													mImpl.AttachTextControl(S_OFFBEAT_VELOCITY)))
								->SetParams(ArpMessage()
									.SetString(ArpTextControl::MinTextStringP, "888")
									.SetString(ArpTextControl::PrefTextStringP, "888")
								)
								->SetConstraints(ArpMessage()
									.SetFloat(ArpRunningBar::WeightC,3)
									.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								)
							)
						)
					)
				)
			);
		} catch(...) {
			throw;
		}
		
		Implementation().RefreshControls(mSettings);
	}
	
protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpMetronomeFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err=values->AddInt32(S_BEAT, mBeat)) != B_OK) return err;
	if ((err=values->AddInt32(S_BEATS_PER_MEASURE, mBeatsPerMeasure)) != B_OK) return err;
	if ((err=values->AddInt32(S_MEASURE_NOTE, mMeasureNote)) != B_OK) return err;
	if ((err=values->AddInt32(S_MEASURE_VELOCITY, mMeasureVelocity)) != B_OK) return err;
	if ((err=values->AddInt32(S_BEAT_NOTE, mBeatNote)) != B_OK) return err;
	if ((err=values->AddInt32(S_BEAT_VELOCITY, mBeatVelocity)) != B_OK) return err;
	if ((err=values->AddInt32(S_OFFBEAT_NOTE, mOffbeatNote)) != B_OK) return err;
	if ((err=values->AddInt32(S_OFFBEAT_VELOCITY, mOffbeatVelocity)) != B_OK) return err;
	if ((err=values->AddInt32(S_FLAG_MASK, mFlags)) != B_OK) return err;
	
	return B_OK;
}

status_t ArpMetronomeFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if (values->FindInt32(S_BEAT, &i) == B_OK) mBeat = i;
	if (mBeat < 1) mBeat = 1;
	if (values->FindInt32(S_BEATS_PER_MEASURE, &i) == B_OK) mBeatsPerMeasure = i;
	if (mBeatsPerMeasure < 1) mBeatsPerMeasure = 1;
	if (values->FindInt32(S_MEASURE_NOTE, &i) == B_OK) mMeasureNote = (uint8)i;
	if (mMeasureNote > 127) mMeasureNote = 127;
	if (values->FindInt32(S_MEASURE_VELOCITY, &i) == B_OK) mMeasureVelocity = (uint8)i;
	if (mMeasureVelocity > 127) mMeasureVelocity = 127;
	if (values->FindInt32(S_BEAT_NOTE, &i) == B_OK) mBeatNote = (uint8)i;
	if (mBeatNote > 127) mBeatNote = 127;
	if (values->FindInt32(S_BEAT_VELOCITY, &i) == B_OK) mBeatVelocity = (uint8)i;
	if (mBeatVelocity > 127) mBeatVelocity = 127;
	if (values->FindInt32(S_OFFBEAT_NOTE, &i) == B_OK) mOffbeatNote = (uint8)i;
	if (mOffbeatNote > 127) mOffbeatNote = 127;
	if (values->FindInt32(S_OFFBEAT_VELOCITY, &i) == B_OK) mOffbeatVelocity = (uint8)i;
	if (mOffbeatVelocity > 127) mOffbeatVelocity = 127;
	if (values->FindInt32(S_FLAG_MASK, &i) == B_OK) {
		mFlags = i;
	}
	return B_OK;
}

status_t ArpMetronomeFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpMetronomeFilterSettings(mHolder, config));
	return B_OK;
}

/* ----------------------------------------------------------------
   ArpMetronomeFilterAddOn Class
   ---------------------------------------------------------------- */
void ArpMetronomeFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I am used to generate click events. I am most appropriate in the
	output pipeline. The output filter for the track I reside in determines which
	of your instruments will perform the click. Controls allow you to set the note value
	and velocity for several different types of clicks. By default, the metronome
	will only click while the song is recording; however, unchecking the <I>On only
	while recording</I> box will allow it to click during playback, as well.</P>";
}

void ArpMetronomeFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpMetronomeFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpMetronomeFilterAddOn(cookie);
	return NULL;
}
