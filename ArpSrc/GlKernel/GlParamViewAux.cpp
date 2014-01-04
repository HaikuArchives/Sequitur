#include <ArpCore/StlVector.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamWrap.h>
#include <GlPublic/GlRootNode.h>
#include <GlKernel/GlParamViewAux.h>

/***************************************************************************
 * _PARAM-VIEW-MIDI-ENTRY
 ***************************************************************************/
class _ParamViewMidiEntry
{
public:
	gl_param_key			key;

	_ParamViewMidiEntry(gl_param_key k) : key(k)		{ }
	virtual ~_ParamViewMidiEntry()						{ }

	virtual status_t	SetValue(float v, GlNode* root) const = 0;
};

/***************************************************************************
 * _PARAM-VIEW-MIDI-FLOAT
 ***************************************************************************/
class _ParamViewMidiFloat : public _ParamViewMidiEntry
{
public:
	float					minV, maxV;

	_ParamViewMidiFloat(gl_param_key key, float minV, float maxV);

	virtual status_t	SetValue(float v, GlNode* n) const;
};

/***************************************************************************
 * _PARAM-VIEW-MIDI-LIST
 ***************************************************************************/
class _ParamViewMidiList
{
public:
	_ParamViewMidiList();
	~_ParamViewMidiList();
	
	uint32				Size() const;
	status_t			Add(_ParamViewMidiEntry* entry);
	status_t			SetValue(float v, GlRootRef& ref);

private:
	vector<_ParamViewMidiEntry*>	mEntries;
};

/***************************************************************************
 * _PARAM-VIEW-TARGET
 ***************************************************************************/
_ParamViewTarget::_ParamViewTarget(const GlRootRef& ref, void* sender)
		: mRef(ref), mSender(sender)
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) mMidi[k] = 0;
}

_ParamViewTarget::~_ParamViewTarget()
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) delete mMidi[k];
}

status_t _ParamViewTarget::ParamEvent(	gl_param_key key, int32 code,
										const GlParamWrap& wrap)
{
//	printf("_ParamViewTarget::ParamEvent()\n");

	ArpVALIDATE(key.nid, return B_ERROR);
	status_t		err = B_ERROR;
	GlRootNode*		r = mRef.WriteLock();
	if (r) {
		GlNode*		n = r->FindNode(0, key.nid);
		ArpASSERT(n);
		if (n) {
			err = n->Params().SetValue(key.key, wrap, key.index);
			if (err == B_OK) n->ParamChanged(key);
		}
		if (mSender) r->SetSender(mSender);
	}
	mRef.WriteUnlock(r);
	return err;
}

status_t _ParamViewTarget::MidiEvent(	GlMidiEvent e, int32 midi,
										bigtime_t time)
{
//	printf("_ParamViewTarget::ParamEvent()\n");
	ArpVALIDATE(midi >= 0 && midi < GL_MIDI_SIZE, return B_ERROR);

	if (!mMidi[midi]) return B_ERROR;

	float			v = 0;
	if (e.type == e.CONTROL_CHANGE) v = e.value2 / 127.0f;
	else return B_ERROR;

	return mMidi[midi]->SetValue(v, mRef);
}

void _ParamViewTarget::Populate(GlControlTargetPopulator& p)
{
}

void _ParamViewTarget::SetState(GlControlState& s) const
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) {
		if (mMidi[k] && mMidi[k]->Size() > 0) s.SetMidi(k);
	}
}

status_t _ParamViewTarget::AddMidiParam(int32 midi, gl_param_key key, 
										const GlParam* p)
{
	ArpVALIDATE(midi >= 0 && midi < GL_MIDI_SIZE, return B_ERROR);
//	printf("Add midi param\n");

	if (!p) return B_ERROR;
	const GlParamType*		pt = p->ParamType();
	if (!pt) return B_ERROR;

	_ParamViewMidiList*		list = MidiListAt(midi, true);
	if (!list) return B_NO_MEMORY;

	if (pt->Type() == GL_FLOAT_TYPE) {
		float				minV, maxV;
		if (((GlFloatParam*)p)->GetRange(&minV, &maxV) != B_OK) {
			minV = ((const GlFloatParamType*)pt)->Min();
			maxV = ((const GlFloatParamType*)pt)->Max();
		}
		return list->Add(new _ParamViewMidiFloat(key, minV, maxV));
	}
	return B_ERROR;
}

