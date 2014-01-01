/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "MDust/learnView.h"
#include "MDust/MPlanet.h"
#include "MDust/MRhythmStar.h"
#include "MDust/MRhythmDust.h"
#include <unistd.h>
#include <opengl/GL/glu.h>

bool steadyFlag;
sem_id read_board_sem, write_board_sem;

static long lifeThread(learnView *);
static long drawThread(learnView *);
static long frameThread(learnView *);

static GLfloat spin = 0.0;

static GLfloat xrotate = 0.0;
static GLfloat yrotate = 0.0;
void spinDisplay(void)
{
	spin = spin + 2.0;
	if (spin > 360.0) spin = spin - 360.0;

	xrotate = xrotate + 5.0;
	if (xrotate > 360.0) xrotate = xrotate - 360.0;
	yrotate = yrotate + 1.0;
	if (yrotate > 360.0) yrotate = yrotate - 360.0;

}

typedef float vec3_t[3]; 

/* Vertex data for our cube */
vec3_t cube[] = 
{
    {-0.5,-0.5,0.5},
    {0.5,-0.5,0.5},
    {0.5,0.5,0.5},
    {-0.5,0.5,0.5},    

    {-0.5,-0.5,-0.5},
    {0.5,-0.5,-0.5},
    {0.5,0.5,-0.5},
    {-0.5,0.5,-0.5},

    {-0.5,0.5,0.5},
    {0.5,0.5,0.5},
    {0.5,0.5,-0.5},
    {-0.5,0.5,-0.5},

    {-0.5,-0.5,0.5},
    {0.5,-0.5,0.5},
    {0.5,-0.5,-0.5},
    {-0.5,-0.5,-0.5},

    {-0.5,-0.5,-0.5},
    {-0.5,-0.5,0.5},
    {-0.5,0.5,0.5},
    {-0.5,0.5,-0.5},

    {0.5,-0.5,-0.5},
    {0.5,-0.5,0.5},
    {0.5,0.5,0.5},
    {0.5,0.5,-0.5},
};

/* Colors for our cube */
vec3_t colors[] = 
{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 1.0f},
};
/* Amount to rotate */
vec3_t rot = {0,0,0};
/* Position of our eye or camera */
vec3_t eye = {0,0,30};


/* Draws the colored cube.  There are 24 verts in the cube data aboce so we have 
to go throught the array 24 times.  This will result in 6 faces which make up
our cube.  
*/
void drawCube (void)
{
    int i;

    glBegin (GL_QUADS);
        for (i = 0; i < 24; i ++)
        {
//            glColor4f ( (colors[i % 4]).[0], (colors[i % 4]).[1], (colors[i % 4]).[2], 0.5);
            glColor4f ( (colors[i % 4])[0], (colors[i % 4])[1], (colors[i % 4])[2], 0.5);
//            glColor3fv (colors[i % 4]);
            glVertex3fv (cube[i]);	
        }
    glEnd ();
}

// FIX: the Frame() and IncrementFrame() methods should have
// a BAutolock on them so the frame value isn't read while written.

learnView::learnView(BRect R)
		: BGLView(	R,
					"lifeglview",
					B_FOLLOW_ALL,
					0, 
					BGL_RGB | BGL_DEPTH | BGL_DOUBLE ),
		mSpace(0), mFrame(0)
{
	mSpace = new MSpace(BOARD_SIZE, BOARD_SIZE, BOARD_SIZE);
	continuous = false;
	singleStep = true;
	xangle = yangle = zangle = 0;
	_QuitRequested = false;
	
	MParticleI*		particle;
	MRhythmStar*	rStar;
	MStar*			star;
	if ((particle = new MPlanet(0.4, -0.2, 0.5, 1, 0)) != 0) mSpace->AddParticle(particle);
//	if ((particle = new MPlanet(0.1, 0.7, -0.1, 1, 1)) != 0) mSpace->AddParticle(particle);
	if ((star = new MStar(1, 0, -1, 1)) != 0) {
		mSpace->AddParticle(star);
//		star->SetControlNumber(10);
	}
	if ((star = new MStar(0.3, 0.7, -0.2, 1)) != 0) {
		mSpace->AddParticle(star);
		star->SetControlNumber(7);
	}
	if ((star = new MStar(-0.4, -0.6, 0.3, 1)) != 0) {
		mSpace->AddParticle(star);
//		star->SetControlNumber(1);
	}
	if ((rStar = new MRhythmStar(-1, -1, -0.8, 1)) != 0) {
		mSpace->AddParticle(rStar);
	}

	MRhythmDust*	rDust;
	if ( (rDust = new MRhythmDust(0, 0, 0, 0)) != 0) mSpace->AddDust(rDust);
	
	// initialize the semaphores
	::read_board_sem = create_sem(0, "read");
	::write_board_sem = create_sem(1, "write");
}

learnView::~learnView()
{
	delete mSpace;
}

MSpace* learnView::Space() const
{
	return mSpace;
}

