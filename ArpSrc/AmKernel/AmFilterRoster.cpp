/* AmFilterRoster.cpp
*/

#define _BUILDING_AmKernel 1

#include <map>

#ifndef AMKERNEL_AMFILTERROSTER_H
#include "AmKernel/AmFilterRoster.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _MEDIA_ROSTER_H
#include <media/MediaRoster.h>
#endif

#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmStdFilters.h"

#include <Autolock.h>
#include <Gehnaphore.h>
#include <List.h>
#include <Locker.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <MidiRoster.h>
#include <string.h>

const char* SZ_FILTER_ADD_ON = "seqf:add-on";

typedef map<BString, int32> dup_name_map;
static void get_dup_consumer_names(map<BString, int32>& dups);
static void get_dup_producer_names(map<BString, int32>& dups);

/***************************************************************************
 * AM-FILTER-ADD-ON-HANDLE
 ****************************************************************************/
AmFilterAddOnHandle::AmFilterAddOnHandle(const entry_ref* ref, const node_ref* node)
	: BAddOnHandle(ref, node),
	  mImage(NULL), mTint(B_TRANSPARENT_COLOR), mAddOn(NULL)
{
}

AmFilterAddOnHandle::AmFilterAddOnHandle(AmFilterAddOn* addOn)
	: BAddOnHandle(),
	  mImage(NULL), mTint(B_TRANSPARENT_COLOR), mAddOn(addOn)
{
	// This is VILE.
	addOn->mCookie = this;
}

BString AmFilterAddOnHandle::Name() const
{
	BString ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Name();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) ident->FindString(SZ_FILTER_NAME, &ret);
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

BString AmFilterAddOnHandle::Key() const
{
	BString ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Key();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) ident->FindString(SZ_FILTER_KEY, &ret);
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

int32 AmFilterAddOnHandle::MaxConnections() const
{
	int32 ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->MaxConnections();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) ident->FindInt32(SZ_FILTER_MAX_CONNECTIONS, &ret);
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

BString AmFilterAddOnHandle::ShortDescription() const
{
	BString ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->ShortDescription();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) ident->FindString(SZ_FILTER_SHORT_DESCRIPTION, &ret);
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

void AmFilterAddOnHandle::LongDescription(BString& name, BString& str) const
{
	Lock();
	if (mAddOn) {
		mAddOn->LongDescription(name, str);
		Unlock();
	} else {
		Unlock();
		const char*		n = NULL;
		const char*		s = NULL;
		const BMessage* ident = LockIdentifiers();
		if (ident) {
			ident->FindString(SZ_FILTER_LONG_DESCRIPTION_NAME, &n);
			ident->FindString(SZ_FILTER_LONG_DESCRIPTION, &s);
			if (n) name << n;
			if (s) str << s;
		}
		UnlockIdentifiers(ident);
	}
}

BString AmFilterAddOnHandle::Author() const
{
	BString ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Author();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) ident->FindString(SZ_FILTER_AUTHOR, &ret);
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

BString AmFilterAddOnHandle::Email() const
{
	BString ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Email();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) ident->FindString(SZ_FILTER_EMAIL, &ret);
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

void AmFilterAddOnHandle::GetVersion(int32* major, int32* minor) const
{
	*major = 0;
	*minor = 0;
	Lock();
	if (mAddOn) {
		mAddOn->GetVersion(major, minor);
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) {
			ident->FindInt32(SZ_FILTER_MAJOR_VERSION, major);
			ident->FindInt32(SZ_FILTER_MINOR_VERSION, minor);
		}
		UnlockIdentifiers(ident);
	}
}

