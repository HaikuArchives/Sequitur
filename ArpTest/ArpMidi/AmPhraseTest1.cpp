
// Export symbols in ArpMidi library from this application.
#define _BUILDING_AmKernel 1

#include <Debug.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmEvents.h"
#include "AmKernel/AmPhrase.h"
#include "AmPhraseTest1.h"
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
	AmPhraseTest1 *myApplication;
	myApplication = new AmPhraseTest1();
	
	delete myApplication;
	return 0;
}




const char *app_signature = "application/x-vnd.Arp-quicklaunch";

AmPhraseTest1::AmPhraseTest1()
		  :BApplication(app_signature)
{
	TestBasic01();
	TestFindNode01();
	printf("*********************TESTS SUCCESSFUL\n");  fflush(stdout);
}

void AmPhraseTest1::TestBasic01()
{
	printf("*********************TestBasic01()\n");  fflush(stdout);
	AmPhrase		phrase;
	phrase.Add( new AmTempoChange(29, PPQN * 29) );
	phrase.Add( new AmTempoChange(5, PPQN * 5) );


printf("anything bad happening?\n");
	AmPhraseEvent*	pe = new AmPhraseEvent();
	pe->Phrase().Add( new AmNoteOn( 64, 100, PPQN*400 ) );
	phrase.Add( pe );
	pe = new AmPhraseEvent();
	pe->Phrase().Add( new AmNoteOn( 60, 90, PPQN*29 ) );
	phrase.Add( pe );
	pe = new AmPhraseEvent();
	pe->Phrase().Add( new AmNoteOn( 50, 80, 0 ) );
	phrase.Add( pe );

	printf("                     TestBasic01() successful\n");  fflush(stdout);
}

void AmPhraseTest1::TestFindNode01()
{
	printf("*********************TestFindNode01()\n");  fflush(stdout);
	AmPhrase		phrase;
	phrase.Add( new AmTempoChange(10, 0) );
	phrase.Add( new AmTempoChange(20, PPQN) );
	phrase.Add( new AmTempoChange(30, PPQN * 30) );
	AssertTempo( phrase.FindNode( 0, FORWARDS_SEARCH ), 10 );
	AssertTempo( phrase.FindNode( 1, FORWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( PPQN - 1, FORWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( PPQN, FORWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( PPQN + 1, FORWARDS_SEARCH ), 30 );
	AssertTempo( phrase.FindNode( PPQN * 15, FORWARDS_SEARCH ), 30 );
	AssertTempo( phrase.FindNode( (PPQN * 30) - 1, FORWARDS_SEARCH ), 30 );
	AssertTempo( phrase.FindNode( PPQN * 30, FORWARDS_SEARCH ), 30 );
	AssertNull(  phrase.FindNode( (PPQN * 30) + 1, FORWARDS_SEARCH ) );

	AssertTempo( phrase.FindNode( 0, BACKWARDS_SEARCH ), 10 );
	AssertTempo( phrase.FindNode( 1, BACKWARDS_SEARCH ), 10 );
	AssertTempo( phrase.FindNode( PPQN - 1, BACKWARDS_SEARCH ), 10 );
	AssertTempo( phrase.FindNode( PPQN, BACKWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( PPQN + 1, BACKWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( PPQN * 15, BACKWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( (PPQN * 30) - 1, BACKWARDS_SEARCH ), 20 );
	AssertTempo( phrase.FindNode( PPQN * 30, BACKWARDS_SEARCH ), 30 );
	AssertTempo( phrase.FindNode( (PPQN * 30) + 1, BACKWARDS_SEARCH ), 30 );

	phrase.DeleteEvents();
	printf("                     TestFindNode01() successful\n");  fflush(stdout);
}

void AmPhraseTest1::AssertTempo(AmNode* node, uint32 tempo) const
{
	printf("\tTempo should equal %ld\n", tempo);
	assert( node );
	AmTempoChange*	event = dynamic_cast<AmTempoChange*>( node->Event() );
	assert( event );
	assert( event->Tempo() == tempo );
}

void AmPhraseTest1::AssertNull(AmNode* node) const
{
printf("\tNode should be null\n");
	assert( !node );
}