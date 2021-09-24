#include <typeinfo>
#include <ArpKernel/ArpDebug.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlProcessStatus.h>

static bool			_bundle_paren(std::vector<GlAlgo*>& vec);
static GlAlgo*		_parse(std::vector<GlAlgo*>& vec, uint32 start, uint32 end);
static status_t		_make_binary(std::vector<GlAlgo*>& vec, uint32 bin);
static bool			_sort_prec(std::vector<GlAlgo*>& vec, std::vector<uint32> bin);

/***************************************************************************
 * _GL-ALGO-DATA
 ****************************************************************************/
class _GlAlgoData
{
public:
	GlAlgo**			nodes;
	uint32				size;

	_GlAlgoData();
	_GlAlgoData(const _GlAlgoData& o);
	~_GlAlgoData();
	
	_GlAlgoData*		Clone() const;

	bool				SetStep(float v);

	status_t			SetChain(GlAlgo* node, uint32 index);
};

/***************************************************************************
 * GL-ALGO
 ****************************************************************************/
GlAlgo::~GlAlgo()
{
	delete mNext;
	delete mData;
}

gl_node_id GlAlgo::NodeId() const
{
	return mNid;
}

uint32 GlAlgo::Flags() const
{
	return 0;
}

bool GlAlgo::SetStep(float v)
{
	bool			ans = false;
	GlAlgo*			a = this;
	while (a) {
		if (a->set_step(v)) ans = true;
		a = a->mNext;
	}
	return ans;
}

status_t GlAlgo::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	return B_ERROR;
}

status_t GlAlgo::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	return B_ERROR;
}

status_t GlAlgo::AddTail(GlAlgo* tail)
{
	ArpVALIDATE(tail, return B_ERROR);
	GlAlgo*		a = this;
	while (a->mNext) a = a->mNext;
	ArpASSERT(a);
	a->mNext = tail;
	return B_OK;
}

GlAlgo1d* GlAlgo::As1d()
{
	printf("GlAlgo::As1d() -- there are very few times I should be here\n");
	return 0;
}

const GlAlgo1d* GlAlgo::As1d() const
{
	printf("GlAlgo::As1d() const -- there are very few times I should be here\n");
	return 0;
}

GlAlgo2d* GlAlgo::As2d()
{
	printf("GlAlgo::As2d() -- there are very few times I should be here\n");
	return 0;
}

const GlAlgo2d* GlAlgo::As2d() const
{
	printf("GlAlgo::As2d() const -- there are very few times I should be here\n");
	return 0;
}

GlAlgoIm* GlAlgo::AsIm()
{
	printf("GlAlgo::AsIm() -- there are very few times I should be here\n");
	return 0;
}

const GlAlgoIm* GlAlgo::AsIm() const
{
	printf("GlAlgo::AsIm() const -- there are very few times I should be here\n");
	return 0;
}

GlAlgoNbr* GlAlgo::AsNbr()
{
	printf("GlAlgo::AsNbr() -- there are very few times I should be here\n");
	return 0;
}

const GlAlgoNbr* GlAlgo::AsNbr() const
{
	printf("GlAlgo::AsNbr() const -- there are very few times I should be here\n");
	return 0;
}

status_t GlAlgo::PerformAll(GlNodeDataList& list, const gl_process_args* args)
{
	GlAlgo*				a = this;
	int32				partitions = 0;
	if (args && args->status) {
		while (a) {
			partitions++;
			a = a->mNext;
		}
		if (partitions > 0) args->status->PushPartition(partitions);		
	}

	a = this;
	while (a) {
		status_t		err = a->Perform(list, args);
		if (err == B_OK && partitions > 0) err = args->status->IncSegment();
		if (err != B_OK) break;
		a = a->mNext;
	}

	if (partitions > 0) args->status->PopPartition();		
	return B_OK;
}

status_t GlAlgo::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	printf("MISSING PERFORM FOR ");
	_print();
	printf("\n");
	return B_ERROR;
}

int32 GlAlgo::Walk(GlAlgoAction& action, uint32 io)
{
	GlAlgo*				a = this;
	while (a) {
		if (io == 0 || io == a->Io()) {
			int32		ans = a->walk(action, io);
			if (ans != GL_CONTINUE) return ans;
		}
		a = a->mNext;
	}
	return GL_CONTINUE;
}

void GlAlgo::UpdateSource(GlNode*)
{
	ArpASSERT(false);
}

int32 GlAlgo::Token() const
{
	return mToken;
}

float GlAlgo::Precedence() const
{
	return mPrecedence;
}