AmFilterAddOn::type AmFilterAddOnHandle::Type() const
{
	AmFilterAddOn::type ret = AmFilterAddOn::THROUGH_FILTER;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Type();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		int32 i = ret;
		if (ident) {
			if (ident->FindInt32(SZ_FILTER_TYPE, &i) != B_OK) {
				i = AmFilterAddOn::THROUGH_FILTER;
			}
		}
		ret = (AmFilterAddOn::type)i;
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

AmFilterAddOn::subtype AmFilterAddOnHandle::Subtype() const
{
	AmFilterAddOn::subtype ret = AmFilterAddOn::NO_SUBTYPE;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Subtype();
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		int32 i = ret;
		if (ident) {
			if (ident->FindInt32(SZ_FILTER_SUBTYPE, &i) != B_OK) {
				i = AmFilterAddOn::NO_SUBTYPE;
			}
		}
		ret = (AmFilterAddOn::subtype)i;
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

BString AmFilterAddOnHandle::KeyForType(AmFilterAddOn::type inType) const
{
	BString ret;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->KeyForType(inType);
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		bool haveInfo = false;
		if (ident) {
			if (ident->FindString(SZ_FILTER_ASSOC_TYPE, (int32)inType, &ret) == B_OK) {
				haveInfo = true;
			}
		}
		UnlockIdentifiers(ident);
		
		if (!haveInfo) {
			if (inType == Type()) return ret = Key();
		}
	}
	
	return ret;
}

BBitmap* AmFilterAddOnHandle::RawImage(BPoint requestedSize) const
{
	BBitmap* ret = NULL;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->Image(requestedSize);
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) {
			BMessage bmArch;
			if (ident->FindMessage(SZ_FILTER_IMAGE, &bmArch) == B_OK) {
				ret = new BBitmap(&bmArch);
			}
		}
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

BBitmap* AmFilterAddOnHandle::Image(BPoint requestedSize) const
{
	return ArpMakeFilterBitmap(RawImage(requestedSize), requestedSize);
}

float AmFilterAddOnHandle::CheckInstantiation(const BMessage* config) const
{
	// First check if they are of the same class.
	BString myClass = Key();
	if (AmFilterAddOn::MatchClassArchive(myClass.String(), config) < 0) {
		return -1;
	}
	
	// Now get filter add-on and let it probe some more.
	float match = -1;
	const_cast<AmFilterAddOnHandle*>(this)->Open();
	if (mAddOn) match = mAddOn->CheckInstantiation(config);
	const_cast<AmFilterAddOnHandle*>(this)->Close();
	
	return match;
}

status_t AmFilterAddOnHandle::GetArchiveTemplate(BMessage* into) const
{
	status_t ret = B_ERROR;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->GetArchiveTemplate(into, AM_ARCHIVE_ALL);
		Unlock();
	} else {
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) {
			*into = *ident;
			ret = B_OK;
		}
		UnlockIdentifiers(ident);
	}
	
	return ret;
}

BBitmap* AmFilterAddOnHandle::FinalImage(BPoint requestedSize) const
{
	BBitmap* ret = NULL;
	
	Lock();
	if (mAddOn) {
		ret = mAddOn->FinalImage(requestedSize);
		Unlock();
	} else {
		rgb_color tint = mTint;
		Unlock();
		const BMessage* ident = LockIdentifiers();
		if (ident) {
			BMessage bmArch;
			if (ident->FindMessage(SZ_FILTER_IMAGE, &bmArch) == B_OK) {
				ret = new BBitmap(&bmArch);
			}
		}
		UnlockIdentifiers(ident);
		ret = AmFilterAddOn::TintImage(ret, mTint);
	}
	
	return ArpMakeFilterBitmap(ret, requestedSize);
}

void AmFilterAddOnHandle::SetTint(rgb_color tint)
{
	Lock();
	if (mAddOn) mAddOn->SetTint(tint);
	mTint = tint;
	Unlock();
}

rgb_color AmFilterAddOnHandle::Tint() const
{
	rgb_color tint;
	Lock();
	if (mAddOn) tint = mAddOn->Tint();
	else tint = mTint;
	Unlock();
	return tint;
}

ArpRef<AmFilterAddOn> AmFilterAddOnHandle::Instantiate()
{
	ArpRef<AmFilterAddOn> result;
	
	if (Open() >= B_OK) {
		result = mAddOn;
		Close();
	}
	
	return result;
}

bool AmFilterAddOnHandle::KeepLoaded() const
{
	return false;
}

bool AmFilterAddOnHandle::IsDynamic() const
{
	return false;
}

size_t AmFilterAddOnHandle::GetMemoryUsage() const
{
	return BAddOnHandle::GetMemoryUsage();
}

AmFilterAddOn* AmFilterAddOnHandle::AddOn() const
{
	return mAddOn;
}
	
AmFilterAddOnHandle::~AmFilterAddOnHandle()
{
	delete mAddOn;
	mAddOn = NULL;
	delete mImage;
	mImage = NULL;
}

