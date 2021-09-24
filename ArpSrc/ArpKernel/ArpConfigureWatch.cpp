/*
	
	ArpConfigureImpl.cpp
	
	Copyright (c)1999 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREWATCH_H
#include <ArpKernel/ArpConfigureWatch.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPCOLLECTIONS_ARPSTLVECTOR_H
#include <ArpCollections/ArpSTLVector.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

ArpMOD();

ArpConfigureWatch::ArpConfigureWatch(BHandler* object,
									 ArpConfigurableI* configure)
	: mObject(object), mConfigure(configure),
	  mWatchers(new ArpSTLVector<BMessenger>)
{
}

ArpConfigureWatch::ArpConfigureWatch()
	: mObject(NULL), mConfigure(this),
	  mWatchers(new ArpSTLVector<BMessenger>)
{
}

ArpConfigureWatch::~ArpConfigureWatch()
{
	delete mWatchers;
}

status_t ArpConfigureWatch::MessageReceived(const BMessage* message)
{
	if( !message ) return B_ERROR;
	
	if( message->what == ARP_PUT_CONFIGURATION_MSG ) {
		BMessage config;
		if( message->FindMessage("settings",&config) == B_NO_ERROR ) {
			mConfigure->PutConfiguration(&config);
			return B_OK;
		}
		
	} else if( message->what == ARP_ADD_WATCHER_MSG ) {
		BMessenger w;
		if( message->FindMessenger("watch",&w) == B_NO_ERROR ) {
			AddWatcher(w);
			ReportState(&w);
			return B_OK;
		}
		
	} else if( message->what == ARP_REM_WATCHER_MSG ) {
		BMessenger w;
		if( message->FindMessenger("watch",&w) == B_NO_ERROR ) {
			RemWatcher(w);
			return B_OK;
		}
	}
	
	return B_ERROR;
}

void ArpConfigureWatch::ReportChange(const BMessage* changes,
									 BMessenger* to)
{
	if( !HaveWatchers() ) return;
	
	BMessage report(ARP_PUT_CONFIGURATION_MSG);
	
	if( changes ) {
		report.AddMessage("settings", changes);
	}
	
	ArpD(cdb << ADH << "Reporting change: " << report << std::endl);
	
	if( to ) {
		ArpD(cdb << "Sending to: " << *to << std::endl);
		to->SendMessage(&report);
		return;
	}
	
	mAccess.Lock();
	
	const int N = mWatchers->size();
	
	for( int i=0; i<N; i++ ) {
		if( mWatchers->at(i).IsValid() ) {
			ArpD(cdb << ADH << "Sending to: " << mWatchers->at(i) << std::endl);
			mWatchers->at(i).SendMessage(&report);
		}
	}
	
	mAccess.Unlock();
}

void ArpConfigureWatch::ReportState(BMessenger* to)
{
	if( !HaveWatchers() ) return;
	
	BMessage state;
	if( mConfigure->GetConfiguration(&state) != B_OK ) return;
	ReportChange(&state, to);
}

bool ArpConfigureWatch::HaveWatchers()
{
	mAccess.Lock();
	
	const int N = mWatchers->size();
	int j=0;
	
	for( int i=0; i<N; i++ ) {
		if( mWatchers->at(i).IsValid() ) j++;
		if( j < i ) mWatchers->at(j) = mWatchers->at(i);
	}
	
	mAccess.Unlock();
	
	return j > 0 ? true : false;
}

status_t ArpConfigureWatch::AddWatcher(const BMessenger& w)
{
	mAccess.Lock();
	
	const int N = mWatchers->size();
	
	for( int i=0; i<N; i++ ) {
		if( !(mWatchers->at(i).IsValid()) ) {
			mWatchers->at(i) = w;
			mAccess.Unlock();
			return B_OK;
		}
	}
	
	mWatchers->push_back(w);
	
	mAccess.Unlock();
	
	return B_OK;
}

status_t ArpConfigureWatch::RemWatcher(const BMessenger& w)
{
	mAccess.Lock();
	
	const int N = mWatchers->size();
	
	for( int i=0; i<N; i++ ) {
		if( mWatchers->at(i) == w ) mWatchers->at(i) = BMessenger();
	}
	
	mAccess.Unlock();
	
	// Compact watch vector
	HaveWatchers();
	
	return B_OK;
}
