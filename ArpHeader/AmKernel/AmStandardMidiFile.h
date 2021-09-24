/* AmStandardMidiFile.h
 * Copyright (c)2000 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Dianne Hackborn
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Angry Red Planet,
 * at <hackborn@angryredplanet.com> or <hackbod@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2000/06/03		hackbod
 * Created this file
 */

#ifndef AMKERNEL_AMSTANDARDMIDIFILE_H
#define AMKERNEL_AMSTANDARDMIDIFILE_H

#include "AmPublic/AmEvents.h"

#include <List.h>
#include <String.h>

class ArpStructuredIO;
class AmNode;
class AmPhraseEvent;

struct smf_track_info
{
	BString title;
	int32 channel;
	bool has_output_device;
	bool has_output_filters;
	bool has_input_device;
	bool has_input_filters;
	bool has_edit_filters;
	bool has_track_settings;
	bool has_phrases;
	bool has_input_connections;
	bool has_output_connections;

	BMessage output_device;
	BMessage output_filters;
	BMessage input_device;
	BMessage input_filters;
	BMessage edit_filters;
	BMessage track_settings;
	BMessage phrases;
	BMessage input_connections;
	BMessage output_connections;
	
	smf_track_info()
		: channel(-1),
		  has_output_device(true), has_output_filters(true),
		  has_input_device(true), has_input_filters(true),
		  has_edit_filters(true), has_track_settings(true),
		  has_phrases(true)
	{
	}
	~smf_track_info()
	{
	}
};


/*****************************************************************************
 *
 *	AM-STANDARD-MIDI-FILE CLASS
 *
 *	Read and write standard midi files.
 *
 *****************************************************************************/

class AmStandardMidiFile {
public:
	AmStandardMidiFile();
	virtual ~AmStandardMidiFile();
	
	status_t ReadFile(BDataIO* file);

	status_t WriteFile(BDataIO* file);
	
	int32 CountTracks() const;
	const AmPhraseEvent* TrackAt(int32 i, smf_track_info* outInfo) const;
	AmPhraseEvent* RemoveTrack(int32 i, smf_track_info* outInfo);
	
	const AmPhraseEvent* TempoTrack() const;
	AmPhraseEvent* DetachTempoTrack();
	
	const AmPhraseEvent* SignatureTrack() const;
	AmPhraseEvent* DetachSignatureTrack();
	
	status_t AddTrack(AmPhraseEvent* track, const smf_track_info& info);
	status_t SetTempoTrack(AmPhraseEvent* track);
	status_t SetSignatureTrack(AmPhraseEvent* track);
	
	void MakeEmpty();

	struct channel_list {
		AmNode* head;
		AmNode* tail;
		
		status_t append(AmEvent* event);
		status_t append_sysex(AmEvent* event);
		
		channel_list() : head(NULL), tail(NULL) { }
	};

private:
	void Reset();
	
	status_t	ReadTrack(ArpStructuredIO& io);
	AmEvent*	ReadPhrases(channel_list& list, BMessage& phrases);
	void		CreateBanks(channel_list& list);
	void		CreateCommands(channel_list& list);
	AmEvent*	CreateRootPhrases(channel_list& list, BMessage& phrases);

	status_t AddChannelEvent(channel_list& into, AmTime time,
								uint8 type, uint8 c1, uint8 c2);
	status_t AddMetaEvent(smf_track_info* into, AmTime time,
							uint8 type, const char* buffer, size_t size);
	
	status_t WriteTrack(ArpStructuredIO& io, int32 channel, AmPhraseEvent* track,
						const smf_track_info* info) const;
	status_t WriteEvent(ArpStructuredIO& io, int32 channel,
						AmEvent* event, AmEvent** queue) const;
	status_t WriteChannelEvent(ArpStructuredIO& io, AmTime time,
							   uint8 status, uint8 c1, uint8 c2) const;
	status_t WriteMetaEvent(ArpStructuredIO& io, AmTime time,
							uint8 type, const void* data, size_t size) const;
	
	AmPhraseEvent* GetTempoChannel();
	AmPhraseEvent* GetSignatureChannel();
	
	AmTime CurrentTime() const;
	int32 MoveTime(AmTime newTime) const;
	
	int16 mDivision;
	mutable int32 mCurTime;
	
	BList mTracksEvents;				// Contains AmPhraseEvent objects.
	BList mTracksInfo;					// Contains smf_track_info objects.
	AmPhraseEvent* mTempoEvents;
	AmPhraseEvent* mSignatureEvents;
	
	// Information about current track being read.
	smf_track_info mCurTrackInfo;
	
	channel_list mChannels[16];
};

#endif