void AmFilterAddOnHandle::ImageLoaded(image_id image)
{
	if (!IsStatic()) {
		ImageUnloading(image);
		
		void *factory;
		status_t err = get_image_symbol(image, "make_nth_filter",
										B_SYMBOL_TYPE_TEXT, &factory);
		if (err >= B_OK) {
			mAddOn = (*((make_nth_filter_type)factory))(0, image, this, 0);
			if (!mAddOn) err = B_ERROR;
		}
	}
}

status_t AmFilterAddOnHandle::LoadIdentifiers(BMessage* into, image_id from)
{
	if (!mAddOn) return B_NO_INIT;
	
	return mAddOn->GetArchiveTemplate(into, AM_ARCHIVE_ALL);
}

void AmFilterAddOnHandle::ImageUnloading(image_id image)
{
	if (!IsStatic()) {
		delete mAddOn;
		mAddOn = NULL;
	}
}

const char* AmFilterAddOnHandle::AttrBaseName() const
{
	return "arp:seq_filter";
}

// ---------------------------------------------------------------

class AmFilterRosterHandler : public BHandler
{
public:
	AmFilterRosterHandler(AmFilterRoster& roster)
		: BHandler("Filter Roster"), mRoster(roster)
	{
	}
	~AmFilterRosterHandler()
	{
	}

	virtual void MessageReceived(BMessage *message)
	{
		if (message->what == B_MIDI_EVENT) {
			
			int32 op, id;
			const char* type;
			if (message->FindInt32("be:op", &op) == B_OK &&
				message->FindInt32("be:id", &id) == B_OK &&
				message->FindString("be:type", &type) == B_OK) {
				
				
				BAutolock _l(mRoster.Locker());
				
				// First look for a filter with this ID.
				AmFilterAddOnHandle* handle = NULL;
				AmConsumerFilterAddOn* caddon = NULL;
				AmProducerFilterAddOn* paddon = NULL;
				for (int32 i=0; i<mRoster.CountAddOns(); i++) {
					handle = dynamic_cast<AmFilterAddOnHandle*>(mRoster.AddOnAt(i));
					if (!handle) continue;
					caddon = dynamic_cast<AmConsumerFilterAddOn*>(handle->AddOn());
					paddon = dynamic_cast<AmProducerFilterAddOn*>(handle->AddOn());
					if (caddon && caddon->ConsumerID() == id) {
						break;
					}
					if (paddon && paddon->ProducerID() == id) {
						break;
					}
				}
				
				if (op == B_MIDI_REGISTERED) {
					// Only add if the newly registered consumer
					// doesn't already exist in the roster.
					if (strcmp(type, "consumer") == 0 && (!caddon
							|| caddon->ConsumerID() != id) ) {
						BMidiConsumer* cons = BMidiRoster::FindConsumer(id);
						if (!cons) return;
						if (cons->IsLocal()) return;
						caddon = new AmConsumerFilterAddOn(NULL, cons);
						if (caddon) {
							printf("Installing consumer %s (#%ld)\n",
									cons->Name(), cons->ID());
							mRoster.InstallAddOn(new AmFilterAddOnHandle(caddon));
						}
					} else if (strcmp(type, "producer") == 0 && (!paddon
							|| paddon->ProducerID() != id) ) {
						BMidiProducer* prod = BMidiRoster::FindProducer(id);
						if (!prod) return;
						if (prod->IsLocal()) return;
						paddon = new AmProducerFilterAddOn( NULL, prod );
						if (paddon) {
							printf("Installing producer %s (#%ld)\n",
									prod->Name(), prod->ID());
							mRoster.InstallAddOn(new AmFilterAddOnHandle(paddon));
						}
					}
					
				} else if (op == B_MIDI_UNREGISTERED) {
					if (handle && caddon || paddon) {
						printf("Removing %s (#%ld)\n",
								caddon ? caddon->Name().String() : paddon->Name().String(),
								caddon ? caddon->ConsumerID() : paddon->ProducerID());
						mRoster.RemoveAddOn(handle);
					}
				}
			}
			return;
		}
		
		BHandler::MessageReceived(message);
	}
	
private:
	AmFilterRoster& mRoster;
};

// #pragma mark -

// ---------------------------------------------------------------

AmFilterRoster::AmFilterRoster(const char* name)
	: BAddOnManager(name), mHandler(NULL)
{
}

AmFilterRoster::~AmFilterRoster()
{
	if (mHandler) RemoveHandler(mHandler);
}

