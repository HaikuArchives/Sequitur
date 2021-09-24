/* AmDevice.cpp
 */

#define _BUILDING_AmKernel 1

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <interface/Bitmap.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmDevice.h"
#include "AmKernel/AmSysExCommand.h"

static const char*	MFG_STR					= "mfg";
static const char*	NAME_STR				= "name";
static const char*	SHORT_DESCRIPTION_STR	= "short_description";
static const char*	AUTHOR_STR				= "author";
static const char*	EMAIL_STR				= "email";
static const char*	INPUT_FILTER_KEY_STR	= "in_filter";
static const char*	CONTROL_COUNT_STR		= "control_count";
static const char*	CONTROL_STR				= "control";
static const char*	BANK_MSG_STR			= "bank_msg";
static const char*	SELECTION_STR			= "selection";
static const char*	ICON_20X20_STR			= "Icon 20x20";

/*************************************************************************
 * AM-DEVICE-I
 *************************************************************************/
BString AmDeviceI::MakeLabel(const BString& mfg, const BString& product)
{
	BString		label;
	label << mfg.String();
	if (label.Length() > 0 && product.Length() > 0) label << " ";
	label << product.String();
	return label;	
}

/*************************************************************************
 * AM-DEVICE
 *************************************************************************/
AmDevice::AmDevice(	const char* name, const char* author,
					const char* email)
	: mName(name), mAuthor(author), mEmail(email), mIsValid(true),
	  mControls(128), mSelectionEvent(NULL), mIcon(NULL), mReadOnly(false)
{
	InitControlNames(0, mControls.size());
}

AmDevice::AmDevice(const BMessage& config, bool readOnly, const char* filePath)
		: mIsValid(true), mSelectionEvent(NULL), mIcon(NULL),
		  mReadOnly(readOnly), mFilePath(filePath)
{
	ReadFrom(config);
}

AmDevice::AmDevice(const AmDevice& o)
		: mManufacturer(o.mManufacturer), mName(o.mName),
		  mShortDescription(o.mShortDescription), mAuthor(o.mAuthor),
		  mEmail(o.mEmail), mInputFilterKey(o.mInputFilterKey), mIsValid(true),
		  mSelectionEvent(NULL), mIcon(NULL), mReadOnly(false),
		  mFilePath(o.mFilePath)
{
	uint32		count = o.mBanks.size();
	SetBankCount(count);
	for (uint32 i = 0; i < count; i++) SetBank(i, o.mBanks[i]->Copy() );
	if (o.mSelectionEvent) mSelectionEvent = dynamic_cast<AmBankChange*>(o.mSelectionEvent->Copy() );
	
	/* Copy the controls.
	 */
	count = o.mControls.size();
	SetControlCount(count);
	for (uint32 i = 0; i < count; i++) SetControlName(i, o.mControls[i].String() );

	/* Copy the icon.
	 */
	if (o.mIcon) SetIcon(o.mIcon);

	for (uint32 k = 0; k < o.mSysExCommands.size(); k++)
		if (o.mSysExCommands[k]) {
			AmSysExCommand*		com = o.mSysExCommands[k]->Copy();
			if (com) mSysExCommands.push_back(com);
		}
}

AmDevice::~AmDevice()
{
	if (mSelectionEvent) mSelectionEvent->Delete();
	delete mIcon;
	for (uint32 k = 0; k < mSysExCommands.size(); k++)
		delete mSysExCommands[k];
	mSysExCommands.resize(0);
}

device_id AmDevice::Id() const
{
	return (void*)this;
}

BString AmDevice::Manufacturer() const
{
	return mManufacturer;
}

BString AmDevice::Product() const
{
	return mName;
}

const BBitmap* AmDevice::Icon(BPoint requestedSize) const
{
	return mIcon;
}

uint32 AmDevice::CountBanks() const
{
	return mBanks.size();
}

ArpCRef<AmBankI> AmDevice::Bank(uint32 number) const
{
	if( number < 0 || number >= mBanks.size() ) {
		return ArpCRef<AmBankI>();
	}
	
	return ArpCRef<AmBankI>(mBanks[number]);
}

