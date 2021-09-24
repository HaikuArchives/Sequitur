/* AmFilterI.cpp
*/

#define _BUILDING_AmKernel 1

#ifndef AMKERNEL_AMFILTERI_H
#include "AmPublic/AmFilterI.h"
#endif

#include "AmKernel/AmFilterRoster.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmMotionI.h"

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include "AmKernel/AmNode.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

#include <Bitmap.h>
#include <Screen.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

ArpMOD();

#define NOISY 0

const char* SZ_FILTER_NAME					= "seqf:name";
const char* SZ_FILTER_KEY					= "seqf:class_name";
const char* SZ_FILTER_MAX_CONNECTIONS		= "seqf:max_connections";
const char* SZ_FILTER_SHORT_DESCRIPTION		= "seqf:short_description";
const char* SZ_FILTER_LONG_DESCRIPTION_NAME	= "seqf:long_description_name";
const char* SZ_FILTER_LONG_DESCRIPTION		= "seqf:long_description";
const char* SZ_FILTER_AUTHOR				= "seqf:author";
const char* SZ_FILTER_EMAIL					= "seqf:email";
const char* SZ_FILTER_MAJOR_VERSION			= "seqf:major_version";
const char* SZ_FILTER_MINOR_VERSION			= "seqf:minor_version";
const char* SZ_FILTER_TYPE					= "seqf:type";
const char* SZ_FILTER_SUBTYPE				= "seqf:subtype";
const char* SZ_FILTER_ASSOC_TYPE			= "seqf:assoc_type";
const char* SZ_FILTER_IMAGE					= "seqf:image_20x20";
const char* SZ_FILTER_LABEL					= "seqf:label";
const char* SZ_FILTER_ARCHIVE				= "seq:filters";

static const char*		FLAGS_STR		= "filter:flags";

// -------------------- recolor_bitmap --------------------

typedef bool (*recolor_bitmap_func)(rgb_color* inout_color, void* data);

static status_t recolor_bitmap(BBitmap* bm, recolor_bitmap_func func, void* data)
{
	rgb_color color;
	
	switch( bm->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		{
			uint8* start = (uint8*)bm->Bits();
			uint8* end = start + bm->BitsLength();
			
			while( start < end ) {
				color.blue = start[0];
				color.green = start[1];
				color.red = start[2];
				color.alpha = start[3];
				if( (*func)(&color, data) ) {
					start[0] = color.blue;
					start[1] = color.green;
					start[2] = color.red;
					start[3] = color.alpha;
				}
				start += 4;
			}
		} break;
		
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		{
			uint8* start = (uint8*)bm->Bits();
			uint8* end = start + bm->BitsLength();
			
			while( start < end ) {
				color.blue = start[3];
				color.green = start[2];
				color.red = start[1];
				color.alpha = start[0];
				if( (*func)(&color, data) ) {
					start[3] = color.blue;
					start[2] = color.green;
					start[1] = color.red;
					start[0] = color.alpha;
				}
				start += 4;
			}
		} break;
		
		case B_RGB16:
		{
			uint16* start = (uint16*)bm->Bits();
			uint16* end = start + bm->BitsLength()/2;
			
			while( start < end ) {
				color.red = (((*start>>11)&0x1f)*0xff)/0x1f;
				color.green = (((*start>>5)&0x3f)*0xff)/0x3f;
				color.blue = ((*start&0x1f)*0xff)/0x1f;
				color.alpha = 255;
				if( (*func)(&color, data) ) {
					*start = (((uint16)color.red & 0xf8) << 8) |
								(((uint16)color.green & 0xfc) << 3) |
								((color.blue & 0xf8) >> 3) ;
				}
				start++;
			}
		} break;
		
		case B_RGB15:
		case B_RGBA15:
		{
			uint16* start = (uint16*)bm->Bits();
			uint16* end = start + bm->BitsLength()/2;
			
			while( start < end ) {
				color.red = (((*start>>10)&0x1f)*0xff)/0x1f;
				color.green = (((*start>>5)&0x1f)*0xff)/0x1f;
				color.blue = ((*start&0x1f)*0xff)/0x1f;
				color.alpha = *start&0x8000 ? 255 : 0;
				if( (*func)(&color, data) ) {
					*start = (((uint16)color.red & 0xf8) << 7) |
								(((uint16)color.green & 0xf8) << 2) |
								((color.blue & 0xf8) >> 3) |
								(color.alpha >= 128 ? 0x8000 : 0) ;
				}
				start++;
			}
		} break;
		
		case B_CMAP8:
		{
			uint8* start = (uint8*)bm->Bits();
			uint8* end = start + bm->BitsLength();
			
			BScreen s;
			const color_map* cm = system_colors();
			
			while( start < end ) {
				if( *start == B_TRANSPARENT_MAGIC_CMAP8 ) {
					color = B_TRANSPARENT_COLOR;
				} else {
					color = cm->color_list[*start];
				}
				if( (*func)(&color, data) ) {
					if( color.alpha < 128 ) {
						*start = B_TRANSPARENT_MAGIC_CMAP8;
					} else {
						*start = cm->index_map[		((color.red&0xF8)<<7)
												|	((color.green&0xF8)<<2)
												|	((color.blue&0xF8)>>3)
											];
					}
				}
				start++;
			}
		} break;
		
		default:
			TRESPASS();
			return B_BAD_VALUE;
	}
	
	return B_OK;
}

