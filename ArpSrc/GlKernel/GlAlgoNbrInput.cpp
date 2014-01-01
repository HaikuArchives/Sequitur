#include <ArpCore/StlVector.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlAlgoNbrInput.h>

/***************************************************************************
 * GL-ALGO-NBR-IN
 ****************************************************************************/
GlAlgoNbrIn::GlAlgoNbrIn(	int32 key, gl_node_id nid, const BString16& k,
							int32 token)
		: inherited(key, nid, token), key(k), values(0), index(-1)
{
}

GlAlgoNbrIn::GlAlgoNbrIn(const GlAlgoNbrIn& o)
		: inherited(o), key(o.key), values(o.values), index(o.index)
{
}

GlAlgoNbrIn* GlAlgoNbrIn::AsInput()
{
	return this;
}

status_t GlAlgoNbrIn::Process(GlArrayF& set)
{
	if (values) set = *values;
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-ALGO-NBR-IN-LIST
 ****************************************************************************/
GlAlgoNbrInList::GlAlgoNbrInList()
		: ins(0), size(0)
{
}

GlAlgoNbrInList::~GlAlgoNbrInList()
{
	MakeEmpty();
}

status_t GlAlgoNbrInList::Add(GlAlgoNbrIn* a)
{
	ArpVALIDATE(a, return B_ERROR);
	
	uint32				newSize = size + 1;
	GlAlgoNbrIn**		newIns = new GlAlgoNbrIn*[newSize];
	if (!newIns) return B_NO_MEMORY;
	
	uint32				k;
	for (k = 0; k < size; k++) newIns[k] = ins[k];
	ArpASSERT(k == newSize - 1);
	newIns[k] = a;
	MakeEmpty();
	ins = newIns;
	size = newSize;
	return B_OK;
}

void GlAlgoNbrInList::MakeEmpty()
{
	delete[] ins;
	ins = 0;
	size = 0;
}

void GlAlgoNbrInList::SetValues(GlArrayF* values, uint32 keyCount, ...)
{
	uint32				k;
	va_list				ap;
	va_start(ap, keyCount);
	for (k = 0; k < keyCount; k++) {
		const char*		n = va_arg(ap, const char*);
		if (n) {
			for (uint32 k = 0; k < size; k++) {
				if (ins[k] && ins[k]->key == n) ins[k]->values = values;
			}
		}
	}
	va_end(ap);
}

void GlAlgoNbrInList::SetIndex(uint32 index, uint32 keyCount, ...)
{
	uint32				k;
	va_list				ap;
	va_start(ap, keyCount);
	for (k = 0; k < keyCount; k++) {
		const char*		n = va_arg(ap, const char*);
		if (n) {
			for (uint32 k = 0; k < size; k++) {
				if (ins[k] && ins[k]->key == n) ins[k]->index = index;
			}
		}
	}
	va_end(ap);
}
