/* SeqEditToolWindow.cpp
 */
#include "ArpKernel/ArpDebug.h"
#if 0
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmGraphicEffect.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "Sequitur/SequiturDefs.h"
#endif
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmViewFactory.h"
#include "Sequitur/SeqFactoryListView.h"

#if 0
static const uint32		ADD_CONTROL_MSG		= 'iAdC';
static const uint32		DELETE_CONTROL_MSG	= 'iDlC';
static const uint32		ADD_TARGET_MSG		= 'iAdT';
static const uint32		DELETE_TARGET_MSG	= 'iDlT';
static const uint32		DESCRIPTION_MOD_MSG			= 'iDsM';
static const uint32		COPY_ICON_MSG				= 'icic';
static const uint32		PASTE_ICON_MSG				= 'ipic';
static const uint32		FLIP_VERTICALLY_ICON_MSG	= 'ifvi';
static const uint32		FLIP_HORIZONTALLY_ICON_MSG	= 'ifhi';
static const uint32		FILL_WITH_ALPHA_MSG			= 'ibfa';

static const char*		GENERAL_STR			= "General";
static const char*		SEED_LIST_STR		= "seed_list";
static const char*		SEED_STR			= "Seed";
static const char*		PIPELINE_STR		= "Pipeline";
static const char*		DESCRIPTION_STR		= "Description";
static const char*		ICON_STR			= "Icon";
static const char*		EFFECTS_STR			= "Effects";
static const char*		EFFECT_STR			= "Effect";
static const char*		GRAPHIC_LIST_STR	= "graphic_list";
static const char*		CONTROL_STR			= "Controls";
#endif

static const char*		NONE_STR			= "None";
static const char*		VIEW_STR			= "View";

static const int32		NAME_FIELD			= 0;
static const int32		KEY_FIELD			= 1;
#if 0
static const int32		TC_INDEX_FIELD		= 0;
static const int32		TC_NAME_FIELD		= 1;

static const int32		CT_PIPELINE_FIELD	= 0;
static const int32		CT_FILTER_FIELD		= 1;
static const int32		CT_PROPERTY_FIELD	= 2;
#endif

/********************************************************
 * SEQ-FACTORY-LIST-VIEW
 ********************************************************/
SeqFactoryListView::SeqFactoryListView(	BRect frame, const char* name,
										uint32 resizingMode)
		: inherited(frame, name, resizingMode, B_WILL_DRAW, B_NO_BORDER)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	AddColumn( new BStringColumn(VIEW_STR, 170, 20, 350, B_TRUNCATE_END), NAME_FIELD);
	BuildRows();
//	SetSelectionMessage( new BMessage(SEED_ROW_SELECTED) );
}

void SeqFactoryListView::SelectionChanged()
{
	inherited::SelectionChanged();
#if 0
//	if (!Window()->Lock() ) return;
	if (!Parent() ) return;
	BView*			v = Parent()->FindView("key_properties");
	if (v && Parent()->RemoveChild(v) ) delete v;

	_ViewRow*		selection = dynamic_cast<_ViewRow*>( CurrentSelection() );
	if (!selection) return;
	BString			factoryKey, viewKey, key;
	selection->GetKeyInfo(factoryKey, viewKey, key);
	v = NewViewForKey(factoryKey, viewKey, key);
	if (!v) return;
	v->SetName("key_properties");
	BRect		f = Frame();
	v->MoveTo(f.left, mPropertyFrame.top);
	v->ResizeTo(f.Width(), mPropertyFrame.Height() );
//	v->MoveTo( mPropertyFrame.LeftTop() );
//	v->ResizeTo( mPropertyFrame.Width(), mPropertyFrame.Height() );
	Parent()->AddChild(v);
#endif
}

status_t SeqFactoryListView::GetCurrentKeys(BString& outFactoryKey,
											BString& outViewKey) const
{
	SeqFactoryRow*	r = dynamic_cast<SeqFactoryRow*>(CurrentSelection());
	if (!r) return B_ERROR;
	BString			k;
	r->GetKeyInfo(outFactoryKey, outViewKey, k);
	return B_OK;
}

void SeqFactoryListView::BuildRows()
{
	SeqFactoryRow*	root = NewRow(true);
	if (!root) return;
	AddRow(root);

	AmViewFactory*		factory;
	for (uint32 k = 0; (factory = AmGlobals().FactoryAt(k)) != NULL; k++) {
		BString			facKey = factory->Signature();
		SeqFactoryRow*	facrow = NewRow(facKey);
		if (facrow) {
			AddRow(facrow, root);
			BString		viewKey;
			for (uint32 k = 0; factory->DataNameAt(k, PRI_VIEW, viewKey) == B_OK; k++) {
				SeqFactoryRow*	viewrow = NewRow(facKey, viewKey);
				if (viewrow) AddRow(viewrow, facrow);
				viewKey = (const char*)NULL;
			}
			for (uint32 k = 0; factory->DataNameAt(k, SEC_VIEW, viewKey) == B_OK; k++) {
				if (!HasRow(facKey, viewKey)) {
					SeqFactoryRow*	viewrow = NewRow(facKey, viewKey);
					if (viewrow) AddRow(viewrow, facrow);
				}
				viewKey = (const char*)NULL;
			}
		}
	}
}