// -------------------- disable_bitmap --------------------

#if 0
static bool disable_bitmap_alpha_func(rgb_color* c, void* background)
{
	(void)background;
	
	if( c->alpha == 0 ) return false;
	
	//uint8 gray = ( c->red/50 + c->green/30 + c->blue/75 ) * 15;
	uint8 gray = ( c->red/3 + c->green/3 + c->blue/3 );
	c->red = c->blue = c->green = gray;
	c->alpha /= 2;
	
	return true;
}

static bool disable_bitmap_color_func(rgb_color* c, void* background)
{
	if( c->alpha == 0 ) return false;
	
	uint8 gray = c->red/6 + c->green/6 + c->blue/6;
	c->red = gray + ((rgb_color*)background)->red/2;
	c->green = gray + ((rgb_color*)background)->green/2;
	c->blue = gray + ((rgb_color*)background)->blue/2;
	
	return true;
}

static status_t disable_bitmap(BBitmap* bm, rgb_color background)
{
	if( background.alpha == 0 ) {
		return recolor_bitmap(bm, disable_bitmap_alpha_func, &background);
	}
	
	return recolor_bitmap(bm, disable_bitmap_color_func, &background);
}
#endif

// -------------------- tint_bitmap --------------------

static bool tint_bitmap_func(rgb_color* c, void* background)
{
	if( c->alpha == 0 ) return false;
	
	const uint8 mix = ((rgb_color*)background)->alpha;
	c->red = (uint8)( ( ((uint16)c->red)*(255-mix)
						+ ((uint16)((rgb_color*)background)->red)*(mix)
						) / 255 );
	c->green = (uint8)( ( ((uint16)c->green)*(255-mix)
						+ ((uint16)((rgb_color*)background)->green)*(mix)
						) / 255 );
	c->blue = (uint8)( ( ((uint16)c->blue)*(255-mix)
						+ ((uint16)((rgb_color*)background)->blue)*(mix)
						) / 255 );
	
	return true;
}

static status_t tint_bitmap(BBitmap* bm, rgb_color color)
{
	return recolor_bitmap(bm, tint_bitmap_func, &color);
}

// #pragma mark -

/* ----------------------------------------------------------------
   AmFilterAddOn Implementation
   ---------------------------------------------------------------- */

AmFilterAddOn::AmFilterAddOn(const void* cookie)
	: mCookie(cookie), mTint(B_TRANSPARENT_COLOR)
{
}

void AmFilterAddOn::AddReference(ArpBaseRef* owner) const
{
	if (mCookie) ((AmFilterAddOnHandle*)mCookie)->Open();
	else ArpRefable::AddReference(owner);
}

void AmFilterAddOn::RemReference(ArpBaseRef* owner) const
{
	if (mCookie) ((AmFilterAddOnHandle*)mCookie)->Close();
	else ArpRefable::RemReference(owner);
}

status_t AmFilterAddOn::GetLabel(	BString& outLabel, bool useName,
									BMessage* archivedFilter) const
{
	if (!useName) return B_ERROR;
	BString		n = Name();
	if (n.Length() < 1) return B_ERROR;
	outLabel << n.String();
	return B_OK;
}

int32 AmFilterAddOn::MaxConnections() const
{
	return 1;
}

void AmFilterAddOn::LongDescription(BString& name, BString& str) const
{
	BString			n = Name();
	if (n.Length() < 1) n = "<unnamed filter>";	
	name << n;
}

BString AmFilterAddOn::Email() const
{
	return "";
}

AmFilterAddOn::subtype AmFilterAddOn::Subtype() const
{
	return NO_SUBTYPE;
}

BString AmFilterAddOn::KeyForType(type inType) const
{
	if (inType == Type()) return Key();
	return NULL;
}
	
BBitmap* AmFilterAddOn::Image(BPoint requestedSize) const
{
	return NULL;
}

