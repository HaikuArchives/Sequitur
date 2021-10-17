
#include <cstdio>

#include <Application.h>

#include "FFont.h"

int main()
{
	BApplication app("application/x-vnd.ARP-TestFFont");
	printf("Running built-in FFont test...\n");
	FFont::Test();
	return 0;
}
