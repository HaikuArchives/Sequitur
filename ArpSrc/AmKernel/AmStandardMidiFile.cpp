/* AmStandardMidiFile.cpp
*/

#define _BUILDING_AmKernel 1

#include "AmKernel/AmStandardMidiFile.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmFilterI.h"

#include "AmKernel/AmPhraseEvent.h"

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef ARPKERNEL_ARPSTRUCTUREDIO_H
#include <ArpKernel/ArpStructuredIO.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include "AmKernel/AmDevice.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmKernelDefs.h"

#include <MidiDefs.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>

ArpMOD();

//#define	SMF_NOISY		(1)

const	uint8	META_EVENT = 0xff;
const	uint8	END_OF_TRACK = 0x2f;

status_t AmStandardMidiFile::channel_list::append(AmEvent* event)
{
	if (!event) return B_ERROR;
	
	AmNode* node = new AmNode(event);
	if (!tail) {
		head = tail = node;
	} else {
		tail->InsertNext(node);
		tail = node;
	}
	
	return B_OK;
}

/* This is a special method because for some reason, sysex events which
 * should be next to each other are being separated by other events at
 * the same time.  All I do is look and see if there is a sysex event
 * prior to the tail at the same time as the supplied event; if there is
 * I add after that, otherwise I default to the standard append().
 */
status_t AmStandardMidiFile::channel_list::append_sysex(AmEvent* event)
{
	if (!event) return B_ERROR;
	if (!tail) return append(event);
	/* Find if there is a sysex event at the same time but before the
	 * most recent.
	 */
	if (tail->Event()->Type() == event->SYSTEMEXCLUSIVE_TYPE) return append(event);	
	AmNode*			prev = tail->prev;
	while (prev && prev->StartTime() == event->StartTime()) {
		if (prev->Event()->Type() == event->SYSTEMEXCLUSIVE_TYPE) {
			AmNode* node = new AmNode(event);
			if (!node) return B_NO_MEMORY;
			prev->InsertNext(node);
			return B_OK;
		}
		prev = prev->prev;
	}
	return append(event);
}

AmStandardMidiFile::AmStandardMidiFile()
{
	mDivision = 0;
	mCurTime = 0;
	mTempoEvents = NULL;
	mSignatureEvents = NULL;
	for (int32 i=0; i<16; i++) mChannels[i] = channel_list();
}

AmStandardMidiFile::~AmStandardMidiFile()
{
	MakeEmpty();
}

status_t AmStandardMidiFile::ReadFile(BDataIO* file)
{
	Reset();
	
	mCurTime = 0;
	
	int32 toBeRead;
	int16 format;
	int16 numTracks;
	
	ssize_t res;
	
	ArpStructuredIO io(file);
	io.SetReadEndian(ARP_BENDIAN_FILE);
	
	if ((res=io.ReadMatch("MThd", 4)) != B_OK) return res;
	
	if ((res=io.ReadInt32(&toBeRead)) != B_OK) return res;
	io.StartReadChunk(toBeRead);
	
	if ((res=io.ReadInt16(&format)) != B_OK) return res;
	if ((res=io.ReadInt16(&numTracks)) != B_OK) return res;
	if ((res=io.ReadInt16(&mDivision)) != B_OK) return res;

	#ifdef SMF_NOISY	
	printf("Format=%d, NumTracks=%d, Division=%d\n",
			format, numTracks, mDivision);
	#endif
	if ((res=io.FinishReadChunk()) != B_OK) return res;

	do {
		res = ReadTrack(io);
		
		bool foundData = false;
		for (int32 i=0; i<16; i++) {
			if (mChannels[i].head ||
					(i == 15 && mCurTrackInfo.title.Length() > 0 && !foundData)) {
				if (mChannels[i].head) foundData = true;
				// Make sure we have a duration for all note on events.
				AmNode* head = mChannels[i].head;
				#ifdef SMF_NOISY
				printf("Creating track %s, channel #%ld:\n",
						mCurTrackInfo.title.String(), i);
				#endif
				while (head) {
					ArpD(head->Print());
					AmNoteOn* note = dynamic_cast<AmNoteOn*>(head->Event());
					if (note && !note->HasDuration()) {
						printf("*** No note off for: "); head->Event()->Print();
						note->SetHasDuration(true);
						note->SetDuration(PPQN);
					}
					head = head->next;
				}
				AmEvent* event = ReadPhrases(mChannels[i], mCurTrackInfo.phrases);
				if (event) {
					#ifdef SMF_NOISY
					event->PrintChain();
					#endif
					mTracksEvents.AddItem(event);
				}
				mChannels[i] = channel_list();
				smf_track_info* info = new smf_track_info(mCurTrackInfo);
				if (info->title.Length() <= 0) {
					info->title << "Track " << (mTracksEvents.CountItems());
				}
				if (foundData) info->channel = i;
				mTracksInfo.AddItem(info);
			}
		}
		mCurTrackInfo = smf_track_info();
		
	} while (res == B_OK);

	return B_OK;
}