static int32 gFilterRosterCreated = 0;
static AmFilterRoster* gFilterRoster = NULL;

/*** Static Destructors are different in Haiku! ****
struct FilterRosterCleanup
{
	~FilterRosterCleanup()
	{
		delete gFilterRoster;
	}
};
static FilterRosterCleanup gFilterRosterCleanup;
************************************************/
// Use this instead:

void closeFilterRoster() {	// Hack for Haiku
	delete gFilterRoster;
}


AmFilterRoster* AmFilterRoster::Default()
{
	if (atomic_or(&gFilterRosterCreated, 1) == 0) {
		gFilterRoster = new AmFilterRoster("Default Filter Roster");
		
		gFilterRoster->AddSearchPath("%A/add-ons/Filters");
		gFilterRoster->AddDirectory(B_SYSTEM_ADDONS_DIRECTORY,"AngryRedPlanet/Sequitur/Filters");
		gFilterRoster->AddDirectory(B_USER_ADDONS_DIRECTORY,"AngryRedPlanet/Sequitur/Filters");
		
		// Here are our standard filter addons...
		AmFilterAddOn *addon;
		
		// Forcibly add in all existing consumers, so that when we
		// return we have everything as it currently exists.  This is
		// to take care of situations where, say, the file loader is the
		// first thing to open the roster and immediately starts trying
		// to instantiate consumer filters.
		BMidiConsumer* cons;
		/* Hack -- keep track of the producer / consumer names that are
		 * installed, and if a name gets installed more than one, increment
		 * a counter for it.  This is so users can run multiple instances of
		 * a softsynth and access each independently.
		 */
		dup_name_map	dups;
		get_dup_consumer_names(dups);
		int32 id = 0;
		while ((cons=BMidiRoster::NextConsumer(&id)) != NULL) {
			if (!cons->IsLocal()) {
				BString msg("Installing MIDI consumer ");
				msg += cons->Name();
				msg += "...";
				am_report_startup_status(msg.String());
				int32					index = 0;
				dup_name_map::iterator	i = dups.find(BString(cons->Name()));
				if (i != dups.end()) {
					index = i->second;
					i->second = i->second + 1;
				}
				addon = new AmConsumerFilterAddOn(NULL, cons, index);
				if (addon) {
					printf("Installing consumer %s (#%ld)\n", cons->Name(), cons->ID());
					gFilterRoster->InstallAddOn(new AmFilterAddOnHandle(addon));
#if 0
					BMessage	props;
					if( cons->GetProperties( &props ) == B_OK ) {
printf("****** CONSUMER HAS PROPERTIES:\n");
props.PrintToStream();
printf("****** END PROPERTIES\n");
					}
#endif
				}
			}
		}

		dups.erase(dups.begin(), dups.end());
		get_dup_producer_names(dups);
		id = 0;
		BMidiProducer* prod;
		while ((prod=BMidiRoster::NextProducer(&id)) != NULL) {
			if (!prod->IsLocal()) {
				BString msg("Installing MIDI producer ");
				msg += prod->Name();
				msg += "...";
				am_report_startup_status(msg.String());
				int32					index = 0;
				dup_name_map::iterator	i = dups.find(BString(prod->Name()));
				if (i != dups.end()) {
					index = i->second;
					i->second = i->second + 1;
				}
				addon = new AmProducerFilterAddOn(NULL, prod, index);
				if (addon) {
					printf("Installing producer%s (#%ld)\n", prod->Name(), prod->ID());
					gFilterRoster->InstallAddOn(new AmFilterAddOnHandle(addon));
				}
			}
		}
		/* Add in a filter for the BeOS MIDI synth if there's any
		 * audio output.
		 */
		status_t		err;
		am_report_startup_status("Installing MIDI Synth consumer...");
		BMediaRoster*	roster = BMediaRoster::Roster( &err );
		if( err == B_OK && roster ) {
			media_node	node;
			err = roster->GetAudioOutput( &node );
			if( err == B_OK ) {
				// Add in a filter for the BeOS MIDI synth.
				if ((addon = new AmConsumerFilterAddOn(NULL)) != NULL)
					gFilterRoster->InstallAddOn(new AmFilterAddOnHandle(addon));
//				node.ReleaseNode();
			}
		}
		/* Add the generic input/output filters, for tool and multi filter pipelines.
		 */
		if ((addon = new AmNullInputAddOn(NULL)) != NULL)
			gFilterRoster->InstallAddOn(new AmFilterAddOnHandle(addon));
		if ((addon = new AmNullOutputAddOn(NULL)) != NULL)
			gFilterRoster->InstallAddOn(new AmFilterAddOnHandle(addon));

		// Go!
		gFilterRoster->Run();
	} else {
		while (!gFilterRoster) sleep(20000);
	}
	return gFilterRoster;
}

