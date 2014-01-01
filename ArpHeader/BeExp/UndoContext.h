/***************************************************************************
//
//	File:			UndoContext.h
//
//	Description:	Generic infrastructure for undo/redo support.
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _UNDO_CONTEXT_H
#define _UNDO_CONTEXT_H

#include <List.h>

namespace BPrivate {
	class UndoState;
}

namespace BResEditor {

class BUndoOperation
{
public:
						BUndoOperation();
virtual					~BUndoOperation();
	
virtual	const void*		Owner() const = 0;
	
virtual	bool			HasData() const;
virtual	bool			MatchOwnerContext(const void* owner) const;
virtual	bool			AllowMerge() const;
	
virtual	void			Commit() = 0;
virtual	void			Undo() = 0;
virtual	void			Redo() = 0;

private:
		/* FBC */
virtual	void			_ReservedUndoOperation1();
virtual	void			_ReservedUndoOperation2();
virtual	void			_ReservedUndoOperation3();
virtual	void			_ReservedUndoOperation4();
virtual	void			_ReservedUndoOperation5();
virtual	void			_ReservedUndoOperation6();
virtual	void			_ReservedUndoOperation7();
virtual	void			_ReservedUndoOperation8();
virtual	void			_ReservedUndoOperation9();
virtual	void			_ReservedUndoOperation10();
virtual	void			_ReservedUndoOperation11();
virtual	void			_ReservedUndoOperation12();
		uint32			_reserved[3];
};

enum undo_merge_mode {
	B_NO_UNDO_MERGE,		// Never merge with last state
	B_UNIQUE_UNDO_MERGE,	// Allow merge if last state contains only your owner
	B_ANY_UNDO_MERGE		// Always allow merge of last state
};

class BUndoContext
{
public:
						BUndoContext();
virtual					~BUndoContext();

		bool			InUpdate() const;
		int32			UpdateCount() const;
		
		void			SetHistorySize(int32 size);
		int32			HistorySize() const;
		
		int32			Undo(int32 count=1);
		int32			Undo(const BList* context, int32 count=1);
		int32			Redo(int32 count=1);
		int32			Redo(const BList* context, int32 count=1);
		
		int32			ForgetUndos(int32 count=-1);
		int32			ForgetUndos(const BList* context, int32 count=-1);
		int32			ForgetRedos(int32 count=-1);
		int32			ForgetRedos(const BList* context, int32 count=-1);
		
		int32			CountUndos(const BList* context = 0) const;
		int32			CountRedos(const BList* context = 0) const;
		
		const char*		UndoName(const BList* context = 0) const;
		const char*		RedoName(const BList* context = 0) const;
		
		void			StartUpdate(const char* name = 0);
		void			SetUndoName(const char* name);
		void			SuggestUndoName(const char* name);
		
		int32			UpdateNestingLevel() const;
		
		bool			HasOperation(const void* owner) const;
		
		BUndoOperation*	LastOperation(	undo_merge_mode mode = B_NO_UNDO_MERGE);
		BUndoOperation*	LastOperation(	const void* owner,
										undo_merge_mode mode = B_NO_UNDO_MERGE);
		void			AddOperation(	BUndoOperation* data,
										undo_merge_mode mode = B_NO_UNDO_MERGE);
		
		void			EndUpdate();
		
		void			CommitState(void* owner = NULL);
		
		void			MarkSaved(bool state=true, bool unmarkPrevious=true);
		bool			IsSaved() const;
	
private:
		/* FBC */
virtual	void			_ReservedUndoContext1();
virtual	void			_ReservedUndoContext2();
virtual	void			_ReservedUndoContext3();
virtual	void			_ReservedUndoContext4();
virtual	void			_ReservedUndoContext5();
virtual	void			_ReservedUndoContext6();
virtual	void			_ReservedUndoContext7();
virtual	void			_ReservedUndoContext8();
virtual	void			_ReservedUndoContext9();
virtual	void			_ReservedUndoContext10();
virtual	void			_ReservedUndoContext11();
virtual	void			_ReservedUndoContext12();
virtual	void			_ReservedUndoContext13();
virtual	void			_ReservedUndoContext14();
virtual	void			_ReservedUndoContext15();
virtual	void			_ReservedUndoContext16();
		uint32			_reserved[32];
		
	BPrivate::UndoState*	TopUndo(const BList* context) const;
	BPrivate::UndoState*	TopRedo(const BList* context) const;
	bool				MatchContext(	const BPrivate::UndoState* state,
										const BList* context) const;
	int32				FindPrevState(	const BList* states,
										const BList* context, int32 from=-1) const;
	int32				FindNextState(	const BList* states,
										const BList* context, int32 from=0) const;
	
	BList				fUndos;
	BList				fRedos;
	int32				fUpdateCount;
	int32				fHistorySize;
	BPrivate::UndoState*	fWorking;
	bool				fInUndo;
	bool				fUpdateUndo;
	bool				fMerged;
};

}	// namespace BResEditor
using namespace BResEditor;

#endif
