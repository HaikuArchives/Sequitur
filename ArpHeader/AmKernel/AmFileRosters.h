/* AmFileRosters.h
 * Copyright (c)2001 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Eric Hackborn
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
 * 2001.03.21		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMKERNEL_AMFILEROSTERS_H
#define AMKERNEL_AMFILEROSTERS_H

#include <vector.h>
#include <app/Handler.h>
#include <support/Locker.h>
#include "AmKernel/AmDevice.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmMultiFilter.h"
#include "AmKernel/AmNotifier.h"
class _AmRosterSearchPath;
class AmMotion;
class AmMotionI;

enum {
	AM_FILE_ROSTER_CHANGED		= 'aRCh'
};

/***************************************************************************
 * AM-FILE-ROSTER
 * This is the abstract superclass for all the rosters that are file-based,
 * i.e. their contents are built dynamically based on the current contents
 * of a given file.
 ***************************************************************************/
class AmFileRoster
{
public:
	AmFileRoster(const char* name);
	virtual ~AmFileRoster();

	BString					Name() const;
	status_t				AddObserver(BHandler* handler);
	status_t				RemoveObserver(BHandler* handler);
	virtual void			SendChangedNotice();
	
	virtual uint32			CountEntries() const = 0;
	virtual BString			KeyAt(uint32 index) const = 0;
	file_entry_id			EntryIdAt(uint32 index) const;
	/* Answer true if there's already an entry with this key.  Subclasses
	 * can reimplement to get better performance if they like.
	 */
	virtual bool			KeyExists(const char* key) const;

	status_t				CreateEntry(const AmFileRosterEntryI* entry);
	status_t				DuplicateEntry(const BString& key, const char* path);
	status_t				DeleteEntry(const BString& key);
	/* This is called with a file name and a flattened object message
	 * whenever a new file appears in one of my search directories.
	 */
	virtual status_t		EntryCreated(	const BMessage& msg,
											bool readOnly,
											const char* filePath) = 0;
	virtual status_t		EntryRemoved(	const char* filePath) = 0;

protected:
	_AmRosterSearchPath*	mSearchPath;	
	_AmRosterSearchPath*	mReadOnlySearchPath;	
	AmNotifier				mNotifier;
	/* All accessing of the entry list needs to be protected.
	 */
	mutable BLocker 		mEntryLock; 

	status_t				CreateEntry(const BString& fileName, const BMessage& msg);
	status_t				DeleteFile(const char* filePath);
	
	void					SetCreationDirectory(const char* path);
	/* Answer a new string based on the supplied one that's unique
	 * across all my entries.  Anyone calling this should have already
	 * locked mEntryLock.
	 */
	BString					UniqueLabelFrom(const BString& origLabel) const;
	BString					UniqueKeyFrom(const BString& origKey) const;
	/* Some subclasses might have a lock around their entries, but
	 * this is safe because I have my own lock that prevents other
	 * modification from occurring while I have an entry.  Note,
	 * though, that these methods don't actually lock -- anyone
	 * calling either of them should have locked the mEntryLock.
	 */
	virtual AmFileRosterEntryI* EntryAt(const BString& key, const char* path = NULL) const = 0;
	virtual AmFileRosterEntryI* EntryAt(uint32 index) const = 0;
	/* Subclasses must answer a newly allocated file roster entry with
	 * a unique key, label, whatever is necessary.  The mEntryLock is
	 * already locked when this method gets called.
	 */
	virtual AmFileRosterEntryI* UniqueDuplicate(AmFileRosterEntryI* entry) const = 0;
	/* Subclasses are required to respond with the MIME type for
	 * the files they create.
	 */
	virtual const char*		MimeType() const = 0;

private:
	BString					mName;
	BString					mCreationDirectory;

	bool					LabelExists(const BString& label) const;
	BString					UniqueFileNameFrom(const BString& fileName);
	bool					FileNameExists(const BString& fileName);
};

/***************************************************************************
 * AM-DEVICE-ROSTER
 ***************************************************************************/