void AmFilterRoster::ShutdownDefault(bool force_unload)
{
	if (gFilterRoster)
		gFilterRoster->Shutdown(force_unload);
}

status_t AmFilterRoster::Run()
{
	am_report_startup_status("Scanning filter add-ons...");
	status_t res = BAddOnManager::Run();
	if (res >= B_OK) {
		mHandler = new AmFilterRosterHandler(*this);
		if (mHandler) {
			AddHandler(mHandler);
			BMessenger m(mHandler);
			BMidiRoster::StartWatching(&m);
		}
	}
	
	return res;
}

void AmFilterRoster::InstallAddOn(BAddOnHandle* addon)
{
	BAddOnManager::InstallAddOn(addon);
	
	AmFilterAddOnHandle* h = dynamic_cast<AmFilterAddOnHandle*>(addon);
	if (!h) return;
	
	BBitmap* rawImage = h->Image(BPoint(20, 20));
	if (!rawImage) return;
	
	BAutolock _l(Locker());
	
	static const int32 NUM_TINTS = 10;

	static rgb_color sTints[NUM_TINTS];
	static int32 sHaveTints = 0;
	if (sHaveTints != 3) {
		atomic_or(&sHaveTints, 1);
		sTints[0] = B_TRANSPARENT_COLOR;
		sTints[1] = Prefs().Color( DUPLICATE_FILTER_1_C );
		sTints[2] = Prefs().Color( DUPLICATE_FILTER_2_C );
		sTints[3] = Prefs().Color( DUPLICATE_FILTER_3_C );
		sTints[4] = Prefs().Color( DUPLICATE_FILTER_4_C );
		sTints[5] = Prefs().Color( DUPLICATE_FILTER_5_C );
		sTints[6] = Prefs().Color( DUPLICATE_FILTER_6_C );
		sTints[7] = Prefs().Color( DUPLICATE_FILTER_7_C );
		sTints[8] = Prefs().Color( DUPLICATE_FILTER_8_C );
		sTints[9] = Prefs().Color( DUPLICATE_FILTER_9_C );
		atomic_or(&sHaveTints, 2);
	} else {
		while (sHaveTints != 3) sleep(50000);
	}
	
	bool foundTints[NUM_TINTS] = {
		false, false, false, false, false, false, false,
		false, false, false
	};
	
	BString key = h->Key();
	
	AmFilterAddOn::type assocType = h->Type();
	BString assocClass;
	BString assocName;
	switch (assocType) {
		case AmFilterAddOn::SOURCE_FILTER:
			assocType = AmFilterAddOn::DESTINATION_FILTER;
			assocClass = h->KeyForType(assocType);
			assocName = h->Name();
			break;
		case AmFilterAddOn::DESTINATION_FILTER:
			assocType = AmFilterAddOn::SOURCE_FILTER;
			assocClass = h->KeyForType(assocType);
			assocName = h->Name();
			break;
		default:
			break;
	}
	
	bool newTint = false;
	
	const int32 N = CountAddOns();
	int32 i;
	for (i=0; i<N; i++) {
		BAddOnHandle* look = AddOnAt(i);
		AmFilterAddOnHandle* lh = dynamic_cast<AmFilterAddOnHandle*>(look);
		if (lh) {
			if (assocClass.Length() > 0 && assocType == lh->Type() &&
					assocClass == lh->Key() && assocName == lh->Name()) {
				// Found a matching associated class for this one --
				// make their tints the same.
				newTint = false;
				h->SetTint(lh->Tint());
				break;
			}
			if (lh == h || lh->Key() != key) continue;
			BBitmap* li = lh->Image(BPoint(20, 20));
			if (!li) continue;
			const bool match = (li->BitsLength() == rawImage->BitsLength()
				&& memcmp(li->Bits(), rawImage->Bits(), li->BitsLength()) == 0);
			delete li;
			if (match) {
				newTint = true;
				rgb_color tint = lh->Tint();
				for (int32 j=0; j<NUM_TINTS; j++) {
					if ((*(uint32*)&tint) == (*(uint32*)&(sTints[j]))) {
						foundTints[j] = true;
						break;
					}
				}
			}
		}
	}
	
	delete rawImage;
	
	if (newTint) {
		for (i=0; i<NUM_TINTS; i++) {
			if (!foundTints[i]) {
				h->SetTint(sTints[i]);
				break;
			}
		}
	}
}

