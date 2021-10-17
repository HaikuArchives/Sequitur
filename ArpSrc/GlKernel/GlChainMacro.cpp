#include <ArpCore/StlVector.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlChainMacro.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParamType.h>
#include <GlKernel/GlDefs.h>

struct _chain_macro_int32
{
	int32						key, value;

	_chain_macro_int32() : key(0), value(0)					{ }
	_chain_macro_int32(int32 k, int32 v) : key(k), value(v)	{ }
};

struct _chain_macro_float
{
	int32						key;
	float						value;

	_chain_macro_float() : key(0), value(0)					{ }
	_chain_macro_float(int32 k, float v) : key(k), value(v)	{ }
};

struct _chain_macro_text
{
	int32						key;
	BString16					value;

	_chain_macro_text() : key(0)												{ }
	_chain_macro_text(int32 k, const BString16& v) : key(k), value(v)			{ }
	_chain_macro_text(const _chain_macro_text& o) : key(o.key), value(o.value)	{ }

	_chain_macro_text& operator=(const _chain_macro_text& o)
	{
		key = o.key;
		value = o.value;
		return *this;
	}
};

class _GlChainMacroEntry
{
public:
	BString16					creator;
	int32						key;
	
	std::vector<_chain_macro_int32>	i32;
	std::vector<_chain_macro_float>	f;
	std::vector<_chain_macro_text>	t;

	/* A macro entry (which is a node) can have sub chains
	 */
	std::vector<GlChainMacro*>		chains;
	
	_GlChainMacroEntry();
	_GlChainMacroEntry(const BString16& c, int32 k);
	_GlChainMacroEntry(const _GlChainMacroEntry& e);
	~_GlChainMacroEntry();

	_GlChainMacroEntry*	Clone() const;

	GlChainMacro*		Sub(uint32 index);

	bool				Matches(const GlNode* n) const;
	bool				MatchInt32(	const GlParamList& params) const;
	bool				MatchFloat(	const GlParamList& params) const;
	bool				MatchText(	const GlParamList& params) const;

	GlNode*				NewNode() const;

	void				Print(uint32 tabs) const;

private:
	status_t			InstallI32(GlNode* n) const;
	status_t			InstallFloat(GlNode* n) const;
	status_t			InstallText(GlNode* n) const;
};

class _GlChainMacroData
{
public:
	std::vector<_GlChainMacroEntry*>	e;
	
	_GlChainMacroData();
	_GlChainMacroData(const _GlChainMacroData* d);
	~_GlChainMacroData();

	status_t			Status() const;
	status_t			Install(GlChain* c) const;
};

/***************************************************************************
 * GL-CHAIN-MACRO
 ***************************************************************************/
GlChainMacro::GlChainMacro()
		: mData(new _GlChainMacroData()), mCur(-1), mStatus(B_OK)
{
	if (!mData) mStatus = B_ERROR;
}

GlChainMacro::GlChainMacro(const GlChainMacro& o)
		: mCur(-1)
{
	mData = new _GlChainMacroData(o.mData);
}

GlChainMacro::~GlChainMacro()
{
	delete mData;
}

GlChainMacro& GlChainMacro::operator=(const GlChainMacro& o)
{
	delete mData;
	mData = 0;
	mCur = -1;
	if (o.mData) mData = new _GlChainMacroData(o.mData);
	return *this;
}

GlChainMacro* GlChainMacro::Clone() const
{
	return new GlChainMacro(*this);
}

status_t GlChainMacro::Status() const
{
	if (mStatus != B_OK) return mStatus;
	if (!mData) return B_NO_MEMORY;
	return mData->Status();
}

GlChainMacro& GlChainMacro::Add(int32 key)
{
	return Add(SZI[SZI_arp], key);
}

GlChainMacro& GlChainMacro::Add(const BString16& creator, int32 key)
{
	ArpVALIDATE(mData, return *this);
	
	_GlChainMacroEntry*	e = new _GlChainMacroEntry(creator, key);
	if (!e) {
		mStatus = B_ERROR;
		return *this;
	}
		
	mData->e.push_back(e);
	mCur = int32(mData->e.size()) - 1;
	
	return *this;
}

