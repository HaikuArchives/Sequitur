#include <UndoContext.h>

#include <Debug.h>
#include <List.h>
#include <String.h>

#include <map>
	
BUndoOperation::BUndoOperation()
{
}

BUndoOperation::~BUndoOperation()
{
}

bool BUndoOperation::HasData() const
{
	return true;
}

bool BUndoOperation::MatchOwnerContext(const void* owner) const
{
	return owner == Owner();
}

bool BUndoOperation::AllowMerge() const
{
	return true;
}

void BUndoOperation::_ReservedUndoOperation1() { }
void BUndoOperation::_ReservedUndoOperation2() { }
void BUndoOperation::_ReservedUndoOperation3() { }
void BUndoOperation::_ReservedUndoOperation4() { }
void BUndoOperation::_ReservedUndoOperation5() { }
void BUndoOperation::_ReservedUndoOperation6() { }
void BUndoOperation::_ReservedUndoOperation7() { }
void BUndoOperation::_ReservedUndoOperation8() { }
void BUndoOperation::_ReservedUndoOperation9() { }
void BUndoOperation::_ReservedUndoOperation10() { }
void BUndoOperation::_ReservedUndoOperation11() { }
void BUndoOperation::_ReservedUndoOperation12() { }

// -------------------------------------------------------------------

namespace BPrivate {
	
	class UndoState : public BUndoOperation
	{
	public:
		UndoState()
			: fSaved(false), fCanMerge(true)
		{
		}
		
		virtual ~UndoState()
		{
			for( int32 i=0; i<fData.CountItems(); i++ ) {
				delete (BUndoOperation*)fData.ItemAt(i);
			}
			fData.MakeEmpty();
			fRecent.MakeEmpty();
			fOwners.clear();
		}
		
		void SetName(const char* name)
		{
			fName = name;
		}
		
		void UpdateName(const char* name)
		{
			if( fName.Length() <= 0 ) fName = name;
		}
		
		const char* Name() const
		{
			return fName.String();
		}
		
		void MarkSaved(bool state)
		{
			fSaved = state;
		}
		
		bool IsSaved() const
		{
			return fSaved;
		}
		
		void SetCanMerge(bool state)
		{
			fCanMerge = state;
		}
		
		bool CanMerge() const
		{
			return fCanMerge;
		}
		
		size_t CountData() const
		{
			return (size_t)fData.CountItems();
		}
		
		size_t CountOwners() const
		{
			return fOwners.size();
		}
		
		bool HasOperation(const void* owner) const
		{
			if (owner == NULL) {
				return !fData.IsEmpty();
			}
			UndoState* This = const_cast<UndoState*>(this);
			std::map<const void*, int32>::iterator i = This->fOwners.find(owner);
			return i != fOwners.end();
		}
		
		void AddOperation(BUndoOperation* data)
		{
			const int32 N = fData.CountItems();
			
			if( fData.AddItem(data) ) {
				fRecent.AddItem(data);
				std::pair<const void* const, int32> val(data->Owner(), N);
				std::pair<std::map<const void*, int32>::iterator, bool> p = fOwners.insert(val);
				p.first->second = N;
			} else {
				delete data;
			}
		}
		
		BUndoOperation* LastOperation(const void* owner) const
		{
			if (owner == NULL) {
				return (BUndoOperation*)fData.LastItem();
			}
			UndoState* This = const_cast<UndoState*>(this);
			std::map<const void*, int32>::iterator i = This->fOwners.find(owner);
			if( i != fOwners.end() ) {
				return (BUndoOperation*)fData.ItemAt(i->second);
			}
			return NULL;
		}
		
		virtual const void* Owner() const
		{
			return const_cast<UndoState*>(this);
		}
		
		bool MatchOwnerContext(const void* owner) const
		{
			const int32 N = fData.CountItems();
			for (int32 i=0; i<N; i++) {
				BUndoOperation* d = (BUndoOperation*)fData.ItemAt(i);
				if (d && d->MatchOwnerContext(owner)) return true;
			}
			return false;
		}
		
		virtual bool HasData() const
		{
			for( int32 i=0; i<fData.CountItems(); i++ ) {
				BUndoOperation* d = (BUndoOperation*)fData.ItemAt(i);
				if( d && d->HasData() ) return true;
			}
			return false;
		}
		