typedef vector< ArpRef<AmDevice> >		device_vec;
class _AmDeviceObserver;

class AmDeviceRoster : public AmFileRoster
{
public:
	AmDeviceRoster();
	virtual ~AmDeviceRoster();

	static AmDeviceRoster*	Default();
	static void				ShutdownDefault(bool force_unload=false);
	/* I allow filter holder observers -- not direct filters
	 * because they aren't ref counted.
	 */
	status_t				AddFilterObserver(AmFilterHolderI* holder);
	status_t				RemoveFilterObserver(AmFilterHolderI* holder);
	virtual void			SendChangedNotice();

	status_t				GetDeviceInfo(	uint32 index,
											BString& outMfg,
											BString& outName,
											BString& outKey,
											bool* outReadOnly = NULL,
											bool* outIsValid = NULL,
											BString* outDescription = NULL,
											BString* outAuthor = NULL,
											BString* outEmail = NULL,
											BBitmap** outIcon = NULL,
											BString* outFilePath = NULL) const;
	AmDevice*				NewDevice(const BString& key, const char* path = NULL) const;
	ArpCRef<AmDeviceI>		DeviceAt(uint32 index) const;
	ArpCRef<AmDeviceI>		DeviceNamed(const char* mfg, const char* name) const;

	virtual uint32			CountEntries() const;
	virtual BString			KeyAt(uint32 index) const;

	virtual status_t		EntryCreated(	const BMessage& msg,
											bool readOnly,
											const char* filePath);
	virtual status_t		EntryRemoved(	const char* filePath);

protected:
	virtual AmFileRosterEntryI* EntryAt(const BString& key, const char* path = NULL) const;
	virtual AmFileRosterEntryI* EntryAt(uint32 index) const;
	virtual AmFileRosterEntryI* UniqueDuplicate(AmFileRosterEntryI* entry) const;
	virtual const char*		MimeType() const;

private:
	typedef AmFileRoster		inherited;
	BLocker 					mObserverLock; 

	device_vec					mDevices;
	_AmDeviceObserver*			mObserver;
};

/***************************************************************************
 * AM-MOTION-ROSTER
 ***************************************************************************/
class AmMotionRoster : public AmFileRoster
{
public:
	AmMotionRoster();
	virtual ~AmMotionRoster();

	static AmMotionRoster*	Default();
	static void				ShutdownDefault(bool force_unload=false);

	status_t				GetMotionInfo(	uint32 index,
											BString& outLabel,
											BString& outKey,
											bool* outReadOnly = NULL,
											bool* outIsValid = NULL,
											BString* outDescription = NULL,
											BString* outAuthor = NULL,
											BString* outEmail = NULL,
											BString* outFilePath = NULL) const;
	AmMotionI*				NewMotion(const BString& key, const char* path = NULL) const;

	virtual uint32			CountEntries() const;
	virtual BString			KeyAt(uint32 index) const;

	virtual status_t		EntryCreated(	const BMessage& msg,
											bool readOnly,
											const char* filePath);
	virtual status_t		EntryRemoved(	const char* filePath);

protected:
	virtual AmFileRosterEntryI* EntryAt(const BString& key, const char* path = NULL) const;
	virtual AmFileRosterEntryI* EntryAt(uint32 index) const;
	virtual AmFileRosterEntryI* UniqueDuplicate(AmFileRosterEntryI* entry) const;
	virtual const char*		MimeType() const;

private:
	typedef AmFileRoster	inherited;

	vector<AmMotion*>		mMotions;
};

/***************************************************************************
 * AM-MULTI-FILTER-ROSTER
 ***************************************************************************/
typedef vector< ArpRef<AmMultiFilterAddOn> >		multifilter_vec;

class AmMultiFilterRoster : public AmFileRoster
{
public:
	AmMultiFilterRoster();
	virtual ~AmMultiFilterRoster();

	static AmMultiFilterRoster*	Default();
	static void					ShutdownDefault(bool force_unload=false);

