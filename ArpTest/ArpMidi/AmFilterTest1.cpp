

#include <Debug.h>
#include <assert.h>

#include <vector>
#include <map>
#include <be/midi/MidiDefs.h>
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmEvents.h"
#include "AmFilterTest1.h"
#include "FakeSupport.h"

FakeImageManager	im;
FakePreferences		pr;
ArpPreferencesI& Prefs()
{
	return pr;
}
ArpImageManagerI& ImageManager()
{
	return im;
}

int main(int argc, char* argv[])
{
	AmFilterTest1 *myApplication;
	myApplication = new AmFilterTest1();
	delete myApplication;
	return 0;
}

static const char *app_signature = "application/x-vnd.Arp-quicklaunch";

AmFilterTest1::AmFilterTest1()
		  :BApplication(app_signature)
{
	printf("*********************TestBasic01\n");  fflush(stdout);
	TestBasic01();
	printf("*********************TESTS SUCCESSFUL\n");  fflush(stdout);
}

void AmFilterTest1::TestBasic01()
{
	int i;
	
	printf("Making a vector.\n");
	vector<int, allocator<int> > myVect(2);
	myVect[0] = 1;
	
	printf("Making a map.\n");
	map< int, int, less<int>, allocator<int> > myMap;
	myMap[0] = 1;
	
	printf("Creating example filter object...\n");
	fflush(stdout);
	ArpExampleFilterAddon* exAddon = new ArpExampleFilterAddon;
	AmFilterHolder* exHolder = new AmFilterHolder;
	exHolder->SetFilter(exAddon->NewInstance(exHolder));
	
	printf("Creating BSynth filter object...\n");
	fflush(stdout);
#if 1
	AmConsumerFilterAddon* syAddon = new AmConsumerFilterAddon();
#else
	AmConsumerFilterAddon* syAddon = new AmConsumerFilterAddon("<midi port name>");
#endif
	/* NOTE:  The second consumer filter is not valid.  You need to
	 * supply the name of the midi port you want to use.
	 */
	AmFilterHolder* syHolder = new AmFilterHolder;
	syHolder->SetFilter(syAddon->NewInstance(syHolder));
	
	printf("Creating debug filter object...\n");
	fflush(stdout);
	ArpDebugFilterAddon* dbAddon = new ArpDebugFilterAddon(true);
	AmFilterHolder* dbHolder = new AmFilterHolder;
	dbHolder->SetFilter(dbAddon->NewInstance(dbHolder));
	
	printf("Attaching BSynth filter to example filter...\n");
	fflush(stdout);
	exHolder->AddSuccessor(syHolder);
	
	printf("Attaching debug filter to BSynth filter...\n");
	fflush(stdout);
	syHolder->AddSuccessor(dbHolder);
	
	printf("Creating a chain of NoteOn events...\n");
	fflush(stdout);
	AmEventI* evHead=NULL;
	AmEventI* evTail=NULL;
	for( i=1; i<=4; i++ ) {
		{
			AmProgramChange* ev = new AmProgramChange((i-1)*10,
																PPQN*4*i-((PPQN*3)/2));
			if( !evHead ) {
				evHead = ev;
			} else {
				evTail->AppendEvent(ev);
			}
			ev->SetNextFilter(exHolder);
			evTail = ev;
		}
		
		{
			AmNoteOn* ev = new AmNoteOn(60+5*i,100,PPQN*4*i);
			ev->SetRelVelocity(80);
			if( !evHead ) {
				evHead = ev;
			} else {
				evTail->AppendEvent(ev);
			}
			ev->SetNextFilter(exHolder);
			evTail = ev;
		}
		
		{
			AmTempoChange* ev = new AmTempoChange(120 + 40*i,PPQN*4*i);
			if( !evHead ) {
				evHead = ev;
			} else {
				evTail->AppendEvent(ev);
			}
			ev->SetNextFilter(exHolder);
			evTail = ev;
		}
	}
	
	printf("Initial Event Chain:\n");
	fflush(stdout);
	PrintEventChain(evHead);
	fflush(stdout);
	
	printf("Processing filters on event chain...\n");
	fflush(stdout);
	evHead = ArpExecFilters(evHead);
	
	printf("Final event chain:\n");
	fflush(stdout);
	PrintEventChain(evHead);
	fflush(stdout);
	
	printf("Performing the song!\n");
	fflush(stdout);
	{
		AmPerformer p;
		p.Start(evHead);
		// this is a bit of a hack to wait for it to finish;
		// it will be better when we flesh out the performer
		// interface to be able to send messages.
		do {
			snooze(1 * 1000000);	// wait for a sec
		} while( p.IsPlaying() );
	}
	fflush(stdout);
	
	printf("Placing the BSynth filter in its own list...\n");
	fflush(stdout);
	exHolder->RemoveSuccessor(syHolder);
	syHolder->RemoveSuccessor(dbHolder);
	
	printf("Creating a chain of sixteenth notes...\n");
	fflush(stdout);
	evHead = evTail = new AmProgramChange(B_KALIMBA, 0);
	evTail->SetNextFilter(syHolder);
	evTail->AppendEvent(new AmTempoChange(480, 0));
	evTail = evTail->TailEvent();
	evTail->SetNextFilter(syHolder);
	for( i=1; i<=4*4*4; i++ ) {
		{
			AmNoteOn* ev = new AmNoteOn(70+2*(i/(4*4)),100,PPQN*i/4);
			ev->SetDuration(PPQN/8);
			ev->SetRelVelocity(80);
			if( !evHead ) {
				evHead = ev;
			} else {
				evTail->AppendEvent(ev);
			}
			ev->SetNextFilter(syHolder);
			evTail = ev;
		}
	}
	
	printf("Performing the next song!\n");
	fflush(stdout);
	{
		AmPerformer p;
		p.Start(evHead);
		// this is a bit of a hack to wait for it to finish;
		// it will be better when we flesh out the performer
		// interface to be able to send messages.
		do {
			snooze(1 * 1000000);	// wait for a sec
		} while( p.IsPlaying() );
	}
	fflush(stdout);
	
	printf("Deleting filters -- WHICH FOR SOME REASON IS CRASHING, SO IT'S BEEN REMOVED.\n");
#if 0
	exHolder->Delete();
	exAddon->RemReference();
	syHolder->Delete();
	syAddon->RemReference();
	dbHolder->Delete();
	dbAddon->RemReference();
#endif	
	DoBigTest();
	
	printf("Repeating test...\n");
	fflush(stdout);
	
	DoBigTest();
}

