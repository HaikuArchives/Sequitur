#include <stdio.h>
#include <be/experimental/ColumnListView.h>
#include <be/experimental/ColumnTypes.h>
#include <be/interface/TextControl.h>
#include <ArpInterface/ArpPrefs.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamTypeList.h>
#include <GlPublic/GlRootNode.h>
#include <GlPublic/GlStrainedParamList.h>
#include <GlKernel/GlConfigView.h>
#include <GlKernel/GlDefs.h>

static const uint32		ON_MSG			= WM_USER + 1;
static const uint32		LABEL_MSG		= WM_USER + 2;
static const uint32		MIDI_MSG		= WM_USER + 3;
static const int32		PARAM_COL		= 0;
static const int32		LABEL_COL		= 1;
static const int32		ON_COL			= 2;
static const int32		MIDI_COL		= 3;

static void _make_midi_menu(BMessage& msg);

/********************************************************
 * _CONFIG-PARAM-LIST-VIEW
 ********************************************************/
class _ConfigListView : public BColumnListView
{
public:
	_ConfigListView(BRect frame);

	virtual void	SelectionChanged();

	void			SetCtrls(	BCheckBox* onCtrl,
								BTextControl* labelCtrl,
								ArpMenuControl* midiCtrl);

private:
	typedef BColumnListView inherited;
	BCheckBox*		mOnCtrl;
	BTextControl*	mLabelCtrl;
	ArpMenuControl*	mMidiCtrl;
};

/********************************************************
 * _PARAM-ROW
 ********************************************************/
class _ParamRow : public BRow
{
public:
	_ParamRow(	gl_node_id paramNid, const GlParamType* type,
				int32 index, const BString16* label, int32 control,
				int32 midi = GL_MIDI_OFF);
	virtual ~_ParamRow();

	virtual bool		HasLatch() const		{ return false; }

	gl_node_id			ParamNodeId() const;
	const GlParamType*	ParamType() const;
	int32				Index() const;
	
	const BString16*	Label() const;
	void				SetLabel(const BString16* label);
	int32				Control() const;
	void				SetControl(int32 control);
	int32				Midi() const;
	void				SetMidi(int32 midi);
	
protected:
	gl_node_id			mParamNid;
	const GlParamType*	mType;
	int32				mIndex;
	BString16			mLabel;
	int32				mControl;
	int32				mMidi;
};

/***************************************************************************
 * GL-CONFIG-PARAM-VIEW
 ***************************************************************************/
GlConfigView::GlConfigView(	const BRect& frame, const GlRootRef& ref,
							const GlNode& node, const GlStrainedParamList& params)
		: inherited(frame), mRef(ref), mNid(node.Id()),
		  mListView(0), mOn(0), mLabel(0), mMidi(0)
{
	BRect						b(Bounds());
	float						w = b.Width() * 0.5f;
	float						fh = ViewFontHeight();
	float						sx = float(Prefs().GetInt32(ARP_PADX)),
								sy = float(Prefs().GetInt32(ARP_PADY));

	/* Add the node label.
	 */	
	BString16					lab(node.AddOn()->Label());
	lab << " Parameters";
	BRect						r(sx, sy, w - sx, sy + fh);
	AddLabel(r, "label", lab);

	float						dataTop = r.bottom;
	r.top = dataTop;
	r.bottom = frame.Height() - 20;

	mListView = new _ConfigListView(r);
	if (mListView) {
		GlStrainedParam			p;
		uint32					size = params.Size();
		for (uint32 k = 0; k < size; k++) {
			if (params.At(k, p) == B_OK) {
				ArpASSERT(p.pt);
				uint32			flags = p.pt->StateFlags();
				if (!(flags&GL_ROOT_INFO_F)) {
					BRow*		r = new _ParamRow(p.nid, p.pt, p.index, p.label, p.control, p.midi);
					if (r) mListView->AddRow(r);
				}
			}
		}
		AddChild(mListView);
		BRect					tmpr(mListView->Frame());
		r = tmpr;
	}

	r.left = r.right + sx;
	r.right = b.right - sx;
	r.top = r.bottom = dataTop;
	ShiftCheckBoxDown(r);
	BRect			cbR(r);
	cbR.right = (cbR.left + 20 + StringWidth(SZ(SZ_On)));
	mOn = new BCheckBox(cbR, "on", SZ(SZ_On), new BMessage(ON_MSG));
	if (mOn) {
		mOn->SetEnabled(false);
		AddChild(mOn);
	}
	BRect			lR(r);
	lR.left = (cbR.right + 5);
	BString16		initL;
	mLabel = AddTextControl(lR, "lbl", SZ(SZ_Label), 0, LABEL_MSG, initL, StringWidth(SZ(SZ_Label)) + 5);

	ShiftMenuDown(r);
	BMessage		items;
	_make_midi_menu(items);
	mMidi = AddMenuControl(r, "midi", SZ(SZ_Midi), MIDI_MSG, items, StringWidth(SZ(SZ_Midi)) + 5);
	if (mMidi) mMidi->SetCurrentIndex(-1);
	
	if (mListView) mListView->SetCtrls(mOn, mLabel, mMidi);
}