status_t AmStandardMidiFile::WriteFile(BDataIO* file)
{
	Reset();
	
	ssize_t res;
	
	ArpStructuredIO io(file);
	io.SetWriteEndian(ARP_BENDIAN_FILE);
	
	if ((res=io.Write("MThd", 4)) != 4) return res < B_OK ? res : B_ERROR;
	
	if ((res=io.WriteInt32(sizeof(int16)*3)) != B_OK) return res;
	io.StartWriteChunk(sizeof(int16)*3);
	
	if ((res=io.WriteInt16(1)) != B_OK) return res;				// Format
	if ((res=io.WriteInt16(mTracksEvents.CountItems())) != B_OK) return res;
	if ((res=io.WriteInt16(PPQN)) != B_OK) return res;
	
	if ((res=io.FinishWriteChunk()) != B_OK) return res;
	
	bool wroteTempos = false;
	for (int32 i=0; i<mTracksEvents.CountItems(); i++) {
		AmPhraseEvent* phrase = NULL;
		smf_track_info* info = NULL;
		
		#if 0
		if (i == -2) {
			phrase = mTempoEvents;
		} else if (i == -1) {
			phrase = mSignatureEvents;
		} else {
			phrase = (AmPhraseEvent*)mTracksEvents.ItemAt(i);
			info = (smf_track_info*)mTracksInfo.ItemAt(i);
		}
		#endif
		
		phrase = (AmPhraseEvent*)mTracksEvents.ItemAt(i);
		info = (smf_track_info*)mTracksInfo.ItemAt(i);
		
		if (!info && phrase) {
			if (phrase->IsEmpty()) phrase = NULL;
		}
		
		if (!wroteTempos && phrase) {
			if (mTempoEvents) {
				if (phrase->Phrase() && mTempoEvents->Phrase() ) {
					phrase->Phrase()->AddAll(mTempoEvents->Phrase());
					mTempoEvents->Phrase()->SetList();
				}
			}
			if (mSignatureEvents) {
				if ( phrase->Phrase() && mSignatureEvents->Phrase() ) {
					phrase->Phrase()->AddAll(mSignatureEvents->Phrase());
					mSignatureEvents->Phrase()->SetList();
				}
			}
			wroteTempos = true;
		}
		
		if (phrase || info) {
			#ifdef SMF_NOISY
			printf("Starting track %s, channel #%ld:\n",
					info ? info->title.String() : "<unknown>", info ? info->channel : -1);
			if (phrase) phrase->PrintChain();
			#endif
			
			BMallocIO mio;
			ArpStructuredIO smio(&mio);
			res = WriteTrack(smio, i&0xf, phrase, info);
			smio.Flush();
			
			if ((res=io.Write("MTrk", 4)) != 4) return res < B_OK ? res : B_ERROR;
			if ((res=io.WriteInt32(mio.BufferLength())) != B_OK) return res;
			if ((res=io.Write(mio.Buffer(), mio.BufferLength())) != (ssize_t)mio.BufferLength()) {
				return res < B_OK ? res : B_ERROR;
			}
		}
	}
	
	return B_OK;
}

int32 AmStandardMidiFile::CountTracks() const
{
	return mTracksEvents.CountItems();
}

const AmPhraseEvent* AmStandardMidiFile::TrackAt(int32 i, smf_track_info* outInfo) const
{
	if (i < 0 || i > mTracksEvents.CountItems()) return NULL;
	
	if (outInfo) {
		if (i < mTracksInfo.CountItems()) {
			*outInfo = *(smf_track_info*)(mTracksInfo.ItemAt(i));
		} else {
			*outInfo = smf_track_info();
		}
	}
	
	return (AmPhraseEvent*)(mTracksEvents.ItemAt(i));
}
	
AmPhraseEvent* AmStandardMidiFile::RemoveTrack(int32 i, smf_track_info* outInfo)
{
	AmPhraseEvent* ev = (AmPhraseEvent*)mTracksEvents.RemoveItem(i);
	if (!ev) return NULL;
	
	smf_track_info* info = (smf_track_info*)mTracksInfo.RemoveItem(i);
	if (info && outInfo) *outInfo = *info;
	delete info;
	
	return ev;
}

const AmPhraseEvent* AmStandardMidiFile::TempoTrack() const
{
	return mTempoEvents;
}

AmPhraseEvent* AmStandardMidiFile::DetachTempoTrack()
{
	AmPhraseEvent* ev = mTempoEvents;
	mTempoEvents = NULL;
	return ev;
}

const AmPhraseEvent* AmStandardMidiFile::SignatureTrack() const
{
	return mSignatureEvents;
}

AmPhraseEvent* AmStandardMidiFile::DetachSignatureTrack()
{
	AmPhraseEvent* ev = mSignatureEvents;
	mSignatureEvents = NULL;
	return ev;
}

status_t AmStandardMidiFile::AddTrack(AmPhraseEvent* track, const smf_track_info& info)
{
	if (track) {
		mTracksEvents.AddItem(track);
		smf_track_info* infoCopy = new smf_track_info(info);
		mTracksInfo.AddItem(infoCopy);
		return B_OK;
	}
	
	return B_BAD_VALUE;
}

status_t AmStandardMidiFile::SetTempoTrack(AmPhraseEvent* track)
{
	if (mTempoEvents) mTempoEvents->Delete();
	mTempoEvents = track;
	return B_OK;
}

status_t AmStandardMidiFile::SetSignatureTrack(AmPhraseEvent* track)
{
	if (mSignatureEvents) mSignatureEvents->Delete();
	mSignatureEvents = track;
	return B_OK;
}

void AmStandardMidiFile::MakeEmpty()
{
	int32 i;
	
	for (i=0; i<mTracksEvents.CountItems(); i++) {
		AmPhraseEvent* ev = (AmPhraseEvent*)(mTracksEvents.ItemAt(i));
		if (ev) ev->Delete();
	}
	mTracksEvents.MakeEmpty();
	for (i=0; i<mTracksInfo.CountItems(); i++) {
		delete (smf_track_info*)(mTracksInfo.ItemAt(i));
	}
	mTracksInfo.MakeEmpty();
	
	if (mTempoEvents) {
		mTempoEvents->Delete();
		mTempoEvents = NULL;
	}
	if (mSignatureEvents) {
		mSignatureEvents->Delete();
		mSignatureEvents = NULL;
	}
	
	Reset();
}

void AmStandardMidiFile::Reset()
{
	for (int32 i=0; i<16; i++) {
		if (mChannels[i].head) {
			mChannels[i].head->DeleteListContents();
			delete mChannels[i].head;
		}
		mChannels[i] = channel_list();
	}
	
	mCurTrackInfo = smf_track_info();
}

// The chanType array is indexed by the high nibble of a status byte.
// The values are either the number of bytes needed (1 or 2) for a
// given channel message, or 0 (which means it's not a channel message.
static const int32 ChannelEventLength[] = {
	0, 0, 0, 0, 0, 0, 0, 0,	// 0x00 through 0x70
	2, 2, 2, 2, 1, 1, 2, 0	// 0x80 through 0xf0
};

