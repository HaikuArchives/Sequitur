#include <stdio.h>
#include <string.h>
#include <be/midi2/MidiProducer.h>
#include <be/midi2/MidiRoster.h>
#include <ArpCore/ArpDebug.h>
#include <GlPublic/GlMidiEvent.h>

static const char*		P_STR		= "p";
static const char*		T_STR		= "t";
static const char*		C_STR		= "c";
static const char*		V1_STR		= "v1";
static const char*		V2_STR		= "v2";

/***************************************************************************
 * GL-MIDI-EVENT
 ***************************************************************************/
GlMidiEvent::GlMidiEvent()
		: portId(0), type(CONTROL_CHANGE), channel(0), value1(0), value2(0)
{
}

GlMidiEvent::GlMidiEvent(	int32 inPortId, int32 inType, int32 inChannel,
							int32 inV1, int32 inV2)
		: portId(inPortId), type(inType), channel(inChannel),
		  value1(inV1), value2(inV2)
{
}

GlMidiEvent::GlMidiEvent(const GlMidiEvent& o)
		: portId(o.portId), type(o.type), channel(o.channel),
		  value1(o.value1), value2(o.value2)
{
}

bool GlMidiEvent::operator==(const GlMidiEvent& o) const
{
	if (portId != o.portId) return false;
	if (type != o.type) return false;
	if (channel != o.channel) return false;
	if (type == CONTROL_CHANGE && value1 != o.value1) return false;
	return true;
}

bool GlMidiEvent::operator!=(const GlMidiEvent& o) const
{
	return !(*this == o);
}

GlMidiEvent& GlMidiEvent::operator=(const GlMidiEvent& o)
{
	portId = o.portId;
	type = o.type;
	channel = o.channel;
	value1 = o.value1;
	value2 = o.value2;
	return *this;
}

const BString16* GlMidiEvent::PortName() const
{
	BMidiProducer*			prod = BMidiRoster::FindProducer(portId);
	ArpVALIDATE(prod, return 0);
	return prod->Name();
}

void GlMidiEvent::GetValue(const GlMidiEvent& o)
{
	if (type == CONTROL_CHANGE) value2 = o.value2;
}

float GlMidiEvent::ScaledValue() const
{
	if (type == CONTROL_CHANGE) return float(value2) / 127.0f;
	return 0;
}

status_t GlMidiEvent::ReadFrom(const BMessage& msg, bool portName)
{
	status_t				err;
	if (portName) {
		BString16			name;
		if ((err = msg.FindString16(P_STR, &name)) != B_OK) return err;
		int32				id = 0;
		BMidiProducer*		prod;
		bool				found = false;
		while ((prod = BMidiRoster::NextProducer(&id)) != NULL) {
			if (prod->IsValid() && prod->Name() && name == prod->Name()) {
				portId = prod->ID();
				found = true;
				break;
			}
		}
		if (!found) return B_ERROR;
	} else {
		if ((err = msg.FindInt32(P_STR, &portId)) != B_OK) return err;
	}
	if ((err = msg.FindInt32(T_STR, &type)) != B_OK) return err;
	if ((err = msg.FindInt32(C_STR, &channel)) != B_OK) return err;
	if ((err = msg.FindInt32(V1_STR, &value1)) != B_OK) return err;
	if ((err = msg.FindInt32(V2_STR, &value2)) != B_OK) return err;
	return B_OK;
}

status_t GlMidiEvent::WriteTo(BMessage& msg, bool portName) const
{
	status_t				err;
	if (portName) {
		const BString16*	name = PortName();
		if (!name) return B_ERROR;
		if ((err = msg.AddString16(P_STR, name)) != B_OK) return err;
	} else {
		if ((err = msg.AddInt32(P_STR, portId)) != B_OK) return err;
	}
	if ((err = msg.AddInt32(T_STR, type)) != B_OK) return err;
	if ((err = msg.AddInt32(C_STR, channel)) != B_OK) return err;
	if ((err = msg.AddInt32(V1_STR, value1)) != B_OK) return err;
	if ((err = msg.AddInt32(V2_STR, value2)) != B_OK) return err;
	return B_OK;
}

status_t GlMidiEvent::ReadFakeFrom(const BMessage& msg, BString16* portName)
{
	status_t		err;
	if (portName) {
		if ((err = msg.FindString16(P_STR, portName)) != B_OK) return err;
	}
	if ((err = msg.FindInt32(T_STR, &type)) != B_OK) return err;
	if ((err = msg.FindInt32(C_STR, &channel)) != B_OK) return err;
	if ((err = msg.FindInt32(V1_STR, &value1)) != B_OK) return err;
	if ((err = msg.FindInt32(V2_STR, &value2)) != B_OK) return err;
	return B_OK;
}

status_t GlMidiEvent::WriteFakeTo(BMessage& msg, const BString16* portName) const
{
	ArpVALIDATE(portName, return B_ERROR);
	status_t			err;
	if ((err = msg.AddString16(P_STR, portName)) != B_OK) return err;
	if ((err = msg.AddInt32(T_STR, type)) != B_OK) return err;
	if ((err = msg.AddInt32(C_STR, channel)) != B_OK) return err;
	if ((err = msg.AddInt32(V1_STR, value1)) != B_OK) return err;
	if ((err = msg.AddInt32(V2_STR, value2)) != B_OK) return err;
	return B_OK;
}

void GlMidiEvent::Print() const
{
	if (type == CONTROL_CHANGE)
		printf("GlMidiEvent Control Change number %ld value %ld (%ld)\n", value1, value2, portId);
	else
		printf("GlMidiEvent unknown event (%ld)\n", portId);
}