status_t _ParamViewTarget::AddMidiParam(int32 midi, gl_param_key key, 
										const GlParamType* pt)
{
	ArpVALIDATE(midi >= 0 && midi < GL_MIDI_SIZE, return B_ERROR);
//	printf("Add midi param type\n");

	_ParamViewMidiList*		list = MidiListAt(midi, true);
	if (!list) return B_NO_MEMORY;

	if (pt->Type() == GL_FLOAT_TYPE) {
		float				minV = ((const GlFloatParamType*)pt)->Min(),
							maxV = ((const GlFloatParamType*)pt)->Max();
		return list->Add(new _ParamViewMidiFloat(key, minV, maxV));
	}
	return B_ERROR;
}

_ParamViewMidiList* _ParamViewTarget::MidiListAt(int32 midi, bool create)
{
	ArpASSERT(midi >= 0 && midi < GL_MIDI_SIZE);

	if (!mMidi[midi] && create) mMidi[midi] = new _ParamViewMidiList();
	return mMidi[midi];
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-ENTRY
 ***************************************************************************/
_ParamViewEntry::_ParamViewEntry(gl_param_key k, uint32 what)
		: key(k), mColumn(-1), mChangingMsg(what)
{
	ArpASSERT(key.nid);
}

_ParamViewEntry::_ParamViewEntry(gl_param_key k, uint32 changingMsg, uint32 changedMsg)
		: key(k), mColumn(-1), mChangingMsg(changingMsg), mChangedMsg(changedMsg)
{
	ArpASSERT(key.nid);
}
				
_ParamViewEntry::~_ParamViewEntry()
{
}

uint32 _ParamViewEntry::MatchesWhat(uint32 what) const
{
	if (mChangingMsg == what) return GL_PARAM_CHANGING;
	else if (mChangedMsg == what) return GL_PARAM_CHANGED;
	return GL_NO_PARAM_CHANGE;
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-BOOL-ENTRY
 ***************************************************************************/
_ParamViewBoolEntry::_ParamViewBoolEntry(BCheckBox* ctrl, gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewBoolEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlBoolWrap(mCtrl->Value() > 0));
}

status_t _ParamViewBoolEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}
	
status_t _ParamViewBoolEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_BOOL_TYPE, return B_ERROR);
	mCtrl->SetValue(((GlBoolWrap&)wrap).v);
	return B_OK;
}

status_t _ParamViewBoolEntry::UpdateControl(const GlNode& node)
{
	if (!mCtrl) return B_ERROR;
	GlBoolWrap		w(mCtrl->Value() > 0);
	status_t		err = node.Params().GetValue(0, key.key, w, key.index);
	if (err != B_OK) return err;
	if ( (mCtrl->Value() > 0) != w.v) mCtrl->SetValue(w.v);
	return B_OK;
}

float _ParamViewBoolEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewBoolEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-COLOR-ENTRY
 ***************************************************************************/
_ParamViewColorEntry::_ParamViewColorEntry(ArpColourControl* ctrl, gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewColorEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlColorWrap(mCtrl->Color()));
}

status_t _ParamViewColorEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewColorEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_COLOR_TYPE, return B_ERROR);
	mCtrl->SetColor(((GlColorWrap&)wrap).v);
	return B_OK;
}

float _ParamViewColorEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewColorEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-FILE-NAME-ENTRY
 ***************************************************************************/
_ParamViewFileNameEntry::_ParamViewFileNameEntry(	ArpFileNameControl* ctrl,
													gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewFileNameEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlTextWrap(mCtrl->FileName()));
}

status_t _ParamViewFileNameEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewFileNameEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_TEXT_TYPE, return B_ERROR);
	mCtrl->SetFileName(&((GlTextWrap&)wrap).v);
	return B_OK;
}

float _ParamViewFileNameEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewFileNameEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-FLOAT-ENTRY
 ***************************************************************************/