/* AIGH.  Because sysex data has no channel, it always ends up in
 * mChannels[0].  However, in Sequitur, sysex data DOES have a channel,
 * namely whatever the track is set to.  So if it's just jammed into
 * mChannels[0] and the track has other data on a different channel,
 * then that track will become two tracks.  So this is a hack to find
 * the correct channel number.  Probably a better way to find it.
 */
static uint8 sysex_channel_hack(smf_track_info& ti)
{
	if (!ti.has_output_device) return 0;
	BMessage	msg;
	if (ti.output_device.FindMessage("seq:filters", &msg) != B_OK) return 0;
	int32		i32;
	if (msg.FindInt32("channel", &i32) != B_OK) return 0;
	if (i32 < 0) return 0;
	else if (i32 > 15) return 15;
	return uint8(i32);
}

status_t AmStandardMidiFile::ReadTrack(ArpStructuredIO& io)
{
	uint8 status = 0;			// Status byte
	uint8 c;
	bool sysExContinue = false;	// If last msg was unfinished sysex
	bool running = false;		// If currently running status

	ArpString buffer;
				
	if (io.ReadMatch("MTrk", 4) != B_OK) return 1;
	
	int32 toBeRead;
	ssize_t res;
	
	if ((res=io.ReadInt32(&toBeRead)) != B_OK) return res;
	io.StartReadChunk(toBeRead);
	
	mCurTrackInfo = smf_track_info();
	mCurTrackInfo.has_output_device =
		mCurTrackInfo.has_output_filters =
		mCurTrackInfo.has_input_device =
		mCurTrackInfo.has_input_filters =
		mCurTrackInfo.has_edit_filters =
		mCurTrackInfo.has_track_settings =
		mCurTrackInfo.has_phrases =
		mCurTrackInfo.has_input_connections =
		mCurTrackInfo.has_output_connections = false;

	mCurTime = 0;
	ArpD(cdb << ADH << "===> Starting to read new track" << std::endl);
	
	while (io.RemainingReadChunk() > B_OK) {
		uint64 timeOff;
		if ((res=io.ReadMidiNumber(&timeOff)) != B_OK) return res;
		mCurTime += timeOff;
		ArpD(cdb << ADH << "Next track event at time " << mCurTime
				<< " (delta " << timeOff << ")" << std::endl);
		if ((res=io.ReadInt8((int8*)&c)) != B_OK) return res;
		
		// Check for running status
		if ((c & 0x80) == 0) running = true;
		else {
			status = c;
			running = false;
		}
		// Get number of bytes need to complete this message
		const int32 needed = ChannelEventLength[(status >> 4) & 0x0f];

		// Check if this is a channel message
		if (needed) {
			uint8 c1, c2;
			
			if (running) c1 = c;
			else if ((res=io.ReadInt8((int8*)&c1)) != B_OK) return res;
			if (needed > 1) {
				if ((res=io.ReadInt8((int8*)&c2)) != B_OK) return res;
			} else c2 = 0;
			#ifdef SMF_NOISY
			printf("Channel event time %Ld: s=0x%x, c1=0x%x, c2=0x%x\n",
					timeOff, status, c1, c2);
			#endif		
			// Get the phrase this event should be added to.
			AddChannelEvent(mChannels[status&0xf], CurrentTime(),
							status&0xf0, c1, c2);
			continue;
		}
		#ifdef SMF_NOISY
		printf("Other event time %Ld: 0x%x\n", timeOff, c);
		#endif		
		switch (c) {

			case META_EVENT: {
				uint8 type;
				uint64 size;
				
				if ((res=io.ReadInt8((int8*)&type)) != B_OK) return res;
				if ((res=io.ReadMidiNumber(&size)) != B_OK) return res;
				
				buffer = "";
				if (size > 0) {
					char* buf = buffer.LockBuffer((size_t)size);
					res = io.Read(buf, (size_t)size);
					buffer.UnlockBuffer((size_t)size);
					if (res != (ssize_t)size) return res < B_OK ? res : B_ERROR;
				}
				
				AddMetaEvent(&mCurTrackInfo, CurrentTime(), type,
							 (const char*)buffer, buffer.Length());
			} break;
	
			case B_SYS_EX_START: {
				uint64 size;
				
				if ((res=io.ReadMidiNumber(&size)) != B_OK) return res;
				#ifdef SMF_NOISY
				printf("This is %lld bytes of sysex data.\n", size);
				#endif
				buffer = "";
				if (size > 0) {
					char* buf = buffer.LockBuffer((size_t)size);
					res = io.Read(buf, (size_t)size);
					buffer.UnlockBuffer((size_t)size);
					if (res != (ssize_t)size) return res < B_OK ? res : B_ERROR;
				}
				
				if (buffer.Length() > 0 &&
						(uint8)buffer[buffer.Length()-1] == B_SYS_EX_END) {
					#ifdef SMF_NOISY
					printf("Found %ld bytes of sysex data.\n", buffer.Length());
					#endif
					sysExContinue = false;
					AmSystemExclusive* se = new AmSystemExclusive(	(const uint8*)(const ichar*)buffer,
																	 buffer.Length(),
																	 CurrentTime());
					AmEvent* event = dynamic_cast<AmEvent*>(se);
					mChannels[sysex_channel_hack(mCurTrackInfo)].append_sysex(event);
				} else if (buffer.Length() > 0) {
					// Need to read more data.
					#ifdef SMF_NOISY
					printf("Found first %ld bytes of sysex data.\n", buffer.Length());
					#endif
					sysExContinue = true;
				}
			} break;
	
			case B_SYS_EX_END: {	// Continuation of SysEx msg or arbitrary data
				uint64 size;
				
				if ((res=io.ReadMidiNumber(&size)) != B_OK) return res;
				
				if (!sysExContinue) buffer = "";
				if (size > 0) {
					int32 curLength = buffer.Length();
					char* buf = buffer.LockBuffer(curLength + (size_t)size);
					res = io.Read(buf+curLength, (size_t)size);
					buffer.UnlockBuffer(curLength+(size_t)size);
					if (res != (ssize_t)size) return res < B_OK ? res : B_ERROR;
				}
				
				if (!sysExContinue) {
					// TO DO: add arbitrary data.
				} else if (buffer.Length() > 0 &&
						(uint8)buffer[buffer.Length()-1] == B_SYS_EX_END) {
					#ifdef SMF_NOISY
					printf("Found %ld bytes of final sysex data.\n", buffer.Length());
					#endif
					sysExContinue = false;
					AmSystemExclusive* se = new AmSystemExclusive(	(const uint8*)(const ichar*)buffer,
																	 buffer.Length(),
																	 CurrentTime());
					AmEvent* event = dynamic_cast<AmEvent*>(se);
					mChannels[sysex_channel_hack(mCurTrackInfo)].append_sysex(event);
				} else if (buffer.Length() > 0) {
					// Need to read more data.
					#ifdef SMF_NOISY
					printf("Found continued %ld bytes of final sysex data.\n", buffer.Length());
					#endif
					sysExContinue = true;
				}
			} break;
			
			default:
				fprintf(stderr, "unexpected byte: 0x%02x\n", c);
		}
	}
	
	io.FinishReadChunk();
	
	return B_OK;
}

