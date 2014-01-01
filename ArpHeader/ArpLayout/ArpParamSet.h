/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpParamSet.h
 *
 * Definition of a group of parameter values.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 *	• 8/30/1998:
 *	  Created this file.
 *
 */

#ifndef ARPLAYOUT_ARPPARAMSET_H
#define ARPLAYOUT_ARPPARAMSET_H

#ifndef _MESSAGE_H
#include <be/app/Message.h>
#endif

#ifndef _PROPERTY_INFO_H
#include <be/app/PropertyInfo.h>
#endif

#ifndef ARPLAYOUT_ARPPARAM_H
#include <ArpLayout/ArpParam.h>
#endif

// Forward reference private implementation.
struct arp_param_item;
class ArpParamSetItem;

struct parameter_suite
{
	size_t						length;
	const char*					name;
	property_info*				properties;
};

class ArpParamSet
{
public:
	ArpParamSet();
	ArpParamSet(const parameter_info* parameters);
	virtual ~ArpParamSet();
	
	status_t AddParameters(const parameter_info* parameters,
						   const parameter_suite* suite = 0);
	status_t RemoveParameters(const parameter_info* parameters);
	
	const parameter_info* InfoForName(const char* name,
									  const parameter_info* in = 0) const;
	const parameter_info* InfoForParam(const ArpParamI* param,
									   const parameter_info* in = 0) const;
	
	void ClearChanges();
	uint32 GetChanges(const parameter_info* in) const;
	uint32 GetGlobalChanges() const;
	
	void SetGlobals(ArpGlobalSetI* globals);
	ArpGlobalSetI* Globals() const;
	
	const parameter_info* AttachParameter(const char* name, ArpParamI* param,
										  const parameter_info* in = 0);
	status_t DetachParameter(ArpParamI* param,
							 const parameter_info* in = 0);
	
	status_t ArchiveParams(BMessage* data,
						   const parameter_info* in = 0) const;
	status_t InstantiateParams(const BMessage* data,
							   const parameter_info* in = 0);
	
	status_t GetParams(BMessage* params, const char* name,
					   const parameter_info* in = 0) const;
	status_t SetParams(const BMessage* params,
					   const parameter_info* in = 0);
	
	status_t ParamMessageReceived(BMessage* msg,
								  const parameter_info* in = 0);
	
	status_t ParamResolveSpecifier(BMessage *msg,
								   int32 index,
								   BMessage *specifier,
								   int32 form,
								   const char *property,
								   const parameter_info* in = 0);
	
	status_t ParamGetSupportedSuites(BMessage* data,
									 const parameter_info* in = 0);
	
private:
	ArpParamSetItem*		NextSet(ArpParamSetItem* current) const;
	status_t				FindParamName(const char* name, const parameter_info* in,
										  ArpParamSetItem** set, size_t* idx) const;
	void					MakeStdArg(ArpParamI::std_arg& std) const;
	
	ArpParamSetItem*		mParameters;
	ArpGlobalSetI*			mGlobals;
	uint32					mChanges;
};

#endif