status_t AmFilterAddOn::GetArchiveTemplate(BMessage* into, uint32 flags) const
{
	status_t res = B_OK;

	BString		bstr;
	
	bstr = Name();
	if (bstr.Length() > 0) res = into->AddString(SZ_FILTER_NAME, bstr);
	if (res != B_OK) return res;
	
	bstr = Key();
	if (bstr.Length() > 0) res = into->AddString(SZ_FILTER_KEY, bstr);
	if (res != B_OK) return res;
	
	res = into->AddInt32(SZ_FILTER_MAX_CONNECTIONS, MaxConnections() );
	if (res != B_OK) return res;
	
	if (flags&AM_ARCHIVE_DESCRIPTION) {
		bstr = ShortDescription();
		if (bstr.Length() > 0) res = into->AddString(SZ_FILTER_SHORT_DESCRIPTION, bstr);
		if (res != B_OK) return res;
		BString		nameStr, longStr;
		LongDescription(nameStr, longStr);
		if (nameStr.Length() > 0) res = into->AddString(SZ_FILTER_LONG_DESCRIPTION_NAME, nameStr.String() );
		if (longStr.Length() > 0) res = into->AddString(SZ_FILTER_LONG_DESCRIPTION, longStr.String() );
		if (res != B_OK) return res;
	}
	
	bstr = Author();
	if (bstr.Length() > 0) res = into->AddString(SZ_FILTER_AUTHOR, bstr);
	if (res != B_OK) return res;
	int32		major, minor;
	GetVersion( &major, &minor );
	res = into->AddInt32( SZ_FILTER_MAJOR_VERSION, major );
	if (res != B_OK) return res;
	res = into->AddInt32( SZ_FILTER_MINOR_VERSION, minor );
	if (res != B_OK) return res;
	
	res = into->AddInt32(SZ_FILTER_TYPE, Type());
	if (res != B_OK) return res;
	
	{
		bool customAssoc = false;
		BString		key = Key();
		for (int32 t = THROUGH_FILTER; t <= DESTINATION_FILTER; t++) {
			BString		assocName = KeyForType((type)t);
			if ((type)t == Type()) {
				if (key.Length() < 1 || assocName.Length() < 1 ||
						strcmp(key.String(), assocName.String() ) != 0) {
					customAssoc = true;
				}
			} else {
				if (assocName.Length() > 0) {
					customAssoc = true;
				}
			}
		}
		
		if (customAssoc) {
			for (int32 t = THROUGH_FILTER; res == B_OK && t <= DESTINATION_FILTER; t++) {
				BString		assocName = KeyForType((type)t);
				res = into->AddString(SZ_FILTER_ASSOC_TYPE,
									  assocName.Length() > 0 ? assocName.String() : "");
			}
		}
	}
	
	if (flags&AM_ARCHIVE_IMAGE) {
		BBitmap* bm = Image(BPoint(20, 20));
		if (bm) {
			BMessage arch;
//	BeOS:	if (bm->Archive(&arch, false) == B_OK) { (saved image data)
			if (bm->Archive(&arch) == B_OK) {	// Haiku bug(?)
				res = into->AddMessage(SZ_FILTER_IMAGE, &arch);
			}
			delete bm;
		}
		if (res != B_OK) return res;
	}
	
	return B_OK;
}

float AmFilterAddOn::CheckInstantiation(const BMessage* config) const
{
	return MatchClassArchive(Key().String(), config);
}

float AmFilterAddOn::MatchClassArchive(const char* key,
									   const BMessage* archive)
{
	const char* archName;
	if (archive->FindString(SZ_FILTER_KEY, &archName) != B_OK) {
		if (archive->FindString("seq:filter_class", &archName) != B_OK) {
			archName = "";
		}
	}
	if (!key) key = "";
	
	// If the class names match, this should be pretty good.
	if (strcmp(key, archName) == 0) return .5;
	
	// Otherwise, this is not a match.
	return -1;
}

AmFilterAddOn::~AmFilterAddOn()
{
	AmGlobals().RemoveClockTarget(this);
}

void AmFilterAddOn::StartClock(AmTime /*interval*/)
{
	AmGlobals().AddClockTarget(this);
}

void AmFilterAddOn::StopClock()
{
	AmGlobals().RemoveClockTarget(this);
}

void AmFilterAddOn::Clock(AmTime /*time*/)
{
}

BBitmap* AmFilterAddOn::FinalImage(BPoint requestedSize) const
{
	return TintImage(Image(requestedSize), mTint);
}

void AmFilterAddOn::SetTint(rgb_color tint)
{
	mTint = tint;
}

rgb_color AmFilterAddOn::Tint() const
{
	return mTint;
}

BBitmap* AmFilterAddOn::TintImage(BBitmap* image, rgb_color tint)
{
	if (image != NULL && tint.alpha != 0) tint_bitmap(image, tint);
	return image;
}

