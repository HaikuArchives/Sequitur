#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmViewFactory.h"

const char*			AM_HEIGHT_PREF_STR	= "height";
static const char*	FACTORIES_STR		= "factories";

/*************************************************************************
 * AM-VIEW-FACTORY
 *************************************************************************/
AmViewFactory::AmViewFactory()
{
}

BString AmViewFactory::PreferredDataName() const
{
	BString		ans;
	if (DataNameAt(0, PRI_VIEW, ans) != B_OK) return BString();
	return ans;
}

// #pragma mark -

/******************************************************************
 * AM-FACTORY-MESSAGE-WRAPPER
 ******************************************************************/
AmFactoryMessageWrapper::AmFactoryMessageWrapper(BMessage* msg)
	: mMsg(msg)
{
}

status_t AmFactoryMessageWrapper::SetInt32Preference(	const char* fac,
														const char* view,
														const char* name,
														int32 i32, int32 n)
{
	ArpVALIDATE(mMsg && fac && view, return B_ERROR);
	BMessage		allFacsMsg;
	status_t		err = B_OK;
	mMsg->FindMessage(FACTORIES_STR, &allFacsMsg);
	BMessage		facMsg;
	allFacsMsg.FindMessage(fac, &facMsg);
	BMessage		viewMsg;
	facMsg.FindMessage(view, &viewMsg);

	if (viewMsg.HasInt32(name, n)) err = viewMsg.ReplaceInt32(name, n, i32);
	else err = viewMsg.AddInt32(name, i32);
	if (err != B_OK) return err;

	if (facMsg.HasMessage(view)) err = facMsg.ReplaceMessage(view, &viewMsg);
	else err = facMsg.AddMessage(view, &viewMsg);
	if (err != B_OK) return err;

	if (allFacsMsg.HasMessage(fac)) err = allFacsMsg.ReplaceMessage(fac, &facMsg);
	else err = allFacsMsg.AddMessage(fac, &facMsg);
	if (err != B_OK) return err;

	if (mMsg->HasMessage(FACTORIES_STR)) return mMsg->ReplaceMessage(FACTORIES_STR, &allFacsMsg);
	else return mMsg->AddMessage(FACTORIES_STR, &allFacsMsg);
}

status_t AmFactoryMessageWrapper::GetInt32Preference(	const char* fac,
														const char* view,
														const char* name,
														int32* outI32, int32 n) const
{
	ArpVALIDATE(mMsg && fac && view, return B_ERROR);
	BMessage		allFacsMsg;
	status_t		err = B_OK;
	if ((err = mMsg->FindMessage(FACTORIES_STR, &allFacsMsg)) != B_OK) return err;
	BMessage		facMsg;
	if ((err = allFacsMsg.FindMessage(fac, &facMsg)) != B_OK) return err;
	BMessage		viewMsg;
	if ((err = facMsg.FindMessage(view, &viewMsg)) != B_OK) return err;
	return viewMsg.FindInt32(name, n, outI32);
}