GlChainMacro& GlChainMacro::Set(int32 key, int32 value)
{
	ArpVALIDATE(mData, return *this);
	if (mCur < 0 || mCur >= int32(mData->e.size())) return *this;
	
	mData->e[mCur]->i32.push_back(_chain_macro_int32(key, value));
	return *this;
}

GlChainMacro& GlChainMacro::SetF(int32 key, float value)
{
	ArpVALIDATE(mData, return *this);
	if (mCur < 0 || mCur >= int32(mData->e.size())) return *this;
	
	mData->e[mCur]->f.push_back(_chain_macro_float(key, value));
	return *this;
}

GlChainMacro& GlChainMacro::Set(int32 key, const BString16& value)
{
	ArpVALIDATE(mData, return *this);
	if (mCur < 0 || mCur >= int32(mData->e.size())) return *this;
	
	mData->e[mCur]->t.push_back(_chain_macro_text(key, value));
	return *this;
}

GlChainMacro& GlChainMacro::Sub(uint32 index)
{
	ArpVALIDATE(mData, return *this);
	if (mCur < 0 || mCur >= int32(mData->e.size())) return *this;

	GlChainMacro*		m = mData->e[mCur]->Sub(index);
	if (m) return *m;
	mStatus = B_ERROR;
	return *this;
}

void GlChainMacro::MakeEmpty()
{
	delete mData;
	mData = new _GlChainMacroData();
	if (!mData) mStatus = B_NO_MEMORY;
	else mStatus = B_OK;
}

bool GlChainMacro::Matches(const GlChain* c) const
{
	ArpVALIDATE(mData && c, return false);

	uint32				k, count = c->NodeCount();
	if (count != mData->e.size()) return false;

	for (k = 0; k < count; k++) {
		if (mData->e[k] == 0) return false;
		const GlNode*	n = c->NodeAt(k);
		if (!n) return false;
		if (!mData->e[k]->Matches(n)) return false;
	}
	return true;
}

status_t GlChainMacro::Install(GlChain* c) const
{
	status_t		err = Status();
	if (err != B_OK) return err;
	ArpASSERT(mData);
	
	return mData->Install(c);
}

void GlChainMacro::Print(uint32 tabs) const
{
	uint32				k, tab;
	for (tab = 0; tab < tabs; tab++) printf("\t");
	printf("GlChainMacro %ld nodes\n", (mData) ? mData->e.size() : 0);
	if (!mData) return;
	for (k = 0; k < mData->e.size(); k++) {
		if (mData->e[k]) mData->e[k]->Print(tabs + 1);
	}
}

// #pragma mark -

/***************************************************************************
 * GL-CHAIN-MACRO
 ***************************************************************************/
_GlChainMacroData::_GlChainMacroData()
{
}

_GlChainMacroData::_GlChainMacroData(const _GlChainMacroData* d)
{
	if (!d) return;
	for (uint32 k = 0; k < d->e.size(); k++) {
		if (d->e[k]) {
			_GlChainMacroEntry*		entry = d->e[k]->Clone();
			if (entry) e.push_back(entry);
		}
	}
}

_GlChainMacroData::~_GlChainMacroData()
{
	for (uint32 k = 0; k < e.size(); k++) delete e[k];
	e.resize(0);
}

status_t _GlChainMacroData::Status() const
{
	status_t		err = B_OK;
	for (uint32 k = 0; k < e.size(); k++) {
		for (uint32 j = 0; j < e[k]->chains.size(); j++) {
			if (e[k]->chains[j]) {
				err = e[k]->chains[j]->Status();
				if (err != B_OK) break;
			}
		}
	}
	return err;
}

