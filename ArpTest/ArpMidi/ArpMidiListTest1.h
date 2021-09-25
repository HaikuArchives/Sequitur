#include <Application.h>

#include <ArpMidi/ArpMidiList.h>
#include <ArpMidi/ArpMidiEvents.h>


class ArpMidiListTest1 : public BApplication {
	public:
		ArpMidiListTest1();

		void TestBasic01();

	private:
		ArpMidiListAccess	*access;
		
		// TESTING SUPPORT
		bool NoteOnShouldBe(uchar noteArg, uchar velocityArg,
						uint32 timeArg, int32 findNode = 0);
		
		// GENERAL SUPPORT
		bool AddNoteOn(uchar noteArg, uchar velocityArg, uint32 timeArg); 
		bool AddEvent(ArpMidiEvent *event);
};
