/**
 * gcc -g -o signaltest signaltest.c -lpthread -lunwind -lunwind-x86_64
 * 
 * signaltest.c 
 * 
 * Tests a simple idea for suspending a thread and inspecting its stack using posix thread signals (pthread_kill()).
 * 
 * 1. Create a simple worker thread that does some (lengthy) work and run it.
 * 2. Pause the main thread for a while so that we can be fairly certain that the worker's signal handler is installed and it is running.
 * 3. In the main thread, lock a mutex that the worker will wait on inside its signal handler.
 * 4. Send SIGUSR1 from the main thread to the worker. The worker's handler tries to lock the mutex. At this point it is suspended.
 * 5. Inspect associated ucontext_t objects (from both the handler and in the main thread).
 *
 */

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>

// #include <libunwind.h>
#include <ucontext.h>

// these variables are essentially the worker thread's state, but
// meant to be (indirectly) accessible to the rest of the program
ucontext_t workerContext;
pthread_mutex_t workerMutex;


// functions that make up the worker thread (two so its stack is
// not entirely trivial)
void worker_signal_handler(int signum, siginfo_t* si, void* ptr);
int do_important_work(int count, int total);



void* worker_thread_func(void* ptr) {
	// find the worker's context
	getcontext(&workerContext);

	{
		printf("[%s] installing signal handler\n", __FUNCTION__);

		// install a signal handler for our worker
		sigset_t sigSet;
		sigemptyset(&sigSet);
		sigaddset(&sigSet, SIGUSR1);

		pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL);

		struct sigaction sa;
		memset(&sa, 0, sizeof(struct sigaction));

		sa.sa_sigaction = worker_signal_handler;
		sa.sa_flags |= SA_SIGINFO;

		if (sigaction(SIGUSR1, &sa, NULL)) {
			perror("sigaction"); exit(-2);
		}
	}

	{
		// do some "important" work
		int i = 0;
    	const int cnt = 12;

		for (i = 0; i < cnt; i++) {
			do_important_work(i + 1, cnt);
		}
	}

	return NULL;
}

int do_important_work (int count, int total) {
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = 500000000;

	printf("[%s] [count=%d/total=%d]\n", __FUNCTION__, count, total);
	nanosleep(&ts, NULL);
	return 0;
}



// worker thread's custom signal handler; kernel will
// create an extra frame on top of the thread's stack
// to run it
//
void worker_signal_handler(int signal, siginfo_t* signal_info, void* raw_context) {
	(void) signal;

	// assumes <signal> corresponds to ctrl+c
	const char* msg = "[handler] caught SIGUSR1!\n";

	if (write(0, msg, strlen(msg)) == -1) {
		perror("write");
		return;
	}

	ucontext_t currContext;
	ucontext_t* pCurrContext = (ucontext_t*) raw_context;
	ucontext_t* pNextContext = currContext.uc_link;

	// get the current context
	// does this structure give the thread's true stack immediately?
	// if it does, what is the link pointer to "successor" used for?
	if (getcontext(&workerContext)) {
		perror("getcontext");
	}

	#if 1
	// find out which stack pointer is contained in the context
	if (currContext.uc_stack.ss_sp == workerContext.uc_stack.ss_sp) {
		msg = "[handler] signal-handler getcontext() sp == worker thread stack sp\n";
		write(0, msg, strlen(msg));
	}
	if (pNextContext != NULL && pNextContext->uc_stack.ss_sp == workerContext.uc_stack.ss_sp) {
		msg = "[handler] signal-handler successor's sp == worker thread stack sp\n";
		write(0, msg, strlen(msg));
	}
	if (pCurrContext->uc_stack.ss_sp == workerContext.uc_stack.ss_sp) {
		msg = "[handler] signal-handler context arg sp == worker thread stack sp\n";
		write(0, msg, strlen(msg));
	}
	#endif

	// try to obtain a lock on our "suspend" mutex
	// (which will effectively suspend this thread)
	if (pthread_mutex_lock(&workerMutex)) {
		perror("pthread_mutex_lock"); return;
	}

	if (pthread_mutex_unlock(&workerMutex)) {
		perror("pthread_mutex_unlock"); return;
	}
}



int main (int argc, char** argv) {
	(void) argc;
	(void) argv;

	// create simple worker thread and run it
	pthread_t worker;

	if (pthread_mutex_init(&workerMutex, NULL)) {
		perror("pthread_mutex_init"); exit(-1);
	}

	printf("[%s] starting worker thread\n", __FUNCTION__);

	if (pthread_create(&worker, NULL, worker_thread_func, &workerMutex)) {
		perror("pthread_create"); exit(-1);
	}

	// pause so we can be fairly certain that the
	// worker's signal handler is 1) installed and
	// 2) running (FIXME)
	sleep(3);

	// lock a mutex that the worker will wait on inside its signal handler
	if (pthread_mutex_lock(&workerMutex)) {
		perror("pthread_mutex_lock"); exit(-1);
	}

	// send SIGUSR1 to the worker in order to suspend it
	if (pthread_kill(worker, SIGUSR1)) {
		perror("pthread_kill"); exit(-1);
	}

	// worker will fill in a ucontext_t structure visible to us
	// inspect it to see if we can obtain a full program stack
	printf("[%s] worker has been suspended\n", __FUNCTION__);
	sleep(2);
	printf("[%s] backtrace of worker thread using libunwind:\n", __FUNCTION__);

	{
		void* buffer[100];
		char names[100][100];

		memset(&buffer, 0, sizeof(void*) * 100);

		unw_cursor_t cursor;
		unw_word_t ip, sp, offp;
		unw_init_local(&cursor, &workerContext);

		int i = 0;
		int cnt = 0;

		// walk along the stack
		while (unw_step(&cursor) > 0) {
			unw_get_reg(&cursor, UNW_REG_IP, &ip);
			unw_get_reg(&cursor, UNW_REG_SP, &sp);
			unw_get_proc_name(&cursor, names[i], 100, &offp);

			if (ip != 0) {
				buffer[i++] = (void*) ip;
				printf("[%s]    ip = 0x%0.10lx, sp = 0x%0.10lx,  %s()\n", __FUNCTION__, (long)ip, (long)sp, names[i - 1]);
			} else {
				break;
			}
		}

		cnt = i;
		char** ssymb = (char**) (intptr_t) backtrace_symbols(buffer, cnt);
		printf("[%s] decoding stack IP values using backtrace_symbols():\n", __FUNCTION__);

		for (i = 0; i < cnt; i++) {
			printf("[%s]    %s\n", __FUNCTION__, (const char*) ssymb[i]);
		}
	}

	sleep(2);
	printf("[%s] now resuming worker\n", __FUNCTION__);

	if (pthread_mutex_unlock(&workerMutex)) {
		perror("pthread_mutex_unlock"); exit(-1);
	}

	if (pthread_join(worker, NULL)) {
		perror("pthread_join"); exit(-1);
	}

	if (pthread_mutex_destroy(&workerMutex)) {
		perror("pthread_mutex_destroy"); exit(-1);
	}

	printf("[%s] finished\n", __FUNCTION__);
}