void AmFilterAddOn::_ReservedAmFilterAddOn1() { }
void AmFilterAddOn::_ReservedAmFilterAddOn2() { }
void AmFilterAddOn::_ReservedAmFilterAddOn3() { }
void AmFilterAddOn::_ReservedAmFilterAddOn4() { }
void AmFilterAddOn::_ReservedAmFilterAddOn5() { }
void AmFilterAddOn::_ReservedAmFilterAddOn6() { }
void AmFilterAddOn::_ReservedAmFilterAddOn7() { }
void AmFilterAddOn::_ReservedAmFilterAddOn8() { }
void AmFilterAddOn::_ReservedAmFilterAddOn9() { }
void AmFilterAddOn::_ReservedAmFilterAddOn10() { }
void AmFilterAddOn::_ReservedAmFilterAddOn11() { }
void AmFilterAddOn::_ReservedAmFilterAddOn12() { }
void AmFilterAddOn::_ReservedAmFilterAddOn13() { }
void AmFilterAddOn::_ReservedAmFilterAddOn14() { }
void AmFilterAddOn::_ReservedAmFilterAddOn15() { }
void AmFilterAddOn::_ReservedAmFilterAddOn16() { }

// #pragma mark -

/* ----------------------------------------------------------------
   am_filter_params Implementation
   ---------------------------------------------------------------- */

am_filter_params::am_filter_params()
	: size(sizeof(am_filter_params)),
	  flags(0), performance_time(0),
	  cur_tempo(NULL), cur_signature(NULL),
	  cur_motion_list(NULL), motion_size(0)
{
}

am_filter_params::am_filter_params(const am_filter_params& o)
	: size(sizeof(am_filter_params)),
	  flags(o.flags), performance_time(o.performance_time),
	  cur_tempo(o.cur_tempo), cur_signature(o.cur_signature),
	  cur_motion_list(o.cur_motion_list), motion_size(o.motion_size)
{
}

am_filter_params::~am_filter_params()
{
}

am_filter_params& am_filter_params::operator=(const am_filter_params& o)
{
	if (this != &o) memcpy(this, &o, sizeof(o));
	return *this;
}

static AmMotionChange* copy_chain(AmNode* node, track_id tid)
{
	AmMotionChange*		curr = NULL;
	while (node) {
		AmMotionChange*	e = dynamic_cast<AmMotionChange*>(node->Event() );
		if (e && (e = dynamic_cast<AmMotionChange*>(e->Copy())) ) {
			e->RemoveEvent();
			e->SetTrackId(tid);
			if (curr) curr->AppendEvent(e);
			curr = e;
		}
		node = node->next;
	}
	if (!curr) return NULL;
	return dynamic_cast<AmMotionChange*>(curr->HeadEvent() );
}

status_t am_filter_params::AddMotionChanges(const AmSong* song)
{
	DeleteMotionChanges();
	uint32				trackCount = song->CountTracks();
	cur_motion_list = (AmMotionChange**)malloc(sizeof(AmMotionChange) * trackCount);
	if (!cur_motion_list) return B_NO_MEMORY;
	for (uint32 k = 0; k < trackCount; k++) {
		const AmTrack*		track = song->Track(k);
		if (track) {
			AmMotionChange*	mc = copy_chain(track->Motions().HeadNode(), track->Id() );
			if (mc) {
				cur_motion_list[motion_size] = mc;					
				motion_size++;
			}
		}
	}
	return B_OK;
}

status_t am_filter_params::DeleteMotionChanges()
{
	if (cur_motion_list) {
		for (int32 k = 0; k < motion_size; k++) {
			if (cur_motion_list[k] ) cur_motion_list[k]->DeleteChain();
		}
		delete cur_motion_list;
	}
	cur_motion_list = NULL;
	motion_size = 0;
	return B_OK;
}

AmMotionChange* am_filter_params::MotionChange(track_id trackId) const
{
	if (!cur_motion_list) return NULL;
	for (int32 k = 0; k < motion_size; k++) {
		if (cur_motion_list[k] && cur_motion_list[k]->TrackId() == trackId)
			return cur_motion_list[k];
	}
	return NULL;
}

/* ----------------------------------------------------------------
   am_tool_filter_params Implementation
   ---------------------------------------------------------------- */

am_tool_filter_params::am_tool_filter_params()
	: size(sizeof(am_tool_filter_params)),
	  flags(0),
	  orig_time(0), cur_time(0),
	  start_time(0), end_time(0),
	  orig_y_pixel(0), cur_y_pixel(0),
	  view_type(AmEvent::NO_TYPE),
	  orig_y_value(0), cur_y_value(0),
	  track_context(0),
	  grid_multiplier(1), grid_value(PPQN), grid_divider(2)
{
}

am_tool_filter_params::am_tool_filter_params(const am_tool_filter_params& o)
	: size(sizeof(am_tool_filter_params)),
	  flags(o.flags),
	  orig_time(o.orig_time), cur_time(o.cur_time),
	  start_time(o.start_time), end_time(o.end_time),
	  orig_y_pixel(o.orig_y_pixel), cur_y_pixel(o.cur_y_pixel),
	  view_type(o.view_type),
	  orig_y_value(o.orig_y_value), cur_y_value(o.cur_y_value),
	  track_context(o.track_context),
	  grid_multiplier(o.grid_multiplier), grid_value(o.grid_value),
	  grid_divider(o.grid_divider)
{
}