SeqFactoryRow* SeqFactoryListView::NewRow(bool root) const
{
	return new SeqFactoryRow(root);
}

SeqFactoryRow* SeqFactoryListView::NewRow(const BString& factoryKey) const
{
	return new SeqFactoryRow(factoryKey);
}

SeqFactoryRow* SeqFactoryListView::NewRow(const BString& factoryKey, const BString& viewKey) const
{
	return new SeqFactoryRow(factoryKey, viewKey);
}

void SeqFactoryListView::ClearRowKeys(BRow* parent)
{
	/* This is a work around -- currently there's
	 * a bug in the column list view that causes
	 * a crash if you try to access the child information
	 * of a child with no children.  If yaknowwhatImean.
	 */
	if (parent && !parent->HasLatch() ) return;
	BRow*	row;
	int32	count = CountRows(parent);
	for (int32 k = 0; k < count; k++) {
		if ((row = RowAt(k, parent)) != NULL) {
			SeqFactoryRow*	vr = dynamic_cast<SeqFactoryRow*>(row);
			if (vr) {
				vr->SetKey(NULL);
				ClearRowKeys(row);
			}
		}
	}
}

SeqFactoryRow* SeqFactoryListView::ViewRow(	const BString& factoryKey,
											const BString& viewKey,
											BRow* parent)
{
	if (parent && !parent->HasLatch() ) return NULL;
	BRow*	row;
	int32	count = CountRows(parent);
	for (int32 k = 0; k < count; k++) {
		if ((row = RowAt(k, parent)) != NULL) {
			SeqFactoryRow*	vr = dynamic_cast<SeqFactoryRow*>(row);
			if (vr && vr->Matches(factoryKey, viewKey) ) return vr;
			vr = ViewRow(factoryKey, viewKey, row);
			if (vr) return vr;
		}
	}
	return NULL;
}

bool SeqFactoryListView::HasRow(const BString& facKey, const BString& viewKey) const
{
	return false;
}

// #pragma mark -

/********************************************************
 * SEQ-FACTORY-ROW
 ********************************************************/
SeqFactoryRow::SeqFactoryRow(bool root)
		: mRoot(root)
{
	if (mRoot) SetField(new BStringField("All Views"), NAME_FIELD);
}

SeqFactoryRow::SeqFactoryRow(const BString& factoryKey)
		: mRoot(false), mFactoryKey(factoryKey)
{
	SetField(new BStringField( mFactoryKey.String() ), NAME_FIELD);
}

SeqFactoryRow::SeqFactoryRow(const BString& factoryKey, const BString& viewKey)
		: mRoot(false), mFactoryKey(factoryKey), mViewKey(viewKey)
{
	SetField(new BStringField( mViewKey.String() ), NAME_FIELD);
}

bool SeqFactoryRow::HasLatch() const
{
	if (mViewKey.Length() > 0) return false;
	return true;
}

bool SeqFactoryRow::Matches(const BString& factoryKey,
							const BString& viewKey) const
{
	return mFactoryKey == factoryKey && mViewKey == viewKey;
}

void SeqFactoryRow::SetKey(const char* key)
{
	mKey = key;
	GetKeyLabel(key, mLabel);
	SetField(new BStringField( mLabel.String() ), KEY_FIELD);
}

void SeqFactoryRow::GetKeyInfo(BString& factoryKey, BString& viewKey, BString& key) const
{
	factoryKey = mFactoryKey;
	viewKey = mViewKey;
	key = mKey;
}

void SeqFactoryRow::Print() const
{
	if (mRoot) printf("root, key: %s\n", mKey.String());
	else {
		printf("factory: %s, view: %s, key: %s\n",
				mFactoryKey.String(),  mViewKey.String(), mKey.String() );
	}
}

void SeqFactoryRow::GetKeyLabel(const char* key, BString& outLabel)
{
	if (!key) {
		outLabel = NONE_STR;
		return;
	}
	outLabel = "A Label";
#if 0
	BString		label;
	if (AmToolSeedI::GetSeedInfo(BString(key), label) != B_OK)
		outLabel = key;
	else
		outLabel = label;	
#endif
}
