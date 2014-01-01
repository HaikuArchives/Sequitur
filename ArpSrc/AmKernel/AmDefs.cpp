/* AmDefs.cpp
 */
#define _BUILDING_AmKernel 1

#include "AmPublic/AmDefs.h"

#include <Message.h>
#include <be/storage/File.h>
#include <stdio.h>

status_t add_time(BMessage& msg, const char* name, AmTime timeVal)
{
	return msg.AddInt32(name, timeVal);
}

status_t find_time(const BMessage& msg, const char* name, int32 i, AmTime* timeVal)
{
	status_t res = msg.FindInt64(name, i, (int64*)timeVal);
	if (res != B_OK) {
		int32 val;
		res = msg.FindInt32(name, i, &val);
		if (res == B_OK) *timeVal = val;
	}
	return res;
}

status_t find_time(const BMessage& msg, const char* name, AmTime* timeVal)
{
	return find_time(msg, name, 0, timeVal);
}

status_t add_rel_time(BMessage& msg, const char* name, AmTime timeVal)
{
	return msg.AddDouble(name, double(timeVal)/PPQN);
}

status_t find_rel_time(const BMessage& msg, const char* name, int32 i, AmTime* timeVal)
{
	double v;
	status_t res = msg.FindDouble(name, i, &v);
	if (res == B_OK) {
		*timeVal = bigtime_t(v*PPQN + .5);
		return res;
	}
	return find_time(msg, name, i, timeVal);
}

status_t find_rel_time(const BMessage& msg, const char* name, AmTime* timeVal)
{
	return find_rel_time(msg, name, 0, timeVal);
}

static am_startup_status_func report_func = NULL;

void am_report_startup_status(const char* msg)
{
	if (report_func) (*report_func)(msg);
}

void am_set_startup_status_func(am_startup_status_func func)
{
	report_func = func;
}

/* ----------------------------------------------------------------
   am_studio_endpoint Implementation
   ---------------------------------------------------------------- */

am_studio_endpoint::am_studio_endpoint()
	: type(AM_PRODUCER_TYPE), id(0), channel(0)
{
}

am_studio_endpoint::am_studio_endpoint(const am_studio_endpoint& o)
	: name(o.name), type(o.type), id(o.id), channel(o.channel)
{
}

am_studio_endpoint::am_studio_endpoint(	const char* name, AmEndpointType type,
										int32 id, uchar channel)
	: name(name), type(type), id(id), channel(channel)
{
}

am_studio_endpoint::~am_studio_endpoint()
{
}

am_studio_endpoint& am_studio_endpoint::operator=(const am_studio_endpoint& o)
{
	if (this != &o) {
		name = o.name;
		type = o.type;
		id = o.id;
		channel = o.channel;
	}
	return *this;
}

bool am_studio_endpoint::operator==(const am_studio_endpoint& o) const
{
	return (type == o.type && channel == o.channel && name == o.name);
//	return (type == o.type && channel == o.channel && id == o.id && name == o.name);
}

const char *gControlNames[] = {
		"Bank Select MSB",	"Modulation",			"Breath",
		"",					"Foot Controller",		"Portamento Time",
		"Data Entry MSB",  	"Volume",				"Balance",
		"",					"Pan",					"Expression",
		"Effect 1",			"Effect 2",				"",
		"",					"General 1",			"General 2",
		"General 3",		"General 4",			"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"Bank Select LSB",
		"",					"",						"",
		"",					"",						"Data Entry LSB",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"Sustain Pedal",		"Portamento On/Off",
		"Sostenuto",		"Soft Pedal",			"Legato Footswitch",
		"Hold 2",			"Variation",			"Timbre",
		"Release Time",		"Attack Time",			"Brightness",
		"Sound 6",			"Sound 7",				"Sound 8",
		"Sound 9",			"Sound 10",				"General 5",
		"General 6",		"General 7",			"General 8",
		"Portamento",		"",						"",
		"",					"",						"",
		"",					"Effects 1 Depth",		"Effects 2 Depth",
		"Effects 3 Depth", 	"Effects 4 Depth",		"Effects 5 Depth",
		"Data Increment",	"Data Decrement",		"NRP LSB",
		"NRP MSB",			"RP LSB",				"RP MSB",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"",					"",						"",
		"Reserved",			"Reserved",				"Reserved",
		"Reserved",			"Reserved",				"Reserved",
		"Reserved",			"Reserved"
};

const char* am_control_name(uint8 controlNumber)
{
	if (controlNumber > 127) return NULL;
	return gControlNames[controlNumber];
}

static bool			gLogged = false;
static const char*	LOG_STR = "/boot/home/sequitur_log.txt";

void am_log_bstr(BString& str)
{
	BFile		file;

	if (!gLogged) {
		gLogged = true;
		file.SetTo(LOG_STR, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	} else {
		file.SetTo(LOG_STR, B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END);
	}
	
	if (file.InitCheck() != B_OK) return;
	file.Write(str.String(), str.Length());
}

void am_log_cstr(const char* str)
{
	BFile		file;

	if (!gLogged) {
		gLogged = true;
		file.SetTo(LOG_STR, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	} else {
		file.SetTo(LOG_STR, B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END);
	}
	
	if (file.InitCheck() != B_OK) return;
	file.Write(str, strlen(str));
}