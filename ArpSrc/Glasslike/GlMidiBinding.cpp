#include <cstdio>
#include <StorageKit.h>
#include <GlPublic/GlGlobalsI.h>
#include "GlPublic/GlParam.h"
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlParamWrap.h"
#include <GlPublic/GlRootNode.h>
#include "GlPublic/GlStrainedParamList.h"
#include <GlKernel/GlDefs.h>
#include "GlNodes/GlEnvelope1d.h"
#include <Glasslike/GlMidiBinding.h>
#include <Glasslike/GlDefs.h>

static const char*		RT_STR				= "rt";
static const char*		FN_STR				= "fn";
static const char*		RI_STR				= "ri";
static const char*		PI_STR				= "pi";
static const char*		MIN_STR				= "min";
static const char*		MAX_STR				= "max";
static const char*		B_STR				= "b";
static const char*		MIDI_BINDINGS_STR	= "midi bindings";

/***************************************************************************
 * GL-MIDI-BINDING
 ***************************************************************************/
GlMidiBinding::GlMidiBinding()
		: rt(-1), rootIndex(0), paramIndex(0), root(0),
		  mMin(0), mMax(1), mFlags(0)
{
}

GlMidiBinding::~GlMidiBinding()
{
	if (root) root->DecRefs();
}

GlMidiBinding& GlMidiBinding::operator=(const GlMidiBinding& o)
{
	event = o.event;
	rt = o.rt;
	filename = o.filename;
	rootIndex = o.rootIndex;
	paramIndex = o.paramIndex;
	if (root) root->DecRefs();
	root = 0;
	portname = o.portname;
	mFlags = o.mFlags;
	return *this;
}

uint32 GlMidiBinding::Flags() const
{
	return mFlags;
}

status_t GlMidiBinding::SetValueRange(float min, float max)
{
	mMin = min;
	mMax = max;
	ArpASSERT(mMin >= 0 && mMax <= 1);
	return B_OK;
}

void GlMidiBinding::GetValueRange(float* min, float* max) const
{
	if (min) *min = mMin;
	if (max) *max = mMax;
}

const BString16* GlMidiBinding::PortName() const
{
	if (Flags()&FAKE_F) return &portname;
	return event.PortName();
}

status_t GlMidiBinding::Load(GlMidiBinding* binding)
{
	ArpVALIDATE(binding, return B_ERROR);
	rt = binding->rt;
	event = binding->event;
	filename = binding->filename;
	rootIndex = binding->rootIndex;
	paramIndex = binding->paramIndex;
	SetValueRange(binding->mMin, binding->mMax);
	return Load();
}

status_t GlMidiBinding::Load()
{
	if (root) root->DecRefs();
	root = 0;
	const GlNodeAddOn*		addon = GlGlobals().GetAddOn(GL_ROOT_KEY);
	if (!addon) return B_ERROR;
	BMessage				projectMsg;
	BFile					file;
	BEntry					entry(filename.String());

	if (entry.InitCheck() != B_OK
			|| entry.IsFile() == false
			|| file.SetTo(&entry, B_READ_ONLY) != B_OK
			|| projectMsg.Unflatten(&file) != B_OK)
		return B_ERROR;
	/* The file format has a series of BMessages storing matrices.
	 */
	int32					msgIndex = 0;
	int32					cmpIndex = 0;
	BMessage				rootMsg;
	while (projectMsg.FindMessage(GL_ROOT_STR, msgIndex, &rootMsg) == B_OK) {
		if (rootMsg.what == GL_ROOT_MSG) {
			if (cmpIndex == rootIndex) {
				root = (GlRootNode*)(addon->NewInstance(&rootMsg));
				break;
			}
			cmpIndex++;
		}
		rootMsg.MakeEmpty();
		msgIndex++;
	}

	if (!root) return B_ERROR;
	root->IncRefs();
	return B_OK;
}

status_t GlMidiBinding::WriteTo(BMessage& msg) const
{
	status_t		err = B_OK;
	if (Flags()&FAKE_F) {
		if ((err = event.WriteFakeTo(msg, &portname)) != B_OK) return err;
	} else {
		if ((err = event.WriteTo(msg, true)) != B_OK) return err;
	}
	
	if ((err = msg.AddInt32(RT_STR, rt)) != B_OK) return err;
	if ((err = msg.AddString(FN_STR, filename)) != B_OK) return err;
	if ((err = msg.AddInt32(RI_STR, rootIndex)) != B_OK) return err;
	if ((err = msg.AddInt32(PI_STR, paramIndex)) != B_OK) return err;
	if ((err = msg.AddFloat(MIN_STR, mMin)) != B_OK) return err;
	if ((err = msg.AddFloat(MAX_STR, mMax)) != B_OK) return err;
	return B_OK;
}

