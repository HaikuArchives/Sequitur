
// Export symbols in ArpMidi library from this application.
#define _BUILDING_AmKernel 1

#include <Debug.h>
#include <cassert>

#include "ArpMidiListTest1.h"


main(int argc, char* argv[])
{
	ArpMidiListTest1 *myApplication;
	myApplication = new ArpMidiListTest1();


//	myApplication->Run();
	
	delete myApplication;
	return 0;
}




const char *app_signature = "application/x-vnd.Arp-quicklaunch";

ArpMidiListTest1::ArpMidiListTest1()
		  :BApplication(app_signature)
{
	printf("*********************TestBasic01\n");  fflush(stdout);
	TestBasic01();
	printf("*********************TESTS SUCCESSFUL\n");  fflush(stdout);
}

void ArpMidiListTest1::TestBasic01()
{
	ArpMidiList	*list = new ArpMidiList();
	assert(list != NULL);
	ArpMidiListAccess		fakeAccess(*list, ArpIndexedListAccess::AccessWrite);
	access = &fakeAccess;

	AddNoteOn(45, 127, 0);
	access->Print();
	NoteOnShouldBe(45, 127, 0);
		
	access->DeleteNodeContents();
	access->DeleteList();
}


/***************************************************************************
* Private - GENERAL SUPPORT
***************************************************************************/

bool ArpMidiListTest1::NoteOnShouldBe(uchar noteArg, uchar velocityArg,
									uint32 timeArg, int32 findNode)
{
	ArpMidiNode		*node;
	if (access->NodeAt(timeArg, (ArpIndexedNode**)&node, 0) != B_OK) {
		printf("NoteOnShouldBe(note %d vel %d time %li)  ERROR!  Event should exist 1\n", noteArg, velocityArg, timeArg);
		fflush(stdout);
		assert(false);
		return false;
	}

	if (node->Time() != timeArg) {
		printf("NoteOnShouldBe(note %d vel %d time %li)  ERROR!  Event should exist 2\n", noteArg, velocityArg, timeArg);
		fflush(stdout);
		assert(false);
		return false;
	}

	for (int32 k=0; k<findNode; k++) {
		node = (ArpMidiNode*)node->next;
		if (node == NULL) {
			printf("NoteOnShouldBe(note %d vel %d time %li)  ERROR!  Event should exist 3\n", noteArg, velocityArg, timeArg);
			fflush(stdout);
			assert(false);
			return false;
		}
	}
	
	ArpMidiEvent	*event = node->Event();
	if (event->Type() == event->NOTEON_EVENT
			&& ( ((ArpMidiNoteOn*)event)->Note() == noteArg)
			&& ( ((ArpMidiNoteOn*)event)->Velocity() == velocityArg) ) {
		return true;
	}

	printf("NoteOnShouldBe(note %d vel %d time %li)  ERROR!  Event should exist 4\n", noteArg, velocityArg, timeArg);
	fflush(stdout);
	assert(false);
	return false;
}


/***************************************************************************
* Private - GENERAL SUPPORT
***************************************************************************/

bool ArpMidiListTest1::AddNoteOn(uchar noteArg, uchar velocityArg,
									uint32 timeArg)
{
	ArpMidiNoteOn	*event;
	if ((event = new ArpMidiNoteOn(noteArg, velocityArg, timeArg)) == NULL)
		return false;

	if (AddEvent(event)) return true;
	delete event;
	return false;
}

bool ArpMidiListTest1::AddEvent(ArpMidiEvent *event)
{
	if (access->Add(event) != B_OK) {
		printf("AddEvent()  ERROR!  Should be allowed to add an event\n");
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}