AmEvent* AmStandardMidiFile::ReadPhrases(channel_list& list, BMessage& phrases)
{
	CreateBanks(list);
	CreateCommands(list);
	return CreateRootPhrases(list, phrases);
}

static bool add_to_bank_change(AmNode* node, AmNode* beCur)
{
	ArpASSERT(node && beCur);
	if (node->Event()->Type() != beCur->Event()->Type() ) return false;
	/* If there are prior nodes in the bank change, they have to be
	 * the same distance.
	 */
	if (beCur->prev) {
		if (!node->prev) return false;
		if (beCur->StartTime() - beCur->prev->StartTime()
				!= node->StartTime() - node->prev->StartTime() )
			return false;
	}

	if (node->Event()->Type() == AmEvent::CONTROLCHANGE_TYPE) {
		AmControlChange*	cc1 = dynamic_cast<AmControlChange*>(node->Event() );
		AmControlChange*	cc2 = dynamic_cast<AmControlChange*>(beCur->Event() );
		if (!cc1 || !cc2 || cc1->ControlNumber() != cc2->ControlNumber() )
			return false;
	}
	return true;
}

void AmStandardMidiFile::CreateBanks(channel_list& list)
{
	/* If this track has an output filter with a device with a bank
	 * change, convert sequences of events that represent bank changes into
	 * actual bank changes.
	 */
	ArpCRef<AmDeviceI>	device = NULL;
	AmBankChange*		be = NULL;
	BMessage			config;
	if (mCurTrackInfo.output_device.FindMessage(SZ_FILTER_ARCHIVE, &config) == B_OK) {
		ArpRef<AmFilterAddOn> addon = am_find_filter_addon(&config);
		if (addon) {
			AmFilterHolder*		holder = new AmFilterHolder(0, 0, 0, NULL);
			if (holder) {
				AmFilterI*	filter = addon->NewInstance(holder, &config);
				if (filter) {
					device = filter->Device();
					if (device) {
						AmEvent*	e = device->NewBankSelection();
						if (e) {
							be = dynamic_cast<AmBankChange*>(e);
							if (!be) e->Delete();
						}
					}
					delete filter;
				}
				holder->Delete();
			}
		}
	}
	AmNode*		beHead;
	if (be && be->Phrase() && (beHead = be->Phrase()->HeadNode()) != NULL) {
		AmNode*		node = list.head;
		AmNode*		beCur = beHead;
		AmNode*		start = NULL;

		while (node) {
			AmNode*	next = node->next;
			if (add_to_bank_change(node, beCur) ) {
				beCur = beCur->next;
				if (!start) start = node;
				if (be->Matches(start, node) ) {
					AmNode*		insertPrev = start->prev;
					AmNode*		insertNext = node->next;
					/* This is the one spot where we actually alter the
					 * list -- take out events from start to node and
					 * wrap them up in a new bank change.
					 */
					AmBankChange*	newBe = new AmBankChange();
					AmNode*			bankNode;
					if (newBe && newBe->Phrase() && (bankNode = new AmNode(newBe)) ) {
						/* Populate the new bank change event.
						 */
						AmNode*			n = start;
						while (n && n != insertNext) {
							newBe->Phrase()->Add(n->Event() );
							n = n->next;
						}
						if (device) {
							ArpCRef<AmBankI>	bank = device->Bank(newBe);
							if (bank) newBe->Set(bank->BankNumber(), bank->Name().String() );
						}
						/* Insert the new node into the list.
						 */
						if (insertPrev) {
							insertPrev->next = bankNode;
							bankNode->prev = insertPrev;
						}
						if (insertNext) {
							insertNext->prev = bankNode;
							bankNode->next = insertNext;
						}
						/* Remove the events from the list.
						 */
						start->prev = NULL;
						node->next = NULL;
						delete start;
						if (!insertPrev) list.head = bankNode;
					}
					beCur = beHead;
					start = NULL;
				}
			} else {
				beCur = beHead;
				start = NULL;
			}
			node = next;
		}

		be->Delete();
	}
}

static void extract_command(AmNode* start, AmNode* end,
							AmStandardMidiFile::channel_list& list, ArpCRef<AmDeviceI> device)
{
	ArpASSERT(start && device);
	AmPhraseEvent*	pe = new AmInnerPhraseEvent();
	if (!pe && !(pe->Phrase())) return;
	AmNode*		n = start;
	AmNode*		endNext = end;
	if (end) endNext = end->next;
	while (n && n != endNext) {
		pe->Add(n->Event());
		n = n->next;
	}
	if (device->ConformSysexCommand(pe) != B_OK) {
		pe->Phrase()->SetList(NULL);
		pe->Delete();
		return;
	}
	AmNode*		newNode = new AmNode(pe);
	if (!newNode) {
		pe->Delete();
		return;
	}
	/* Insert the new node, remove the newly bundled events.
	 */
	start->ReplaceRun(end, newNode);
	if (start == list.head) list.head = newNode;

	delete start;
}

