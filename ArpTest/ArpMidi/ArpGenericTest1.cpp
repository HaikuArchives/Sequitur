

#include <Debug.h>
#include <assert.h>

#include "ArpGenericTest1.h"


main(int argc, char* argv[])
{
	ArpGenericTest1Application *app;
	app = new ArpGenericTest1Application();


//	app->Run();
	
	delete app;
	return 0;
}




const char *app_signature = "application/x-vnd.Arp-quicklaunch";

ArpGenericTest1Application::ArpGenericTest1Application()
		  :BApplication(app_signature)
{
	printf("*********************TestAddNode01\n");  fflush(stdout);
	TestAddNode01();
	printf("*********************TestChainHeadFrom01\n");  fflush(stdout);
	TestChainHeadFrom01();
	printf("*********************TestHeadNode01\n");  fflush(stdout);
	TestHeadNode01();
	printf("*********************TestTailNode01\n");  fflush(stdout);
	TestTailNode01();
	printf("*********************TestNodeAt_Backwards01\n");  fflush(stdout);
	TestNodeAt_Backwards01();
	printf("*********************TestNodeAt_Forwards01\n");  fflush(stdout);
	TestNodeAt_Forwards01();
	printf("*********************TestCount01\n");  fflush(stdout);
	TestCount01();
	printf("*********************TESTS SUCCESSFUL\n");  fflush(stdout);
}