GlConfigView::~GlConfigView()
{
}

void GlConfigView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mOn) mOn->SetTarget(BMessenger(this));
}

void GlConfigView::GetPreferredSize(float* width, float* height)
{
	inherited::GetPreferredSize(width, height);
	if (mListView) {
		BRect		r(mListView->Frame());
		if (r.right > *width) *width = r.right;
		if (r.bottom + 14 > *height) *height = r.bottom + 14;
	}
	if (mOn) {
		BRect		r(mOn->Frame());
		if (r.right > *width) *width = r.right;
		if (r.bottom + 14 > *height) *height = r.bottom + 14;
	}
}

void GlConfigView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case ON_MSG: {
			if (mListView && mOn) {
				_ParamRow*		row = (_ParamRow*)(mListView->CurrentSelection());
				if (row) {
					if (mOn->Value() == B_CONTROL_ON) row->SetControl(GL_CONTROL_ON);
					else row->SetControl(GL_CONTROL_OFF);
					mListView->UpdateRow(row);
					SetStrainedParam(row);
				}
			}
		} break;
		case LABEL_MSG: {
			if (mListView && mLabel) {
				_ParamRow*		row = (_ParamRow*)(mListView->CurrentSelection());
				if (row) {
					row->SetLabel(mLabel->Text());
					mListView->UpdateRow(row);
					SetStrainedParam(row);
				}
			}
		} break;
		case MIDI_MSG: {
			if (mListView && mMidi) {
				_ParamRow*		row = (_ParamRow*)(mListView->CurrentSelection());
				if (row) {
					row->SetMidi(mMidi->CurrentIndex());
					mListView->UpdateRow(row);
					SetStrainedParam(row);
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

status_t GlConfigView::ControlMessage(uint32 what)
{
//	if (what == LABEL_MSG) printf("Label Msg (control)\n");
	return B_ERROR;
}

status_t GlConfigView::SetStrainedParam(const _ParamRow* row)
{
	ArpVALIDATE(mListView && row, return B_ERROR);
	status_t				err = B_ERROR;
	GlRootNode*				root = mRef.WriteLock();
	if (root) {
		GlNode*				n = root->FindNode(0, mNid);
		ArpASSERT(n);
		if (n) {
			gl_param_key	key(row->ParamNodeId(), row->ParamType()->Key(), row->Index());
			n->ParamStrainer().SetLabel(key, row->Label());
			n->ParamStrainer().SetControl(key, row->Control());
			n->ParamStrainer().SetMidi(key, row->Midi());
			err = B_OK;
//			n->ParamStrainer().Print();
		}
	}
	mRef.WriteUnlock(root);
	return err;
}

// #pragma mark -

/********************************************************
 * _CONFIG-LIST-VIEW
 ********************************************************/
_ConfigListView::_ConfigListView(BRect frame)
		: inherited(frame, "lv", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW, B_PLAIN_BORDER),
		  mOnCtrl(0), mLabelCtrl(0), mMidiCtrl(0)
{
	AddColumn(new BStringColumn(SZ(SZ_Parameter), 70, 20, 250, B_TRUNCATE_END), PARAM_COL);
	AddColumn(new BStringColumn(SZ(SZ_Label), 70, 20, 250, B_TRUNCATE_END), LABEL_COL);
	AddColumn(new BStringColumn(SZ(SZ_Control), 40, 20, 40, B_TRUNCATE_END), ON_COL);
	AddColumn(new BStringColumn(SZ(SZ_Midi), 40, 20, 40, B_TRUNCATE_END), MIDI_COL);
	SetSortingEnabled(false);
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
}

void _ConfigListView::SelectionChanged()
{
	inherited::SelectionChanged();
	BRow*		row = CurrentSelection();
	if (!row) {
		if (mOnCtrl && mOnCtrl->IsEnabled() == true) mOnCtrl->SetEnabled(false);
		if (mLabelCtrl && mLabelCtrl->IsEnabled() == true) mLabelCtrl->SetEnabled(false);
		if (mMidiCtrl && mMidiCtrl->IsEnabled() == true) mMidiCtrl->SetEnabled(false);
	} else {
		_ParamRow*	prow = (_ParamRow*)row;
		if (mLabelCtrl) {
			if (mLabelCtrl->IsEnabled() == false) mLabelCtrl->SetEnabled(true);
			mLabelCtrl->SetText(prow->Label());
		}
		if (mOnCtrl) {
			if (mOnCtrl->IsEnabled() == false) mOnCtrl->SetEnabled(true);
			if (prow->Control() == GL_CONTROL_ON && mOnCtrl->Value() == B_CONTROL_OFF) mOnCtrl->SetValue(B_CONTROL_ON);
			else if (prow->Control() == GL_CONTROL_OFF && mOnCtrl->Value() == B_CONTROL_ON) mOnCtrl->SetValue(B_CONTROL_OFF);
		}
		if (mMidiCtrl) {
			if (mMidiCtrl->IsEnabled() == false) mMidiCtrl->SetEnabled(true);
			mMidiCtrl->SetCurrentIndex(prow->Midi());
		}
	}
}

void _ConfigListView::SetCtrls(	BCheckBox* onCtrl, BTextControl* labelCtrl,
								ArpMenuControl* midiCtrl)
{
	mOnCtrl = onCtrl;
	mLabelCtrl = labelCtrl;
	mMidiCtrl = midiCtrl;
}

// #pragma mark -

/********************************************************
 * _PARAM-ROW
 ********************************************************/
_ParamRow::_ParamRow(	gl_node_id paramNid,
						const GlParamType* type,
						int32 index, const BString16* label,
						int32 control, int32 midi)
		: mParamNid(paramNid), mType(type), mIndex(index),
		  mControl(GL_CONTROL_OFF), mMidi(GL_MIDI_OFF)
{
	ArpASSERT(type);
	SetField(new BStringField(type->Label()), PARAM_COL);
	SetLabel(label);
//	SetOn(on);
	SetControl(control);
	SetMidi(midi);
}

_ParamRow::~_ParamRow()
{
}

gl_node_id _ParamRow::ParamNodeId() const
{
	return mParamNid;
}

const GlParamType* _ParamRow::ParamType() const
{
	return mType;
}

int32 _ParamRow::Index() const
{
	return mIndex;
}

#if 0
	bool				IsOn() const;
	void				SetOn(bool on);

bool _ParamRow::IsOn() const
{
	return mOn;
}

void _ParamRow::SetOn(bool on)
{
	mOn = on;
	if (on) SetField(new BStringField("Yes"), ON_COL);
	else SetField(new BStringField("No"), ON_COL);
}
#endif

const BString16* _ParamRow::Label() const
{
	return &mLabel;
}

void _ParamRow::SetLabel(const BString16* label)
{
	mLabel.WinRelease();
	if (label) mLabel = *label;
	SetField(new BStringField(&mLabel), LABEL_COL);
}

int32 _ParamRow::Control() const
{
	return mControl;
}

void _ParamRow::SetControl(int32 control)
{
	mControl = control;
	if (mControl == GL_CONTROL_ON) SetField(new BStringField(SZ(SZ_On)), ON_COL);
	else SetField(new BStringField(SZ(SZ_Off)), ON_COL);
}

int32 _ParamRow::Midi() const
{
	return mMidi;
}

void _ParamRow::SetMidi(int32 midi)
{
	mMidi = midi;
ArpFINISH();
//	SetField(new BStringField(gl_midi_label(mMidi)), MIDI_COL);
}

// #pragma mark -

/********************************************************
 * Misc
 ********************************************************/
static void _make_midi_menu(BMessage& msg)
{
	for (int32 k = -1; k < 27; k++) {
		const BString16*	str = gl_midi_label(k);
		if (str) {
			msg.AddString16("item", str);
			msg.AddInt32("i", k);
		}
	}
}