am_tool_filter_params::~am_tool_filter_params()
{
}

am_tool_filter_params& am_tool_filter_params::operator=(const am_tool_filter_params& o)
{
	if (this != &o) memcpy(this, &o, sizeof(o));
	return *this;
}

/* ----------------------------------------------------------------
   am_realtime_filter_params Implementation
   ---------------------------------------------------------------- */

am_realtime_filter_params::am_realtime_filter_params()
	: size(sizeof(am_realtime_filter_params))
{
}

am_realtime_filter_params::am_realtime_filter_params(const am_realtime_filter_params& o)
	: size(sizeof(am_realtime_filter_params))
{
}

am_realtime_filter_params::~am_realtime_filter_params()
{
}

am_realtime_filter_params& am_realtime_filter_params::operator=(const am_realtime_filter_params& o)
{
	if (this != &o) memcpy(this, &o, sizeof(o));
	return *this;
}

// #pragma mark -

/* ----------------------------------------------------------------
   AmFilterI Implementation
   ---------------------------------------------------------------- */

AmFilterI::AmFilterI(AmFilterAddOn* addon)
	: mAddOn(addon), mFlags(0)
{
}

filter_id AmFilterI::Id() const
{
	return (void*)this;
}

BString AmFilterI::Name() const
{
	AmFilterAddOn*	addon = AddOn();
	if (!addon) return BString();
	return addon->Name();
}

bool AmFilterI::HasLabel() const
{
	return mLabel.Length() > 0;
}

BString AmFilterI::Label() const
{
	if (mLabel.Length() > 0) {
		return mLabel;
	}
	return Name();
}

AmFilterAddOn* AmFilterI::AddOn() const
{
	return mAddOn;
}

uint32 AmFilterI::Flags() const
{
	return mFlags;
}

void AmFilterI::SetFlag(uint32 flag, bool state)
{
	if (state) mFlags |= flag;
	else mFlags &= ~flag;
}

AmTime AmFilterI::LookaheadTime() const
{
	return 0;
}

status_t AmFilterI::GetConfiguration(BMessage* values) const
{
	status_t	err = values->AddString(SZ_FILTER_LABEL, mLabel);
	if (err != B_OK) return err;
	return values->AddInt32(FLAGS_STR, mFlags);
}

status_t AmFilterI::PutConfiguration(const BMessage* values)
{
	const char*	s;
	int32		i;
	if (values->FindString(SZ_FILTER_LABEL, &s) == B_OK) mLabel = s;
	if (values->FindInt32(FLAGS_STR, &i) == B_OK) mFlags = i;
	return B_OK;
}

status_t AmFilterI::Configure(ArpVectorI<BView*>& )
{
	return B_OK;
}

status_t AmFilterI::GetProperties(BMessage* properties) const
{
	return B_OK;
}

status_t AmFilterI::Archive(BMessage* into, uint32 flags) const
{
	if (!mAddOn) return B_NO_INIT;
	
	status_t res = mAddOn->AmFilterAddOn::GetArchiveTemplate(into, flags);
	if (res != B_OK) return res;
	
	return GetConfiguration(into);
}

status_t AmFilterI::GetNextConfiguration(BMessage*) const
{
	return B_ERROR;
}

#if 0
	/* It might be convenient for a window to display a miniature
	 * control to quickly access some of a filter's functionality.
	 * Filters that want to take part in this can answer a BView with
	 * the relevant controls.
 	 * THIS SHOULD BE OBSOLETE NOW.
	 */
	virtual BView*		NewEditView(BPoint requestedSize) const;


BView* AmFilterI::NewEditView(BPoint requestedSize) const
{
	return 0;
}
#endif

status_t AmFilterI::GetToolTipText(BString* out) const
{
	AmFilterAddOn* addon = AddOn();
	if (mLabel.Length() > 0) {
		*out = mLabel.String();
		if (addon) {
			*out << " (" << addon->Name() << ")";
		}
		return B_OK;
	}
	if (addon) {
		*out = addon->Name();
		return B_OK;
	}
	return B_ERROR;
}

BBitmap* AmFilterI::Image(BPoint requestedSize) const
{
	AmFilterAddOn	*addon = AddOn();
	if( !addon ) return 0;
	return addon->FinalImage(requestedSize);
}

void AmFilterI::MouseAction(BRect /*frame*/, BPoint /*where*/,
							uint32 /*buttons*/, bool /*released*/)
{
}

ArpCRef<AmDeviceI> AmFilterI::Device() const
{
	// Default is no device.
	return ArpCRef<AmDeviceI>(0);
}

