#ifndef _Gehnaphore_H
#include <Gehnaphore.h>
#endif
#ifndef _ATOMIC_H
#include <atomic.h>
#endif
#ifndef _DEBUG_H
#include <Debug.h>
#endif

void init_gehnaphore(int32* value)
{
	*value = -1;
}

void lock_gehnaphore(int32* value)
{
	int32 chainedThread=-1,myThread=find_thread(NULL),msg=-1;
	bool success = false;
	
	while (!success) {
		while (!success && (chainedThread == -1))
			success = cmpxchg32(value,&chainedThread,0);
		while (!success && (chainedThread != -1))
			success = cmpxchg32(value,&chainedThread,myThread);
	};

	do {
		if (chainedThread != -1)
			while ( (msg=receive_data(NULL,NULL,0)) == B_INTERRUPTED ) ;
		if ((chainedThread > 0) && (chainedThread != msg))
			while ( send_data(chainedThread,msg,NULL,0) == B_INTERRUPTED ) ;
	} while ((chainedThread > 0) && (chainedThread != msg));
}

void unlock_gehnaphore(int32* value)
{
	int32 chainedThread=0,myThread=find_thread(NULL);
	bool success;
	
	success = cmpxchg32(value,&chainedThread,-1);
	if (!success && (chainedThread == myThread))
		success = cmpxchg32(value,&chainedThread,-1);
	
	if (!success) {
		while ( send_data(chainedThread,myThread,NULL,0) == B_INTERRUPTED ) ;
	}
}
