#include <Window.h>
#include "MDust/AttView.h"
#include "MDust/ExBitmapCache.h"

class AttWin : public BWindow, public ExBitmapCache
{
public:
	AttWin();
	virtual ~AttWin();
	bool QuitRequested();

private:
	AttView	*mView;
};