void AmStandardMidiFile::CreateCommands(channel_list& list)
{
	/* If this track has an output filter with a device with commands,
	 * convert any sysex that represents a command into a command. --
	 * all this really means is wrapping the sysex into a phrase, the
	 * trick is knowing which sysex to wrap up.
	 */
	ArpCRef<AmDeviceI>	device = NULL;
	BMessage			config;
	if (mCurTrackInfo.output_device.FindMessage(SZ_FILTER_ARCHIVE, &config) == B_OK) {
		ArpRef<AmFilterAddOn> addon = am_find_filter_addon(&config);
		if (addon) {
			AmFilterHolder*		holder = new AmFilterHolder(0, 0, 0, NULL);
			if (holder) {
				AmFilterI*	filter = addon->NewInstance(holder, &config);
				if (filter) {
					device = filter->Device();
					delete filter;
				}
				holder->Delete();
			}
		}
	}
	/* I only need to do anything in the event that the device has
	 * multi commands -- in that case, all commands need to be identified
	 * and wrapped into a phrase.
	 */
	if (!device || device->SysexCommandType() != AM_COMMAND_MULTI) return;

	AmNode*		node = list.head;
	AmNode*		start = NULL;
	while (node) {
		AmNode*	next = node->next;
		if (start && start->StartTime() != node->StartTime()) start = NULL;
		if (node->Event()->Type() == AmEvent::SYSTEMEXCLUSIVE_TYPE) {
			if (!start) start = node;
			if (!next || next->StartTime() != start->StartTime() || next->Event()->Type() != AmEvent::SYSTEMEXCLUSIVE_TYPE) {
				extract_command(start, node, list, device);
				start = NULL;
			}
		}
		node = next;
	}
}

AmEvent* AmStandardMidiFile::CreateRootPhrases(channel_list& list, BMessage& phrases)
{
	AmPhraseEvent* root = new AmRootPhraseEvent;
	if (!root || !root->Phrase() ) return NULL;
	/* Compile all the events into their respective phrases.
	 */
	int32		phraseIndex = 0;
	while (list.head != NULL) {
		BMessage	curPhrase;
		status_t	curErr = phrases.FindMessage("phrase", phraseIndex, &curPhrase);
		phraseIndex++;
		BMessage	nextPhrase;
		status_t	nextErr = phrases.FindMessage("phrase", phraseIndex, &nextPhrase);

		AmTime		end = -1;
		if (nextErr == B_OK) {
			AmTime	t;
			if (find_time(nextPhrase, AM_START_TIME_STR, &t) == B_OK) end = t;
		}
		AmNode*		head = list.head;
		AmNode*		n = list.head;
		if (!n->Event() ) return NULL;
		/* By default, set the list head to NULL and feed my entire current
		 * list to the new phrase event.  If I need to, I'll iterate through
		 * the list, and if a cutoff point is found, then I'll break the
		 * list and give it a new head.
		 */
		list.head = NULL;
		if (end >= 0) {
			while (n && n->EndTime() < end) {
//			while (n && n->StartTime() < end) {
				n = n->next;
			}
			if (n && n == head) n = n->next;
			ArpASSERT(n != head);
#if 0
if (n == head) {
printf("Dup: "); n->Event()->Print();
	debugger("FUCK OFF!");
}
#endif
			if (n) {
				list.head = n;
				AmNode*		prev = n->prev;
				n->prev = NULL;
				if (prev) prev->next = NULL;
			}
		}

		AmPhraseEvent* pe = new AmRootPhraseEvent;
		if (!pe || !pe->Phrase() ) return NULL;
		pe->Phrase()->SetList(head);
		if (curErr == B_OK) pe->Phrase()->SetProperties(curPhrase);
		root->Add(pe);
	}
	return root;
}

status_t AmStandardMidiFile::AddChannelEvent(channel_list& into, AmTime time,
												uint8 type, uint8 c1, uint8 c2)
{
	AmEvent* event = NULL;
	
	#ifdef SMF_NOISY
	const bool show = true;
	#else
	const bool show = false; //(&into == &mChannels[1]) ? true : false;
	#endif
		
	switch (type) {
		case B_NOTE_OFF:
		case B_NOTE_ON:
		{
			// Look for last note in phrase with this value, and use
			// the current time as its end time.
			if (show) {
				printf("Note: %s at time %lld, c1=%ld, c2=%ld; finding previous...\n",
						type == B_NOTE_OFF ? "note-off" : "note-on",
						time, (int32)c1, (int32)c2);
			}
			AmNode* tail = into.tail;
			bool found=false;
			while (tail && !found) {
				AmNoteOn* note = dynamic_cast<AmNoteOn*>(tail->Event());
				if (note && tail->Event()->StartTime() < time && note->Note() == c1) {
					// Found it!
					if (!note->HasDuration()) {
						// Haven't yet found a note-off for this one, so
						// use the time we now have.
						if (show) printf("Found it!  Start time=%lld\n", note->StartTime());
						note->SetHasDuration(true);
						note->SetEndTime(time);
						if (type == B_NOTE_OFF) {
							note->SetReleaseVelocity(c2);
						}
					} else {
						if (show) printf("Already have it!\n");
					}
					found = true;
				} else {
					tail = tail->prev;
				}
			}
			if (show) if (!tail) printf("No note-on found!\n");
			
			// If this is a note-on with a non-zero velocity, create
			// a new event for it.
			if (type == B_NOTE_ON && c2 != 0) {
				AmNoteOn* note = new AmNoteOn(c1, c2, time);
				note->SetHasDuration(false);
				event = dynamic_cast<AmEvent*>(note);
			
			// Otherwise, nothing else to do.
			} else {
				return B_OK;
			}
		} break;
		case B_KEY_PRESSURE: {
			// Event not yet defined.
		} break;
		case B_CONTROL_CHANGE: {
			AmControlChange* cc = new AmControlChange(c1, c2, time);
			event = dynamic_cast<AmEvent*>(cc);
		} break;
		case B_PITCH_BEND: {
			AmPitchBend* pb = new AmPitchBend(c1, c2, time);
			event = dynamic_cast<AmEvent*>(pb);
		} break;
		case B_PROGRAM_CHANGE: {
//printf("Read program change at %lld\n", time);
			AmProgramChange* pc = new AmProgramChange(c1, time);
			event = dynamic_cast<AmEvent*>(pc);
		} break;
		case B_CHANNEL_PRESSURE: {
			AmChannelPressure* cp = new AmChannelPressure(c1, time);
			event = dynamic_cast<AmEvent*>(cp);
		} break;
	}
	
	return into.append(event);
}

