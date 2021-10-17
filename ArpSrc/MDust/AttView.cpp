#include <cstdio>
#include <unistd.h>
#include "MDust/AttView.h"
#include "MDust/ExBitmapCache.h"
#include "MDust/MStar.h"

static int32 drawThread(AttView*);

AttView::AttView(BRect R)
	: BView(R, "attview", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE),
	mSpace(0),
	singleStep(false), mStep(0), _QuitRequested(false)
{
	mSpace = new MSpace(100, 100, 100);
}

AttView::~AttView()
{
	delete mSpace;
}

void AttView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	// start a thread which does all of the drawing
	drawTID = spawn_thread((thread_entry)drawThread, "drawThread",
								B_NORMAL_PRIORITY, this);
	resume_thread(drawTID);
	MakeFocus(true);
}

void AttView::Draw(BRect updateRect)
{
	BView* into = this;
	
	ExBitmapCache* cache = dynamic_cast<ExBitmapCache*>(Window());
	if( cache ) into = cache->StartDrawing(this);
	
	DrawOn(updateRect, into);
	if( cache ) cache->FinishDrawing(into);
}

void AttView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		default:
			inherited::MessageReceived(msg);
	}
}

void AttView::MouseDown(BPoint pt)
{
//	SetMouseEventMask(B_POINTER_EVENTS);
}

void AttView::MouseUp(BPoint pt)
{
	if (mSpace == 0) return;

	BRect	b = Bounds();
	float	halfWidth = b.Width() / 2, halfHeight = b.Height() / 2;
	float	newX = (pt.x - halfWidth) / halfWidth;
	float	newY = (pt.y - halfHeight) / halfHeight;
	MStar*	particle = new MStar(newX, newY, 0, 0.1);
	mSpace->AddParticle(particle);
//	if (mSpace->ParticleAt(1) == 0) {
//		particle->SetFriction(0);
//	}
}

void AttView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
}

void AttView::KeyDown(const char *bytes, int32 numBytes)
{
	// Single byte values can be tested against ascii character codes...
	if (numBytes == 1) {
		if (bytes[0] == 32) {
			if (SingleStepMode()) SingleStepMode(false);
			else SingleStepMode(true);
		} else if (bytes[0] == 122) {
			mStep++;
		}
	}
	inherited::KeyDown(bytes, numBytes);
}

void AttView::CalculateStep(bool print)
{
	if (mSpace != 0) mSpace->Increment(print);
}

void AttView::ExitThreads()
{
	status_t status;
	int32 count = 0;
	_QuitRequested = true;

//	release_sem(write_board_sem);
//	release_sem(read_board_sem);
	
	while (Window()->IsLocked()) {
		Window()->Unlock();
		count++;
	};
	wait_for_thread(drawTID, &status);
	while (count--) Window()->Lock();
}

void AttView::DrawOn(BRect updateRect, BView *view)
{
	BRect		b = Bounds();
	view->SetHighColor(0, 0, 0);
	view->FillRect(b);

	if (mSpace == 0) return;
	MParticleI*		particle;
	for (uint k=0; (particle = mSpace->ParticleAt(k)) != 0; k++) {
		particle->Draw(view, Bounds().Width() / 2, Bounds().Height() / 2);
	}
}



static int32 drawThread(AttView* mv)
{
	uint32	step = mv->Step(), nextStep = 0;
	while(!(mv->QuitPending()))
	{
		if ( !(mv->SingleStepMode())
				|| ( (nextStep = mv->Step()) != step) ) {
			if (mv->Window()->Lock()) {
				bool	print = false;
				if (step != nextStep) print = true;
				mv->Draw( mv->Bounds() );
				mv->CalculateStep(print);
				mv->Window()->Unlock();
			}
			step = nextStep;
		}
		snooze(2000);
//		mv->Display(displayBoard, generations, steadyFlag);
		
/*		if(mv->continuousMode() && steadyFlag)
		{
			mv->continuousMode(false);
		}
						
		if(mv->continuousMode() || mv->singleStepMode())
		{
			if(acquire_sem_etc(read_board_sem, 1, B_TIMEOUT, 
								100) != B_TIMED_OUT)
			{
				//printf("got semaphore...\n");
				if(displayBoard) 
				{
					delete []displayBoard;
				}
			
				displayBoard = board;
				board = NULL;
				generations++;
				mv->singleStepMode(false);
				release_sem(write_board_sem);
			}
		}
		else
		{
			// if the display isn't rotating, give the cpu a break!
			if(!mv->spinning())
				snooze(500000);		
		}
*/
	}
	
	return 1;
}
