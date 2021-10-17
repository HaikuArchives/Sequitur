/* AmSysExCommand.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmSysExCommand.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmPublic/AmEvents.h"

/***************************************************************************
 * AM-INT-CODEC-I
 * The interface for all codecs that read and write integers.
 ***************************************************************************/
class AmIntCodecI
{
public:
	virtual ~AmIntCodecI() { }

	virtual status_t ReadSysEx(	const AmSystemExclusive* sysex,
								size_t startByte, size_t stopByte,
								int32 *answer) = 0;
	virtual status_t WriteSysEx(AmSystemExclusive* sysex,
								size_t startByte, size_t stopByte,
								int32 value) = 0;
};

static AmIntCodecI* int_codec_for(uint32 bitNum, uint32 flags);

/***************************************************************************
 * AM-SYS-EX-COMMAND
 ***************************************************************************/
AmSysExCommand::AmSysExCommand(	const BString& key, const _AmCommandValue& deviceId,
								const _AmCommandValue& value, std::vector<BString>* valueLabels,
								uint32 inBitNum, uint32 inFlags)
		: mKey(key), mDeviceId(deviceId), mValue(value),
		  mBitNum(inBitNum), mFlags(inFlags)
{
	if (valueLabels) {
		for (uint32 k = 0; k < valueLabels->size(); k++)
			mValueLabels.push_back((*valueLabels)[k]);
	}
}

AmSysExCommand::AmSysExCommand(const AmSysExCommand& o)
		: mKey(o.mKey), mDeviceId(o.mDeviceId), mValue(o.mValue),
		  mBitNum(o.mBitNum), mFlags(o.mFlags)
{
	for (uint32 k = 0; k < mValueLabels.size(); k++) {
		mValueLabels.push_back(o.mValueLabels[k]);
	}
}

AmSysExCommand::~AmSysExCommand()
{
}

bool AmSysExCommand::Matches(const BString& key) const
{
	return mKey == key;
}

status_t AmSysExCommand::GetKey(BString& outKey) const
{
	outKey = mKey;
	return B_OK;
}

status_t AmSysExCommand::GetLabel(	const AmEvent* event,
									BString& outLabel,
									bool showKey, bool showValue) const
{
	if (showKey) {
		outLabel = mKey;
		if (outLabel.Length() < 1) outLabel = "Command";
	}
	if (showValue) {
		int32		value;
		if (GetValue(event, &value) == B_OK) {
			if (outLabel.Length() > 0) outLabel << ": ";
			int32	index = value - mValue.min;
			if (index >= 0 && uint32(index) < mValueLabels.size() && mValueLabels[index].Length() > 0)
				outLabel << mValueLabels[index];
			else
				outLabel << value;
		}
	}
	return B_OK;
}

status_t AmSysExCommand::GetValue(const AmEvent* event, int32* outValue) const
{
	ArpVALIDATE(event, return B_ERROR);
	const AmSystemExclusive*	sx = SysExAt(mValue.index, event);
	if (!sx) return B_ERROR;
	return GetSxValue(sx, outValue);
}

status_t AmSysExCommand::SetValue(AmEvent* event, int32 value) const
{
	ArpVALIDATE(event, return B_ERROR);
	AmSystemExclusive*			sx = SysExAt(mValue.index, event);
	if (!sx) return B_ERROR;
	return SetSxValue(sx, value);
}

status_t AmSysExCommand::GetSxValue(const AmSystemExclusive* sx, int32* outValue) const
{
	ArpVALIDATE(sx, return B_ERROR);
	AmIntCodecI*				codec = int_codec_for(mBitNum, mFlags);
	if (!codec) return B_ERROR;
	return codec->ReadSysEx(sx, mValue.start, mValue.end, outValue);
}

status_t AmSysExCommand::SetSxValue(AmSystemExclusive* sx, int32 value) const
{
	ArpVALIDATE(sx, return B_ERROR);
	AmIntCodecI*		codec = int_codec_for(mBitNum, mFlags);
	if (!codec) return B_ERROR;
	if (value < mValue.min) value = mValue.min;
	else if (value > mValue.max) value = mValue.max;
	if (!sx) return B_ERROR;
	return codec->WriteSysEx(sx, mValue.start, mValue.end, value);
}