void AmFilterI::DeviceChanged()
{
}

int32 AmFilterI::HintChannel() const
{
	return -1;
}

bigtime_t AmFilterI::Latency() const
{
	return 0;
}

AmEvent* AmFilterI::StartSection(AmTime /*firstTime*/, AmTime /*lastTime*/,
								 const am_filter_params* /*params*/)
{
	return NULL;
}

AmEvent* AmFilterI::FinishSection(AmTime /*firstTime*/, AmTime /*lastTime*/,
								  const am_filter_params* /*params*/)
{
	return NULL;
}

void AmFilterI::Start(uint32 context)
{
}

void AmFilterI::Stop(uint32 context)
{
}

AmEvent* AmFilterI::HandleToolEvent(AmEvent* event,
									const am_filter_params* params,
									const am_tool_filter_params* toolParams)
{
	return HandleEvent(event, params);
}

AmEvent* AmFilterI::HandleRealtimeEvent(AmEvent* event,
										const am_filter_params* params,
										const am_realtime_filter_params* realtimeParams)
{
	return HandleEvent(event, params);
}

AmEvent* AmFilterI::HandleBatchEvents(	AmEvent* event,
										const am_filter_params* params,
										const AmEvent* /*lookahead*/)
{
	AmEvent* result = NULL;
	while (event) {
		AmEvent* next = event->RemoveEvent();
		AmEvent* filtered = HandleEvent(event, params);
		if (filtered) result = result->MergeList(filtered);
		event = next;
	}
	return result ? result->HeadEvent() : NULL;
}

AmEvent* AmFilterI::HandleBatchToolEvents(	AmEvent* event,
											const am_filter_params* params,
											const am_tool_filter_params* /*toolParams*/,
											const AmEvent* lookahead)
{
	return HandleBatchEvents(event, params, lookahead);
	// I think the above is a better default implementation.
	// (And it is currently what ArpEatDuplicates depends on.)
	#if 0
	AmEvent* result = NULL;
	while (event) {
		AmEvent* next = event->RemoveEvent();
		AmEvent* filtered = HandleToolEvent(event, params, toolParams);
		if (filtered) result = result->MergeList(filtered);
		event = next;
	}
	return result ? result->HeadEvent() : NULL;
	#endif
}

AmEvent* AmFilterI::HandleBatchRealtimeEvents(	AmEvent* event,
												const am_filter_params* params,
												const am_realtime_filter_params* /*realtimeParams*/,
												const AmEvent* lookahead)
{
	return HandleBatchEvents(event, params, lookahead);
	// I think the above is a better default implementation.
	// (And it is currently what ArpEatDuplicates depends on.)
	#if 0
	AmEvent* result = NULL;
	while (event) {
		AmEvent* next = event->RemoveEvent();
		AmEvent* filtered = HandleRealtimeEvent(event, params, realtimeParams);
		if (filtered) result = result->MergeList(filtered);
		event = next;
	}
	return result ? result->HeadEvent() : NULL;
	#endif
}

AmEvent* AmFilterI::OscPulse(	AmTime start, AmTime end,
								const am_filter_params* params)
{
	return 0;
}

void AmFilterI::Ready(const am_filter_params*)
{
}

AmEvent* AmFilterI::Flush(const am_filter_params*)
{
	return NULL;
}

void AmFilterI::ConfigWindowOpened()
{
}

void AmFilterI::ConfigWindowClosed()
{
}

void AmFilterI::SetLabel(const char* label)
{
	mLabel = label;
}

void AmFilterI::_ReservedAmFilterI1() { }
void AmFilterI::_ReservedAmFilterI2() { }
void AmFilterI::_ReservedAmFilterI3() { }
void AmFilterI::_ReservedAmFilterI4() { }
void AmFilterI::_ReservedAmFilterI5() { }
void AmFilterI::_ReservedAmFilterI6() { }
void AmFilterI::_ReservedAmFilterI7() { }
void AmFilterI::_ReservedAmFilterI8() { }
void AmFilterI::_ReservedAmFilterI9() { }
void AmFilterI::_ReservedAmFilterI10() { }
void AmFilterI::_ReservedAmFilterI11() { }
void AmFilterI::_ReservedAmFilterI12() { }
void AmFilterI::_ReservedAmFilterI13() { }
void AmFilterI::_ReservedAmFilterI14() { }
void AmFilterI::_ReservedAmFilterI15() { }
void AmFilterI::_ReservedAmFilterI16() { }

// #pragma mark -

/*****************************************************************************
 * Filter utilities
 *****************************************************************************/