		virtual void Commit()
		{
			for( int32 i=0; i<fRecent.CountItems(); i++ ) {
				BUndoOperation* d = (BUndoOperation*)fRecent.ItemAt(i);
				if( d ) d->Commit();
			}
			fRecent.MakeEmpty();
		}
		
		virtual void Undo()
		{
			fCanMerge = false;
			for( int32 i=fData.CountItems()-1; i>=0; i-- ) {
				BUndoOperation* d = (BUndoOperation*)fData.ItemAt(i);
				if( d ) d->Undo();
			}
		}
		
		virtual void Redo()
		{
			fCanMerge = false;
			for( int32 i=0; i<fData.CountItems(); i++ ) {
				BUndoOperation* d = (BUndoOperation*)fData.ItemAt(i);
				if( d ) d->Redo();
			}
		}
		
	private:
		std::map<const void*, int32> fOwners;
		BList fData;					// BUndoOperation*
		BList fRecent;					// BUndoOperation*
		BString fName;
		bool fSaved;
		bool fCanMerge;
	};
}

using namespace BPrivate;

// -------------------------------------------------------------------

BUndoContext::BUndoContext()
	: fUpdateCount(0), fHistorySize(50), fWorking(0),
	  fInUndo(false), fUpdateUndo(false), fMerged(false)
{
}

BUndoContext::~BUndoContext()
{
	ForgetUndos();
	ForgetRedos();
	delete fWorking;
}

bool BUndoContext::InUpdate() const
{
	return fUpdateCount > 0 ? true : false;
}

int32 BUndoContext::UpdateCount() const
{
	return fUpdateCount;
}

void BUndoContext::SetHistorySize(int32 size)
{
	fHistorySize = size;
	if( fHistorySize >= 0 && CountUndos() > fHistorySize ) {
		ForgetUndos(CountUndos()-fHistorySize);
	}
}

int32 BUndoContext::HistorySize() const
{
	return fHistorySize;
}

int32 BUndoContext::Undo(int32 count)
{
	return Undo(0, count);
}

int32 BUndoContext::Undo(const BList* context, int32 count)
{
	int32 num=0;
	int32 i=-1;
	
	fInUndo = fUpdateUndo = true;
	
	UndoState* us = TopUndo(NULL);
	if (us) us->SetCanMerge(false);
	
	while( count>0 && (i=FindPrevState(&fUndos, context, i)) >= 0 ) {
		BUndoOperation* d = (BUndoOperation*)fUndos.RemoveItem(i);
		d->Undo();
		fRedos.AddItem(d);
		count--;
		num++;
	}
	
	fInUndo = false;
	
	return num;
}

int32 BUndoContext::Redo(int32 count)
{
	return Redo(0, count);
}

int32 BUndoContext::Redo(const BList* context, int32 count)
{
	int32 num=0;
	int32 i=-1;
	
	fInUndo = fUpdateUndo = true;
	
	while( count>0 && (i=FindPrevState(&fRedos, context, i)) >= 0 ) {
		BUndoOperation* d = (BUndoOperation*)fRedos.RemoveItem(i);
		d->Redo();
		fUndos.AddItem(d);
		count--;
		num++;
	}
	
	fInUndo = false;
	
	return num;
}

int32 BUndoContext::ForgetUndos(int32 count)
{
	return ForgetUndos(0, count);
}

int32 BUndoContext::ForgetUndos(const BList* context, int32 count)
{
	int32 i, j;
	
	if( count < 0 ) count = fUndos.CountItems();
	
	for( i=j=0; i<fUndos.CountItems(); i++ ) {
		UndoState* s = (UndoState*)fUndos.ItemAt(i);
		if( count > 0 && MatchContext(s, context) ) {
			PRINT(("Forgetting undo state %p (#%ld)\n", s, i));
			delete s;
			count--;
		} else {
			fUndos.ReplaceItem(j++, s);
		}
	}
	
	if( i > j ) fUndos.RemoveItems(j, i-j);
	PRINT(("Forgot %ld undo states, now have %ld states\n",
			i-j, fUndos.CountItems()));
	return i - j;
}

int32 BUndoContext::ForgetRedos(int32 count)
{
	return ForgetRedos(0, count);
}