status_t GlMidiBinding::ReadFrom(const BMessage& msg, uint32 flags)
{
	status_t				err = B_OK;
	if (flags&FAKE_F) {
		BString16			pn;
		if ((err = event.ReadFakeFrom(msg, &pn)) != B_OK) return err;
		portname = pn;
		mFlags |= FAKE_F;
	} else {
		if ((err = event.ReadFrom(msg, true)) != B_OK) return err;
		mFlags &= ~FAKE_F;
	}

	if ((err = msg.FindInt32(RT_STR, &rt)) != B_OK) return err;
	if ((err = msg.FindString(FN_STR, &filename)) != B_OK) return err;
	if ((err = msg.FindInt32(RI_STR, &rootIndex)) != B_OK) return err;
	if ((err = msg.FindInt32(PI_STR, &paramIndex)) != B_OK) return err;
	if ((err = msg.FindFloat(MIN_STR, &mMin)) != B_OK) return err;
	if ((err = msg.FindFloat(MAX_STR, &mMax)) != B_OK) return err;
	SetValueRange(mMin, mMax);
	return B_OK;
}

void GlMidiBinding::Print() const
{
	printf("GlMidiBinding rt %s on %s (root %ld param %ld)\n\t", gl_midi_label(rt)->String(),
			filename.String(), rootIndex, paramIndex);
	event.Print();
}

// #pragma mark -

/***************************************************************************
 * GL-MIDI-BINDING-LIST
 ***************************************************************************/
GlMidiBindingList::GlMidiBindingList()
		: mDirty(false)
{
}

GlMidiBindingList::~GlMidiBindingList()
{
	Free();
}

bool GlMidiBindingList::IsDirty() const
{
	return mDirty;
}

uint32 GlMidiBindingList::Size() const
{
	return uint32(mBindings.size());
}

const GlMidiBinding* GlMidiBindingList::At(uint32 index) const
{
	if (index >= mBindings.size()) return 0;
	return mBindings[index];
}

GlMidiBinding* GlMidiBindingList::At(GlMidiEvent event)
{
	for (uint32 k = 0; k < mBindings.size(); k++) {
		if (mBindings[k] && mBindings[k]->event == event)
			return mBindings[k];
	}
	return 0;
}

status_t GlMidiBindingList::Update(GlMidiBinding* binding)
{
	ArpVALIDATE(binding, return B_ERROR);
	status_t			err;
	for (uint32 k = 0; k < mBindings.size(); k++) {
		if (mBindings[k] && mBindings[k]->event == binding->event) {
			err = mBindings[k]->Load(binding);
			mDirty = true;
			return err;
		}
	}
	GlMidiBinding*		b = new GlMidiBinding();
	if (!b) return B_NO_MEMORY;
	err = b->Load(binding);
	mBindings.push_back(b);
	mDirty = true;
	return err;
}

status_t GlMidiBindingList::Delete(GlMidiEvent event)
{
	for (uint32 k = 0; k < mBindings.size(); k++) {
		if (mBindings[k] && mBindings[k]->event == event) {
			delete mBindings[k];
			mBindings.erase(mBindings.begin() + k);
			mDirty = true;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t GlMidiBindingList::WriteTo(BMessage& msg) const
{
	BMessage			bindingsMsg;
	status_t			err;
	for (uint32 k = 0; k < mBindings.size(); k++) {
		if (mBindings[k]) {
			BMessage	m;
			if ((err = mBindings[k]->WriteTo(m)) != B_OK) return err;
			if ((err = bindingsMsg.AddMessage(B_STR, &m)) != B_OK) return err;
		}
	}
	return msg.AddMessage(MIDI_BINDINGS_STR, &bindingsMsg);
}

status_t GlMidiBindingList::ReadFrom(const BMessage& msg)
{
	Free();
	mDirty = false;
	BMessage			bindingMsg, m;
	status_t			err = msg.FindMessage(MIDI_BINDINGS_STR, &bindingMsg);
	if (err != B_OK) return err;
	int32				msgIndex = 0;
	while (bindingMsg.FindMessage(B_STR, msgIndex, &m) == B_OK) {
		GlMidiBinding*	binding = new GlMidiBinding();
		if (binding) {
			if (binding->ReadFrom(m) == B_OK) {
				binding->Load();
				mBindings.push_back(binding);
			} else if (binding->ReadFrom(m, binding->FAKE_F) == B_OK) {
				mBindings.push_back(binding);
			} else delete binding;
		}
		m.MakeEmpty();
		msgIndex++;
	}
	return B_OK;
}

void GlMidiBindingList::Free()
{
	for (uint32 k = 0; k < mBindings.size(); k++) {
		delete mBindings[k];
	}
	mBindings.resize(0);
}