ArpCRef<AmBankI> AmDevice::Bank(const AmEvent* bankChange) const
{
	const AmBankChange*	be = dynamic_cast<const AmBankChange*>(bankChange);
	if (!be) return NULL;
	for (uint32 k = 0; k < mBanks.size(); k++) {
		if (mBanks[k]->BankSelection() ) {
			if (be->Matches(mBanks[k]->BankSelection(), true) )
				return ArpCRef<AmBankI>(mBanks[k]);
		}
	}
	return NULL;
}

AmEvent* AmDevice::NewBankSelection() const
{
	if (!mSelectionEvent) return NULL;
	return mSelectionEvent->Copy();
}

uint32 AmDevice::CountControls() const
{
	return mControls.size();
}

BString AmDevice::ControlName(uint32 number, bool prependNumber) const
{
	if (number < 0 || number >= mControls.size() ) return BString();
	BString		name;
	if (prependNumber) name << number;
	if (mControls[number].Length() > 0) {
		if (prependNumber) name << " - ";
		name << mControls[number].String();
	}
	return name;
}

BString AmDevice::ControlName(ulong number, bool prependNumber) const
{
	if (number < 0 || number >= mControls.size() ) return BString();
	BString		name;
	if (prependNumber) name << number;
	if (mControls[number].Length() > 0) {
		if (prependNumber) name << " - ";
		name << mControls[number].String();
	}
	return name;
}

AmCommandType AmDevice::SysexCommandType() const
{
	if (mSysExCommands.size() < 1) return AM_COMMAND_NONE;
	return mSysExCommands[0]->Type();
}

status_t AmDevice::GetSysexCommandKey(uint32 index, BString& outKey) const
{
	if (index >= mSysExCommands.size()) return B_ERROR;
	return mSysExCommands[index]->GetKey(outKey);
}