uint32 learnView::Frame() const
{
	return mFrame;
}

void learnView::IncrementFrame(uint32 delta)
{
	mFrame += delta;
}

void
learnView::AttachedToWindow()
{
	BGLView::AttachedToWindow();
	LockGL();

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FILL);
//	glShadeModel(GL_SMOOTH);
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport( 0, 0, (GLsizei)Bounds().Width(), (GLsizei)Bounds().Height() );
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);
	glMatrixMode(GL_MODELVIEW);

	UnlockGL();

	// start a thread to compute the board generations
	lifeTID = spawn_thread((thread_entry)lifeThread, "lifeThread", 
							B_NORMAL_PRIORITY, this);
	resume_thread(lifeTID);

	// start a thread which does all of the drawing
	drawTID = spawn_thread((thread_entry)drawThread, "drawThread",
								B_NORMAL_PRIORITY, this);
	resume_thread(drawTID);

	// Start 'er up!
	continuous = true;
	Start();
}

void
learnView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	 case 'strt':
		Start();
	 	break;
	 case CONTINUOUS:
	 	continuous = (continuous) ? false : true;
	 	BMenuItem *item;
	 	msg->FindPointer("source", (void**)&item);
	 	item->SetMarked(continuous);
	 	break;
	 case SINGLE_STEP:
	 	singleStep = true;
	 	break;
	 default:
	 	BView::MessageReceived(msg);
	}
}

void learnView::Display()
{
#if 0
	spinDisplay();
//	xrotate = 45;
	LockGL();
//	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear ( GL_COLOR_BUFFER_BIT );
	glColor3f(1.0, 1.0, 1.0);
	glLoadIdentity();
	gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glScalef(1.0, 1.0, 1.0);
	glRotatef(xrotate, 1, 0, 0);
	glRotatef(yrotate, 0, 1, 0);
	DrawFrame();
	glFlush();

	//printf("calling swap buffers...\n");
	SwapBuffers();
	
	UnlockGL();
#endif
}


void
learnView::ExitThreads()
{
	status_t status;
	int32 count = 0;
	_QuitRequested = true;

	release_sem(write_board_sem);
	release_sem(read_board_sem);
	
	while (Window()->IsLocked()) {
		Window()->Unlock();
		count++;
	};
	wait_for_thread(drawTID, &status);
	wait_for_thread(lifeTID, &status);
	if (frameTID >= 0) kill_thread(frameTID);
	
	while (count--) Window()->Lock();
}

void learnView::Start()
{
	if (frameTID >= 0) kill_thread(frameTID);
	if (mSpace == 0) return;
	mFrame = 0;
	// start a thread to increment the frame
	frameTID = spawn_thread((thread_entry)frameThread, "frameThread", 
							B_NORMAL_PRIORITY, this);
	resume_thread(frameTID);
}

void
learnView::SpinCalc()
{
}

void learnView::DrawFrame()
{
	if (mSpace != 0) mSpace->Draw(this);
}

static long drawThread(learnView *mv)
{
	int generations = -1;
	while(!(mv->QuitPending()))
	{
		mv->SpinCalc();
		mv->Display();
		
		if(mv->continuousMode() && steadyFlag)
		{
			mv->continuousMode(false);
		}

		if(mv->continuousMode() || mv->singleStepMode())
		{
			if(acquire_sem_etc(read_board_sem, 1, B_TIMEOUT, 
								100) != B_TIMED_OUT)
//			if (mv->mSpaceLock.ReadLock())
			{
//printf("draw!\n");
				//printf("got semaphore...\n");
				generations++;
				mv->singleStepMode(false);
				release_sem(write_board_sem);
//				mv->mSpaceLock.ReadUnlock();
			}
		}
		else
		{
			// if the display isn't rotating, give the cpu a break!
			if(!mv->spinning())
				snooze(500000);		
		}
	}
	
	return 1;
}

static long
lifeThread(learnView *mv)
{
	uint32		lastFrame = mv->Frame();
	
	while(!(mv->QuitPending()))
	{
//printf("try!\n");
//		if (mv->mSpaceLock.WriteLock()) {
		acquire_sem(write_board_sem);
//printf("\tsucceed!\n");
			if(mv->QuitPending())
			{
				// semaphore was released from ExitThreads
				break;
			}
			if ( (mv->Frame() != lastFrame)
					&& (mv->Space() != 0) ) {
				lastFrame = mv->Frame();
				mv->Space()->Increment(false);
			}
			// "post" the next generation		

//			mv->mSpaceLock.WriteUnlock();
//		}
		release_sem(read_board_sem);
	}

	return 1;
}

static long frameThread(learnView *mv)
{
	while(!(mv->QuitPending()))
	{
//		acquire_sem(write_board_sem);
		if(mv->QuitPending()) break;
		snooze(2000);
//		snooze(1000000);
		mv->IncrementFrame(1);
//		release_sem(read_board_sem);
	}
	return 1;
}