int32 BUndoContext::ForgetRedos(const BList* context, int32 count)
{
	int32 i, j;
	
	if( count < 0 ) count = fRedos.CountItems();
	
	for( i=j=0; i<fRedos.CountItems(); i++ ) {
		UndoState* s = (UndoState*)fRedos.ItemAt(i);
		if( count > 0 && MatchContext(s, context) ) {
			PRINT(("Forgetting redo state %p (#%ld)\n", s, i));
			delete s;
			count--;
		} else {
			fRedos.ReplaceItem(j++, s);
		}
	}
	
	if( i > j ) fRedos.RemoveItems(j, i-j);
	PRINT(("Forgot %ld redo states, now have %ld states\n",
			i-j, fRedos.CountItems()));
	return i - j;
}

int32 BUndoContext::CountUndos(const BList* context) const
{
	if( context == 0 ) return fUndos.CountItems();
	int32 count=0;
	int32 i=0;
	while( (i=FindNextState(&fUndos, context, i)) >= 0 ) {
		count++;
		i++;
	}
	return count;
}

int32 BUndoContext::CountRedos(const BList* context) const
{
	if( context == 0 ) return fRedos.CountItems();
	int32 count=0;
	int32 i=0;
	while( (i=FindNextState(&fRedos, context, i)) >= 0 ) {
		count++;
		i++;
	}
	return count;
}

const char* BUndoContext::UndoName(const BList* context) const
{
	UndoState* s = TopUndo(context);
	if( s ) return s->Name();
	return "";
}

const char* BUndoContext::RedoName(const BList* context) const
{
	UndoState* s = TopRedo(context);
	if( s ) return s->Name();
	return "";
}

void BUndoContext::StartUpdate(const char* name)
{
	if( !fUpdateCount ) {
		ASSERT(fWorking == 0);
		fWorking = new UndoState();
		fUpdateUndo = false;
		fMerged = false;
	}
	
	ASSERT(fWorking != 0);
	fWorking->UpdateName(name);
	
	fUpdateCount++;
}

void BUndoContext::SetUndoName(const char* name)
{
	if( fWorking == 0 ) debugger("SuggestUndoName() not called during update");
	fWorking->SetName(name);
}

void BUndoContext::SuggestUndoName(const char* name)
{
	if( fWorking == 0 ) debugger("SuggestUndoName() not called during update");
	fWorking->UpdateName(name);
}

int32 BUndoContext::UpdateNestingLevel() const
{
	return fUpdateCount;
}

bool BUndoContext::HasOperation(const void* owner) const
{
	if( fUpdateCount == 0 ) debugger("HasDataOrOperation() when not in update.");
	return fWorking->HasOperation(owner);
}

BUndoOperation* BUndoContext::LastOperation(undo_merge_mode mode)
{
	return LastOperation(NULL, mode);
}

BUndoOperation* BUndoContext::LastOperation(const void* owner, undo_merge_mode mode)
{
	if( fUpdateCount == 0 ) debugger("FindData() when not in update.");
	if (mode != B_NO_UNDO_MERGE && !fMerged && !fWorking->HasData()) {
		UndoState* us = TopUndo(NULL);
		BUndoOperation* last = NULL;
		if (us && (mode == B_ANY_UNDO_MERGE || us->CountOwners() == 1) &&
					us->CanMerge() && (last=us->LastOperation(owner)) != NULL) {
			if (last->AllowMerge()) {
				delete fWorking;
				fWorking = us;
				fUndos.RemoveItem(us);
				fMerged = true;
				return last;
			}
		}
	}
	return fWorking->LastOperation(owner);
}

void BUndoContext::AddOperation(BUndoOperation* data, undo_merge_mode mode)
{
	if( fUpdateCount == 0 ) debugger("AddOperation() when not in update.");
	if (mode != B_NO_UNDO_MERGE && !fMerged && !fWorking->HasData()) {
		UndoState* us = TopUndo(NULL);
		if (us && (mode == B_ANY_UNDO_MERGE || us->CountOwners() == 1) &&
					us->CanMerge() && us->HasOperation(data->Owner())) {
			delete fWorking;
			fWorking = us;
			fUndos.RemoveItem(us);
			fMerged = true;
		}
	}
	fWorking->AddOperation(data);
}