_ParamViewFloatEntry::_ParamViewFloatEntry(	ArpFloatControl* ctrl, gl_param_key key,
											uint32 changingMsg, uint32 changedMsg)
		: inherited(key, changingMsg, changedMsg), mCtrl(ctrl)
{
}

status_t _ParamViewFloatEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlFloatWrap(mCtrl->Value()));
}

status_t _ParamViewFloatEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewFloatEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_FLOAT_TYPE, return B_ERROR);
	mCtrl->SetValue(((GlFloatWrap&)wrap).v);
	return B_OK;
}

status_t _ParamViewFloatEntry::UpdateControl(const GlNode& node)
{
	if (!mCtrl) return B_ERROR;
	GlFloatWrap		w(mCtrl->Value());
	status_t		err = node.Params().GetValue(0, key.key, w, key.index);
	if (err != B_OK) return err;
	if (mCtrl->Value() != w.v) mCtrl->SetValue(w.v);
	return B_OK;
}

float _ParamViewFloatEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewFloatEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-FONT-ENTRY
 ***************************************************************************/
_ParamViewFontEntry::_ParamViewFontEntry(ArpFontControl* ctrl, gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewFontEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	ArpFont			f = mCtrl->Font();
	return channel->ParamEvent(key, code, GlFontWrap(&f));
}

status_t _ParamViewFontEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewFontEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_FONT_TYPE, return B_ERROR);
	mCtrl->SetFont(((GlFontWrap&)wrap).v);
	return B_OK;
}

float _ParamViewFontEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewFontEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-INT32-ENTRY
 ***************************************************************************/
_ParamViewInt32Entry::_ParamViewInt32Entry(	ArpIntControl* ctrl,
											gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewInt32Entry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlInt32Wrap(mCtrl->Value()));
}

status_t _ParamViewInt32Entry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewInt32Entry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_INT32_TYPE, return B_ERROR);
	mCtrl->SetValue(((GlInt32Wrap&)wrap).v);
	return B_OK;
}

status_t _ParamViewInt32Entry::UpdateControl(const GlNode& node)
{
	if (!mCtrl) return B_ERROR;
	GlInt32Wrap		w(mCtrl->Value());
	status_t		err = node.Params().GetValue(0, key.key, w, key.index);
	if (err != B_OK) return err;
	if (mCtrl->Value() != w.v) mCtrl->SetValue(w.v);
	return B_OK;
}

float _ParamViewInt32Entry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewInt32Entry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-MENU-ENTRY
 ***************************************************************************/
_ParamViewMenuEntry::_ParamViewMenuEntry(ArpMenuControl* ctrl, gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewMenuEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlInt32Wrap(mCtrl->CurrentIndex()));
}

status_t _ParamViewMenuEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewMenuEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_INT32_TYPE, return B_ERROR);
	mCtrl->SetCurrentIndex(((GlInt32Wrap&)wrap).v);
	return B_OK;
}

status_t _ParamViewMenuEntry::UpdateControl(const GlNode& node)
{
	if (!mCtrl) return B_ERROR;
	int32			i32 = mCtrl->CurrentIndex();
	GlInt32Wrap		w(i32);
	status_t		err = node.Params().GetValue(0, key.key, w, key.index);
	if (err != B_OK) return err;
	if (i32 != w.v) mCtrl->SetCurrentIndex(w.v);
	return B_OK;
}

float _ParamViewMenuEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewMenuEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-POINT-ENTRY
 ***************************************************************************/
_ParamViewPointEntry::_ParamViewPointEntry(	ArpFloatControl* xCtrl, uint32 xChanging, uint32 xChanged,
											ArpFloatControl* yCtrl, uint32 yChanging, uint32 yChanged,
											gl_param_key key)
		: inherited(key, xChanging, xChanged), mXCtrl(xCtrl), mYCtrl(yCtrl),
		  mXChanging(xChanging), mXChanged(xChanged),
		  mYChanging(yChanging), mYChanged(yChanged)
{
}

uint32 _ParamViewPointEntry::MatchesWhat(uint32 what) const
{
	if (mXChanging == what || mYChanging == what) return GL_PARAM_CHANGING;
	else if (mXChanged == what || mYChanged == what) return GL_PARAM_CHANGED;
	return GL_NO_PARAM_CHANGE;
}

