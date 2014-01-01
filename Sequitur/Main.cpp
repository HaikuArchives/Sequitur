/* Main.cpp
 */
#include <malloc.h>
#include <stdio.h>
#include "Sequitur/SeqApplication.h"

int main(int argc, char **argv)
{
	SeqApplication	*seq;
	if( !( seq = new SeqApplication() )	) {
		return 1;
	}
	seq->Run();

	delete seq;
	return 0;
}

