/* AmGlobalsI.h
 * Copyright (c)2000 - 2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This file defines classes that provide global, application-wide
 * behaviour, along with functions for accessing instances of each
 * of these classes.  The application implementing this library needs
 * to implement the accessing functions.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 05.15.00		hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef AMPUBLIC_AMGLOBALSI_H
#define AMPUBLIC_AMGLOBALSI_H

#include <app/Handler.h>
#include <support/SupportDefs.h>
class BClipboard;

#include "ArpKernel/ArpRef.h"
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmPipelineMatrixRef.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmToolBarRef.h"
#include "AmPublic/AmToolRef.h"
class AmDeviceI;
class AmMotionI;
class AmPhrase;
class AmPhraseEvent;
class AmSignature;
class AmTool;
class AmTrack;
class AmViewFactory;

enum {
	AM_DEFAULT_UNDO_HISTORY		= 50,
	AM_DEFAULT_TRACK_HEIGHT		= 23
};
/***************************************************************************
 * AM-GLOBALS-I
 * A class that stores all the globally-available methods and data.
 ***************************************************************************/

// Forward reference
class AmGlobalsI;
// Accessing function
AmGlobalsI& AmGlobals();

// Let application attach globals object
void SetGlobals(AmGlobalsI& globals);

class AmGlobalsI
{
public:
	virtual ~AmGlobalsI()			{ }

	virtual void	Initialize()	{ }

	/*---------------------------------------------------------
	 * CHANGE NOTIFICATION
	 *---------------------------------------------------------*/
	/* These are the events a client can register to hear about
	 * via the AddObserver() method.
	 */
	enum ObserverMessages {
		DEVICE_OBS				= 'Odev',	// Clients will hear when devices are added or removed.
		TOOL_BAR_OBS			= '0tbr'	// Clients will hear when tool bars are added or removed.
	};
	virtual void	AddObserver(uint32 code, BHandler* handler) = 0;
	virtual void	RemoveObserver(uint32 code, BHandler* handler) = 0;
	
	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* This method constructs a new song and sets the supplied ref
	 * to it.  B_OK is returned for success, B_ERROR if anything went wrong.
	 * Clients really should supply the current undo history preference.
	 */
	virtual status_t			NewSong(AmSongRef& songRef, int32 undoHistory) = 0;
	/* Remove the supplied song from the system.  If the song doesn't
	 * exist, answer with B_ERROR.
	 */
	virtual status_t			RemoveSong(song_id id) = 0;
	/* Answer a song ref for the supplied song ID.  If the ID doesn't exist,
	 * the ref will be invalid.
	 */
	virtual AmSongRef			SongRef(song_id id) const = 0;
	/* Select a song as "active".  This is the song from which MIDI clock
	 * events will be sent.
	 */
	virtual	void				SelectSong(const AmSongRef& song) = 0;
	/* Answer the pipeline matrix ref for the supplied pipeline matrix ID.
	 * Currently, only songs and tools are pipeline matrices.  Since both
	 * of these items are stored in the globals, I can do a search for
	 * the proper matrix.  If there is ever a point where there are matrix
	 * objects that aren't stored in the globals, this method will not be
	 * reliable.
	 */
	virtual AmPipelineMatrixRef	PipelineMatrixRef(pipeline_matrix_id id) const = 0;
	/* Answer the tool at the supplied index.  Clients can use this to
	 * iterate over all installed tools.
	 */
	virtual AmToolRef			ToolAt(uint32 index) const = 0;
	/* Answer the tool at the supplied button.  modifierKeys is just the
	 * modifier() call.
	 */
	virtual AmToolRef			Tool(uint32 button, uint32 modifierKeys) const = 0;
	virtual AmToolRef			FindTool(const BString& uniqueName) const = 0;
	/* Answer the tool mapped to the supplied key, if any.
	 */
	virtual AmToolRef			FindTool(int32 key) const = 0;
	virtual void				SetTool(const BString& toolName, uint32 button, uint32 modifierKeys) = 0;
	virtual AmToolBarRef		ToolBarAt(uint32 index, bool init = true) const = 0;
	virtual AmToolBarRef		FindToolBar(const BString& toolBarName, bool init = true) const = 0;
	/* Answer the view factory at the supplied index.  Clients can use
	 * this to iterate over all installed view factories.
	 */
	virtual AmViewFactory*		FactoryAt(uint32 index) const = 0;
	/* Answer the installed view factory with the supplied signature.  If you want the
	 * always-available default factory, use a BString with DEFAULT_FACTORY_SIGNATURE.
	 */
	virtual AmViewFactory*		FactoryNamed(const BString& signature) const = 0;	
	/* Answer the device at the supplied index.
	 */
	virtual status_t			GetDeviceInfo(	const am_studio_endpoint& endpoint,
												BString& deviceMfg,
												BString& deviceProduct,
												BString& deviceLabel) const = 0;
	virtual ArpCRef<AmDeviceI>	DeviceAt(uint32 index) const = 0;
	virtual ArpCRef<AmDeviceI>	DeviceAt(am_studio_endpoint& endpoint) const = 0;
	virtual ArpCRef<AmDeviceI>	DeviceNamed(const char* mfg, const char* name) const = 0;
	/* Answer the endpoint and device info at the given index.  This will answer
	 * dormant endpoints -- ones that may have existed at one point but no longer do
	 * (and may again) as well as currently active endpoints.
	 */
	virtual status_t			GetStudioInfoAt(uint32 index, am_studio_endpoint& outEndpoint,
												BString* outLabel = NULL, BString* outDevMfg = NULL,
												BString* outDevName = NULL) const = 0;
	/* Motion accessing.  These are pass-throughs to the motion roster.
	 */
	virtual status_t			GetMotionInfo(	uint32 index,
												BString& outLabel,
												BString& outKey) const = 0;
	virtual AmMotionI*			NewMotion(const BString& key) const = 0;

	/* It's useful to have a buffer between when the song actually
	 * ends and where the UI thinks it ends, otherwise scrollbars give
	 * no extra room beyond the immediate end of the song for adding events.
	 */
	virtual AmTime				EndTimeBuffer() const = 0;

	/* Answer the phrase event closest to the supplied time, provided it's
	 * within tolerance.  If none is within tolerance, answer 0.
	 */
	virtual AmPhraseEvent*		 PhraseEventNear(const AmTrack* track, AmTime time) const = 0;
	/* If clients wich, they can specify their own tolerance.  Note that in
	 * general, its better to take the default, which may become a user preference.
	 */
	virtual AmPhraseEvent*		 PhraseEventNear(	const AmTrack* track,
													AmTime time,
													AmTime tolerance) const = 0;
	/* Populate the supplied signature object with the signature information at
	 * the given time.
	 */
	virtual status_t			GetMeasure(const AmPhrase& signatures, AmTime time, AmSignature* signature) const = 0;
	/* A convenience to set the undo history size of all existing songs.
	 */
	virtual void				SetUndoHistory(int32 size) = 0;

	/* This is only temporary.  As soon as events can write themselves out
	 * to a stream, all copying and pasting will be done via the system-wide
	 * clipboard.
	 */
	virtual BClipboard*			Clipboard() const = 0;

	/* Write and read my state from the supplied message.
	 */
	virtual status_t			WriteTo(BMessage* state) = 0;
	virtual status_t			ReadFrom(const BMessage* state) = 0;
	
	/* Add and remove AmFilterAddOn objects that are to receive clock
	 * notifications.
	 */
	virtual	void				AddClockTarget(AmFilterAddOn* target) = 0;
	virtual	status_t			RemoveClockTarget(AmFilterAddOn* target) = 0;
};

#endif
