#include <View.h>

#include "MDust/MSpace.h"
//#include "MDust/MultiLocker.h"

class AttView : public BView
{
public:
	AttView(BRect R);
	virtual ~AttView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* msg);
	virtual	void MouseDown(BPoint pt);
	virtual	void MouseUp(BPoint pt);
	virtual	void MouseMoved(BPoint pt, uint32 code, const BMessage* msg);
	virtual	void KeyDown(const char *bytes, int32 numBytes);

	void CalculateStep(bool print);
	void ExitThreads();

	inline bool SingleStepMode();
	inline void SingleStepMode(bool val);
	inline uint32 Step();
	inline bool QuitPending();
	inline void QuitPending(bool val);	

protected:
	void DrawOn(BRect updateRect, BView* view);

private:
	typedef BView		inherited;
 	/* The data representing this space
 	 */
 	MSpace*		mSpace;
	
	bool	singleStep;
	uint32	mStep;
	bool	_QuitRequested;

	thread_id drawTID; 
};

// inline functions
bool AttView::SingleStepMode() { return singleStep; }
void AttView::SingleStepMode(bool val) { singleStep = val; }
uint32 AttView::Step() { return mStep; }
bool AttView::QuitPending() { return _QuitRequested; }
void AttView::QuitPending(bool val) { _QuitRequested = val; }