void BUndoContext::EndUpdate()
{
	if( fUpdateCount == 0 ) debugger("EndUpdate() when not in update.");
	else fUpdateCount--;

	if( fUpdateCount == 0 ) {
		ASSERT(fWorking != 0);
		if( !fUpdateUndo && fWorking->HasData() ) {
			fUndos.AddItem(fWorking);
			ForgetRedos();
			fWorking->Commit();
		} else delete fWorking;
		fWorking = 0;
		
		if( fHistorySize >= 0 && CountUndos() > fHistorySize ) {
			ForgetUndos(CountUndos()-fHistorySize);
		}
	}
}

void BUndoContext::CommitState(void* owner)
{
	if (fWorking && fWorking->CountData() > 0) {
		if (!owner || fWorking->HasOperation(owner)) {
			fWorking->SetCanMerge(false);
		}
	} else {
		UndoState* us = TopUndo(NULL);
		if (us && (!owner || us->HasOperation(owner))) {
			us->SetCanMerge(false);
		}
	}
}

void BUndoContext::MarkSaved(bool state, bool unmarkPrevious)
{
	UndoState* s = NULL;
	int32 i = FindPrevState(&fRedos, NULL);
	if( i >= 0 ) s = (UndoState*)fRedos.ItemAt(i);
	if( s ) {
		s->MarkSaved(state);
		if (unmarkPrevious) {
			while ((--i) >= 0) {
				s = (UndoState*)(fRedos.ItemAt(i));
				if (s) s->MarkSaved(false);
			}
		}
	}
}

bool BUndoContext::IsSaved() const
{
	UndoState* s = TopRedo(NULL);
	if( s ) return s->IsSaved();
	return false;
}
	
UndoState* BUndoContext::TopUndo(const BList* context) const
{
	if( fUndos.CountItems() <= 0 ) return 0;
	int32 i = FindPrevState(&fUndos, context);
	if( i >= 0 ) return (UndoState*)fUndos.ItemAt(i);
	return 0;
}

UndoState* BUndoContext::TopRedo(const BList* context) const
{
	if( fRedos.CountItems() <= 0 ) return 0;
	int32 i = FindPrevState(&fRedos, context);
	if( i >= 0 ) return (UndoState*)fRedos.ItemAt(i);
	return 0;
}

bool BUndoContext::MatchContext(const BPrivate::UndoState* state,
								const BList* context) const
{
	if( !context ) return true;
	for( int32 i=0; i<context->CountItems(); i++ ) {
		if( state->MatchOwnerContext(context->ItemAt(i)) ) {
			return true;
		}
	}
	return false;
}

int32 BUndoContext::FindPrevState(const BList* states, const BList* context,
								  int32 from) const
{
	if( from < 0 ) from = states->CountItems()-1;
	if( from >= states->CountItems() ) return -1;
	if( !context ) return from;
	
	while( from >= 0 ) {
		UndoState* us = (UndoState*)(states->ItemAt(from));
		if( us && MatchContext(us, context) ) {
			return from;
		}
		from--;
	}
	
	return -1;
}

int32 BUndoContext::FindNextState(const BList* states, const BList* context,
								  int32 from) const
{
	if( from < 0 ) from = 0;
	if( from >= states->CountItems() ) return -1;
	if( !context ) return from;
	
	while( from < states->CountItems() ) {
		UndoState* us = (UndoState*)(states->ItemAt(from));
		if( us && MatchContext(us, context) ) {
			return from;
		}
		from++;
	}
	
	return -1;
}

void BUndoContext::_ReservedUndoContext1() { }
void BUndoContext::_ReservedUndoContext2() { }
void BUndoContext::_ReservedUndoContext3() { }
void BUndoContext::_ReservedUndoContext4() { }
void BUndoContext::_ReservedUndoContext5() { }
void BUndoContext::_ReservedUndoContext6() { }
void BUndoContext::_ReservedUndoContext7() { }
void BUndoContext::_ReservedUndoContext8() { }
void BUndoContext::_ReservedUndoContext9() { }
void BUndoContext::_ReservedUndoContext10() { }
void BUndoContext::_ReservedUndoContext11() { }
void BUndoContext::_ReservedUndoContext12() { }
void BUndoContext::_ReservedUndoContext13() { }
void BUndoContext::_ReservedUndoContext14() { }
void BUndoContext::_ReservedUndoContext15() { }
void BUndoContext::_ReservedUndoContext16() { }