GlAlgo* GlAlgo::Parse()
{
	std::vector<GlAlgo*>			vec;
	GlAlgo*					n = 0;
	uint32					k;
	while ( (n = (n) ? (n->ParseNext()) : this) != 0) vec.push_back(n);
	ArpVALIDATE(vec.size() > 0, return this);

	/* Cause any nodes that might contain nodes themselves
	 * to parse.
	 */
	for (k = 0; k < vec.size(); k++) {
		vec[k]->ParseDetach();
// At some point I might need to add something like this in -- if
// there's ever a node that has chains, like a multi.  But right
// now this will cause an infinite loop.
//		vec[k]->Parse();
	}
	/* Bundle up anything in parantheses.
	 */
	while (_bundle_paren(vec)) ;
	return _parse(vec, 0, uint32(vec.size() - 1));
}

status_t GlAlgo::AssignBinary(GlAlgo* lh, GlAlgo* rh)
{
	return B_ERROR;
}

status_t GlAlgo::AssignUnary(GlAlgo* v)
{
	return B_ERROR;
}

GlAlgo::GlAlgo(gl_node_id nid, int32 token)
		: mNid(nid), mToken(token), mPrecedence(0),
		  mNext(0), mData(0)
{
}

GlAlgo::GlAlgo(const GlAlgo& o)
		: mNid(o.mNid), mToken(o.mToken), mPrecedence(o.mPrecedence),
		  mNext(0), mData(0)
{
	if (o.mNext) mNext = o.mNext->Clone();
	if (o.mData) mData = o.mData->Clone();
}

status_t GlAlgo::SetChain(GlAlgo* node, uint32 index)
{
	if (!mData) mData = new _GlAlgoData();
	if (!mData) return B_NO_MEMORY;
	status_t		err = mData->SetChain(node, index);
	if (err != B_OK) delete node;
	return err;
}

uint32 GlAlgo::ChainSize() const
{
	if (mData) return mData->size;
	return 0;
}

GlAlgo* GlAlgo::ChainAt(uint32 index)
{
	if (!mData) return 0;
	if (index >= mData->size) return 0;
//	ArpVALIDATE(!(index >= mData->size), return 0);
	return mData->nodes[index];
}

const GlAlgo* GlAlgo::ChainAt(uint32 index) const
{
	if (!mData) return 0;
	ArpVALIDATE(!(index >= mData->size), return 0);
	return mData->nodes[index];
}

GlAlgo* GlAlgo::AlgoAt(uint32 index)
{
	if (mData && index < mData->size) return mData->nodes[index];
	return 0;
}

const GlAlgo* GlAlgo::AlgoAt(uint32 index) const
{
	if (mData && index < mData->size) return mData->nodes[index];
	return 0;
}

GlAlgo1d* GlAlgo::Algo1dAt(uint32 index)
{
	GlAlgo*				n = ChainAt(index);
	if (n) return n->As1d();
	return 0;
}

const GlAlgo1d* GlAlgo::Algo1dAt(uint32 index) const
{
	const GlAlgo*		n = ChainAt(index);
	if (n) return n->As1d();
	return 0;
}

GlAlgo2d* GlAlgo::Algo2dAt(uint32 index)
{
	GlAlgo*				n = ChainAt(index);
	if (n) return n->As2d();
	return 0;
}

const GlAlgo2d* GlAlgo::Algo2dAt(uint32 index) const
{
	const GlAlgo*		n = ChainAt(index);
	if (n) return n->As2d();
	return 0;
}

bool GlAlgo::set_step(float v)
{
	if (!mData) return false;
	return mData->SetStep(v);
}

int32 GlAlgo::walk(GlAlgoAction& action, uint32 io)
{
	int32		err = action.PerformSwitch(this);
	if (err != GL_CONTINUE) return err;
	if (!mData) return err;
	for (uint32 k = 0; k < mData->size; k++) {
		if (mData->nodes[k] && mData->nodes[k]->Io()&io) {
			err = mData->nodes[k]->Walk(action, io);
			if (err != GL_CONTINUE) return err;
		}
	}
	return GL_CONTINUE;
}

void GlAlgo::_print() const
{
	printf("%s", typeid(*this).name());
}

void GlAlgo::_print_additional(uint32 tabs) const
{
}

GlAlgo* GlAlgo::ParseNext() const
{
	return mNext;
}

status_t GlAlgo::ParseDetach()
{
	mNext = 0;
	return B_OK;
}

void GlAlgo::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	_print();
	printf("\n");
	if (mData) {
		for (uint32 k = 0; k < mData->size; k++) {
			if (mData->nodes[k]) mData->nodes[k]->Print(tabs + 1);
		}	
	}
	if (mNext) mNext->Print(tabs);
}

// #pragma mark -

/***************************************************************************
 * GL-PARSE-ALGO-DATA
 ****************************************************************************/
_GlAlgoData::_GlAlgoData()
		: nodes(0), size(0)
{
}