status_t _ParamViewPointEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel && mXCtrl && mYCtrl);
	return channel->ParamEvent(key, code, GlPointWrap(BPoint(mXCtrl->Value(), mYCtrl->Value())));
}

status_t _ParamViewPointEntry::EnableControl(bool enable)
{
	mXCtrl->SetEnabled(enable);
	mYCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewPointEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_POINT_TYPE, return B_ERROR);
	mXCtrl->SetValue(((GlPointWrap&)wrap).v.x);
	mYCtrl->SetValue(((GlPointWrap&)wrap).v.y);
	return B_OK;
}

float _ParamViewPointEntry::RightEdge() const
{
	if (!mXCtrl && !mYCtrl) return 0;
	if (!mXCtrl) return mYCtrl->Frame().right;
	if (!mYCtrl) return mXCtrl->Frame().right;
	return mYCtrl->Frame().right;
}

void _ParamViewPointEntry::SetLeftEdge(float left)
{
//	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-POINT3D-ENTRY
 ***************************************************************************/
_ParamViewPoint3dEntry::_ParamViewPoint3dEntry(	ArpFloatControl* xCtrl, uint32 xChanging, uint32 xChanged,
												ArpFloatControl* yCtrl, uint32 yChanging, uint32 yChanged,
												ArpFloatControl* zCtrl, uint32 zChanging, uint32 zChanged,
												gl_param_key key)
		: inherited(key, xChanging, xChanged), mXCtrl(xCtrl), mYCtrl(yCtrl),
		  mZCtrl(zCtrl), mXChanging(xChanging), mXChanged(xChanged),
		  mYChanging(yChanging), mYChanged(yChanged),
		  mZChanging(zChanging), mZChanged(zChanged)
{
}

uint32 _ParamViewPoint3dEntry::MatchesWhat(uint32 what) const
{
	if (mXChanging == what || mYChanging == what) return GL_PARAM_CHANGING;
	else if (mXChanged == what || mYChanged == what) return GL_PARAM_CHANGED;
	else if (mZChanged == what || mZChanged == what) return GL_PARAM_CHANGED;
	return GL_NO_PARAM_CHANGE;
}

status_t _ParamViewPoint3dEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel && mXCtrl && mYCtrl && mZCtrl);
	return channel->ParamEvent(key, code, GlPoint3dWrap(ArpPoint3d(mXCtrl->Value(), mYCtrl->Value(), mZCtrl->Value())));
}

status_t _ParamViewPoint3dEntry::EnableControl(bool enable)
{
	mXCtrl->SetEnabled(enable);
	mYCtrl->SetEnabled(enable);
	mZCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewPoint3dEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_POINT_3D_TYPE, return B_ERROR);
	mXCtrl->SetValue(((GlPoint3dWrap&)wrap).v.x);
	mYCtrl->SetValue(((GlPoint3dWrap&)wrap).v.y);
	mZCtrl->SetValue(((GlPoint3dWrap&)wrap).v.z);
	return B_OK;
}

float _ParamViewPoint3dEntry::RightEdge() const
{
	if (mZCtrl) return mZCtrl->Frame().right;
	if (mYCtrl) return mYCtrl->Frame().right;
	if (mXCtrl) return mXCtrl->Frame().right;
	return 0;
}

void _ParamViewPoint3dEntry::SetLeftEdge(float left)
{
//	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-POINT-ENTRY
 ***************************************************************************/
_ParamViewRelAbsEntry::_ParamViewRelAbsEntry(	ArpFloatControl* rel, uint32 relChanging, uint32 relChanged,
												ArpIntControl* abs, uint32 absChanging, uint32 absChanged,
												gl_param_key key)
		: inherited(key, relChanging, relChanged), mRel(rel), mAbs(abs),
		  mRelChanging(relChanging), mRelChanged(relChanged),
		  mAbsChanging(absChanging), mAbsChanged(absChanged)
{
}

uint32 _ParamViewRelAbsEntry::MatchesWhat(uint32 what) const
{
	if (mRelChanging == what || mAbsChanging == what) return GL_PARAM_CHANGING;
	else if (mRelChanged == what || mAbsChanged == what) return GL_PARAM_CHANGED;
	return GL_NO_PARAM_CHANGE;
}

status_t _ParamViewRelAbsEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel && mRel && mAbs);
	return channel->ParamEvent(key, code, GlRelAbsWrap(GlRelAbs(mRel->Value(), mAbs->Value())));
}