ArpRef<AmFilterAddOn> AmFilterRoster::FindFilterAddOn(const BMessage* config) const
{
	BAutolock _l(Locker());
	
	AmFilterAddOnHandle* match = NULL;
	float matchQuality = -1;
	
	const int32 N = CountAddOns();
	for (int32 i=0; i<N; i++) {
		BAddOnHandle* h = AddOnAt(i);
		AmFilterAddOnHandle* fh = dynamic_cast<AmFilterAddOnHandle*>(h);
		if (fh) {
			float q = fh->CheckInstantiation(config);
			if (q >= 0 && q > matchQuality) {
				matchQuality = q;
				match = fh;
			}
		}
	}
	
	if (match) return match->Instantiate();
	return ArpRef<AmFilterAddOn>();
}

ArpRef<AmFilterAddOn> AmFilterRoster::FindFilterAddOn(const char* key) const
{
	BAutolock _l(Locker());
	
	const int32 N = CountAddOns();
	for (int32 i=0; i<N; i++) {
		BAddOnHandle* h = AddOnAt(i);
		AmFilterAddOnHandle* fh = dynamic_cast<AmFilterAddOnHandle*>(h);
		if (fh && fh->Key() == key) {
			return fh->Instantiate();
		}
	}
	
	return ArpRef<AmFilterAddOn>();
}

AmFilterI* AmFilterRoster::InstantiateFilter(AmFilterHolderI* holder,
											 const BMessage* config) const
{
	ArpRef<AmFilterAddOn> addOn = FindFilterAddOn(config);
	if (addOn == NULL) return NULL;
	return addOn->NewInstance(holder, config);
}
	
BAddOnHandle* AmFilterRoster::InstantiateHandle(const entry_ref* entry,
												const node_ref* node)
{
	return new AmFilterAddOnHandle(entry, node);
}




/***************************************************************************
 * Miscellaneous functions
 ****************************************************************************/
static void get_dup_consumer_names(map<BString, int32>& dups)
{
	int32 id = 0;
	BMidiConsumer* cons;
	while ((cons=BMidiRoster::NextConsumer(&id)) != NULL) {
		if (!cons->IsLocal()) {
			BString		name(cons->Name());
			dup_name_map::iterator	it = dups.find(name);
			if (it != dups.end()) (*it).second = (*it).second + 1;
			else {
				dups.insert(dup_name_map::value_type(name, 1));
			}
		}
	}
	dup_name_map::iterator		i;
	for (i = dups.begin(); i != dups.end(); i++) {
		if (i->second < 2) {
			dups.erase(i);
			/* THIS IS ABSOLUTELY NECESSARY -- if you try to iterate
			 * over begin() when the map is empty(), it just hangs.
			 */
			if (dups.empty()) break;
			i = dups.begin();
		}
	}
	for (i = dups.begin(); i != dups.end(); i++) {
		i->second = 1;
	}
}		

static void get_dup_producer_names(map<BString, int32>& dups)
{
	int32 id = 0;
	BMidiProducer* prod;
	while ((prod=BMidiRoster::NextProducer(&id)) != NULL) {
		if (!prod->IsLocal()) {
			BString		name(prod->Name());
			dup_name_map::iterator	it = dups.find(name);
			if (it != dups.end()) (*it).second = (*it).second + 1;
			else {
				dups.insert(dup_name_map::value_type(name, 1));
			}
		}
	}
	dup_name_map::iterator		i;
	for (i = dups.begin(); i != dups.end(); i++) {
		if (i->second < 2) {
			dups.erase(i);
			/* THIS IS ABSOLUTELY NECESSARY -- if you try to iterate
			 * over begin() when the map is empty(), it just hangs.
			 */
			if (dups.empty()) break;
			i = dups.begin();
		}
	}
	for (i = dups.begin(); i != dups.end(); i++) {
		i->second = 1;
	}
}		
