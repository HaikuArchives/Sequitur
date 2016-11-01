/* AmSongFunctionRoster.cpp
*/

#define _BUILDING_AmKernel 1

#ifndef AMKERNEL_AMSONGFUNCTIONROSTER_H
#include "AmKernel/AmSongFunctionRoster.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <algo.h>
#include <interface/OutlineListView.h>
#include <interface/ScrollView.h>
#include <interface/Window.h>

#include "AmPublic/AmFilterI.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

/*************************************************************************
 * AM-PROGRAM-CHANGE-REPORT
 * This is the definition of an object that may become an add-on.  These
 * objects are provided songs and allowed to process them however they like.
 *************************************************************************/
class AmProgramChangeReport : public AmSongFunctionI
{
public:
	AmProgramChangeReport();

	virtual const char* Name() const			{ return "Program change report"; }
	virtual bool WriteMode() const				{ return false; }

	virtual void ReadSong(const AmSong* song);
};

// #pragma mark -

/*************************************************************************
 * AM-CONTROL-CHANGE-REPORT
 * This is the definition of an object that may become an add-on.  These
 * objects are provided songs and allowed to process them however they like.
 *************************************************************************/
class AmControlChangeReport : public AmSongFunctionI
{
public:
	AmControlChangeReport();

	virtual const char* Name() const			{ return "Control change report"; }
	virtual bool WriteMode() const				{ return false; }

	virtual void ReadSong(const AmSong* song);
};

// #pragma mark -

/*************************************************************************
 * AM-SONG-FUNCTION-I
 *************************************************************************/
AmSongFunctionI::~AmSongFunctionI()
{
}

// #pragma mark -

/***************************************************************************
 * AM-SONG-FUNCTION-ROSTER
 * This class contains objects that can operate on songs.
 ***************************************************************************/
AmSongFunctionRoster::AmSongFunctionRoster()
{
	AmSongFunctionI*	function = new AmControlChangeReport();
	if (function) mFunctions.push_back(function);
	function = new AmProgramChangeReport();
	if (function) mFunctions.push_back(function);
}

AmSongFunctionRoster::~AmSongFunctionRoster()
{
}

static int32					gRosterCreated = 0;
static AmSongFunctionRoster*	gRoster = NULL;

AmSongFunctionRoster* AmSongFunctionRoster::Default()
{
	if (atomic_or(&gRosterCreated, 1) == 0) {
		gRoster = new AmSongFunctionRoster();
	} else {
		while (!gRoster) sleep(20000);
	}
	return gRoster;
}

void AmSongFunctionRoster::ShutdownDefault(bool force_unload)
{
	if (gRoster) {
		delete gRoster;
		gRoster = NULL;
	}
}

AmSongFunctionI* AmSongFunctionRoster::FunctionAt(uint32 index) const
{
	if (index >= mFunctions.size() ) return NULL;
	return mFunctions[index];
}

AmSongFunctionI* AmSongFunctionRoster::FindFunction(const char* name) const
{
	if (!name) return NULL;
	for (uint32 k = 0; k < mFunctions.size(); k++) {
		if ( mFunctions[k]->Name() ) {
			if (strcmp(mFunctions[k]->Name(), name) == 0) return mFunctions[k];
		}
	}
	return NULL;
}

// #pragma mark -

/*************************************************************************
 * _AM-PROGRAM-ENTRY
 *************************************************************************/
class _AmProgramEntry
{
public:
	_AmProgramEntry(AmFilterHolderI* holder);
	_AmProgramEntry(const _AmProgramEntry &o);

	_AmProgramEntry&	operator=(const _AmProgramEntry &o);
	void				AddProgram(uint8 program);
	bool				Matches(const _AmProgramEntry &o) const;
	void				Merge(const _AmProgramEntry &o);
	void				AddTo(BOutlineListView* list) const;
	
	mutable vector<uint8>	programs;
	BString					name;
};

_AmProgramEntry::_AmProgramEntry(AmFilterHolderI* holder)
{
	if ( holder && holder->Filter() && holder->Filter()->AddOn() )
		name = holder->Filter()->AddOn()->Name();
}

_AmProgramEntry::_AmProgramEntry(const _AmProgramEntry &o)
{
	name = o.name;
	for (uint32 k = 0; k < o.programs.size(); k++)
		programs.push_back( o.programs[k] );
}

_AmProgramEntry& _AmProgramEntry::operator=(const _AmProgramEntry &o)
{
	name = o.name;
	programs.resize(0);
	for (uint32 k = 0; k < o.programs.size(); k++)
		programs.push_back( o.programs[k] );
	return *this;
}