bool AmSysExCommand::PrimMatches(	int32 index, const AmSystemExclusive* sx1,
									const AmSystemExclusive* sx2) const
{
	ArpVALIDATE(sx1 && sx2, return false);
	if (sx1->Length() != sx2->Length()) return false;
	size_t		len = sx1->Length();
	for (size_t k = 0; k < len; k++) {
		if (!MutableByte(index, k)) {
			if (sx1->Data()[k] != sx2->Data()[k]) return false;
		}
	}
	return true;
}

status_t AmSysExCommand::CopyInto(	const AmSystemExclusive* src,
									AmSystemExclusive* dest) const
{
	ArpVALIDATE(src && dest, return B_ERROR);
	const uint8*	data = src->Data();
	size_t			length = src->Length();
	int32			s, e;
	src->GetChannelBytes(&s, &e);
	dest->SetData(data, length);
	dest->SetChannelBytes(s, e);
	return B_OK;
}

bool AmSysExCommand::MutableByte(int32 index, uint32 b) const
{
	if (mDeviceId.MutableByte(index, b)) return true;
	if (mValue.MutableByte(index, b)) return true;
	return false;
}

void AmSysExCommand::Print() const
{
	printf("\tDevice Id: ");
	mDeviceId.Print();
	printf("\tValue: ");
	mValue.Print();
	printf("\tBit num: %ld\n", mBitNum);
	printf("\tFlags: %ld\n", mFlags);
}

/***************************************************************************
 * AM-SYS-EX-SINGLE-COMMAND
 ***************************************************************************/
AmSysExSingleCommand::AmSysExSingleCommand(	AmSystemExclusive* sysex, int32 initValue,
											const BString& key, const _AmCommandValue& deviceId,
											const _AmCommandValue& value, std::vector<BString>* valueLabels,
											uint32 bitNum, uint32 flags)
		: inherited(key, deviceId, value, valueLabels, bitNum, flags),
		  mSysEx(sysex)
{
	if (mSysEx) SetValue(mSysEx, initValue);
}

AmSysExSingleCommand::AmSysExSingleCommand(const AmSysExSingleCommand& o)
		: inherited(o), mSysEx(NULL)
{
	if (o.mSysEx) mSysEx = dynamic_cast<AmSystemExclusive*>(o.mSysEx->Copy());
}

AmSysExSingleCommand::~AmSysExSingleCommand()
{
	if (mSysEx) mSysEx->Delete();
}

AmSysExCommand* AmSysExSingleCommand::Copy() const
{
	return new AmSysExSingleCommand(*this);
}

AmCommandType AmSysExSingleCommand::Type() const
{
	return AM_COMMAND_SINGLE;
}

bool AmSysExSingleCommand::Matches(const AmEvent* event) const
{
	ArpVALIDATE(event && mSysEx, return false);
	if (event->Type() != event->SYSTEMEXCLUSIVE_TYPE) return false;
	const AmSystemExclusive*	sx = dynamic_cast<const AmSystemExclusive*>(event);
	if (!sx) return false;
	return PrimMatches(0, sx, mSysEx);
}

AmEvent* AmSysExSingleCommand::NewEvent() const
{
	if (!mSysEx) return NULL;
	return mSysEx->Copy();
}

status_t AmSysExSingleCommand::ImposeSysEx(AmEvent* event) const
{
	ArpVALIDATE(event && mSysEx, return B_ERROR);
	if (event->Type() != event->SYSTEMEXCLUSIVE_TYPE) return B_ERROR;
	AmSystemExclusive*	sx = dynamic_cast<AmSystemExclusive*>(event);
	if (!sx) return B_ERROR;
	status_t		err = CopyInto(mSysEx, sx);
	if (err != B_OK) return err;
	int32			value;
	if (GetValue(mSysEx, &value) == B_OK) SetValue(event, value);
	return B_OK;
}

status_t AmSysExSingleCommand::Conform(AmEvent* event) const
{
	ArpASSERT(false);
	return B_ERROR;
}

const AmSystemExclusive* AmSysExSingleCommand::SysExAt(int32 index, const AmEvent* event) const
{
	ArpASSERT(index == 0);
	if (event->Type() != event->SYSTEMEXCLUSIVE_TYPE) return NULL;
	return dynamic_cast<const AmSystemExclusive*>(event);
}

AmSystemExclusive* AmSysExSingleCommand::SysExAt(int32 index, AmEvent* event) const
{
	ArpASSERT(index == 0);
	if (event->Type() != event->SYSTEMEXCLUSIVE_TYPE) return NULL;
	return dynamic_cast<AmSystemExclusive*>(event);
}