status_t am_get_measure(	AmTime time, const AmSignature* sigList,
							AmSignature& outMeasure,
							const AmSignature** outSignature)
{
	ArpASSERT(sigList);
	if (!sigList) return B_ERROR;
	sigList = dynamic_cast<const AmSignature*>(sigList->HeadEvent() );
	if (!sigList) return B_ERROR;
	ArpASSERT(sigList->StartTime() == 0);

	const AmSignature*	nextSig = dynamic_cast<const AmSignature*>(sigList->NextEvent() );
	AmSignature			currentSig(*sigList);
	currentSig.SetPrevEvent(NULL);
	currentSig.SetNextEvent(NULL);
	AmTime				sigLength = currentSig.Duration();
	if (outSignature) *outSignature = sigList;
	
	while (currentSig.EndTime() < time) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if (nextSig && (currentSig.Measure() == nextSig->Measure() ) ) {
			currentSig.Set(*nextSig);
			if (outSignature) *outSignature = nextSig;
			sigLength = currentSig.Duration();
			nextSig = dynamic_cast<const AmSignature*>(nextSig->NextEvent() );
		}
	}
	/* Technically this check isn't necessary but I'm a safe guy.
	 */
	if (time >= currentSig.StartTime() && time <= currentSig.EndTime() ) {
		outMeasure.Set(currentSig);
		return B_OK;
	}
	return B_ERROR;
}

static void inc_motion(AmMotionChange** curMotionEvent, AmMotionChange** nextMot,
						int32 measure,
						uint32* count, int32* motionMeasureOffset)
{
	if (*nextMot && (*nextMot)->Measure() == measure) {
		*curMotionEvent = *nextMot;
		*motionMeasureOffset = 0;
		*count = (*curMotionEvent)->CountMeasures();
		if ( (*curMotionEvent)->NextEvent() )
			*nextMot = dynamic_cast<AmMotionChange*>( (*curMotionEvent)->NextEvent() );
	} else if (*count > 0) {
		*motionMeasureOffset = *motionMeasureOffset + 1;
		if (*motionMeasureOffset >= int32(*count) ) *motionMeasureOffset = 0;
	}
}

static void seek_motion(AmTime time, AmMotionChange** curMotionEvent,
						int32* motionMeasureOffset, AmSignature& measure)
{
	ArpASSERT(time > measure.EndTime() );
	uint32				count = (*curMotionEvent)->CountMeasures();
	AmSignature*		nextSig = NULL;
	AmMotionChange*		nextMot = NULL;
	if (measure.NextEvent() ) nextSig = dynamic_cast<AmSignature*>(measure.NextEvent() );
	if ( (*curMotionEvent)->NextEvent() ) nextMot = dynamic_cast<AmMotionChange*>( (*curMotionEvent)->NextEvent() );
		
	while (time > measure.EndTime() ) {
		measure.Set(measure.StartTime() + measure.Duration(),
					measure.Measure() + 1);
		if (nextSig && nextSig->Measure() == measure.Measure() ) {
			measure.Set(*nextSig);
			if (nextSig->NextEvent() ) nextSig = dynamic_cast<AmSignature*>(nextSig->NextEvent() );
			else nextSig = NULL;
		}
		inc_motion(curMotionEvent, &nextMot, measure.Measure(), &count, motionMeasureOffset);
	}
}

status_t am_motion_hit_at_time(	AmTime time, AmMotionChange** curMotionEvent,
								int32* motionMeasureOffset,
								AmSignature& measure,
								float* outHitY)
{
	ArpASSERT(curMotionEvent && *curMotionEvent);
	if (time < measure.StartTime() ) return B_ERROR;
	if (time > measure.EndTime() ) seek_motion(time, curMotionEvent, motionMeasureOffset, measure);
	if (measure.Measure() < (*curMotionEvent)->Measure() ) return B_ERROR;
	if (time >= measure.StartTime() && time <= measure.EndTime() )
		return (*curMotionEvent)->GetHitY(	(double(time - measure.StartTime() ) / (double)measure.Duration()) + *motionMeasureOffset,
											outHitY);
	return B_ERROR;
}