_GlAlgoData::_GlAlgoData(const _GlAlgoData& o)
		: nodes(0), size(0)
{
	if (o.size > 0) {
		nodes = new GlAlgo*[o.size];
		if (!nodes) return;
		size = o.size;
		for (uint32 k = 0; k < size; k++) {
			if (o.nodes[k]) nodes[k] = o.nodes[k]->Clone();
			else nodes[k] = 0;
		}
	}
}

_GlAlgoData::~_GlAlgoData()
{
	for (uint32 k = 0; k < size; k++) delete nodes[k];
	delete[] nodes;
}
	
_GlAlgoData* _GlAlgoData::Clone() const
{
	return new _GlAlgoData(*this);
}

bool _GlAlgoData::SetStep(float v)
{
	bool			ans = false;
	for (uint32 k = 0; k < size; k++) {
		if (nodes[k]) {
			if (nodes[k]->SetStep(v)) ans = true;
		}
	}
	return ans;
}

status_t _GlAlgoData::SetChain(GlAlgo* node, uint32 index)
{
	if (index >= size) {
		GlAlgo**		newNodes = new GlAlgo*[index + 1];
		if (!newNodes) return B_NO_MEMORY;
		for (uint32 k = 0; k < index + 1; k++) {
			if (k < size) newNodes[k] = nodes[k];
			else newNodes[k] = 0;
		}
		delete[] nodes;
		nodes = newNodes;
		size = index + 1;
	}
	ArpVALIDATE(index < size, return B_ERROR);
	if (nodes[index]) delete nodes[index];
	nodes[index] = node;
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * Parsing
 ****************************************************************************/
/* Get the first innermost pair of parentheses, parse the
 * contents, than bundle all that up into a single algo and
 * reinsert it into the list.
 */
static bool _bundle_paren(std::vector<GlAlgo*>& vec)
{
	uint32						left = uint32(vec.size());
	for (uint32 k = 0; k < vec.size(); k++) {
		if (vec[k]) {
			if (vec[k]->Token() == GL_OPEN_PAREN) left = k;
			if (vec[k]->Token() == GL_CLOSE_PAREN) {
				if (left >= k) return false;
				GlAlgo*			a = _parse(vec, left + 1, k);
				if (!a) return false;
				vec[left]->AssignUnary(a);		
				for (uint32 k2 = left + 1; k2 <= k; k2++) vec[k2] = 0;
				return true;
			}
		}
	}
	return false;
}

/* Turn the supplied range of algos into a single node
 * representing the start of a parse tree.  First bundle up
 * any binaries.  Anyone left over gets thrown into a chain.
 */
static GlAlgo* _parse(std::vector<GlAlgo*>& vec, uint32 start, uint32 end)
{
	ArpVALIDATE(vec.size() > 0, return 0);
	ArpVALIDATE(end < vec.size(), end = uint32(vec.size() - 1));
	ArpVALIDATE(start <= end, start = end);

	std::vector<uint32>		binaries;
	uint32				k;
	for (k = start; k <= end; k++) {
		if (vec[k]) {
			if (vec[k]->Token() == GL_BINARY) binaries.push_back(k);
		}
	}
	/* Sort binaries on precedence.
	 */
	while (_sort_prec(vec, binaries)) ;
	/* Replace binaries.
	 */
	for (k = 0; k < binaries.size(); k++) {
		status_t		err = _make_binary(vec, binaries[k]);
		ArpASSERT(err == B_OK);
	}
	/* Everyone that's left gets chained.
	 */
	GlAlgo*				h = 0;
	for (k = start; k <= end; k++) {
		if (vec[k]) {
			if (!h) h = vec[k];
			else h->AddTail(vec[k]);
		}
	}
	return h;
}

static status_t	_make_binary(std::vector<GlAlgo*>& vec, uint32 bin)
{
	ArpVALIDATE(vec[bin] != 0 && vec[bin]->Token() == GL_BINARY, return B_ERROR);
	GlAlgo*				a = vec[bin];
	GlAlgo*				lh = 0;
	GlAlgo*				rh = 0;
	int32				k;
	for (k = int32(bin) - 1; k >= 0; k--) {
		if (vec[k]) {
			lh = vec[k];
			vec[k] = 0;
			break;
		}
	}
	for (k = bin + 1; k < int32(vec.size()); k++) {
		if (vec[k]) {
			rh = vec[k];
			vec[k] = 0;
			break;
		}
	}
	return a->AssignBinary(lh, rh);
}

static bool _sort_prec(std::vector<GlAlgo*>& vec, std::vector<uint32> bin)
{
	bool			ans = false;
	for (uint32 k = 1; k < bin.size(); k++) {
		if (vec[bin[k]]->Precedence() < vec[bin[k-1]]->Precedence()) {
			uint32	t = bin[k-1];
			bin[k-1] = bin[k];
			bin[k] = t;
			ans = true;
		}
	}
	return ans;
}