void AmFilterTest1::PrintEventChain(AmEventI* first)
{
	if( !first ) {
		printf("Empty!\n");
		return;
	}
	int i = 1;
	while( first ) {
		printf("%d: ", i);
		first->Print();
		first = first->NextEvent();
		i++;
	}
}

void AmFilterTest1::DoBigTest()
{
	int i;

	printf("Creating example filter object...\n");
	fflush(stdout);
	ArpExampleFilterAddon* exAddon = new ArpExampleFilterAddon;
	AmFilterHolder* exHolder = new AmFilterHolder;
	exHolder->SetFilter(exAddon->NewInstance(exHolder));
	
	printf("Creating debug filter object (as an output)...\n");
	fflush(stdout);
	ArpDebugFilterAddon* dbAddon = new ArpDebugFilterAddon(true);
	AmFilterHolder* dbHolder = new AmFilterHolder;
	dbHolder->SetFilter(dbAddon->NewInstance(dbHolder));
	
	printf("Attaching debug filter to example filter...\n");
	fflush(stdout);
	exHolder->AddSuccessor(dbHolder);
	
	printf("Creating a chain of NoteOn events...\n");
	fflush(stdout);
	AmEventI* evHead=NULL;
	AmEventI* evTail=NULL;
	
	bigtime_t startTime, endTime;
	
	const int NUMEVENTS = 10000;
	
	printf("Now creating and filtering %d events.\n", NUMEVENTS);
	fflush(stdout);
	
	startTime = system_time();
	evHead = evTail = NULL;
	for( i=1; i<=NUMEVENTS; i++ ) {
		AmEvent* ev = new AmNoteOn((30+2*i)%127,40,PPQN*4*i);
		if( !evHead ) {
			evHead = ev;
		} else {
			evTail->AppendEvent(ev);
		}
		ev->SetNextFilter(exHolder);
		evTail = ev;
	}
	evHead = ArpExecFilters(evHead);
	
	endTime = system_time();
	printf("Done.  Total time = %f seconds.\n", double(endTime-startTime)/1000000.0);
	
	printf("Counting events.\n");
	evTail=evHead;
	i=0;
	while( evTail ) {
		evTail = evTail->NextEvent();
		i++;
	}
	
	printf("Ended up with %d events.  %s\n", i, (i==(NUMEVENTS*3)) ? "Good!" : "**ERROR**");
	
	printf("Deleting result.\n");
	fflush(stdout);
	evHead->DeleteChain();

	printf("Deleting filters -- WHICH FOR SOME REASON IS CRASHING, SO IT'S BEEN REMOVED.\n");
#if 0
	exHolder->Delete();
	exAddon->RemReference();
	dbHolder->Delete();
	dbAddon->RemReference();
#endif
}
