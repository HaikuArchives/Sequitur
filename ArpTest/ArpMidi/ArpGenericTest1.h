#include <Application.h>

#include "ArpTestNode.h"
#include "ArpIndexedList.h"

class ArpGenericTest1Application : public BApplication {
	public:
		ArpGenericTest1Application();

	private:
		void TestAddNode01();
		void TestChainHeadFrom01();
		void TestHeadNode01();
		void TestTailNode01();
		void TestNodeAt_Backwards01();
		void TestNodeAt_Forwards01();
		void TestCount01();
		void TestRemoveNode01();

		// FUNCTION TESTS
		bool ChainHeadAtIndexShouldBe(int32 indexToHash, int32 index,
						ArpIndexedListAccess *access);
		bool NoChainHeadAtIndex(int32 indexToHash,
						ArpIndexedListAccess *access);
		bool HeadNodeShouldBe(int32 index, ArpIndexedListAccess *access);
		bool TailNodeShouldBe(int32 index, ArpIndexedListAccess *access);
		bool NodeAtShouldBe(int32 indexToFind, int32 index, int32 findNode,
						int32 direction, ArpIndexedListAccess *access);
		bool NodeAtShouldNotExist(int32 indexToFind, int32 findNode,
						int32 direction, ArpIndexedListAccess *access);
		bool CountShouldBe(int32 count, ArpIndexedListAccess *access);

		// TEsTING SUPPORT
		bool NodeShouldBe(ArpIndexedNode *node, int32 index);
		
		// GENERAL SUPPORT
		ArpTestNode* NewNode(int32 index);
		bool AddNodeIndex(ArpIndexedListAccess *access, int32 index);
};