void _AmProgramEntry::AddProgram(uint8 program)
{
	for (uint32 k = 0; k < programs.size(); k++)
		if (programs[k] == program) return;
	programs.push_back(program);
}

bool _AmProgramEntry::Matches(const _AmProgramEntry &o) const
{
	if (name.Length() <= 0 && o.name.Length() <= 0) return true;
	return name.Compare(o.name) == 0;
}

void _AmProgramEntry::Merge(const _AmProgramEntry &o)
{
	for (uint32 k = 0; k < o.programs.size(); k++)
		AddProgram( o.programs[k] );
}

void _AmProgramEntry::AddTo(BOutlineListView* list) const
{
	const char*		n = "(no name)";
	if (name.Length() > 0) n = name.String();
	BStringItem*	superitem = new BStringItem(n);
	if (!superitem) return;
	list->AddItem(superitem);

	sort( programs.begin(), programs.end(), greater<uint8>() );
	for (uint32 k = 0; k < programs.size(); k++) {
		BString			str;
		str << (int)programs[k];
		BStringItem*	item = new BStringItem( str.String() );
		if (item) list->AddUnder(item, superitem);
	}
}

// #pragma mark -

/*************************************************************************
 * _AM-PROGRAM-ENTRY-COLLECTION
 *************************************************************************/
class _AmProgramEntryCollection
{
public:
	_AmProgramEntryCollection()		{ }
	
	void		AddEntry(const _AmProgramEntry& entry);
	void		AddTo(BOutlineListView* list) const;
	void		Print() const;
	
private:
	vector<_AmProgramEntry>	mEntries;
};

void _AmProgramEntryCollection::AddEntry(const _AmProgramEntry& entry)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if ( mEntries[k].Matches(entry) ) {
			mEntries[k].Merge(entry);
			return;
		}
	}
	mEntries.push_back( _AmProgramEntry(entry) );
}

void _AmProgramEntryCollection::AddTo(BOutlineListView* list) const
{
	if (mEntries.size() <= 0) {
		BStringItem*	item = new BStringItem("No program changes");
		list->AddItem(item);
		return;
	}

	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].AddTo(list);
}

void _AmProgramEntryCollection::Print() const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		printf("Device %s has programs:\n", mEntries[k].name.String() );
		for (uint32 j = 0; j < mEntries[k].programs.size(); j++) {
			printf("\t%d\n", mEntries[k].programs[j]);
		}
	}
}

// #pragma mark -

/*************************************************************************
 * AM-PROGRAM-CHANGE-REPORT
 *************************************************************************/
AmProgramChangeReport::AmProgramChangeReport()
{
}

static void program_report(const AmPhrase* phrase, _AmProgramEntry& entry)
{
	if (!phrase) return;
	AmNode*		node = phrase->HeadNode();
	while (node) {
		if ( node->Event() ) {
			if (node->Event()->Type() == node->Event()->PHRASE_TYPE) {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( node->Event() );
				if (pe) program_report(pe->Phrase(), entry);
			} else if (node->Event()->Type() == node->Event()->PROGRAMCHANGE_TYPE) {
				AmProgramChange*	pc = dynamic_cast<AmProgramChange*>( node->Event() );
				if (pc) entry.AddProgram( pc->ProgramNumber() );
			}
		}
		node = node->next;
	}
}

