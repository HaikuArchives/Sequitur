
#include <kernel/OS.h>
#include <cstdio>

sem_id sem;

static void print_error(status_t err)
{
	if( err != B_NO_ERROR ) printf("Error returned: %s\n",strerror(err));
}

static int32 my_thread(void* arg)
{
	printf("Thread: Started.  Locking on semaphore\n");
	print_error( acquire_sem(sem) );
	printf("Thread: Finished.\n");
	return 0;
}

int main()
{
	sem = create_sem(0, "lock sem");
	if( sem < 0 ) {
		printf("Error creating semaphore: %s\n",strerror(sem));
		return sem;
	}
	
	thread_id thread;
	printf("Main: Spawning thread\n");
	thread = spawn_thread(my_thread, "Test Thread",
							B_NORMAL_PRIORITY, NULL);
	if( thread < 0 ) {
		printf("Error starting test thread: %s\n",strerror(thread));
		return thread;
	}
	
	printf("Main: Starting thread\n");
	print_error( resume_thread(thread) );
	
	printf("Main: Waiting a bit\n");
	print_error( snooze(100000) );
	
	printf("Main: Suspending thread\n");
	print_error( suspend_thread(thread) );
	
	printf("Main: Resuming thread\n");
	print_error( resume_thread(thread) );
	
	printf("Main: Waiting for thread\n");
	int32 ret;
	print_error( wait_for_thread(thread,&ret) );
	
	printf("Main: All done\n");
	return 0;
}
