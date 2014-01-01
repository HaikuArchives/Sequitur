#include <Application.h>

#include <AmKernel/AmPhrase.h>
#include <AmKernel/AmEvents.h>

class AmNode;

class AmPhraseTest1 : public BApplication
{
public:
	AmPhraseTest1();

	void TestBasic01();
	void TestFindNode01();

private:
	void	AssertTempo(AmNode* node, uint32 tempo) const;
	void	AssertNull(AmNode* node) const;
};
