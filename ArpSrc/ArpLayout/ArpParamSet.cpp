/*
	
	ArpParamSet.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#include <ArpLayout/ArpGlobalSet.h>
#endif

#ifndef ARPLAYOUT_ARPPARAMSET_H
#include <ArpLayout/ArpParamSet.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

// --------------------------------------------------------------------------
//                                ArpParamSetItem
// --------------------------------------------------------------------------

struct arp_param_item {
	ArpParamI*		param;
	
	arp_param_item() : param(0) { }
};

class ArpParamSetItem {
public:
	ArpParamSetItem();
	~ArpParamSetItem();
	
	status_t Init(const parameter_info* parameters,
				  const parameter_suite* suite);
	
	void SetNext(ArpParamSetItem* next);
	ArpParamSetItem* Next() const;
	
	const parameter_info* Parameters() const;
	
	ssize_t FindName(const char* name) const;
	ssize_t FindParam(const ArpParamI* param) const;
	const parameter_info* InfoOf(size_t idx) const;
	ArpParamI* ParamOf(size_t idx) const;
	
	void ClearChanges();
	void MarkChange(size_t idx, uint32* global_flags);
	
	uint32 Changes() const;
	
	void SetGlobals(ArpParamI::std_arg& std, uint32* global_flags);
	
	const parameter_info* AttachParameter(ArpParamI::std_arg& std,
										  const char* name,
										  ArpParamI* param,
										  uint32* global_flags);
	status_t DetachParameter(ArpParamI::std_arg& std, ArpParamI* param);
	
	status_t ArchiveParams(ArpParamI::std_arg& std, BMessage* data) const;
	status_t InstantiateParams(ArpParamI::std_arg& std, const BMessage* data,
							   uint32* global_flags);
	
	status_t GetParams(ArpParamI::std_arg& std,
					   BMessage* params, const char* name) const;
	status_t SetParams(ArpParamI::std_arg& std, const BMessage* params,
					   uint32* global_flags);

	status_t ResolveSpecifier(BMessage *msg,
							  int32 index,
							  BMessage *specifier,
							  int32 form,
							  const char *property);
	
	status_t GetSuite(BMessage* data);
	
	void UpdateStdArg(ArpParamI::std_arg& std, size_t item) const;
	
private:
	ArpParamSetItem*		mNext;
	const parameter_info*	mParamInfo;
	const parameter_suite*	mSuiteInfo;
	size_t					mCount;
	arp_param_item*			mItems;
	uint32					mChanges;
};

ArpParamSetItem::ArpParamSetItem()
	: mNext(0), mParamInfo(0), mSuiteInfo(0),
	  mCount(0), mItems(0), mChanges(0)
{
}

ArpParamSetItem::~ArpParamSetItem()
{
	delete[] mItems;
}

status_t
ArpParamSetItem::Init(const parameter_info* parameters,
					  const parameter_suite* suite)
{
	ArpVALIDATE(mParamInfo == 0, return B_ERROR);
	
	mParamInfo = parameters;
	mSuiteInfo = suite;
	
	size_t i;
	
	mCount = 99999;
	for( i=0; InfoOf(i)->property != 0; i++ ) ;
	mCount = i;
	
	mItems = new arp_param_item[mCount];
	
	for( i=0; i < mCount; i++ ) {
		mItems[i].param = 0;
	}
	
	return B_OK;
}

void
ArpParamSetItem::SetNext(ArpParamSetItem* next)
{
	mNext = next;
}

ArpParamSetItem*
ArpParamSetItem::Next() const
{
	return mNext;
}

const parameter_info*
ArpParamSetItem::Parameters() const
{
	return mParamInfo;
}

ssize_t
ArpParamSetItem::FindName(const char* name) const
{
	for( size_t i = 0; i < mCount; i++ ) {
		if( strcmp(InfoOf(i)->property->name, name) == 0 ) {
			return i;
		}
	}
	
	return B_NAME_NOT_FOUND;
}

ssize_t
ArpParamSetItem::FindParam(const ArpParamI* param) const
{
	for( size_t i = 0; i < mCount; i++ ) {
		if( mItems[i].param == param ) return i;
	}
	
	return B_NAME_NOT_FOUND;
}

const parameter_info*
ArpParamSetItem::InfoOf(size_t idx) const
{
	ArpVALIDATE(idx >= 0 && idx < mCount, return 0);
	const int8* p = reinterpret_cast<const int8*>(mParamInfo);
	p += mParamInfo->length*idx;
	return reinterpret_cast<const parameter_info*>(p);
}

ArpParamI*
ArpParamSetItem::ParamOf(size_t idx) const
{
	ArpVALIDATE(idx >= 0 && idx < mCount, return 0);
	return mItems[idx].param;
}

void
ArpParamSetItem::ClearChanges()
{
	mChanges = 0;
}

void
ArpParamSetItem::MarkChange(size_t idx, uint32* global_flags)
{
	ArpVALIDATE(idx >= 0 && idx < mCount, return);
	mChanges |= InfoOf(idx)->local_change_flags;
	if( global_flags ) *global_flags |= InfoOf(idx)->local_change_flags;
	ArpD(cdb << ADH << "Change param set " << Parameters()
			<< ": local=" << mChanges << ", global="
			<< (global_flags ? *global_flags : 0) << endl);
}

uint32
ArpParamSetItem::Changes() const
{
	return mChanges;
}

void
ArpParamSetItem::SetGlobals(ArpParamI::std_arg& std, uint32* global_flags)
{
	for( size_t i=0; i<mCount; i++ ) {
		if( mItems[i].param ) {
			UpdateStdArg(std, i);
			status_t err = mItems[i].param->SetParam(std, 0, 0, 0);
			if( !err ) {
				MarkChange(i, global_flags);
			}
		}
	}
}

const parameter_info*
ArpParamSetItem::AttachParameter(ArpParamI::std_arg& std,
								 const char* name, ArpParamI* param,
								 uint32* global_flags)
{
	const parameter_info* info = 0;
	
	size_t i;
	for( i=0; i<mCount; i++ ) {
		if( strcmp(InfoOf(i)->property->name, name) == 0 ) {
			ArpASSERT(mItems[i].param != 0 || mItems[i].param != param);
			mItems[i].param = param;
			if( !info ) info = InfoOf(i);
			if( std.globals ) {
				UpdateStdArg(std, i);
				status_t err = mItems[i].param->SetParam(std, 0, 0, 0);
				if( !err ) {
					MarkChange(i, global_flags);
				}
			}
		}
	}
	
	return info;
}

status_t
ArpParamSetItem::DetachParameter(ArpParamI::std_arg& std, ArpParamI* param)
{
	status_t err = B_BAD_VALUE;
	
	size_t i;
	for( i=0; i<mCount; i++ ) {
		if( mItems[i].param == param ) {
			mItems[i].param = 0;
			err = B_OK;
		}
	}
	
	return err;
}

status_t
ArpParamSetItem::ArchiveParams(ArpParamI::std_arg& std, BMessage* data) const
{
	status_t err = B_OK;
	
	for( size_t i=0; i<mCount; i++ ) {
		if( mItems[i].param ) {
			status_t result;
			UpdateStdArg(std, i);
			result = mItems[i].param->ArchiveParam(std, data);
			if( result || !err ) err = result;
		}
	}
	
	return err;
}

status_t
ArpParamSetItem::InstantiateParams(ArpParamI::std_arg& std,
								   const BMessage* data,
								   uint32* global_flags)
{
	status_t err = B_OK;
	
	for( size_t i=0; i<mCount; i++ ) {
		if( mItems[i].param ) {
			status_t result;
			UpdateStdArg(std, i);
			result = mItems[i].param->InstantiateParam(std, data);
			if( result == B_OK ) {
				MarkChange(i, global_flags);
			}
			if( result || !err ) err = result;
		}
	}
	
	return err;
}

status_t
ArpParamSetItem::GetParams(ArpParamI::std_arg& std,
						   BMessage* params, const char* name) const
{
	status_t err = B_NAME_NOT_FOUND;
	
	for( size_t i=0;
		(err == B_NAME_NOT_FOUND || err == B_OK ) && i<mCount;
		i++ ) {
		if( mItems[i].param ) {
			if( !name || !strcmp(InfoOf(i)->property->name, name) ) {
				UpdateStdArg(std, i);
				status_t res;
				res = mItems[i].param->GetParam(std, params,
												InfoOf(i)->property->name);
				if( res != B_NAME_NOT_FOUND && err == B_NAME_NOT_FOUND ) {
					err = res;
				}
			}
		}
	}
	
	return err;
}

status_t
ArpParamSetItem::SetParams(ArpParamI::std_arg& std, const BMessage* params,
						   uint32* global_flags)
{
	status_t err = B_NAME_NOT_FOUND;
	
	for( size_t i=0;
		(err == B_NAME_NOT_FOUND || err == B_OK ) && i<mCount;
		i++ ) {
		if( mItems[i].param ) {
			UpdateStdArg(std, i);
			status_t res;
			res = mItems[i].param->SetParam(std, params,
											InfoOf(i)->property->name, 0);
			if( res == B_OK ) {
				MarkChange(i, global_flags);
			}
			if( res != B_NAME_NOT_FOUND && err == B_NAME_NOT_FOUND ) {
				err = res;
			}
		}
	}
	
	return err;
}

status_t
ArpParamSetItem::ResolveSpecifier(BMessage *msg,
								   int32 index,
								   BMessage *specifier,
								   int32 form,
								   const char *property)
{
	if( !mSuiteInfo || !mSuiteInfo->properties ) return B_BAD_SCRIPT_SYNTAX;
	
	BPropertyInfo p(mSuiteInfo->properties);
	int32 i = p.FindMatch(msg, index, specifier, form, property);
	if( i >= 0 ) {
		return B_OK;
#if 0
		// Make sure this is actually a directly-specified property.
		if( mSuiteInfo->properties[i].specifiers[0] == B_DIRECT_SPECIFIER ) {
			return B_OK;
		}
#endif
	}
	
	return B_BAD_SCRIPT_SYNTAX;
}

status_t
ArpParamSetItem::GetSuite(BMessage* data)
{
	if( !mSuiteInfo || !mSuiteInfo->name || !mSuiteInfo->properties ) {
		return B_OK;
	}
	
	status_t err = data->AddString("suites", mSuiteInfo->name);
	BPropertyInfo prop_info(mSuiteInfo->properties);
	if( !err ) err = data->AddFlat("messages", &prop_info);
	
	return err;
}

void
ArpParamSetItem::UpdateStdArg(ArpParamI::std_arg& std, size_t item) const
{
	std.info = InfoOf(item);
}

// --------------------------------------------------------------------------
//                                  ArpParamSet
// --------------------------------------------------------------------------
	
ArpParamSet::ArpParamSet()
	: mParameters(0), mGlobals(0), mChanges(0)
{
}

ArpParamSet::ArpParamSet(const parameter_info* parameters)
	: mParameters(0), mGlobals(0), mChanges(0)
{
	AddParameters(parameters);
}

ArpParamSet::~ArpParamSet()
{
	ArpParamSetItem* set = mParameters;
	while( set ) {
		ArpParamSetItem* next = set->Next();
		delete set;
		set = next;
	}
	mParameters = 0;
}

status_t
ArpParamSet::AddParameters(const parameter_info* parameters,
						   const parameter_suite* suite)
{
	ArpParamSetItem* p = new ArpParamSetItem;
	p->Init(parameters, suite);
	p->SetNext(mParameters);
	mParameters = p;
	return B_OK;
}

status_t
ArpParamSet::RemoveParameters(const parameter_info* parameters)
{
	ArpParamSetItem* set = 0;
	ArpParamSetItem* last = 0;
	while( (set=NextSet(set)) != 0 ) {
		if( set->Parameters() == parameters ) {
			if( !last ) mParameters = set->Next();
			else last->SetNext(set->Next());
			delete set;
			return B_OK;
		}
		last = set;
	}
	
	return B_NAME_NOT_FOUND;
}

const parameter_info*
ArpParamSet::InfoForName(const char* name,
						 const parameter_info* in) const
{
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			ssize_t err = set->FindName(name);
			if( err >= 0 ) {
				return set->InfoOf((size_t)err);
			}
		}
	}
	
	return 0;
}

const parameter_info*
ArpParamSet::InfoForParam(const ArpParamI* param,
						  const parameter_info* in) const
{
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			ssize_t err = set->FindParam(param);
			if( err >= 0 ) {
				return set->InfoOf((size_t)err);
			}
		}
	}
	
	return 0;
}

void
ArpParamSet::ClearChanges()
{
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		set->ClearChanges();
	}
	mChanges = 0;
}

uint32
ArpParamSet::GetChanges(const parameter_info* in) const
{
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		if( in == set->Parameters() ) {
			return set->Changes();
		}
	}
	
	return 0;
}

uint32
ArpParamSet::GetGlobalChanges() const
{
	return mChanges;
}

void
ArpParamSet::SetGlobals(ArpGlobalSetI* globals)
{
	if( !globals ) {
		mGlobals = 0;
		return;
	}
	if( !globals->IsGlobalUpdate() ) mGlobals = globals;
	
	ArpParamI::std_arg std;
	MakeStdArg(std);
	std.globals = globals;
	
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		set->SetGlobals(std, &mChanges);
	}
}

ArpGlobalSetI*
ArpParamSet::Globals() const
{
	return mGlobals;
}

const parameter_info*
ArpParamSet::AttachParameter(const char* name, ArpParamI* param,
							 const parameter_info* in)
{
	ArpParamI::std_arg std;
	MakeStdArg(std);
	
	const parameter_info* p = 0;
	
	ArpParamSetItem* set = 0;
	while( !p && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			p = set->AttachParameter(std, name, param, &mChanges);
		}
	}
	
	return p;
}

status_t
ArpParamSet::DetachParameter(ArpParamI* param,
							 const parameter_info* in)
{
	ArpParamI::std_arg std;
	MakeStdArg(std);
	
	status_t err = B_OK;
	
	ArpParamSetItem* set = 0;
	while( !err && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			status_t res = set->DetachParameter(std, param);
			if( res != B_BAD_VALUE ) err = res;
		}
	}
	
	return err;
}

status_t
ArpParamSet::ArchiveParams(BMessage* data,
						   const parameter_info* in) const
{
	ArpParamI::std_arg std;
	MakeStdArg(std);
	
	status_t err = B_OK;
	
	ArpParamSetItem* set = 0;
	while( !err && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			err = set->ArchiveParams(std, data);
		}
	}
	
	return err;
}

status_t
ArpParamSet::InstantiateParams(const BMessage* data,
							   const parameter_info* in)
{
	ArpParamI::std_arg std;
	MakeStdArg(std);
	
	status_t err = B_OK;
	
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			err = set->InstantiateParams(std, data, &mChanges);
		}
	}
	
	return B_OK;
}

status_t
ArpParamSet::GetParams(BMessage* params, const char* name,
					   const parameter_info* in) const
{
	ArpParamI::std_arg std;
	MakeStdArg(std);
	
	status_t err = B_NAME_NOT_FOUND;
	
	ArpParamSetItem* set = 0;
	while( (err == B_NAME_NOT_FOUND || !err) && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			status_t res = set->GetParams(std, params, name);
			if( res != B_NAME_NOT_FOUND ) err = res;
		}
	}
	
	return err;
}

status_t
ArpParamSet::SetParams(const BMessage* params,
					   const parameter_info* in)
{
	ArpParamI::std_arg std;
	MakeStdArg(std);
	
	status_t err = B_NAME_NOT_FOUND;
	
	ArpParamSetItem* set = 0;
	while( (err == B_NAME_NOT_FOUND || !err) && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			status_t res = set->SetParams(std, params, &mChanges);
			if( res != B_NAME_NOT_FOUND ) err = res;
		}
	}
	
	return err;
}

status_t
ArpParamSet::ParamMessageReceived(BMessage* msg, const parameter_info* in)
{
	status_t err = B_BAD_SCRIPT_SYNTAX;
	
	if( msg->what == B_SET_PROPERTY || msg->what == B_GET_PROPERTY ) {
		int32 index = 0;
		BMessage spec;
		const char* property = 0;
		int32 form=0;
		err = msg->GetCurrentSpecifier(&index, &spec, &form, &property);
		if( err ) return err;
		
		ArpParamSetItem* set = 0;
		size_t i = 0;
		err = FindParamName(property, in, &set, &i);
		if( err ) return err;
		if( !set ) return B_ERROR;
		
		ArpParamI* param = set->ParamOf(i);
		if( param == 0 ) return B_BAD_SCRIPT_SYNTAX;
		
		ArpParamI::std_arg std;
		MakeStdArg(std);
		std.info = set->InfoOf(i);
		
		if( msg->what == B_SET_PROPERTY ) {
			err = param->SetParam(std, msg, "data", "value");
			if( !err ) {
				set->MarkChange(i, &mChanges);
			}
		} else if( msg->what == B_GET_PROPERTY ) {
			BMessage result(B_REPLY);
			err = param->GetParam(std, &result, "result", "value");
			if( !err ) {
				result.AddInt32("error", B_OK);
				msg->SendReply(&result);
			}
		}
	}
	
	return err;
}

status_t
ArpParamSet::ParamResolveSpecifier(BMessage *msg,
								   int32 index,
								   BMessage *specifier,
								   int32 form,
								   const char *property,
								   const parameter_info* in)
{
	status_t err = B_BAD_SCRIPT_SYNTAX;
	
	ArpParamSetItem* set = 0;
	while( (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			err = set->ResolveSpecifier(msg, index, specifier, form, property);
			if( !err ) return err;
		}
	}
	
	return err;
}

status_t
ArpParamSet::ParamGetSupportedSuites(BMessage* data,
									 const parameter_info* in)
{
	status_t err = B_OK;
	
	ArpParamSetItem* set = 0;
	while( !err && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			err = set->GetSuite(data);
		}
	}
	
	return err;
}

ArpParamSetItem*
ArpParamSet::NextSet(ArpParamSetItem* current) const
{
	if( !current ) return mParameters;
	return current->Next();
}

status_t
ArpParamSet::FindParamName(const char* name, const parameter_info* in,
						   ArpParamSetItem** fset, size_t* fidx) const
{
	ssize_t err = B_NAME_NOT_FOUND;
	
	ArpParamSetItem* set = 0;
	while( err == B_NAME_NOT_FOUND && (set=NextSet(set)) != 0 ) {
		if( !in || in == set->Parameters() ) {
			err = set->FindName(name);
			if( err >= 0 ) {
				if( fset ) *fset = set;
				if( fidx ) *fidx = (size_t)err;
				return B_OK;
			}
		}
	}
	
	return err;
}

void
ArpParamSet::MakeStdArg(ArpParamI::std_arg& std) const
{
	std.set = const_cast<ArpParamSet*>(this);
	std.globals = mGlobals;
	std.info = 0;
}