	status_t					GetFilterInfo(	uint32 index,
												BString& outLabel,
												BString& outKey,
												bool* outReadOnly,
												BString* outAuthor = NULL,
												BString* outEmail = NULL) const;
	AmMultiFilterAddOn*			NewFilter(const BString& key, const char* path = NULL) const;
	ArpRef<AmMultiFilterAddOn>	FilterAt(uint32 index) const;
	ArpRef<AmMultiFilterAddOn>	FindFilterAddOn(const BMessage* archive) const;
	ArpRef<AmMultiFilterAddOn>	FindFilterAddOn(const char* uniqueName) const;

	virtual uint32				CountEntries() const;
	virtual BString				KeyAt(uint32 index) const;

	virtual status_t			EntryCreated(	const BMessage& msg,
												bool readOnly,
												const char* filePath);
	virtual status_t			EntryRemoved(	const char* filePath);

protected:
	virtual AmFileRosterEntryI* EntryAt(const BString& key, const char* path = NULL) const;
	virtual AmFileRosterEntryI* EntryAt(uint32 index) const;
	virtual AmFileRosterEntryI* UniqueDuplicate(AmFileRosterEntryI* entry) const;
	virtual const char*			MimeType() const;

private:
	typedef AmFileRoster	inherited;

	multifilter_vec			mFilters;
};

/***************************************************************************
 * AM-TOOL-ROSTER
 ***************************************************************************/
class AmToolRoster : public AmFileRoster
{
public:
	AmToolRoster();
	virtual ~AmToolRoster();

	static AmToolRoster*	Default();
	static void				ShutdownDefault(bool force_unload=false);

	status_t				GetToolInfo(uint32 index,
										BString& outLabel,
										BString& outUniqueName,
										bool* outReadOnly = NULL,
										bool* outIsValid = NULL,
										BString* outAuthor = NULL,
										BString* outEmail = NULL,
										BString* outShortDescription = NULL,
										BString* outLongDescription = NULL,
										BBitmap** outIcon = NULL,
										BString* outFilePath = NULL) const;
	/* If a path is supplied, then both the key and file path
	 * must match, or else nothing is returned.
	 */
	AmTool*					NewTool(const BString& key, const char* path = NULL) const;

	AmToolRef				ToolAt(uint32 index) const;
	AmToolRef				Tool(uint32 button, uint32 modifierKeys) const;
	AmToolRef				FindTool(const BString& uniqueName) const;
	AmToolRef				FindTool(int32 key) const;
	void					SetTool(const BString& toolName, uint32 button, uint32 modifierKeys);

	virtual uint32			CountEntries() const;
	virtual BString			KeyAt(uint32 index) const;
	virtual bool			KeyExists(const char* key) const;

	virtual status_t		EntryCreated(	const BMessage& msg,
											bool readOnly,
											const char* filePath);
	virtual status_t		EntryRemoved(	const char* filePath);

protected:
	virtual AmFileRosterEntryI* EntryAt(const BString& key, const char* path = NULL) const;
	virtual AmFileRosterEntryI* EntryAt(uint32 index) const;
	virtual AmFileRosterEntryI* UniqueDuplicate(AmFileRosterEntryI* entry) const;
	virtual const char*		MimeType() const;

private:
	typedef AmFileRoster	inherited;

	toolref_vec				mToolRefs;
	/* You can map tools to the primary, secondary, and tertiary buttons.
	 * You can have an alternate set of mappings if the CTRL key is down.
	 */
	BString					mPriToolName;
	BString					mSecToolName;
	BString					mTerToolName;

	BString					mPriCtrlToolName;
	BString					mSecCtrlToolName;
	BString					mTerCtrlToolName;

	/* A convenience that wipes out any tool mappings I might have and sets
	 * me to the defaults.  THIS IS OBSOLETE, need to probably store tools
	 * as simple strings and dynamically look them up.
	 */
	void					ResetToolMaps();
};

#endif
