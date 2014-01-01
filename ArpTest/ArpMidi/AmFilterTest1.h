#include <Application.h>

#include <AmKernel/AmPhrase.h>
#include <AmKernel/AmStdFilters.h>
#include <AmKernel/AmFilterHolder.h>

class AmEventI;

class AmFilterTest1 : public BApplication {
	public:
		AmFilterTest1();

		void TestBasic01();

	private:
		void PrintEventChain(AmEventI* first);
		void DoBigTest();
};