void AmSysExSingleCommand::Print() const
{
	printf("AmSysExSingleCommand key %s\n", mKey.String());
	if (mSysEx) {
		printf("\t");
		mSysEx->Print();
	}
	inherited::Print();
}

/***************************************************************************
 * AM-SYS-EX-MULTI-COMMAND
 ***************************************************************************/
AmSysExMultiCommand::AmSysExMultiCommand(	std::vector<AmSystemExclusive*>& sysex, int32 initValue,
											const BString& key, const _AmCommandValue& deviceId,
											const _AmCommandValue& value,
											const _AmCommandValue* channel,
											std::vector<BString>* valueLabels,
											uint32 bitNum, uint32 flags)
		: inherited(key, deviceId, value, valueLabels, bitNum, flags)
{
	for (uint32 k = 0; k < sysex.size(); k++) {
		if (sysex[k]) mSysEx.push_back(sysex[k]);
	}
	if (mValue.index >= 0 && uint32(mValue.index) < mSysEx.size())
		SetSxValue(mSysEx[mValue.index], initValue);
	if (channel && channel->index >= 0 && channel->index < int32(mSysEx.size())) {
		if (mSysEx[channel->index]) mSysEx[channel->index]->SetChannelBytes(channel->start, channel->end);
	}
}

AmSysExMultiCommand::AmSysExMultiCommand(const AmSysExMultiCommand& o)
		: inherited(o)
{
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		if (o.mSysEx[k]) {
			AmSystemExclusive* sysex = dynamic_cast<AmSystemExclusive*>(o.mSysEx[k]->Copy());
			if (sysex) mSysEx.push_back(sysex);
		}
	}
}

AmSysExMultiCommand::~AmSysExMultiCommand()
{
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		if (mSysEx[k]) mSysEx[k]->Delete();
	}
	mSysEx.resize(0);
}

AmSysExCommand* AmSysExMultiCommand::Copy() const
{
	return new AmSysExMultiCommand(*this);
}

AmCommandType AmSysExMultiCommand::Type() const
{
	return AM_COMMAND_MULTI;
}

bool AmSysExMultiCommand::Matches(const AmEvent* event) const
{
	ArpVALIDATE(event && mSysEx.size() > 0, return false);
	if (event->Type() != event->PHRASE_TYPE) return false;
	const AmPhraseEvent*		pe = dynamic_cast<const AmPhraseEvent*>(event);
	if (!pe || !pe->Phrase()) return false;
	AmNode*		n = pe->Phrase()->HeadNode();
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		if (!n || !n->Event()) return false;
		if (n->Event()->Type() != event->SYSTEMEXCLUSIVE_TYPE) return false;
		const AmSystemExclusive*	sx = dynamic_cast<const AmSystemExclusive*>(n->Event());
		if (!sx) return false;
		if (!PrimMatches(k, sx, mSysEx[k])) return false;
		n = n->next;
	}
	/* Even if everything else checked, if there are more events in the
	 * phrase than I have sysex items, that's a fail.
	 */
	if (n) return false;
	return true;
}

AmEvent* AmSysExMultiCommand::NewEvent() const
{
	AmPhraseEvent*		pe = new AmInnerPhraseEvent();
	if (!pe || !pe->Phrase()) return NULL;
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		if (mSysEx[k]) {
			AmEvent*	e = mSysEx[k]->Copy();
				if (!e) {
				pe->Delete();
				return NULL;
			}
			pe->Add(e);
		}
	}
	if (pe->IsEmpty()) {
		pe->Delete();
		return NULL;
	}
	return pe;
}