void AmProgramChangeReport::ReadSong(const AmSong* song)
{
	_AmProgramEntryCollection	entries;
	const AmTrack*				track;
	for (uint32 k = 0; (track = song->Track(k)); k++) {
		_AmProgramEntry			entry( track->Filter(DESTINATION_PIPELINE) );
		program_report(&(track->Phrases()), entry);
		if (entry.programs.size() > 0) entries.AddEntry(entry);
	}

	BRect		f(100, 100, 400, 400);
	BWindow*	window = new BWindow(	f, "Program change report", B_DOCUMENT_WINDOW_LOOK,
										B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS);
	if (window) {
		BOutlineListView*	lv = new BOutlineListView(	BRect(0, 0, f.Width() - 15, f.Height() - 15), "outline",
														B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
		if (lv) {
			entries.AddTo(lv);
			BScrollView*	scroll = new BScrollView("scroll", lv, B_FOLLOW_ALL, 0, true, true);
			if (scroll) window->AddChild(scroll);
			else window->AddChild(lv);
		}
		window->Show();
	}
}


// #pragma mark -

/*************************************************************************
 * _AM-CONTROL-ENTRY
 *************************************************************************/
class _AmControlEntry
{
public:
	_AmControlEntry();
	_AmControlEntry(const AmTrack* Track);
	_AmControlEntry(const _AmControlEntry &o);

	_AmControlEntry&	operator=(const _AmControlEntry &o);
	void				AddControl(uint8 control);
	void				AddTo(BOutlineListView* list) const;
	
	mutable vector<uint8>	controls;
	const AmTrack*			track;
//	BString					name;
};

_AmControlEntry::_AmControlEntry()
		: track(NULL)
{
}

_AmControlEntry::_AmControlEntry(const AmTrack* Track)
{
	track = Track;
}

_AmControlEntry::_AmControlEntry(const _AmControlEntry &o)
{
	track = o.track;
	for (uint32 k = 0; k < o.controls.size(); k++)
		controls.push_back(o.controls[k] );
}

_AmControlEntry& _AmControlEntry::operator=(const _AmControlEntry &o)
{
	track = o.track;
	controls.resize(0);
	for (uint32 k = 0; k < o.controls.size(); k++)
		controls.push_back( o.controls[k] );
	return *this;
}

void _AmControlEntry::AddControl(uint8 control)
{
	for (uint32 k = 0; k < controls.size(); k++)
		if (controls[k] == control) return;
	controls.push_back(control);
}

void _AmControlEntry::AddTo(BOutlineListView* list) const
{
	const char*		n = "(no name)";
	if (track) n = track->Title();
	BStringItem*	superitem = new BStringItem(n);
	if (!superitem) return;
	list->AddItem(superitem);

	sort( controls.begin(), controls.end(), greater<uint8>() );
	for (uint32 k = 0; k < controls.size(); k++) {
		BString			str;
		str << (int)controls[k];
		BStringItem*	item = new BStringItem( str.String() );
		if (item) list->AddUnder(item, superitem);
	}
}

// #pragma mark -

/*************************************************************************
 * _AM-CONTROL-ENTRY-COLLECTION
 *************************************************************************/
class _AmControlEntryCollection
{
public:
	_AmControlEntryCollection()		{ }
	
	void		AddEntry(const _AmControlEntry& entry);
	void		AddTo(BOutlineListView* list) const;
	void		Print() const;
	
private:
	vector<_AmControlEntry>	mEntries;
};

void _AmControlEntryCollection::AddEntry(const _AmControlEntry& entry)
{
	mEntries.push_back(_AmControlEntry(entry) );
}

void _AmControlEntryCollection::AddTo(BOutlineListView* list) const
{
	if (mEntries.size() <= 0) {
		BStringItem*	item = new BStringItem("No control changes");
		list->AddItem(item);
		return;
	}

	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].AddTo(list);
}

void _AmControlEntryCollection::Print() const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		BString		name("<error>");
		if (mEntries[k].track) name = mEntries[k].track->Title();
		printf("Track %s has control changes:\n", name.String() );
		for (uint32 j = 0; j < mEntries[k].controls.size(); j++) {
			printf("\t%d\n", mEntries[k].controls[j]);
		}
	}
}

// #pragma mark -

/*************************************************************************
 * AM-CONTROL-CHANGE-REPORT
 *************************************************************************/
AmControlChangeReport::AmControlChangeReport()
{
}

static void control_report(const AmPhrase* phrase, _AmControlEntry& entry)
{
	if (!phrase) return;
	AmNode*		node = phrase->HeadNode();
	while (node) {
		if (node->Event() ) {
			if (node->Event()->Type() == node->Event()->PHRASE_TYPE) {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(node->Event() );
				if (pe) control_report(pe->Phrase(), entry);
			} else if (node->Event()->Type() == node->Event()->CONTROLCHANGE_TYPE) {
				AmControlChange*	cc = dynamic_cast<AmControlChange*>(node->Event() );
				if (cc) entry.AddControl(cc->ControlNumber() );
			}
		}
		node = node->next;
	}
}

void AmControlChangeReport::ReadSong(const AmSong* song)
{
	_AmControlEntryCollection	entries;
	const AmTrack*				track;
	for (uint32 k = 0; (track = song->Track(k)); k++) {
		_AmControlEntry			entry(track);
		control_report(&(track->Phrases()), entry);
		if (entry.controls.size() > 0) entries.AddEntry(entry);
	}

	BRect		f(100, 100, 400, 400);
	BWindow*	window = new BWindow(	f, "Control change report", B_DOCUMENT_WINDOW_LOOK,
										B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS);
	if (window) {
		BOutlineListView*	lv = new BOutlineListView(	BRect(0, 0, f.Width() - 15, f.Height() - 15), "outline",
														B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
		if (lv) {
			entries.AddTo(lv);
			BScrollView*	scroll = new BScrollView("scroll", lv, B_FOLLOW_ALL, 0, true, true);
			if (scroll) window->AddChild(scroll);
			else window->AddChild(lv);
		}
		window->Show();
	}
}
