// This is a small class designed just to override the necessary methods
// from ArpIndexedNode so we can test the list behaviour.

#ifndef ARP_TEST_NODE_H
#define ARP_TEST_NODE_H


#ifndef ARP_INDEXED_NODE_H
#include "ArpIndexedNode.h"
#endif


class ArpTestIndex;

class ArpTestNode : public ArpIndexedNode {
	public:
		ArpTestNode(ArpTestIndex *dataArg);

		virtual int32 Index();
		void PrintContents();
		
	private:
		virtual void pDeleteContents();
};


class ArpTestIndex {
	public:
		ArpTestIndex(int32 indexArg);
		~ArpTestIndex();
		
		int32 Index();
	
	private:
		int32	index;
};


#endif 