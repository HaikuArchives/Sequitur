#define _BUILDING_AmKernel 1

#ifndef AMKERNEL_AMBANK_H
#include "AmKernel/AmBank.h"
#endif

#include <stdio.h>
#include <stdlib.h>

static const char*		NAME_STR				= "name";
static const char*		PATCH_STR				= "patch";
static const char*		FIRST_PATCH_NUMBER_STR	= "First Patch Number";
static const char*		SELECTION_STR			= "selection";

/*************************************************************************
 * AM-BANK
 *************************************************************************/
AmBank::AmBank(const char* name, uint32 patchSize)
		: mBank(0), mName(name), mPatches(patchSize),
		  mFirstPatchNumber(0), mSelectionEvent(NULL)
{
	InitPatchNames(0, patchSize);
}

AmBank::AmBank(const BMessage& msg)
		: mBank(0), mName(NULL), mPatches(0),
		  mFirstPatchNumber(0), mSelectionEvent(NULL)
{
	ReadFrom(msg);
}

AmBank::AmBank(const AmBank& o)
		: mBank(o.mBank), mName(o.mName), mPatches(o.mPatches.size() ),
		  mFirstPatchNumber(o.mFirstPatchNumber),
		  mSelectionEvent(NULL)
{
	InitPatchNames(0, o.mPatches.size() );
	for (uint32 k = 0; k < o.mPatches.size(); k++) {
		SetPatchName(k, o.mPatches[k].String() );
	}
	if (o.mSelectionEvent) SetBankSelection(o.mSelectionEvent);
}

AmBank::~AmBank()
{
	if (mSelectionEvent) mSelectionEvent->Delete();
}

uint32 AmBank::BankNumber() const
{
	return mBank;
}

BString AmBank::Name() const
{
	return mName;
}

uint32 AmBank::CountPatches() const
{
	return mPatches.size();
}

BString AmBank::PatchName(uint32 number) const
{
	if( number < 0 || number >= mPatches.size() ) return BString();
	return mPatches[number];
}

uint32 AmBank::FirstPatchNumber() const
{
	return mFirstPatchNumber;
}

AmEvent* AmBank::NewBankSelection() const
{
	if (!mSelectionEvent) return NULL;
	return mSelectionEvent->Copy();
}

void AmBank::SetBankNumber(uint32 number)
{
	mBank = number;
	if (mSelectionEvent) mSelectionEvent->SetBankNumber(mBank);
}

void AmBank::SetName(const char* name)
{
	mName = name;
	if (mSelectionEvent) mSelectionEvent->SetName(mName.String());
}

void AmBank::SetPatchCount(uint32 size)
{
	uint32 oldSize = mPatches.size();
	mPatches.resize(size);
	if( oldSize < size ) InitPatchNames(oldSize, size);
}

void AmBank::SetPatchName(uint32 number, const char* name)
{
	mPatches[number] = name;
}

void AmBank::SetFirstPatchNumber(uint32 number)
{
	mFirstPatchNumber = number;
}

void AmBank::SetBankSelection(const AmBankChange* bankEvent)
{
	if (mSelectionEvent) delete mSelectionEvent;
	mSelectionEvent = NULL;
	if (bankEvent) mSelectionEvent = dynamic_cast<AmBankChange*>( bankEvent->Copy() );
	if (mSelectionEvent) mSelectionEvent->Set(mBank, mName.String());
}

AmBankChange* AmBank::BankSelection() const
{
	return mSelectionEvent;
}

AmBank* AmBank::Copy() const
{
	return new AmBank(*this);
}

status_t AmBank::WriteTo(BMessage& msg) const
{
	if (mName.Length() > 0) msg.AddString(NAME_STR, mName.String());
	uint32		count = CountPatches();
	for (uint32 k = 0; k < count; k++) {
		msg.AddString(PATCH_STR, mPatches[k].String());
	}
	msg.AddInt32(FIRST_PATCH_NUMBER_STR, mFirstPatchNumber);
	if (mSelectionEvent) {
		BMessage	flatEvent;
		if (mSelectionEvent->GetAsMessage(flatEvent) == B_OK)
			msg.AddMessage(SELECTION_STR, &flatEvent);
	}
	return B_OK;
}

status_t AmBank::ReadFrom(const BMessage& msg)
{
	const char*		str;
	if (msg.FindString(NAME_STR, &str) == B_OK) mName = str;
	mPatches.resize(0);
	for (uint32 k = 0; msg.FindString(PATCH_STR, k, &str) == B_OK; k++) {
		mPatches.push_back( BString(str) );
	}
	int32			i;
	if (msg.FindInt32(FIRST_PATCH_NUMBER_STR, &i) == B_OK) mFirstPatchNumber = i;
	if (mSelectionEvent) SetBankSelection(NULL);
	BMessage		flatEvent;
	if (msg.FindMessage(SELECTION_STR, &flatEvent) == B_OK) {
		AmBankChange*	be = dynamic_cast<AmBankChange*>( am_get_as_event(flatEvent) );
		if (be) SetBankSelection(be);
	}
	return B_OK;
}

void AmBank::InitPatchNames(uint32 from, uint32 to)
{
	for (uint32 i=from; i<to; i++) {
		BString& str = mPatches[i];
		str = "";
	}
}