/*------------------------------------------------------------*/
// Beats-per-minute <--> Microseconds-per-quarter-note
#define BPM(mspqn) ((1000000 * 60) / mspqn)
#define MSPQN(bpm) ((1000000 * 60) / bpm)
/*------------------------------------------------------------*/

status_t AmStandardMidiFile::AddMetaEvent(smf_track_info* into, AmTime time,
										uint8 type, const char* buffer, size_t size)
{
	if (!into) return B_ERROR;

	BString str;
	
	switch (type) {
	case 0x00:
		#ifdef SMF_NOISY
		printf("Found sequence number size %ld: $ %02x %02x\n",
				size, (int)buffer[0], (int)buffer[1]);
		#endif
		break;
	case 0x01:	// Text event
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found text event size %ld: %s\n", size, str.String());
		#endif
		break;
	case 0x02:	// Copyright notice
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found copyright event size %ld: %s\n", size, str.String());
		#endif
		break;
	case 0x03: 	// Sequence/Track name
		into->title.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found track name event size %ld: %s\n", size, into->title.String());
		#endif
		break;
	case 0x04:	// Instrument name
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found instrument name event size %ld: %s\n", size, str.String());
		#endif
		break;
	case 0x05:	// Lyric
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found lyric event size %ld: %s\n", size, str.String());
		#endif
		break;
	case 0x06:	// Marker
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found marker event size %ld: %s\n", size, str.String());
		#endif
		break;
	case 0x07:	// Cue point
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found cue event size %ld: %s\n", size, str.String());
		#endif
		break;
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
		str.SetTo(buffer, size);
		#ifdef SMF_NOISY
		printf("Found text #%d event size %ld: %s\n", type, size, str.String());
		#endif
		break;
	case END_OF_TRACK:	// End of track
		//???AddEOT();
		break;
	case B_TEMPO_CHANGE: {
		if (size == 3) {
			AmPhraseEvent* ev = GetTempoChannel();
			if (ev) {
				AmPhrase* phrase = ev->Phrase();
				if (phrase) {
					uint32 tempo = ( ((uint8)(buffer[0])) << 16 )
								 | ( ((uint8)(buffer[1])) << 8 )
								 | ( ((uint8)(buffer[2])) );
					#ifdef SMF_NOISY
					printf("Adding tempo event: %lu (%lx) at time %lld\n",
							BPM(tempo), tempo, time);
					#endif
					AmTempoChange* tc = new AmTempoChange(BPM(tempo), time);
					AmEvent* event = dynamic_cast<AmEvent*>(tc);
					if (event) phrase->Add(event);
				}
			}
		}
	} break;
	case 0x54:
		//???AddSMPTE(buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
		break;
	case 0x58: {
		if (size == 3 || size == 4) {
			AmPhraseEvent* ev = GetSignatureChannel();
			if (ev) {
				AmPhrase* phrase = ev->Phrase();
				if (phrase) {
					AmSignature* sig = new AmSignature;
					#ifdef SMF_NOISY
					printf("Adding signature event: %d/%d (ppqn=%d, 32nd=%d) at time %lld\n",
							buffer[0], 1<<buffer[1], buffer[2], buffer[3], time);
					#endif
					sig->Set(time, buffer[0], 1<<buffer[1]);
					phrase->Add(sig);
				}
			}
		}
	} break;
	case 0x59:
		#ifdef SMF_NOISY
		printf("Found key signature size %ld: $ %02x\n",
				size, (int)buffer[0]);
		#endif
		break;
	case 0x7f: {			// SeqSpecific
		BMessage msg;
		if (msg.Unflatten(buffer) == B_OK) {
			#ifdef SMF_NOISY
			printf("Read sequencer data: "); msg.PrintToStream();
			#endif
			into->output_filters.MakeEmpty();
			into->output_device.MakeEmpty();
			into->input_filters.MakeEmpty();
			into->input_device.MakeEmpty();
			into->edit_filters.MakeEmpty();
			into->track_settings.MakeEmpty();
			into->phrases.MakeEmpty();
			into->input_connections.MakeEmpty();
			into->output_connections.MakeEmpty();
			if (msg.FindMessage("arp:output_filters", &(into->output_filters)) == B_OK) {
				into->has_output_filters = true;
			}
			if (msg.FindMessage("arp:output_device", &(into->output_device)) == B_OK) {
				into->has_output_device = true;
			}
			if (msg.FindMessage("arp:input_device", &(into->input_device)) == B_OK) {
				into->has_input_device = true;
			}
			if (msg.FindMessage("arp:input_filters", &(into->input_filters)) == B_OK) {
				into->has_input_filters = true;
			}
			if (msg.FindMessage("arp:edit_filters", &(into->edit_filters)) == B_OK) {
				into->has_edit_filters = true;
			}
			if (msg.FindMessage("arp:track_settings", &(into->track_settings)) == B_OK) {
				into->has_track_settings = true;
			}
			if (msg.FindMessage("arp:phrases", &(into->phrases)) == B_OK) {
				into->has_phrases = true;
			}
			if (msg.FindMessage("arp:input_connections", &(into->input_connections)) == B_OK) {
				into->has_input_connections = true;
			}
			if (msg.FindMessage("arp:output_connections", &(into->output_connections)) == B_OK) {
				into->has_output_connections = true;
			}
			#ifdef SMF_NOISY
			BMessage tmp;
			int32 i;
			for (i=0; into->output_filters.FindMessage("seq:filters", i, &tmp) == B_OK; i++) {
				if (i==0) printf("Found output filters:\n");
				printf("#%ld: ", i); tmp.PrintToStream();
			}
			for (i=0; into->output_device.FindMessage("seq:filters", i, &tmp) == B_OK; i++) {
				if (i==0) printf("Found output device:\n");
				printf("#%ld: ", i); tmp.PrintToStream();
			}
			#endif
		}
	} break;
	default:
		;//???AddMetaMisc(type, length, m);
	}
	
	return B_OK;
}