status_t _ParamViewRelAbsEntry::EnableControl(bool enable)
{
	mRel->SetEnabled(enable);
	mAbs->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewRelAbsEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_REL_ABS_TYPE, return B_ERROR);
	mRel->SetValue(((GlRelAbsWrap&)wrap).v.rel);
	mAbs->SetValue(((GlRelAbsWrap&)wrap).v.abs);
	return B_OK;
}

float _ParamViewRelAbsEntry::RightEdge() const
{
	if (!mRel && !mAbs) return 0;
	if (!mRel) return mAbs->Frame().right;
	if (!mAbs) return mRel->Frame().right;
	return mAbs->Frame().right;
}

void _ParamViewRelAbsEntry::SetLeftEdge(float left)
{
//	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-TEXT-ENTRY
 ***************************************************************************/
_ParamViewTextEntry::_ParamViewTextEntry(	BTextControl* ctrl,
											gl_param_key k, uint32 what)
		: inherited(k, what), mCtrl(ctrl)
{
}

status_t _ParamViewTextEntry::Callback(GlControlChannel* channel, uint32 code)
{
	ArpASSERT(channel);
	return channel->ParamEvent(key, code, GlTextWrap(mCtrl->Text()));
}

status_t _ParamViewTextEntry::EnableControl(bool enable)
{
	mCtrl->SetEnabled(enable);
	return B_OK;
}

status_t _ParamViewTextEntry::SetControl(GlParamWrap& wrap)
{
	ArpVALIDATE(wrap.Type() == GL_TEXT_TYPE, return B_ERROR);
	mCtrl->SetText(((GlTextWrap&)wrap).v.String());
	return B_OK;
}

float _ParamViewTextEntry::RightEdge() const
{
	if (!mCtrl) return 0;
	return mCtrl->Frame().right;
}

void _ParamViewTextEntry::SetLeftEdge(float left)
{
	if (mCtrl) mCtrl->MoveTo(left, mCtrl->Frame().top);
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-MIDI-LIST
 ***************************************************************************/
_ParamViewMidiList::_ParamViewMidiList()
{
}

_ParamViewMidiList::~_ParamViewMidiList()
{
	for (uint32 k = 0; k < mEntries.size(); k++) delete mEntries[k];
	mEntries.resize(0);
}

uint32 _ParamViewMidiList::Size() const
{
	return uint32(mEntries.size());
}

status_t _ParamViewMidiList::Add(_ParamViewMidiEntry* entry)
{
	ArpVALIDATE(entry, return B_ERROR);
	mEntries.push_back(entry);
	return B_OK;
}

status_t _ParamViewMidiList::SetValue(float v, GlRootRef& ref)
{
	if (mEntries.size() < 1) return B_OK;

	status_t			err = B_OK;
	GlRootNode*			n = ref.WriteLock();
	if (n) {
		for (uint32 k = 0; k < mEntries.size(); k++) {
			ArpASSERT(mEntries[k]);
			status_t	err2 = mEntries[k]->SetValue(v, n);
			if (err2 != B_OK) err = err2;
		}
	}
	ref.WriteUnlock(n);
	return err;
}

// #pragma mark -

/***************************************************************************
 * _PARAM-VIEW-MIDI-FLOAT
 ***************************************************************************/
_ParamViewMidiFloat::_ParamViewMidiFloat(gl_param_key k, float m1, float m2)
		: _ParamViewMidiEntry(k), minV(m1), maxV(m2)
{
}

status_t _ParamViewMidiFloat::SetValue(float v, GlNode* root) const
{
	ArpASSERT(root);
	GlNode*			n = root->FindNode(0, key.nid);
	if (!n) return B_ERROR;

	float			r = maxV - minV;
//	if (a) v = mMin + (a->At(v) * r);
	v = minV + (r * v);
	GlFloatWrap		w(v);

	status_t		err = n->Params().SetValue(key.key, w, key.index);
	if (err == B_OK) n->ParamChanged(key);
	return err;
}