status_t AmSysExMultiCommand::ImposeSysEx(AmEvent* event) const
{
	ArpVALIDATE(event, return B_ERROR);
	if (event->Type() != event->PHRASE_TYPE) return B_ERROR;
	AmPhraseEvent*		pe = dynamic_cast<AmPhraseEvent*>(event);
	if (!pe || !pe->Phrase()) return B_ERROR;
	/* We can't just delete the events in the incoming event -- if we
	 * do that, the time of the event gets set to 0 and this really
	 * messes things up.  Instead we copy each piece of sysex into
	 * its relevant destination, removing any excess or adding if
	 * we need to.
	 */
	if (!pe->Phrase()->IncludesOnly(event->SYSTEMEXCLUSIVE_TYPE)) return B_ERROR;
	AmNode*				n = pe->Phrase()->HeadNode();
	AmTime				time = pe->StartTime();
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		if (mSysEx[k]) {
			if (!n) {
				AmEvent*	copy = mSysEx[k]->Copy();
				if (copy) pe->Add(copy);
			} else {
				AmSystemExclusive*	dest = dynamic_cast<AmSystemExclusive*>(n->Event());
				if (!dest) return B_ERROR;
				CopyInto(mSysEx[k], dest);
				n = n->next;
			}
		}
	}
	/* If any nodes remain, clean them out.
	 */
	AmNode*		next = NULL;
	while (n) {
		next = n->next;
		AmEvent*	e = n->Event();
		pe->Remove(e);
		e->Delete();
		n = next;
	}
	if (mValue.index >= 0 && uint32(mValue.index) < mSysEx.size()) {
		int32			value;
		if (GetSxValue(mSysEx[mValue.index], &value) == B_OK)
			SetValue(event, value);
	}
	/* If all the nodes were cleaned out, the event was automatically moved
	 * to time 0.  Probably should just remove the thing, then add it back in.
	 */
	if (pe->StartTime() != time) pe->SetStartTime(time);	
	return B_OK;
}

status_t AmSysExMultiCommand::Conform(AmEvent* event) const
{
	ArpVALIDATE(event && mSysEx.size() > 0, return B_ERROR);
	if (event->Type() != event->PHRASE_TYPE) return B_ERROR;
	const AmPhraseEvent*		pe = dynamic_cast<const AmPhraseEvent*>(event);
	if (!pe || !pe->Phrase()) return B_ERROR;
	AmNode*		n = pe->Phrase()->HeadNode();
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		if (!n || !n->Event()) return B_ERROR;
		if (n->Event()->Type() != event->SYSTEMEXCLUSIVE_TYPE) return B_ERROR;
		AmSystemExclusive*		sx = dynamic_cast<AmSystemExclusive*>(n->Event());
		if (!sx) return B_ERROR;
		int32				s, e;
		mSysEx[k]->GetChannelBytes(&s, &e);
		sx->SetChannelBytes(s, e);
		n = n->next;
	}
	return B_OK;
}

const AmSystemExclusive* AmSysExMultiCommand::SysExAt(int32 index, const AmEvent* event) const
{
	ArpASSERT(index >= 0);
	if (event->Type() != event->PHRASE_TYPE) return NULL;
	const AmPhraseEvent*	pe = dynamic_cast<const AmPhraseEvent*>(event);
	if (!pe || !pe->Phrase()) return NULL;
	AmNode*					n = pe->Phrase()->HeadNode();
	int32					i = 0;
	while (n) {
		if (!n->Event() || n->Event()->Type() != event->SYSTEMEXCLUSIVE_TYPE) return NULL;
		if (i == index) {
			return dynamic_cast<const AmSystemExclusive*>(n->Event());
		}
		n = n->next;
		i++;
	}
	return NULL;
}

AmSystemExclusive* AmSysExMultiCommand::SysExAt(int32 index, AmEvent* event) const
{
	ArpASSERT(index >= 0);
	if (event->Type() != event->PHRASE_TYPE) return NULL;
	AmPhraseEvent*			pe = dynamic_cast<AmPhraseEvent*>(event);
	if (!pe || !pe->Phrase()) return NULL;
	AmNode*					n = pe->Phrase()->HeadNode();
	int32					i = 0;
	while (n) {
		if (!n->Event() || n->Event()->Type() != event->SYSTEMEXCLUSIVE_TYPE) return NULL;
		if (i == index) {
			const AmSystemExclusive*	sx = dynamic_cast<const AmSystemExclusive*>(n->Event());
			if (!sx) return NULL;
			return const_cast<AmSystemExclusive*>(sx);
		}
		n = n->next;
		i++;
	}
	return NULL;
}

void AmSysExMultiCommand::Print() const
{
	printf("AmSysExMultiCommand key %s\n", mKey.String());
	for (uint32 k = 0; k < mSysEx.size(); k++) {
		printf("\t");
		mSysEx[k]->Print();
	}
	inherited::Print();
}

/***************************************************************************
 * _AM-COMMAND-VALUE
 ***************************************************************************/