void ArpGenericTest1Application::TestAddNode01()
{
	ArpIndexedNode	*node = new ArpIndexedNode(NULL);
	ArpIndexedList	*list = new ArpIndexedList();
	assert(node != NULL);
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	if (access.AddNode(node) == B_OK) {
		printf("Test1()  ERROR!  Shouldn't be allowed to add!\n");  fflush(stdout);
	} else {
		delete node;
	}
	
	AddNodeIndex(&access, 30);
	
	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestChainHeadFrom01()
{
	ArpIndexedList	*list = new ArpIndexedList(10);	
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	AddNodeIndex(&access, 0);
	AddNodeIndex(&access, 9);
	AddNodeIndex(&access, 10);
	AddNodeIndex(&access, 11);
	AddNodeIndex(&access, 33);
	AddNodeIndex(&access, 31);
	AddNodeIndex(&access, 44);

//	access.Print();

	ChainHeadAtIndexShouldBe(0, 0, &access);
	ChainHeadAtIndexShouldBe(9, 0, &access);
	ChainHeadAtIndexShouldBe(10, 10, &access);
	ChainHeadAtIndexShouldBe(11, 10, &access);
	ChainHeadAtIndexShouldBe(14, 10, &access);
	NoChainHeadAtIndex(20, &access);
	NoChainHeadAtIndex(21, &access);
	NoChainHeadAtIndex(29, &access);
	ChainHeadAtIndexShouldBe(30, 31, &access);
	ChainHeadAtIndexShouldBe(39, 31, &access);
	ChainHeadAtIndexShouldBe(40, 44, &access);
	ChainHeadAtIndexShouldBe(44, 44, &access);
	NoChainHeadAtIndex(50, &access);

	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestHeadNode01()
{
	ArpIndexedList	*list = new ArpIndexedList(10);	
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	ArpIndexedNode	*node;
	if (access.HeadNode(&node) == B_OK) {
		printf("ERROR  TestHeadNode01() -- Head node should not exist\n");
		fflush(stdout);
		assert(false);
	}
	
	AddNodeIndex(&access, 44);
	HeadNodeShouldBe(44, &access);
	AddNodeIndex(&access, 19);
	HeadNodeShouldBe(19, &access);
	AddNodeIndex(&access, 24);
	HeadNodeShouldBe(19, &access);
	AddNodeIndex(&access, 1);
	HeadNodeShouldBe(1, &access);
	AddNodeIndex(&access, 0);
	HeadNodeShouldBe(0, &access);

	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestTailNode01()
{
	ArpIndexedList	*list = new ArpIndexedList(10);	
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	ArpIndexedNode	*node;
	if (access.TailNode(&node) == B_OK) {
		printf("ERROR  TestTailNode01() -- Tail node should not exist\n");
		fflush(stdout);
		assert(false);
	}
	
	AddNodeIndex(&access, 9);
	TailNodeShouldBe(9, &access);
	AddNodeIndex(&access, 10);
	TailNodeShouldBe(10, &access);
	AddNodeIndex(&access, 0);
	TailNodeShouldBe(10, &access);
	AddNodeIndex(&access, 32);
	TailNodeShouldBe(32, &access);

	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestNodeAt_Backwards01()
{
	ArpIndexedList	*list = new ArpIndexedList(10);	
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	ArpIndexedNode	*node;
	NodeAtShouldNotExist(0, 0, ARP_BACKWARDS, &access);
	NodeAtShouldNotExist(43, 0, ARP_BACKWARDS, &access);
	
	AddNodeIndex(&access, 0);
	// Test the ability to find the requested.
	NodeAtShouldBe(0, 0, 0, ARP_BACKWARDS, &access);
	// Test the ability to find the requested node, even when in a different chain.
	NodeAtShouldBe(9, 0, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(30, 0, 0, ARP_BACKWARDS, &access);
	// Test the ability to find multiple nodes at the same index is failing properly.
	NodeAtShouldNotExist(30, 1, ARP_BACKWARDS, &access);
	
	// Now make another node right next to the first one and make sure nothing
	// flubs up.
	AddNodeIndex(&access, 1);
	// Test the ability to find the requested node.
	NodeAtShouldBe(0, 0, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(1, 1, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(3, 1, 0, ARP_BACKWARDS, &access);

	AddNodeIndex(&access, 13);
	NodeAtShouldBe(12, 1, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(13, 13, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(19, 13, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(20, 13, 0, ARP_BACKWARDS, &access);
	NodeAtShouldBe(21, 13, 0, ARP_BACKWARDS, &access);

	AddNodeIndex(&access, 13);
	NodeAtShouldBe(21, 13, 1, ARP_BACKWARDS, &access);

	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestNodeAt_Forwards01()
{
	ArpIndexedList	*list = new ArpIndexedList(10);	
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	ArpIndexedNode	*node;
	NodeAtShouldNotExist(0, 0, ARP_FORWARDS, &access);
	NodeAtShouldNotExist(43, 0, ARP_FORWARDS, &access);
	
	AddNodeIndex(&access, 0);
	// Test the ability to find the requested.
	NodeAtShouldBe(0, 0, 0, ARP_FORWARDS, &access);
	// Test the ability to find multiple nodes at the same index is failing properly.
	NodeAtShouldNotExist(0, 1, ARP_FORWARDS, &access);
	
	// Now make another node right next to the first one and make sure nothing
	// flubs up.
	AddNodeIndex(&access, 1);
	// Test the ability to find the requested node when two exist.
	NodeAtShouldBe(0, 0, 0, ARP_FORWARDS, &access);
	NodeAtShouldBe(1, 1, 0, ARP_FORWARDS, &access);

	AddNodeIndex(&access, 34);
	// Test the ability to find the requested node across chain boundaries.
	NodeAtShouldBe(2, 34, 0, ARP_FORWARDS, &access);
	NodeAtShouldNotExist(2, 1, ARP_FORWARDS, &access);

	// Test the ability to find the second in a series of nodes at the
	// same index.
	AddNodeIndex(&access, 34);
	NodeAtShouldBe(2, 34, 1, ARP_FORWARDS, &access);


	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestCount01()
{
	ArpIndexedList	*list = new ArpIndexedList(10);	
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	CountShouldBe(0, &access);
	AddNodeIndex(&access, 10);
	CountShouldBe(1, &access);
	AddNodeIndex(&access, 9);
	CountShouldBe(2, &access);
	AddNodeIndex(&access, 65);
	CountShouldBe(3, &access);
	AddNodeIndex(&access, 32);
	CountShouldBe(4, &access);

	access.DeleteNodeContents();
	access.DeleteList();
}

void ArpGenericTest1Application::TestRemoveNode01()
{
	ArpIndexedNode	*node = new ArpIndexedNode(NULL);
	ArpIndexedList	*list = new ArpIndexedList();
	assert(node != NULL);
	assert(list != NULL);
	ArpIndexedListAccess	access(*list, ArpIndexedListAccess::AccessWrite);

	if (access.AddNode(node) == B_OK) {
		printf("Test1()  ERROR!  Shouldn't be allowed to add!\n");  fflush(stdout);
	} else {
		delete node;
	}
	
	AddNodeIndex(&access, 30);
	
	access.DeleteNodeContents();
	access.DeleteList();
}


/***************************************************************************
* Private - FUNCTION TESTS
***************************************************************************/

// Clients must supply an index that will be hashed to a chain head.  The chain
// head found (none is an error condition) will have its actual index compared
// with the supplied index argument.  If they don't match, an error results.
bool ArpGenericTest1Application::ChainHeadAtIndexShouldBe(int32 indexToHash,
				int32 index, ArpIndexedListAccess *access) {
	ArpIndexedNode	*node;
	if (access->ChainHeadFrom(indexToHash, &node) != B_OK) {
		printf("ERROR  ChainHeadAtIndexShouldBe(%li, %li) -- No head\n", indexToHash, index);
		fflush(stdout);
		assert(false);
		return false;
	}
	if (node->Index() != index) {		
		printf("ERROR  ChainHeadAtIndexShouldBe(%li, %li) -- Wrong index (%li)\n", indexToHash, index, node->Index());
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}

// Clients wmust supply an index that gets hashed to a chain head.  If a chain head
// exists, that is an error.
bool ArpGenericTest1Application::NoChainHeadAtIndex(int32 indexToHash,
				ArpIndexedListAccess *access)
{
	ArpIndexedNode	*node;
	if (access->ChainHeadFrom(indexToHash, &node) == B_OK) {
		printf("ERROR  NoChainHeadAtIndex(%li) -- There should not be a head (%li)\n", indexToHash, node->Index());
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}	

bool ArpGenericTest1Application::HeadNodeShouldBe(int32 index,
				ArpIndexedListAccess *access)
{
	ArpIndexedNode	*node;
	if (access->HeadNode(&node) != B_OK) {
		printf("ERROR  HeadNodeShouldBe(%li) -- There should be a head\n", index);
		fflush(stdout);
		assert(false);
		return false;
	}
	return(NodeShouldBe(node, index));
}

bool ArpGenericTest1Application::TailNodeShouldBe(int32 index,
				ArpIndexedListAccess *access)
{
	ArpIndexedNode	*node;
	if (access->TailNode(&node) != B_OK) {
		printf("ERROR  TailNodeShouldBe(%li) -- There should be a tail\n", index);
		fflush(stdout);
		assert(false);
		return false;
	}
	return(NodeShouldBe(node, index));
}

bool ArpGenericTest1Application::NodeAtShouldBe(int32 indexToFind,
				int32 index, int32 findNode, int32 direction,
				ArpIndexedListAccess *access)
{
	ArpIndexedNode	*node;
	char	buf[64];
	sprintf(buf, "Unknown direction");
	if (direction == ARP_FORWARDS) sprintf(buf, "%s", "ARP_FORWARDS");
	if (direction == ARP_BACKWARDS) sprintf(buf, "%s", "ARP_BACKWARDS");
	if (access->NodeAt(indexToFind, &node, findNode, direction) != B_OK) {
		printf("ERROR  NodeAtShouldBe(%li, %li, %li, %s) -- Node should exist\n", indexToFind, index, findNode, buf);
		fflush(stdout);
		assert(false);
		return false;
	}
	if (node->Index() != index) {
		printf("ERROR  NodeAtShouldBe(%li, %li, %li, %s) -- Wrong node found (%li)\n", indexToFind, index, findNode, buf, node->Index());
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}

bool ArpGenericTest1Application::NodeAtShouldNotExist(int32 indexToFind,
				int32 findNode, int32 direction,
				ArpIndexedListAccess *access)
{
	ArpIndexedNode	*node;
	if (access->NodeAt(indexToFind, &node, findNode, direction) == B_OK) {
		printf("ERROR  NodeAtShouldNotExist(%li, %li) -- Node should not exist\n", indexToFind, findNode);
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}

bool ArpGenericTest1Application::CountShouldBe(int32 count,
				ArpIndexedListAccess *access)
{
	if (access->Count() != count) {
		printf("ERROR  CountShouldBe(%li) -- Count is wrong (%li)\n", count, access->Count());
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}


/***************************************************************************
* Private - TESTING SUPPORT
***************************************************************************/

bool ArpGenericTest1Application::NodeShouldBe(ArpIndexedNode *node, int32 index)
{
	if (node == NULL) {
		printf("ERROR  NodeShouldBe(NULL, %li) -- Node is NULL\n", index);
		fflush(stdout);
		assert(false);
		return false;
	}
	if (node->Index() != index) {
		printf("ERROR  NodeShouldBe(%li, %li) -- Node is wrong\n", node->Index(),  index);
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}


/***************************************************************************
* Private - GENERAL SUPPORT
***************************************************************************/

ArpTestNode* ArpGenericTest1Application::NewNode(int32 index)
{
	ArpTestIndex	*newIndex = new ArpTestIndex(index);
	assert(newIndex != NULL);
	ArpTestNode		*newNode = new ArpTestNode(newIndex);
	assert(newNode != NULL);
	return newNode;
}

bool ArpGenericTest1Application::AddNodeIndex(ArpIndexedListAccess *access,
				int32 index)
{
	ArpTestNode		*node = NewNode(index);
	if (access->AddNode(node) != B_OK) {
		printf("AddNodeIndex()  ERROR!  Should be allowed to add node %li\n", index);
		fflush(stdout);
		assert(false);
		return false;
	}
	return true;
}