status_t AmStandardMidiFile::WriteTrack(ArpStructuredIO& io, int32 channel,
										AmPhraseEvent* track,
										const smf_track_info* info) const
{
	status_t res;
	
	mCurTime = 0;
	
	AmTime endTime = 0;
	
	if (info) {
		if ((res=WriteMetaEvent(io, 0, 0x03, info->title.String(),
								info->title.Length())) < B_OK) return res;
		BMessage metaData;
		if (info->has_output_filters)
			metaData.AddMessage("arp:output_filters", &(info->output_filters));
		if (info->has_output_device)
			metaData.AddMessage("arp:output_device", &(info->output_device));
		if (info->has_input_filters)
			metaData.AddMessage("arp:input_filters", &(info->input_filters));
		if (info->has_input_device)
			metaData.AddMessage("arp:input_device", &(info->input_device));
		if (info->has_edit_filters)
			metaData.AddMessage("arp:edit_filters", &(info->edit_filters));
		if (info->has_track_settings)
			metaData.AddMessage("arp:track_settings", &(info->track_settings));
		if (info->has_phrases)
			metaData.AddMessage("arp:phrases", &(info->phrases));
		if (!(info->input_connections.IsEmpty()) )
			metaData.AddMessage("arp:input_connections", &(info->input_connections));
		if (!(info->output_connections.IsEmpty()) )
			metaData.AddMessage("arp:output_connections", &(info->output_connections));
		if (!metaData.IsEmpty()) {
			BMallocIO flat;
			res = AmFlatten(metaData, &flat);
			if (res >= B_OK) {
				res = WriteMetaEvent(io, 0, 0x7f, flat.Buffer(), flat.BufferLength());
				if (res < B_OK) return res;
			}
		}
	}
	
	if (track) {
		AmPhrase* phrase = track->Phrase();
		AmNode* head = NULL;
		if (phrase) head = phrase->HeadNode();
		AmEvent* queue = NULL;
		while (head || queue) {
			AmEvent* ev = head ? head->Event() : NULL;
			if (queue && (!ev || ev->QuickCompare(queue) > 0)) {
				ev = queue;
			} else if (head) {
				head = head->next;
			}
			int32 realChannel = channel;
			AmFilterHolderI* filter = ev->NextFilter();
			while (filter) {
				const int32 c = filter->Filter()
							  ? filter->Filter()->HintChannel()
							  : -1;
				if (c >= 0) {
					realChannel = c;
					break;
				}
				filter = filter->NextInLine();
			}
			
			if (ev->Type() != AmEvent::SIGNATURE_TYPE &&
					ev->Type() != AmEvent::TEMPOCHANGE_TYPE) {
				if (endTime < ev->EndTime()) endTime = ev->EndTime();
			}
			
			#ifdef SMF_NOISY
			printf("Writing event: "); ev->Print();
			#endif
			const bool fromQueue = (queue == ev) ? true : false;
			if ((res=WriteEvent(io, realChannel, ev, &queue)) < B_OK) return res;
			if (fromQueue) {
				queue = ev->RemoveEvent();
				if (queue) queue = queue->HeadEvent();
				ev->Delete();
			}
		}
	}
	
	// Write end-of-track marker.
	WriteMetaEvent(io, endTime, 0x2F, NULL, 0);
	
	return B_OK;
}