_AmCommandValue::_AmCommandValue()
		: index(0), start(0), end(0), min(0), max(0)
{
}

_AmCommandValue::_AmCommandValue(	int32 inIndex, uint32 inStart, uint32 inEnd,
									int32 inMin, int32 inMax)
		: index(inIndex), start(inStart), end(inEnd), min(inMin), max(inMax)
{
}
		
_AmCommandValue::_AmCommandValue(const _AmCommandValue& o)
		: index(o.index), start(o.start), end(o.end), min(o.min), max(o.max)
{
}

_AmCommandValue& _AmCommandValue::operator=(const _AmCommandValue& o)
{
	index = o.index;
	start = o.start;
	end = o.end;
	min = o.min;
	max = o.max;
	return *this;
}

bool _AmCommandValue::MutableByte(int32 inIndex, uint32 b) const
{
	if (index != inIndex) return false;
	if (b >= start && b <= end) return true;
	return false;
}

void _AmCommandValue::Print() const
{
	printf("index %ld, bytes %ld - %ld, values %ld - %ld\n", index, start, end, min, max);
}

/***************************************************************************
 * AM-INT-CODEC-I
 ***************************************************************************/
class Am7BitIntCodec : public AmIntCodecI
{
public:
	Am7BitIntCodec()				{ }
	virtual ~Am7BitIntCodec()		{ }

	virtual status_t ReadSysEx(	const AmSystemExclusive* sysex,
								size_t startByte, size_t stopByte,
								int32 *answer)
	{
		void*		data = (void*)sysex->Data();
		size_t		dataLength = sysex->Length();
		if ( (startByte < 0) || (dataLength < startByte) )
			return B_ERROR;
		int32	tmp = (int32)((uchar*)data)[startByte];
		if (tmp > 127) *answer = tmp - 256;
		else *answer = tmp;

		return B_OK;
	}
	
	virtual status_t WriteSysEx(AmSystemExclusive* sysex,
								size_t startByte, size_t stopByte,
								int32 value)
	{
		void*		data = (void*)sysex->Data();
		size_t		dataLength = sysex->Length();
		if ( (startByte < 0) || (dataLength < startByte) )
			return B_ERROR;

		int32		tmp;
		if (value < 0) tmp = value + 256;
		else tmp = value;
		((uchar*)data)[startByte] = (uchar)tmp;
		return B_OK;
	}
};
static Am7BitIntCodec		g7BitInt;

class Am14BitIntCodec2 : public AmIntCodecI
{
public:
	Am14BitIntCodec2()				{ }
	virtual ~Am14BitIntCodec2()		{ }

	virtual status_t ReadSysEx(	const AmSystemExclusive* sysex,
								size_t startByte, size_t stopByte,
								int32 *answer)
	{
		void*		data = (void*)sysex->Data();
		size_t		dataLength = sysex->Length();
		if ( (startByte < 0) || (dataLength < (startByte + 1)) )
			return B_ERROR;
	
		uchar	c1, c2;
		int32	temp;
		c1 = (uchar)((uchar*)data)[startByte];
		c2 = (uchar)((uchar*)data)[stopByte];
		temp = (c2 * 128) + c1;
		if (temp >= 8192) temp = temp - 16384;
		*answer = temp;
		return B_OK;
	}
	
	virtual status_t WriteSysEx(AmSystemExclusive* sysex,
								size_t startByte, size_t stopByte,
								int32 value)
	{
		void*		data = (void*)sysex->Data();
		size_t		dataLength = sysex->Length();
		if ( (startByte < 0) || (dataLength < (startByte + 1)) )
			return B_ERROR;

		uchar	c1, c2;
		int32	temp = value;
		if (temp < 0) temp += 16384;
		c2 = (uchar)floor(temp / 128);
		c1 = (uchar)(temp % 128);
		((uchar*)data)[startByte] = c1;
		((uchar*)data)[startByte + 1] = c2;
		return B_OK;
	}
};
static Am14BitIntCodec2		g14BitInt2;

static AmIntCodecI* int_codec_for(uint32 bitNum, uint32 flags)
{
	if (flags&ARP_TWOS_COMPLEMENT) {
		if (bitNum == 7) return &g7BitInt;
		else if (bitNum == 14) return &g14BitInt2;
	} else {
		if (bitNum == 7) return &g7BitInt;
	}
	return NULL;
}