status_t am_get_motion_y(	AmTime time, const AmMotionI* motion,
							const am_filter_params* params, float* outY)
{
	ArpVALIDATE(motion && params && params->cur_signature && outY, return B_ERROR);
	AmSignature					measure;
	status_t					err = am_get_measure(time, params->cur_signature, measure);
	if (err != B_OK) return err;

	uint32						measureCount = motion->CountMeasures();
	if (measureCount == 0) return B_ERROR;
	/* Get the correct measure in the motion that time is mapped to.
	 */
	int32						m = 0;
	if (measureCount > 1) m = (measure.Measure() -1) % measureCount;
	/* Translate my time into a floating value based on the measure.
	 */
	AmRange						range = measure.TimeRange();
	float						x = float((time - range.start) / float(range.end - range.start));
	BPoint						pt;
	float						end;
	AmMotionMode				mode = motion->EditingMode();
	for (uint32 k = 0; motion->GetHit(k, &pt, &end) == B_OK; k++) {
		if (m == int32(floor(pt.x)) || m == int32(floor(end))) {
			/* If the measure is greater than 0, I want only the floating portion
			 * of the x locations in the motion.
			 */
			float				start = pt.x;
			if (m > 0) start = pt.x - floor(pt.x);
			if (x < start) return B_BAD_VALUE;			
			if (x >= start) {
				float			startY = pt.y;
				float			endY = pt.y;
				if (mode == ENVELOPE_MODE && motion->GetHit(k + 1, &pt, 0) == B_OK) {
					end = pt.x;
					endY = pt.y;
				}
				if (m > 0) end = end - floor(end);
				if (x <= end) {
//printf("Time %lld (%f) %f to %f (y %f to %f)\n", time, x, start, end, startY, endY);
					if (mode == RHYTHM_MODE) {
						*outY = startY;
						return B_OK;
					} else if (mode == PROGRESSION_MODE) {
						*outY = startY;
						return B_OK;
					} else if (mode == ENVELOPE_MODE) {
						float		r1 = end - start, r2 = endY - startY;
						*outY = startY + (((x - start) * r2) / r1);
//printf("\tans %f\n", *outY);
						return B_OK;
					}
					ArpASSERT(false);
					return B_ERROR;
				}
			}
		}		
	}
	return B_ERROR;
}

status_t am_get_motion_hits(AmTime time, const AmMotionI* motion, const am_filter_params* params,
							BPoint* outHitPt, float* outEnd, BPoint* outNextPt, AmRange* outHitRange)
{
	ArpVALIDATE(motion && params && params->cur_signature, return B_ERROR);
	AmSignature		measure;
	status_t		err = am_get_measure(time, params->cur_signature, measure);
	if (err != B_OK) return err;
	return am_get_motion_hits(time, motion, measure, outHitPt, outEnd, outNextPt, outHitRange);
}

status_t am_get_motion_hits(AmTime time, const AmMotionI* motion,
							const AmSignature& measure,
							BPoint* outHitPt, float* outEnd, BPoint* outNextPt,
							AmRange* outHitRange)
{
	ArpVALIDATE(motion && outHitPt, return B_ERROR);
	uint32			measureCount = motion->CountMeasures();
	if (measureCount == 0) return B_ERROR;
	/* Get the correct measure in the motion that time is mapped to.
	 */
	int32			m = 0;
	if (measureCount > 1) m = (measure.Measure() -1) % measureCount;
	BPoint		pt;
	float		end;
	for (uint32 k = 0; motion->GetHit(k, &pt, &end) == B_OK; k++) {
		if (m == int32(floor(pt.x)) || m == int32(floor(end))) {
			/* If the measure is greater than 0, I want only the floating portion
			 * of the x locations in the motion.
			 */
			float	shiftedPt = pt.x;
			if (m > 0) shiftedPt = pt.x - floor(pt.x);
			AmTime	hitTime = AmTime(measure.StartTime() + (shiftedPt * measure.Duration()));
			if (time < hitTime) return B_BAD_VALUE;
			if (time >= hitTime) {
				float	shiftedEnd = end;
				if (m > 0) shiftedEnd = end - floor(pt.x);
				AmTime	hitEndTime = AmTime(measure.StartTime() + (shiftedEnd * measure.Duration()));
				if (time <= hitEndTime) {
					*outHitPt = pt;
					if (outEnd) *outEnd = end;
					if (outNextPt) {
						if (motion->GetHit(k + 1, &pt, &end) == B_OK) *outNextPt = pt;
						else *outNextPt = *outHitPt;
					}
					if (outHitRange) {
						outHitRange->start = hitTime;
						outHitRange->end = hitEndTime;
					}
					return B_OK;
				}
			}
		}		
	}
	return B_ERROR;
}

float am_x_amount(	AmTime centerTime, AmTime curTime,
					AmTime start, AmTime end)
{
	float 	amt = 0;
	if (centerTime < start) {
		if (curTime <= centerTime) amt = 1;
		else if (curTime >= end) amt = 0;
		else amt = fabs(1 - ( (double)(curTime - centerTime) / (double)(end - centerTime) ));
	} else if (centerTime <= end) {
		if (curTime <= start || curTime >= end) amt = 0;
		else if (curTime < centerTime) amt = (float)( (double)(curTime - start) / (double)(centerTime - start) );
		else if (curTime > centerTime) amt = fabs(1 - ( (double)(curTime - centerTime) / (double)(end - centerTime) ));
		else amt = 1;
	} else {
		if (curTime >= centerTime) amt = 1;
		else if (curTime <= start) amt = 0;
		else amt = (double)(curTime - start) / (double)(centerTime - start);
	}

	return amt;
}

float am_x_amount(const am_tool_filter_params* params, AmTime curTime)
{
	return am_x_amount(params->cur_time, curTime,
						params->start_time, params->end_time);
}
