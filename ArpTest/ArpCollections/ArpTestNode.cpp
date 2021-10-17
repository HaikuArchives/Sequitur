#include <cassert>

#ifndef ARP_TEST_NODE
#include "ArpTestNode.h"
#endif

int32	indexCount = 0;


ArpTestNode::ArpTestNode(ArpTestIndex *dataArg)
		: ArpIndexedNode(dataArg) {
}

int32 ArpTestNode::Index() {
	assert(rawData != NULL);
	return ((ArpTestIndex*)rawData)->Index();
}

void ArpTestNode::pDeleteContents() {
	assert(rawData != NULL);
	delete ((ArpTestIndex*)rawData);
}

void ArpTestNode::PrintContents() {
	assert(rawData != NULL);
	char		buf[128];
	sprintf(buf, "\tTestNode on %li\n", ((ArpTestIndex*)rawData)->Index());
	printf(buf);
	fflush(stdout);
}


/***************************************************************************
* class ArpTestIndex
***************************************************************************/

ArpTestIndex::ArpTestIndex(int32 indexArg) {
	index = indexArg;

	// debug...  shared with other constructor
	indexCount++;
	printf("created index obj %d\n", indexCount);
}

ArpTestIndex::~ArpTestIndex() {
	//  debug... make sure we're releasing all the data
	if (indexCount == 1) {
		printf("deleted FINAL index obj %li\n", indexCount);
	} else {
		printf("deleted index obj %li\n", indexCount);
	}
	indexCount--;
}

int32 ArpTestIndex::Index() {
	return index;
}