status_t AmDevice::GetSysexCommandInfo( const AmEvent* event,
										uint32* outIndex, BString* outKey) const
{
	for (uint32 k = 0; k < mSysExCommands.size(); k++) {
		if (mSysExCommands[k]->Matches(event)) {
			if (outIndex) *outIndex = k;
			if (outKey) mSysExCommands[k]->GetKey(*outKey);
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t AmDevice::GetSysexCommandLabel(const AmEvent* event, BString& outLabel,
										bool key, bool value) const
{
	for (uint32 k = 0; k < mSysExCommands.size(); k++) {
		if (mSysExCommands[k]->Matches(event)) {
			return mSysExCommands[k]->GetLabel(event, outLabel, key, value);
		}
	}
	return B_ERROR;
}

AmEvent* AmDevice::NewSysexCommand(const BString& key) const
{
	for (uint32 k = 0; k < mSysExCommands.size(); k++) {
		if (mSysExCommands[k]->Matches(key))
			return mSysExCommands[k]->NewEvent();
	}
	return NULL;
}

status_t AmDevice::GetSysexCommandValue(const AmEvent* event,
										int32* outValue) const
{
	for (uint32 k = 0; k < mSysExCommands.size(); k++) {
		if (mSysExCommands[k]->Matches(event))
			return mSysExCommands[k]->GetValue(event, outValue);
	}
	return B_ERROR;
}

status_t AmDevice::SetSysexCommandValue(AmEvent* event,
										int32 value) const
{
	for (uint32 k = 0; k < mSysExCommands.size(); k++) {
		if (mSysExCommands[k]->Matches(event))
			return mSysExCommands[k]->SetValue(event, value);
	}
	return B_ERROR;
}

status_t AmDevice::TransformSysexCommand(AmEvent* event, uint32 index) const
{
	if (index >= mSysExCommands.size()) return B_ERROR;
	return mSysExCommands[index]->ImposeSysEx(event);
}

status_t AmDevice::ConformSysexCommand(AmEvent* event) const
{
	for (uint32 k = 0; k < mSysExCommands.size(); k++) {
		if (mSysExCommands[k]->Matches(event)) {
			mSysExCommands[k]->Conform(event);
			return B_OK;
		}
	}
	return B_ERROR;
}

BString AmDevice::Label() const
{
	return Product();
}

BString AmDevice::Key() const
{
	BString		key;
	if (mManufacturer.Length() > 0) key << mManufacturer;
	if (mName.Length() > 0) key << mName;
	return key;
}

BString AmDevice::ShortDescription() const
{
	return mShortDescription;
}

BString AmDevice::Author() const
{
	return mAuthor;
}

BString AmDevice::Email() const
{
	return mEmail;
}

BString AmDevice::InputFilterKey() const
{
	return mInputFilterKey;
}

bool AmDevice::IsValid() const
{
	return mIsValid;
}

void AmDevice::SetBankCount(uint32 size)
{
	mBanks.resize(size);
}

ArpRef<AmBank> AmDevice::Bank(uint32 number)
{
	if (number < 0 || number >= mBanks.size() ) {
		return ArpRef<AmBank>();
	}
	
	return mBanks[number];
}

void AmDevice::SetManufacturer(const char* mfg)
{
	mManufacturer = mfg;
}

void AmDevice::SetName(const char* name)
{
	mName = name;
}

void AmDevice::SetShortDescription(const char* s)
{
	mShortDescription = s;
}

void AmDevice::SetAuthor(const char* author)
{
	mAuthor = author;
}

void AmDevice::SetEmail(const char* email)
{
	mEmail = email;
}

void AmDevice::SetIsValid(bool isValid)
{
	mIsValid = isValid;
}

void AmDevice::SetControlCount(uint32 size)
{
	uint32 oldSize = mControls.size();
	mControls.resize(size);
	if (oldSize < size) InitControlNames(oldSize, size);
}

void AmDevice::SetControlName(uint32 number, const char* name)
{
	mControls[number] = name;
}

void AmDevice::SetBank(uint32 number, AmBank* bank)
{
	ArpASSERT(bank);
	mBanks[number] = bank;
	if (bank) bank->SetBankNumber(number);
}

status_t AmDevice::AddBank(AmBank* bank)
{
	ArpASSERT(bank);
	uint32			oldSize = mBanks.size();
	mBanks.push_back(bank);
	uint32			newSize = mBanks.size();
	if (newSize <= oldSize) return B_NO_MEMORY;
	bank->SetBankNumber(newSize - 1);
	/* Copy my default bank event into the new bank, but only
	 * if the new bank doesn't already have one that fits my pattern.
	 */
	if (!mSelectionEvent) bank->SetBankSelection(NULL);
	else if (!bank->BankSelection() || !bank->BankSelection()->Matches(mSelectionEvent) )
		bank->SetBankSelection(mSelectionEvent);
	return B_OK;
}

status_t AmDevice::RemoveBank(ArpRef<AmBank> bank)
{
	ArpASSERT(bank);
	for (uint32 k = 0; k < mBanks.size(); k++) {
		if (mBanks[k] == bank) {
			mBanks.erase(mBanks.begin() + k);
			for (uint32 j = 0; j < mBanks.size(); j++) {
				mBanks[j]->SetBankNumber(j);
			}
			return B_OK;
		}
	}
	return B_ERROR;
}

void AmDevice::SetBankSelection(const AmBankChange* bankEvent)
{
	if (mSelectionEvent) delete mSelectionEvent;
	mSelectionEvent = NULL;
	if (bankEvent) mSelectionEvent = dynamic_cast<AmBankChange*>(bankEvent->Copy() );

	for (uint32 k = 0; k < mBanks.size(); k++) {
		if (!mSelectionEvent)
			mBanks[k]->SetBankSelection(NULL);
		else if (!mBanks[k]->BankSelection() || !mBanks[k]->BankSelection()->Matches(mSelectionEvent) )
			mBanks[k]->SetBankSelection(mSelectionEvent);
	}
}

AmBankChange* AmDevice::BankSelection() const
{
	return mSelectionEvent;
}

status_t AmDevice::SetIcon(const BBitmap* bitmap)
{
	if (mIcon) {
		delete mIcon;
		mIcon = NULL;
	}
	/* Apparently the user is just clearing out the icon.
	 */
	if (!bitmap) return B_OK;
	if (bitmap->Bounds() == IconBounds() ) {
		mIcon = new BBitmap(bitmap);
	}
	if (!mIcon) return B_NO_MEMORY;
	return B_OK;
}

BRect AmDevice::IconBounds() const
{
	return BRect(0, 0, 19, 19);
}

BBitmap* AmDevice::Icon(BPoint requestedSize)
{
	return mIcon;
}

void AmDevice::SetInputFilterKey(const char* key)
{
	if (key) mInputFilterKey = key;
	else mInputFilterKey = "";
}

bool AmDevice::IsReadOnly() const
{
	return mReadOnly;
}

AmDevice* AmDevice::Copy() const
{
	return new AmDevice(*this);
}

BString AmDevice::LocalFileName() const
{
	BString		name;
	if (mManufacturer.Length() > 0) name << mManufacturer;
	if (mManufacturer.Length() > 0 && mName.Length() > 0) name << " ";
	if (mName.Length() > 0) name << mName;
	return convert_to_filename(name);
}

BString AmDevice::LocalFilePath() const
{
	return mFilePath;
}

status_t AmDevice::WriteTo(BMessage& config) const
{
	if (mManufacturer.Length() > 0) config.AddString(MFG_STR, mManufacturer);
	if (mName.Length() > 0) config.AddString(NAME_STR, mName);
	if (mShortDescription.Length() > 0) config.AddString(SHORT_DESCRIPTION_STR, mShortDescription);
	if (mAuthor.Length() > 0) config.AddString(AUTHOR_STR, mAuthor);
	if (mEmail.Length() > 0) config.AddString(EMAIL_STR, mEmail);
	if (mInputFilterKey.Length() > 0) config.AddString(INPUT_FILTER_KEY_STR, mInputFilterKey);
	/* Controls
	 */
	uint32		count = CountControls();
	config.AddInt32(CONTROL_COUNT_STR, count);
	for (uint32 k = 0; k < count; k++) {
		config.AddString(CONTROL_STR, mControls[k].String());
	}
	/* Banks
	 */
	count = CountBanks();
	for (uint32 k = 0; k < count; k++) {
		BMessage	bankMsg('bank');
		mBanks[k]->WriteTo(bankMsg);
		config.AddMessage(BANK_MSG_STR, &bankMsg);
	}
	/* Bank selection
	 */
	if (mSelectionEvent) {
		BMessage	flatEvent;
		if (mSelectionEvent->GetAsMessage(flatEvent) == B_OK)
			config.AddMessage(SELECTION_STR, &flatEvent);
	}
	/* Icon
	 */
	if (mIcon) {
		BMessage	iconMsg;
		if (mIcon->Archive(&iconMsg) == B_OK)
			config.AddMessage(ICON_20X20_STR, &iconMsg);
	}
	return B_OK;
}

status_t AmDevice::ReadFrom(const BMessage& config)
{
	const char*		str;
	if (config.FindString(MFG_STR, &str) == B_OK) mManufacturer = str;
	if (config.FindString(NAME_STR, &str) == B_OK) mName = str;
	if (config.FindString(SHORT_DESCRIPTION_STR, &str) == B_OK) mShortDescription = str;
	if (config.FindString(AUTHOR_STR, &str) == B_OK) mAuthor = str;
	if (config.FindString(EMAIL_STR, &str) == B_OK) mEmail = str;
	if (config.FindString(INPUT_FILTER_KEY_STR, &str) == B_OK) mInputFilterKey = str;
	/* Controls
	 */
	mControls.resize(0);
	int32			controlCount;
	if (config.FindInt32(CONTROL_COUNT_STR, &controlCount) != B_OK) controlCount = 128;
	mControls.resize(controlCount);
	for (uint32 k = 0; config.FindString(CONTROL_STR, k, &str) == B_OK; k++)
		mControls[k] = str;
	/* Banks
	 */
	mBanks.resize(0);
	BMessage		bankMsg;
	for (uint32 k = 0; config.FindMessage(BANK_MSG_STR, k, &bankMsg) == B_OK; k++) {
		ArpRef<AmBank>		bank = new AmBank(bankMsg);
		if (bank) {
			mBanks.push_back(bank);
			bank->SetBankNumber(k);
		}
		bankMsg.MakeEmpty();
	}
	/* Bank selection
	 */
	if (mSelectionEvent) SetBankSelection(NULL);
	BMessage		flatEvent;
	if (config.FindMessage(SELECTION_STR, &flatEvent) == B_OK) {
		AmBankChange*	be = dynamic_cast<AmBankChange*>( am_get_as_event(flatEvent) );
		if (be) SetBankSelection(be);
	}
	/* Icon
	 */
	delete mIcon;
	mIcon = NULL;
	BMessage		iconMsg;
	if (config.FindMessage(ICON_20X20_STR, &iconMsg) == B_OK) {
		mIcon = dynamic_cast<BBitmap*>( BBitmap::Instantiate(&iconMsg) );
	}
	/* SysEx commands
	 */
#if 1
	BMessage		sysexcMsg;
	for (uint32 k = 0; config.FindMessage("syxc", k, &sysexcMsg) == B_OK; k++) {
		ReadSysExCommand(sysexcMsg);
		sysexcMsg.MakeEmpty();
	}
#endif	
	return B_OK;
}

static status_t sc_get_syx(const BMessage& msg, std::vector<AmSystemExclusive*>& sysexVec)
{
	uint8*						data;
	ssize_t						size;
	for (uint32 k = 0; msg.FindData("syx", B_INT8_TYPE, k, (const void**)&data, &size) == B_OK; k++) {
		AmSystemExclusive*			syx = new AmSystemExclusive(data, size, 0);
		if (!syx) return B_NO_MEMORY;
		sysexVec.push_back(syx);
	}
	if (sysexVec.size() < 1) return B_ERROR;
	return B_OK;
}

static status_t sc_get_cv(const BMessage& msg, const char* name, _AmCommandValue& cv)
{
	BMessage		cvMsg;
	status_t		err;
	if ((err = msg.FindMessage(name, &cvMsg)) != B_OK) return err;
	int32			i32;
	if ((err = cvMsg.FindInt32("i", &i32)) != B_OK) return err;
	cv.index = i32;
	if ((err = cvMsg.FindInt32("s", &i32)) != B_OK) return err;
	cv.start = i32;
	if ((err = cvMsg.FindInt32("e", &i32)) != B_OK) return err;
	cv.end = i32;
	if ((err = cvMsg.FindInt32("mi", &i32)) != B_OK) return err;
	cv.min = i32;
	if ((err = cvMsg.FindInt32("ma", &i32)) != B_OK) return err;
	cv.max = i32;
	return B_OK;
}

static status_t sc_get_labels(const BMessage& msg, std::vector<BString>& labelVec)
{
	BString		str;
	for (uint32 k = 0; msg.FindString("vl", k, &str) == B_OK; k++) {
		labelVec.push_back(str);
	}
	return B_OK;
}

status_t AmDevice::ReadSysExCommand(const BMessage& msg)
{
	status_t						err;
	std::vector<AmSystemExclusive*>		sysexVec;
	if ((err = sc_get_syx(msg, sysexVec)) != B_OK) return err;
	int32							initValue;
	if ((err = msg.FindInt32("iv", &initValue)) != B_OK) return err;
	BString							key;
	if ((err = msg.FindString("key", &key)) != B_OK) return err;
	_AmCommandValue					deviceId, value, channelC;
	_AmCommandValue*				channel = NULL;
	if ((err = sc_get_cv(msg, "cv_did", deviceId)) != B_OK) return err;
	if ((err = sc_get_cv(msg, "cv_v", value)) != B_OK) return err;
	if (sc_get_cv(msg, "cv_c", channelC) == B_OK) channel = &channelC;
	std::vector<BString>					valueLabels;
	if ((err = sc_get_labels(msg, valueLabels)) != B_OK) return err;

	AmSysExCommand*			com = new AmSysExMultiCommand(sysexVec, initValue, key, deviceId, value, channel, &valueLabels);
	if (!com) {
		for (uint32 k = 0; k < sysexVec.size(); k++)
			if (sysexVec[k]) sysexVec[k]->Delete();
		return B_ERROR;
	}
	mSysExCommands.push_back(com);
	return B_OK;
	
}

void AmDevice::InitControlNames(uint32 from, uint32 to)
{
	for (uint32 i=from; i<to; i++) {
		BString& str = mControls[i];
		const char*		name = am_control_name(i);
		if (name) str = name;
		else str = "";
	}
}