status_t _GlChainMacroData::Install(GlChain* c) const
{
	ArpVALIDATE(c, return B_ERROR);
	c->DeleteNodes();
	uint32				k;
	for (k = 0; k < e.size(); k++) {
		GlNode*			n = e[k]->NewNode();
		if (!n) return B_NO_MEMORY;
		c->AddNode(n);
	}
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-CHAIN-MACRO-ENTRY
 ***************************************************************************/
_GlChainMacroEntry::_GlChainMacroEntry()
		: key(0)
{
}

_GlChainMacroEntry::_GlChainMacroEntry(const BString16& c, int32 k)
		: creator(c), key(k)
{
}

_GlChainMacroEntry::_GlChainMacroEntry(const _GlChainMacroEntry& e)
		: creator(e.creator), key(e.key)
{
	uint32			k;
	for (k = 0; k < e.i32.size(); k++) i32.push_back(e.i32[k]);
	for (k = 0; k < e.f.size(); k++) f.push_back(e.f[k]);
	for (k = 0; k < e.t.size(); k++) t.push_back(e.t[k]);

	for (k = 0; k < e.chains.size(); k++) {
		if (e.chains[k]) {
			chains.push_back(e.chains[k]->Clone());
		}
	}
}

_GlChainMacroEntry::~_GlChainMacroEntry()
{
	for (uint32 k = 0; k < chains.size(); k++) delete chains[k];
	chains.resize(0);
}

_GlChainMacroEntry* _GlChainMacroEntry::Clone() const
{
	return new _GlChainMacroEntry(*this);
}

GlChainMacro* _GlChainMacroEntry::Sub(uint32 index)
{
	while (index >= chains.size()) chains.push_back(0);
	if (chains[index] == 0) chains[index] = new GlChainMacro();
	return chains[index];
}

static bool _key_exists(std::vector<int32>& vec, int32 v)
{
	for (uint32 k = 0; k < vec.size(); k++) {
		if (vec[k] == v) return true;
	}
	return false;
}

bool _GlChainMacroEntry::Matches(const GlNode* n) const
{
	ArpASSERT(n);
	const GlNodeAddOn*		a = n->AddOn();
	ArpVALIDATE(a, return false);
	float					v = a->GetMatch(creator, key);
	if (v < 0.999) return false;

	const GlParamList&		params = n->Params();

	if (!MatchInt32(params)) return false;
	if (!MatchFloat(params)) return false;
	return MatchText(params);
}

/* INT32s
 */
#define _MATCH		matches.push_back(i32[k].key)
#define _NO_MATCH	nomatch.push_back(i32[k].key)

bool _GlChainMacroEntry::MatchInt32(const GlParamList& params) const
{
	std::vector<int32>			matches, nomatch;
	GlInt32Wrap				w;
	uint32					k;
	/* Categorize every key -- either it's a match or not.
	 */
	for (k = 0; k < i32.size(); k++) {
		if (_key_exists(matches, i32[k].key) == false) {
			if (params.GetValue(0, i32[k].key, w) == B_OK) {
				if (w.v != i32[k].value) _NO_MATCH;
				else _MATCH;
			}
		}
	}
	/* Check to see if everything in the no match pile received
	 * a match at some point.  If so, we're fine.
	 */
	for (k = 0; k < nomatch.size(); k++) {
		if (_key_exists(matches, nomatch[k]) == false) return false;
	}
	return true;
}

#undef _MATCH
#undef _NO_MATCH

/* FLOATs
 */
#define _MATCH		matches.push_back(f[k].key)
#define _NO_MATCH	nomatch.push_back(f[k].key)

bool _GlChainMacroEntry::MatchFloat(const GlParamList& params) const
{
	std::vector<int32>			matches, nomatch;
	GlFloatWrap				w;
	uint32					k;
	/* Categorize every key -- either it's a match or not.
	 */
	for (k = 0; k < f.size(); k++) {
		if (_key_exists(matches, f[k].key) == false) {
			if (params.GetValue(0, f[k].key, w) == B_OK) {
				if (w.v != f[k].value) _NO_MATCH;
				else _MATCH;
			}
		}
	}
	/* Check to see if everything in the no match pile received
	 * a match at some point.  If so, we're fine.
	 */
	for (k = 0; k < nomatch.size(); k++) {
		if (_key_exists(matches, nomatch[k]) == false) return false;
	}
	return true;
}

#undef _MATCH
#undef _NO_MATCH

/* TEXT
 */
#define _MATCH		matches.push_back(t[k].key)
#define _NO_MATCH	nomatch.push_back(t[k].key)

bool _GlChainMacroEntry::MatchText(const GlParamList& params) const
{
	std::vector<int32>			matches, nomatch;
	GlTextWrap				w;
	uint32					k;
	/* Categorize every key -- either it's a match or not.
	 */
	for (k = 0; k < t.size(); k++) {
		if (_key_exists(matches, t[k].key) == false) {
			if (params.GetValue(0, t[k].key, w) == B_OK) {
				if (w.v != t[k].value) _NO_MATCH;
				else _MATCH;
			}
		}
	}
	/* Check to see if everything in the no match pile received
	 * a match at some point.  If so, we're fine.
	 */
	for (k = 0; k < nomatch.size(); k++) {
		if (_key_exists(matches, nomatch[k]) == false) return false;
	}
	return true;
}

#undef _MATCH
#undef _NO_MATCH

GlNode* _GlChainMacroEntry::NewNode() const
{
	GlNode*			n = GlGlobals().NewNode(creator, key, 0);
	if (!n) return 0;

	status_t		err = InstallI32(n);	
	if (err != B_OK) return n;
	if ((err = InstallFloat(n)) != B_OK) return n;
	InstallText(n);

	/* If I've got a subchain, wipe out what I have and install
	 * the new one.
	 */
	if (chains.size() < 1) return n;
	
	uint32			k, size = n->ChainSize();
	for (k = 0; k < size; k++) {
		GlChain*	c = n->ChainAt(k);
		if (c) {
			c->DeleteNodes();
			if (k < chains.size() && chains[k])
				chains[k]->Install(c);
		}
	}
	
	return n;
}

status_t _GlChainMacroEntry::InstallI32(GlNode* n) const
{
	if (i32.size() < 1) return B_OK;
	
	GlInt32Wrap			w;
	for (uint32 k = 0; k < i32.size(); k++) {
		w.v = i32[k].value;
		status_t		err = n->Params().SetValue(i32[k].key, w);
		if (err != B_OK) return err;
	}
	return 0;
}

status_t _GlChainMacroEntry::InstallFloat(GlNode* n) const
{
	if (f.size() < 1) return B_OK;
	
	GlFloatWrap			w;
	for (uint32 k = 0; k < f.size(); k++) {
		w.v = f[k].value;
		status_t		err = n->Params().SetValue(f[k].key, w);
		if (err != B_OK) return err;
	}
	return 0;
}

status_t _GlChainMacroEntry::InstallText(GlNode* n) const
{
	if (t.size() < 1) return B_OK;
	
	GlTextWrap			w;
	for (uint32 k = 0; k < t.size(); k++) {
		w.v = t[k].value;
		status_t		err = n->Params().SetValue(t[k].key, w);
		if (err != B_OK) return err;
	}
	return 0;
}

void _GlChainMacroEntry::Print(uint32 tabs) const
{
	uint32			tab, k;
	for (tab = 0; tab < tabs; tab++) printf("\t");
	printf("%s ", creator.String());
	gl_print_key(key);
	printf(" (%ld)\n", key);
	for (k = 0; k < i32.size(); k++) {
		for (tab = 0; tab < tabs + 1; tab++) printf("\t");
		printf("i32 "); gl_print_key(i32[k].key);
		printf(", "); gl_print_key(i32[k].value); printf(" (%ld)\n", i32[k].value);
	}
	for (k = 0; k < f.size(); k++) {
		for (tab = 0; tab < tabs + 1; tab++) printf("\t");
		printf("float "); gl_print_key(f[k].key); printf(", %f\n", f[k].value);
	}
	for (k = 0; k < t.size(); k++) {
		for (tab = 0; tab < tabs + 1; tab++) printf("\t");
		printf("text "); gl_print_key(t[k].key); printf(", %s\n", t[k].value.String());
	}
	for (k = 0; k < chains.size(); k++) {
		if (chains[k]) chains[k]->Print(tabs + 1);
	}
}