status_t AmStandardMidiFile::WriteEvent(ArpStructuredIO& io, int32 channel,
										AmEvent* event, AmEvent** queue) const
{
	uint8 status = 0;
	uint8 c1 = 0;
	uint8 c2 = 0;
	
	switch( event->Type() ) {
		case AmEvent::NOTEON_TYPE: {
			AmNoteOn* noteon = dynamic_cast<AmNoteOn*>(event);
			if( noteon ) {
				status = B_NOTE_ON;
				c1 = noteon->Note();
				c2 = noteon->Velocity();
				
				// First check if there is already a note-off event in the
				// queue for this note.  If so, we want to write that note-off
				// now, so that we don't write a malformed event list in the form
				// "note-on note-on note-off note-off".
				AmEvent* qp = queue ? *queue : NULL;
				while (qp) {
					if (qp->Type() == AmEvent::NOTEOFF_TYPE) {
						AmNoteOff* noteoff = dynamic_cast<AmNoteOff*>(qp);
						if (noteoff && noteoff->Note() == c1) {
							// Gotcha!
							#ifdef SMF_NOISY
							printf("Stopping note %d early: orig time=%Ld, new=%Ld\n",
									c1, noteoff->StartTime(), event->StartTime());
							#endif
							status_t err = WriteChannelEvent(io, event->StartTime(),
												B_NOTE_OFF|channel,  c1, noteoff->Velocity());
							if (err < B_OK)
								return err;
							if (qp == *queue)
								*queue = qp->RemoveEvent();
							else
								qp->RemoveEvent();
							qp->Delete();
							break;
						}
					}
					qp = qp->NextEvent();
				}
				
				try {
					AmNoteOff* noteoff =
						new AmNoteOff(	noteon->Note(),
										noteon->ReleaseVelocity(),
										noteon->EndTime());
					AmEvent* offev = dynamic_cast<AmEvent*>(noteoff);
					if( offev ) {
						AmEvent* onev = dynamic_cast<AmEvent*>(noteon);
						if (onev) offev->SetNextFilter(onev->NextFilter());
						if (*queue) {
							(*queue)->MergeEvent(offev);
							*queue = (*queue)->HeadEvent();
						} else *queue = offev;
					}
				} catch (std::bad_alloc& e) {
				}
			}
		} break;
		case AmEvent::NOTEOFF_TYPE: {
			AmNoteOff* noteoff = dynamic_cast<AmNoteOff*>(event);
			if( noteoff ) {
				status = B_NOTE_OFF;
				c1 = noteoff->Note();
				c2 = noteoff->Velocity();
			}
		} break;
		case AmEvent::CHANNELPRESSURE_TYPE: {
			AmChannelPressure* cp = dynamic_cast<AmChannelPressure*>(event);
			if( cp ) {
				status = B_CHANNEL_PRESSURE;
				c1 = cp->Pressure();
			}
		} break;
		case AmEvent::TEMPOCHANGE_TYPE: {
			AmTempoChange* tc = dynamic_cast<AmTempoChange*>(event);
			if( tc ) {
				int32 mspqn = MSPQN( uint32(tc->Tempo()) );
				uint8 buffer[3];
				buffer[0] = (mspqn>>16) & 0xff;
				buffer[1] = (mspqn>>8) & 0xff;
				buffer[2] = mspqn & 0xff;
				return WriteMetaEvent(io, event->StartTime(), B_TEMPO_CHANGE,
									  buffer, sizeof(buffer));
			}
		} break;
		case AmEvent::SIGNATURE_TYPE: {
			AmSignature* sig = dynamic_cast<AmSignature*>(event);
			if( sig ) {
				uint8 buffer[4];
				buffer[0] = sig->Beats();
				int32 bv = sig->BeatValue();
				buffer[1] = 0;
				while ((bv>>=1) > 0) buffer[1]++;
				buffer[2] = 128;
				buffer[3] = 8;
				return WriteMetaEvent(io, event->StartTime(), 0x58,
									  buffer, sizeof(buffer));
			}
		} break;
		case AmEvent::CONTROLCHANGE_TYPE: {
			AmControlChange* cc = dynamic_cast<AmControlChange*>(event);
			if( cc ) {
				status = B_CONTROL_CHANGE;
				c1 = cc->ControlNumber();
				c2 = cc->ControlValue();
			}
		} break;
		case AmEvent::PITCHBEND_TYPE: {
			AmPitchBend* pb = dynamic_cast<AmPitchBend*>(event);
			if( pb ) {
				status = B_PITCH_BEND;
				c1 = pb->Lsb();
				c2 = pb->Msb();
			}
		} break;
		case AmEvent::PROGRAMCHANGE_TYPE: {
			AmProgramChange* pc = dynamic_cast<AmProgramChange*>(event);
			if( pc ) {
				status = B_PROGRAM_CHANGE;
				c1 = pc->ProgramNumber();
			}
		} break;
		case AmEvent::SYSTEMEXCLUSIVE_TYPE: {
			AmSystemExclusive* se = dynamic_cast<AmSystemExclusive*>(event);
			if( se ) {
				ssize_t res;
				if ((res=io.WriteMidiNumber(MoveTime(event->StartTime()))) < B_OK) return res;
				if ((res=io.WriteInt8(B_SYS_EX_START)) < B_OK) return res;
				if ((res=io.WriteMidiNumber(se->Length())) < B_OK) return res;
				if ((res=io.Write(se->Data(), se->Length())) != (ssize_t)se->Length())
					return res < B_OK ? res : B_ERROR;
				return B_OK;
			}
		} break;
		default: {
			// skip ones we don't know how to handle.
			return B_OK;
		} break;
	}
	
	return WriteChannelEvent(io, event->StartTime(), status|channel,  c1, c2);
}

status_t AmStandardMidiFile::WriteChannelEvent(ArpStructuredIO& io, AmTime time,
								   uint8 status, uint8 c1, uint8 c2) const
{
	// Get number of bytes need to complete this message
	const int32 needed = ChannelEventLength[(status >> 4) & 0x0f];
	if (!needed) {
		debugger("Bad status value for channel event");
		return B_ERROR;
	}

	#ifdef SMF_NOISY
	printf("Writing channel event: time=%Ld, status=0x%x, c1=0x%x, c2=0x%x\n",
			time, status, c1, c2);
	#endif
	ssize_t res;
	if ((res=io.WriteMidiNumber(MoveTime(time))) < B_OK) return res;
	if ((res=io.WriteInt8(status)) < B_OK) return res;
	if ((res=io.WriteInt8(c1&0x7f)) < B_OK) return res;
	if (needed > 1) if ((res=io.WriteInt8(c2&0x7f)) < B_OK) return res;
	return B_OK;
}

status_t AmStandardMidiFile::WriteMetaEvent(ArpStructuredIO& io, AmTime time,
									uint8 type, const void* data, size_t size) const
{
	ssize_t res;
	if ((res=io.WriteMidiNumber(MoveTime(time))) < B_OK) return res;
	if ((res=io.WriteInt8(META_EVENT)) < B_OK) return res;
	if ((res=io.WriteInt8(type)) < B_OK) return res;
	if ((res=io.WriteMidiNumber(size)) < B_OK) return res;
	if (size > 0 && (res=io.Write(data, size)) != (ssize_t)size) return res < B_OK ? res : B_ERROR;
	return B_OK;
}

AmPhraseEvent* AmStandardMidiFile::GetTempoChannel()
{
	if (!mTempoEvents) mTempoEvents = new AmRootPhraseEvent();
	return mTempoEvents;
}

AmPhraseEvent* AmStandardMidiFile::GetSignatureChannel()
{
	if (!mSignatureEvents) mSignatureEvents = new AmRootPhraseEvent();
	return mSignatureEvents;
}

AmTime AmStandardMidiFile::CurrentTime() const
{
	return (AmTime)((int64(mCurTime)*PPQN)/mDivision);
}

int32 AmStandardMidiFile::MoveTime(AmTime newTime) const
{
	#ifdef SMF_NOISY
	printf("Moving time to %Ld, delta is %Ld\n", newTime, (newTime-mCurTime));
	#endif
	if (newTime < mCurTime) {
		printf("*** WARNING: Event start time before previous event time!");
		return 0;
	} else if (newTime == mCurTime) {
		return 0;
	}
	
	const int32 lastTime = mCurTime;
	mCurTime = newTime;
	return newTime-lastTime;
}
